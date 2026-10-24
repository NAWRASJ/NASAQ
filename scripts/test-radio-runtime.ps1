# The codec probe must be built from this source revision. No audio is sent to speakers.
[CmdletBinding()]
param([Parameter(Mandatory)][string]$StageDir,
      [Parameter(Mandatory)][string]$ProbePath,
      [string]$StreamUrl='https://ice1.somafm.com/doomed-128-mp3')
$ErrorActionPreference='Stop'
if($StreamUrl -notmatch '^https://[^\s"!]+$'){throw 'Expected a simple HTTPS stream URL'}
$stage=(Resolve-Path -LiteralPath $StageDir).Path
$temp=Join-Path ([IO.Path]::GetTempPath()) ('nasaq-radio-'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory $temp | Out-Null
Copy-Item "$stage/*" $temp -Recurse
Copy-Item -LiteralPath $ProbePath -Destination "$temp/nasaq_codec_probe.exe" -Force
$keys=@('PATH','GST_PLUGIN_PATH','GST_PLUGIN_PATH_1_0','GST_PLUGIN_SYSTEM_PATH','GST_PLUGIN_SYSTEM_PATH_1_0','GST_PLUGIN_SCANNER','GST_PLUGIN_SCANNER_1_0','GST_REGISTRY','GST_REGISTRY_1_0','GIO_EXTRA_MODULES')
$saved=@{}; foreach($key in $keys){$saved[$key]=[Environment]::GetEnvironmentVariable($key,'Process')}
try {
 $env:PATH="$temp;$env:WINDIR/System32;$env:WINDIR"
 $env:GST_PLUGIN_PATH=$env:GST_PLUGIN_PATH_1_0="$temp/gstreamer-plugins"
 $env:GST_PLUGIN_SYSTEM_PATH=$env:GST_PLUGIN_SYSTEM_PATH_1_0="$temp/gstreamer-plugins"
 $env:GST_PLUGIN_SCANNER=$env:GST_PLUGIN_SCANNER_1_0="$temp/gst-plugin-scanner.exe"
 $env:GST_REGISTRY=$env:GST_REGISTRY_1_0="$temp/qa-registry.bin"
 $env:GIO_EXTRA_MODULES="$temp/gio-modules"
 & "$temp/nasaq_codec_probe.exe" "souphttpsrc location=$StreamUrl ! decodebin ! audioconvert ! identity eos-after=100 ! fakesink sync=false"
 if($LASTEXITCODE){throw "HTTPS decoding failed: $LASTEXITCODE"}
 Write-Output 'PASS: HTTPS stream decoded 100 audio buffers using only the staged runtime.'
} finally {
 foreach($key in $keys){[Environment]::SetEnvironmentVariable($key,$saved[$key],'Process')}
}
Write-Output "Evidence runtime retained: $temp"
