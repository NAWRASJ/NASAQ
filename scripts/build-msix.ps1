[CmdletBinding()]
param(
 [Parameter(Mandatory)][string]$StageDir,
 [string]$IdentityConfig = "$PSScriptRoot/../packaging/msix/store-identity.json",
 [string]$OutDir = "$PSScriptRoot/../build-msix",
 [switch]$TestSign
)
$ErrorActionPreference = 'Stop'
& "$PSScriptRoot/../packaging/msix/Build-NASAQ-MSIX.ps1" -SourceDir $StageDir -IdentityConfig $IdentityConfig -OutDir $OutDir -TestSign:$TestSign
