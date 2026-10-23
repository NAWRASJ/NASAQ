param(
 [Parameter(Mandatory=$true)][string]$StrawberrySource,
 [Parameter(Mandatory=$true)][string]$DependencyPrefix,
 [Parameter(Mandatory=$true)][string]$VisualStudioPath,
 [Parameter(Mandatory=$true)][string]$BuildDirectory,
 [switch]$StoreBuild
)
$ErrorActionPreference='Stop'
& "$VisualStudioPath\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
$env:PATH="$DependencyPrefix\bin;$env:PATH"
$env:PKG_CONFIG_PATH="$DependencyPrefix\lib\pkgconfig"
$env:PKG_CONFIG_ALLOW_SYSTEM_CFLAGS='1'
$env:PKG_CONFIG_ALLOW_SYSTEM_LIBS='1'
$env:QT_PLUGIN_PATH="$DependencyPrefix\plugins"
$env:QT_MEDIA_BACKEND='windows'
$storeFlag = if($StoreBuild){'ON'}else{'OFF'}
# The official archive contains absolute pkg-config prefixes from its build host.
Get-ChildItem "$DependencyPrefix\lib\pkgconfig" -Filter '*.pc' | ForEach-Object {
 $value=[IO.File]::ReadAllText($_.FullName).Replace('c:/strawberry_msvc_x86_64_release',$DependencyPrefix.Replace('\','/')).Replace('C:/strawberry_msvc_x86_64_release',$DependencyPrefix.Replace('\','/'))
 [IO.File]::WriteAllText($_.FullName,$value)
}
# DependencyPrefix must contain Qt Base, ShaderTools, Multimedia, Speech and the
# exact Strawberry dependency set. Use a short BuildDirectory on Windows.
cmake "-DNASAQ_STORE_BUILD=$storeFlag" -S $StrawberrySource -B $BuildDirectory -G Ninja "-DCMAKE_PREFIX_PATH=$DependencyPrefix" "-DICU_ROOT=$DependencyPrefix" "-DPKG_CONFIG_EXECUTABLE=$DependencyPrefix/bin/pkgconf.exe" -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DCMAKE_BUILD_TYPE=Release -DBUILD_WERROR=OFF -DENABLE_TIDAL=OFF -DENABLE_SPOTIFY=OFF -DENABLE_QOBUZ=OFF -DENABLE_SUBSONIC=OFF -DENABLE_PLEX=OFF -DENABLE_JELLYFIN=OFF -DENABLE_DISCORD_RPC=OFF -DENABLE_AUDIOCD=OFF -DENABLE_MTP=OFF -DENABLE_GPOD=OFF -DENABLE_MOODBAR=OFF -DENABLE_GSTFASTSPECTRUM=OFF
if($LASTEXITCODE){throw 'Configure failed'}
cmake --build $BuildDirectory --parallel 4
if($LASTEXITCODE){throw 'Build failed'}
ctest --test-dir "$BuildDirectory\cam" --output-on-failure
if($LASTEXITCODE){throw 'Tests did not pass; inspect the log and Windows Code Integrity events'}
