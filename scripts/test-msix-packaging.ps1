# Non-destructive packaging contract tests; no certificate installation or startup changes.
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$temp=Join-Path ([IO.Path]::GetTempPath()) ('nasaq-msix-test-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp | Out-Null
$stage=Join-Path $temp 'stage'
New-Item -ItemType Directory -Path $stage,"$stage/platforms","$stage/gstreamer-plugins","$stage/gio-modules" | Out-Null
foreach($f in 'NASAQ.exe','gst-plugin-scanner.exe','platforms/qwindows.dll','gstreamer-plugins/gstcoreelements.dll','gstreamer-plugins/gstsoup.dll','gstreamer-plugins/gsticydemux.dll','soup-3.0-0.dll','gio-modules/gioopenssl.dll') { Set-Content -LiteralPath (Join-Path $stage $f) -Value 'fixture' }
Copy-Item -LiteralPath "$stage/platforms/qwindows.dll" -Destination "$stage/qwindows.dll"
Set-Content -LiteralPath "$stage/Qt6Core.dll" -Value 'core library must remain'
& "$PSScriptRoot/normalize-runtime.ps1" -StageDir $stage
if(Test-Path -LiteralPath "$stage/qwindows.dll"){throw 'Duplicate not removed'}
if(-not(Test-Path -LiteralPath "$stage/Qt6Core.dll")){throw 'Core DLL removed'}
if((Get-Content -LiteralPath "$stage/qt.conf" -Raw) -notmatch 'Plugins=\.'){throw 'qt.conf missing'}
Write-Output 'PASS: exact plugin duplicate removed; core preserved; relative qt.conf'
& "$PSScriptRoot/normalize-runtime.ps1" -StageDir $stage
Write-Output 'PASS: normalization is idempotent'
Move-Item "$stage/gstreamer-plugins/gstsoup.dll" "$temp/gstsoup.dll"
$rejected=$false
try { & "$PSScriptRoot/normalize-runtime.ps1" -StageDir $stage } catch { $rejected=$_.Exception.Message -like '*gstsoup.dll*' }
if(-not $rejected){throw 'Missing HTTPS plugin accepted'}
Move-Item "$temp/gstsoup.dll" "$stage/gstreamer-plugins/gstsoup.dll"
Write-Output 'PASS: missing HTTPS source rejects the runtime'
Set-Content -LiteralPath "$stage/qwindows.dll" -Value 'conflicting version'
$rejected=$false
try { & "$PSScriptRoot/normalize-runtime.ps1" -StageDir $stage } catch { $rejected=$_.Exception.Message -like '*Conflicting plugin copies*' }
if(-not $rejected){throw 'Conflicting copy not rejected'}
if(-not(Test-Path -LiteralPath "$stage/qwindows.dll")){throw 'Conflicting file was deleted'}
Write-Output 'PASS: conflicting versions rejected without deletion'
$rejected=$false
$placeholderIdentity=Join-Path $temp 'placeholder-identity.json'
@{IdentityName='REPLACE';Publisher='REPLACE';PublisherDisplayName='REPLACE';DisplayName='NASAQ'} | ConvertTo-Json | Set-Content -LiteralPath $placeholderIdentity
try { & "$PSScriptRoot/build-msix.ps1" -StageDir $stage -IdentityConfig $placeholderIdentity -OutDir "$temp/out" } catch { $rejected=$_.Exception.Message -like '*Partner Center*' }
if(-not $rejected){throw 'Placeholder identity not rejected'}
Write-Output 'PASS: placeholder identity rejected before SDK or filesystem staging'
foreach($script in @("$PSScriptRoot/build-msix.ps1","$PSScriptRoot/normalize-runtime.ps1","$repo/packaging/msix/Build-NASAQ-MSIX.ps1")) {
 $tokens=$null;$errors=$null
 [void][Management.Automation.Language.Parser]::ParseFile($script,[ref]$tokens,[ref]$errors)
 if($errors.Count){throw ($errors|Out-String)}
}
Write-Output 'PASS: PowerShell syntax'
Write-Output "Fixtures retained for inspection: $temp"

# Invalid four-part source version fails before staging, even if ending in zero.
$fixtureRepo=Join-Path $temp 'repo'
New-Item -ItemType Directory -Path "$fixtureRepo/packaging/msix","$fixtureRepo/cmake" -Force | Out-Null
Copy-Item -LiteralPath "$repo/packaging/msix/Build-NASAQ-MSIX.ps1" -Destination "$fixtureRepo/packaging/msix/Build-NASAQ-MSIX.ps1"
Set-Content -LiteralPath "$fixtureRepo/cmake/NasaqVersion.cmake" -Value 'set(NASAQ_VERSION 0.3.10.1)'
$rejected=$false
try { & "$fixtureRepo/packaging/msix/Build-NASAQ-MSIX.ps1" -SourceDir $stage } catch { $rejected=$_.Exception.Message -like '*Invalid CMake NASAQ_VERSION*' }
if(-not $rejected){throw 'Four-part source version accepted'}
Write-Output 'PASS: nonzero fourth version component rejected'
Set-Content -LiteralPath "$fixtureRepo/cmake/NasaqVersion.cmake" -Value 'set(NASAQ_VERSION 0.65536.10)'
$rejected=$false
try { & "$fixtureRepo/packaging/msix/Build-NASAQ-MSIX.ps1" -SourceDir $stage } catch { $rejected=$_.Exception.Message -like '*65535*' }
if(-not $rejected){throw 'Overflowing component accepted'}
Write-Output 'PASS: overflowing version component rejected'
