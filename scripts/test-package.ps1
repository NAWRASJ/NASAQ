param([Parameter(Mandatory=$true)][string]$StageDirectory,
      [Parameter(Mandatory=$true)][string]$EvidenceDirectory)
$ErrorActionPreference='Stop'
$stage=(Resolve-Path -LiteralPath $StageDirectory).Path
New-Item -ItemType Directory -Force $EvidenceDirectory | Out-Null
$evidence=(Resolve-Path -LiteralPath $EvidenceDirectory).Path
$env:PATH="$stage;$env:PATH"
$env:QT_PLUGIN_PATH=$stage
$env:QT_MEDIA_BACKEND='windows'
foreach($test in @('cam_tests','cam_audio_tests')){
 $result=Join-Path $evidence "$test.xml"
 $p=Start-Process -FilePath "$stage\tests\$test.exe" -ArgumentList @('-o',"`"$result,junitxml`"") -WindowStyle Hidden -PassThru
 if(!$p.WaitForExit(60000)){throw "Timed out: $test; inspect the process and Windows logs"}
 if($p.ExitCode -ne 0 -or !(Test-Path $result)){throw "Failed/blocked: $test, exit $($p.ExitCode)"}
 [xml]$xml=Get-Content -LiteralPath $result
 if([int]$xml.testsuite.failures -ne 0 -or [int]$xml.testsuite.errors -ne 0){throw "$test reported failures"}
}
$env:CAM_SMOKE_OUTPUT=Join-Path $evidence ('integrated-smoke-'+(Get-Date -Format yyyyMMdd-HHmmss))
$env:CAM_TEST_WAV=Join-Path $stage 'tests\smoke.wav'
try {
 $p=Start-Process -FilePath "$stage\NASAQ.exe" -WorkingDirectory $stage -WindowStyle Hidden -PassThru -RedirectStandardOutput "$evidence\smoke-stdout.txt" -RedirectStandardError "$evidence\smoke-stderr.txt"
 if(!$p.WaitForExit(60000)){throw 'Smoke timed out; inspect the application before retrying'}
 $result=$env:CAM_SMOKE_OUTPUT+'.json'
 if($p.ExitCode -ne 0 -or !(Test-Path $result)){throw "Smoke blocked/failed: exit $($p.ExitCode). Keep Windows protection enabled; inspect Code Integrity logs."}
 if(!(Get-Content -LiteralPath $result | ConvertFrom-Json).passed){throw 'Integrated smoke assertions failed'}
 Write-Output 'Package unit tests and integrated smoke passed. Physical speaker/microphone and installation acceptance remain separate.'
} finally {
 Remove-Item Env:CAM_SMOKE_OUTPUT,Env:CAM_TEST_WAV -ErrorAction SilentlyContinue
}
