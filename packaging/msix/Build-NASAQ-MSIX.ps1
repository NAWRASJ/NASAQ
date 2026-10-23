<#
.SYNOPSIS
  Builds an MSIX package of NASAQ for submission to the Microsoft Store.
  Optional -TestSign also produces a self-signed copy for installing on this PC.

.EXAMPLE
  .\Build-NASAQ-MSIX.ps1 -SourceDir C:/stage/NASAQ -IdentityConfig ./store-identity.json

  Copy IdentityName / Publisher / PublisherDisplayName EXACTLY from
  Partner Center > your app > Product management > Product identity.
#>
[CmdletBinding()]
param(
  [Parameter(Mandatory)][string]$SourceDir,
  [string]$IdentityConfig = (Join-Path $PSScriptRoot 'store-identity.json'),
  [string]$OutDir = (Join-Path $PSScriptRoot 'out'),
  [switch]$TestSign
)
$ErrorActionPreference = 'Stop'

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$versionText = Get-Content (Join-Path $repo 'cmake/NasaqVersion.cmake') -Raw
if ($versionText -notmatch 'set\(NASAQ_VERSION ([0-9]+\.[0-9]+\.[0-9]+)\)') { throw 'Invalid CMake NASAQ_VERSION; expected MAJOR.MINOR.PATCH' }
$Version = $Matches[1] + '.0'
if ($Version -notmatch '^\d+\.\d+\.\d+\.0$' -or @($Version.Split('.') | Where-Object { [int]$_ -gt 65535 }).Count) { throw 'Store version must be MAJOR.MINOR.PATCH.0, each component <= 65535' }
$identity = Get-Content -LiteralPath $IdentityConfig -Raw | ConvertFrom-Json
foreach ($field in 'IdentityName','Publisher','PublisherDisplayName','DisplayName') {
  $value = [string]$identity.$field
  if ([string]::IsNullOrWhiteSpace($value) -or $value -match 'REPLACE|PLACEHOLDER|XXXXXXXX') { throw "Set public $field in $IdentityConfig from Partner Center > Product identity" }
}
$IdentityName=$identity.IdentityName; $Publisher=$identity.Publisher
$PublisherDisplayName=$identity.PublisherDisplayName; $DisplayName=$identity.DisplayName
$SourceDir=(Resolve-Path -LiteralPath $SourceDir).Path
$OutDir=[IO.Path]::GetFullPath($OutDir)
if ($OutDir.StartsWith($SourceDir + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase) -or $OutDir -eq $SourceDir) { throw 'OutDir must be outside SourceDir' }
if (-not (Test-Path (Join-Path $SourceDir 'NASAQ.exe'))) { throw "NASAQ.exe not found in $SourceDir" }
$exePath=Join-Path $SourceDir 'NASAQ.exe'
$binary=[IO.File]::ReadAllBytes($exePath)
if($binary.Length -lt 64){throw 'Invalid NASAQ executable'}
$pe=[BitConverter]::ToInt32($binary,60)
if($pe -lt 0 -or $pe+6 -gt $binary.Length -or [BitConverter]::ToUInt32($binary,$pe) -ne 0x4550 -or [BitConverter]::ToUInt16($binary,$pe+4) -ne 0x8664){throw 'MSIX stage must contain an x64 PE executable'}
$fileVersion=(Get-Item -LiteralPath $exePath).VersionInfo
$binaryVersion='{0}.{1}.{2}.{3}' -f $fileVersion.FileMajorPart,$fileVersion.FileMinorPart,$fileVersion.FileBuildPart,$fileVersion.FilePrivatePart
if($binaryVersion -ne $Version){throw "Stale runtime version $binaryVersion; rebuild NASAQ $Version before packaging"}
if (Get-ChildItem -LiteralPath $SourceDir -Recurse -Force | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint }) {
  throw 'Runtime stage contains a junction/symlink. Supply an ordinary, self-contained staged runtime.'
}

