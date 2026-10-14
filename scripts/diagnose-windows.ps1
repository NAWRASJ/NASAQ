param([Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
Get-MpComputerStatus | Select-Object AntivirusEnabled,RealTimeProtectionEnabled,SmartAppControlState | ConvertTo-Json | Set-Content (Join-Path $OutputDirectory 'protection-status.json')
Get-WinEvent -LogName 'Microsoft-Windows-CodeIntegrity/Operational' -MaxEvents 100 | Where-Object {$_.Id -in 3077,3033 -and $_.Message -match 'CAM|strawberry'} | Select-Object TimeCreated,Id,Message | ConvertTo-Json | Set-Content (Join-Path $OutputDirectory 'application-control.json')
# Read-only. No exclusion, policy change, certificate import, unblocking or test signing.
