# Run on a staged copy BEFORE signing (Inno and MSIX share this layout).
[CmdletBinding()]
param([Parameter(Mandatory)][string]$StageDir)
$ErrorActionPreference='Stop'
$root=(Resolve-Path -LiteralPath $StageDir).Path
if(-not (Test-Path -LiteralPath (Join-Path $root 'NASAQ.exe'))) { throw 'Not a NASAQ stage' }
$plugins=@('platforms','imageformats','multimedia','networkinformation','sqldrivers','styles','texttospeech','tls','generic','gstreamer-plugins','gio-modules')
foreach($dir in $plugins) {
 $folder=Join-Path $root $dir
 if(-not (Test-Path -LiteralPath $folder)){continue}
 foreach($plugin in Get-ChildItem -LiteralPath $folder -File -Filter '*.dll') {
  $duplicate=Join-Path $root $plugin.Name
  if(Test-Path -LiteralPath $duplicate) {
   if((Get-FileHash -LiteralPath $duplicate).Hash -ne (Get-FileHash -LiteralPath $plugin.FullName).Hash) { throw "Conflicting plugin copies: $duplicate and $($plugin.FullName)" }
   # Delete only an exact duplicate in the verified stage root; never library DLLs.
   Remove-Item -LiteralPath $duplicate
  }
 }
}
foreach($required in @('platforms/qwindows.dll','gstreamer-plugins/gstcoreelements.dll','gst-plugin-scanner.exe','gstreamer-plugins/gstsoup.dll','gstreamer-plugins/gsticydemux.dll','soup-3.0-0.dll','gio-modules/gioopenssl.dll')) {
 if(-not(Test-Path -LiteralPath (Join-Path $root $required))){throw "Missing plugin runtime: $required"}
}
Set-Content -LiteralPath (Join-Path $root 'qt.conf') -Encoding ascii -Value "[Paths]`nPrefix=.`nPlugins=."
