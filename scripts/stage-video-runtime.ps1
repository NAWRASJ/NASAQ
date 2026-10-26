# Run in a Visual Studio developer shell, using the SAME dependency prefix as NASAQ.
[CmdletBinding()]
param([Parameter(Mandatory)][string]$StageDir,
      [Parameter(Mandatory)][string]$DependencyPrefix)
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
foreach($name in 'gstapp.dll','gstisomp4.dll','gstlibav.dll','gstmatroska.dll'){
 Copy-RuntimeDll "$prefix/lib/gstreamer-1.0/$name" "$stage/gstreamer-plugins/$name"
}
Copy-RuntimeDll "$prefix/bin/gstvideo-1.0-0.dll" "$stage/gstvideo-1.0-0.dll"
Write-Output "Staged and checked $($visited.Count) video runtime libraries."