function Find-SdkTool([string]$name) {
  $cmd = Get-Command $name -ErrorAction SilentlyContinue
  if ($cmd) { return $cmd.Source }
  $roots = @("${env:ProgramFiles(x86)}\Windows Kits\10\bin", "$env:ProgramFiles\Windows Kits\10\bin")
  $hits = foreach ($r in $roots) {
    if (Test-Path $r) { Get-ChildItem $r -Recurse -Filter $name -ErrorAction SilentlyContinue | Where-Object { $_.FullName -match '\\x64\\' } }
  }
  $best = $hits | Sort-Object FullName -Descending | Select-Object -First 1
  if (-not $best) { throw "$name not found. Install the Windows SDK: winget install Microsoft.WindowsSDK.10.0.26100" }
  return $best.FullName
}
$makeappx = Find-SdkTool 'makeappx.exe'
$makepri  = Find-SdkTool 'makepri.exe'
Write-Host "makeappx: $makeappx"
Write-Host "makepri : $makepri"

# ---- 1. Stage the app files -------------------------------------------------
New-Item -ItemType Directory -Force $OutDir | Out-Null
$stage = Join-Path $OutDir ('stage-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory $stage | Out-Null
# vc_redist is not allowed/needed in MSIX: the VC runtime DLLs are already app-local.
$exclude = @('_msix-kit','vc_redist.x64.exe','tests','scripts','nasaq_codec_probe.exe','NASAQ-audit.exe','unins000.exe','unins000.dat','AppxManifest.xml','resources.pri','ابدأ-هنا.txt')
# Preserve Unicode-named resources; exclude only known tooling, never by character set.
Get-ChildItem $SourceDir -Force | Where-Object { ($exclude -notcontains $_.Name) -and ($_.Name -notlike 'unins*') -and ($_.Name -notlike 'resources*.pri') } |
  Copy-Item -Destination $stage -Recurse -Force
& (Join-Path $repo 'scripts/normalize-runtime.ps1') -StageDir $stage
Copy-Item -LiteralPath (Join-Path $repo 'COPYING') -Destination $stage -Force
Copy-Item -LiteralPath (Join-Path $repo 'docs') -Destination $stage -Recurse -Force
Write-Host "Staged files: $((Get-ChildItem $stage -Recurse -File).Count)"

# ---- 2. Generate tile/icon assets from branding/nasaq-512.png ----------------
Add-Type -AssemblyName System.Drawing
$assets = Join-Path $stage 'Assets'
New-Item -ItemType Directory -Force $assets | Out-Null
$srcPng = Join-Path $PSScriptRoot 'assets/nasaq-512.png'
if (-not (Test-Path $srcPng)) { throw "Missing $srcPng" }
$src = [System.Drawing.Image]::FromFile($srcPng)

function Save-Asset([string]$name, [int]$w, [int]$h, [double]$fill = 1.0) {
  $bmp = New-Object System.Drawing.Bitmap($w, $h, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
  $g.SmoothingMode     = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
  $g.PixelOffsetMode   = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
  $g.Clear([System.Drawing.Color]::Transparent)
  $side = [int][math]::Round([math]::Min($w, $h) * $fill)
  $g.DrawImage($src, [int](($w - $side) / 2), [int](($h - $side) / 2), $side, $side)
  $g.Dispose()
  $bmp.Save((Join-Path $assets $name), [System.Drawing.Imaging.ImageFormat]::Png)
  $bmp.Dispose()
}
foreach ($s in 100, 200, 400) {
  $k = $s / 100
  Save-Asset "Square44x44Logo.scale-$s.png"   (44 * $k)  (44 * $k)
  Save-Asset "Square150x150Logo.scale-$s.png" (150 * $k) (150 * $k) 0.6
  Save-Asset "Wide310x150Logo.scale-$s.png"   (310 * $k) (150 * $k) 0.6
  Save-Asset "StoreLogo.scale-$s.png"         (50 * $k)  (50 * $k)
}
foreach ($t in 16, 24, 32, 48, 256) {
  Save-Asset "Square44x44Logo.targetsize-$t.png" $t $t
  Save-Asset "Square44x44Logo.targetsize-$($t)_altform-unplated.png" $t $t
}
$src.Dispose()

# ---- 3. Write the manifest --------------------------------------------------
$esc = { param($v) [System.Security.SecurityElement]::Escape($v) }
$manifest = Get-Content (Join-Path $PSScriptRoot 'AppxManifest.template.xml') -Raw -Encoding UTF8
$manifest = $manifest.Replace('{IDENTITY_NAME}', (& $esc $IdentityName))
$manifest = $manifest.Replace('{PUBLISHER_DISPLAY_NAME}', (& $esc $PublisherDisplayName))
$manifest = $manifest.Replace('{PUBLISHER}', (& $esc $Publisher))
$manifest = $manifest.Replace('{DISPLAY_NAME}', (& $esc $DisplayName))
$manifest = $manifest.Replace('{VERSION}', $Version)
$manifestPath = Join-Path $stage 'AppxManifest.xml'
[System.IO.File]::WriteAllText($manifestPath, $manifest, (New-Object System.Text.UTF8Encoding($false)))

# ---- 4. Build resources.pri (lets Windows pick the right icon sizes) --------
$priConfig = Join-Path $OutDir 'priconfig.xml'
& $makepri createconfig /cf $priConfig /dq en-US /pv 10.0.0 /o
if ($LASTEXITCODE) { throw "makepri createconfig failed ($LASTEXITCODE)" }
# This is one MSIX, not a bundle of language/scale resource packages. Keep all
# scale candidates in its main PRI instead of referencing absent resource packs.
[xml]$priXml = Get-Content -LiteralPath $priConfig -Raw
$packagingNode = $priXml.SelectSingleNode('/resources/packaging')
if ($packagingNode) { [void]$packagingNode.ParentNode.RemoveChild($packagingNode) }
$priXml.Save($priConfig)
& $makepri new /pr $stage /cf $priConfig /mn $manifestPath /of (Join-Path $stage 'resources.pri') /o
if ($LASTEXITCODE) { throw "makepri new failed ($LASTEXITCODE)" }

# ---- 5. Pack ----------------------------------------------------------------
$msix = Join-Path $OutDir ("NASAQ_{0}_x64.msix" -f $Version)
& $makeappx pack /d $stage /p $msix /o
if ($LASTEXITCODE) { throw "makeappx pack failed ($LASTEXITCODE)" }
@{
  Version=$Version; IdentityName=$IdentityName; Publisher=$Publisher
  SourceDirectory=$SourceDir; StageDirectory=$stage
  ExecutableSHA256=(Get-FileHash -LiteralPath (Join-Path $stage 'NASAQ.exe')).Hash
  PackageSHA256=(Get-FileHash -LiteralPath $msix).Hash
  Signed=$false; InstalledPackageTested=$false
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $OutDir 'build-report.json') -Encoding utf8
Write-Host ""
Write-Host "UNSIGNED PACKAGE (submit only with your exact Partner Center identity):" -ForegroundColor Green
Write-Host "  $msix"

# ---- 6. Optional self-signed copy for local testing --------------------------
if ($TestSign) {
  $signtool = Find-SdkTool 'signtool.exe'
  $cert = Get-ChildItem Cert:\CurrentUser\My | Where-Object { $_.Subject -eq $Publisher -and $_.FriendlyName -eq 'NASAQ MSIX local test' -and $_.HasPrivateKey -and $_.NotAfter -gt (Get-Date) } | Select-Object -First 1
  if (-not $cert) {
    $cert = New-SelfSignedCertificate -Type Custom -Subject $Publisher -KeyUsage DigitalSignature `
      -FriendlyName 'NASAQ MSIX local test' -CertStoreLocation Cert:\CurrentUser\My `
      -TextExtension @('2.5.29.37={text}1.3.6.1.5.5.7.3.3', '2.5.29.19={text}')
  }
  $testMsix = $msix -replace '\.msix$', '_LOCAL-TEST.msix'
  Copy-Item $msix $testMsix -Force
  & $signtool sign /fd SHA256 /sha1 $cert.Thumbprint $testMsix
  if ($LASTEXITCODE) { throw "signtool failed ($LASTEXITCODE)" }
  $cer = Join-Path $OutDir 'NASAQ-local-test.cer'
  Export-Certificate -Cert $cert -FilePath $cer | Out-Null
  Write-Host ""
  Write-Host "LOCAL TEST PACKAGE: $testMsix" -ForegroundColor Yellow
  Write-Host "To install it, run ONCE in an *Administrator* PowerShell:"
  Write-Host "  Import-Certificate -FilePath `"$cer`" -CertStoreLocation Cert:\LocalMachine\TrustedPeople"
  Write-Host "then double-click the _LOCAL-TEST.msix (or: Add-AppxPackage `"$testMsix`")."
  Write-Host "Do NOT upload the LOCAL-TEST file to the Store."
}
