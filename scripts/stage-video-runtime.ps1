# Run in a Visual Studio developer shell, using the SAME dependency prefix as NASAQ.
[CmdletBinding()]
param([Parameter(Mandatory)][string]$StageDir,
      [Parameter(Mandatory)][string]$DependencyPrefix,
      [Parameter(Mandatory)][string]$FormatPluginsDir)
$ErrorActionPreference='Stop'
$stage=(Resolve-Path -LiteralPath $StageDir).Path
$prefix=(Resolve-Path -LiteralPath $DependencyPrefix).Path
if(-not(Test-Path "$stage/NASAQ.exe")){throw 'Not a NASAQ stage'}
$dumpbin=(Get-Command dumpbin.exe -ErrorAction Stop).Source
$visited=@{}
function Copy-RuntimeDll([string]$source,[string]$destination) {
 if($visited.ContainsKey($source)){return}
 $visited[$source]=$true
 if(-not(Test-Path -LiteralPath $source)){throw "Missing video dependency: $source"}
 if(Test-Path -LiteralPath $destination){
  if((Get-FileHash $source).Hash -ne (Get-FileHash $destination).Hash){throw "Mixed runtime versions: $destination"}
 } else { Copy-Item -LiteralPath $source -Destination $destination }
 $imports=& $dumpbin /dependents $source
 if($LASTEXITCODE){throw "Cannot inspect dependencies: $source"}
 foreach($line in $imports){
  if($line -match '^\s+([\w.-]+\.dll)\s*$'){
   $name=$Matches[1]
   $dependency=Join-Path "$prefix/bin" $name
   if(Test-Path -LiteralPath $dependency){Copy-RuntimeDll $dependency (Join-Path $stage $name)}
   elseif(-not(Test-Path -LiteralPath (Join-Path "$env:WINDIR/System32" $name)) -and $name -notlike 'api-ms-win-*' -and $name -notlike 'ext-ms-win-*'){throw "Unresolved dependency: $name"}
  }
 }
}
New-Item -ItemType Directory -Force "$stage/gstreamer-plugins","$stage/gio-modules" | Out-Null
foreach($name in 'gstapp.dll','gstisomp4.dll','gstmatroska.dll'){
 Copy-RuntimeDll "$prefix/lib/gstreamer-1.0/$name" "$stage/gstreamer-plugins/$name"
}
Copy-RuntimeDll "$prefix/bin/gstvideo-1.0-0.dll" "$stage/gstvideo-1.0-0.dll"
Write-Output "Staged and checked $($visited.Count) video runtime libraries."

$formatRoot=(Resolve-Path -LiteralPath $FormatPluginsDir).Path
$manifest=Get-Content -LiteralPath "$formatRoot/format-runtime.json" -Raw | ConvertFrom-Json
if($manifest.gstreamer_version -ne '1.28.8'){throw 'Unexpected format plugin version'}
if(-not(Test-Path -LiteralPath "$formatRoot/licenses")){throw 'Missing format library licenses'}
$requiredFormats=@('gstavi.dll','gstflv.dll','gstauparse.dll','gstmpegpsdemux.dll','gstvideoparsersbad.dll','gstaom.dll','gstlibav.dll','gsttheora.dll','libaom.dll','libwinpthread-1.dll')
foreach($name in $requiredFormats){
 $source=Join-Path $formatRoot $name
 $expected=$manifest.files.$name
 if(-not $expected -or -not(Test-Path -LiteralPath $source) -or (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected){throw "Invalid format dependency: $name"}
}
foreach($name in $requiredFormats){
 $target=if($name.StartsWith('gst')){Join-Path "$stage/gstreamer-plugins" $name}else{Join-Path $stage $name}
 Copy-Item -LiteralPath (Join-Path $formatRoot $name) -Destination $target -Force
}
Copy-Item -LiteralPath "$formatRoot/format-runtime.json" -Destination "$stage/format-runtime.json" -Force
Write-Output 'Staged verified video/audio format extensions.'

New-Item -ItemType Directory -Force "$stage/format-licenses" | Out-Null
Copy-Item -Path "$formatRoot/licenses/*" -Destination "$stage/format-licenses" -Recurse -Force
