param(
 [Parameter(Mandatory=$true)][string]$StageDirectory,
 [Parameter(Mandatory=$true)][ValidatePattern('^[0-9A-Fa-f]{40}$')][string]$CertificateThumbprint,
 [Parameter(Mandatory=$true)][string]$SignTool,
 [uri]$TimestampServer='http://timestamp.digicert.com'
)
$ErrorActionPreference='Stop'
$stage=(Resolve-Path -LiteralPath $StageDirectory).Path
$certificate=Get-Item -LiteralPath "Cert:\CurrentUser\My\$CertificateThumbprint"
if(!$certificate.HasPrivateKey -or $certificate.NotAfter -le (Get-Date)){throw 'A current code-signing certificate with accessible private key is required.'}
if(!($certificate.EnhancedKeyUsageList.ObjectId.Value -contains '1.3.6.1.5.5.7.3.3')){throw 'Certificate is not authorized for code signing.'}
if(!(Test-Path -LiteralPath $SignTool)){throw 'SignTool not found'}
$binaries=Get-ChildItem -LiteralPath $stage -Recurse -File | Where-Object Extension -in '.exe','.dll'
if(!$binaries){throw 'No binaries to sign'}
foreach($binary in $binaries){
 # Preserve already-valid vendor signatures. Sign all remaining executables/libraries.
 if((Get-AuthenticodeSignature -LiteralPath $binary.FullName).Status -ne 'Valid'){
  & $SignTool sign /sha1 $CertificateThumbprint /fd SHA256 /tr $TimestampServer.AbsoluteUri /td SHA256 $binary.FullName
  if($LASTEXITCODE){throw "Signing failed: $($binary.Name)"}
 }
 & $SignTool verify /pa /all $binary.FullName
 if($LASTEXITCODE){throw "Signature verification failed: $($binary.Name)"}
}
$manifest=foreach($binary in $binaries){
 $signature=Get-AuthenticodeSignature -LiteralPath $binary.FullName
 [pscustomobject]@{Path=$binary.FullName.Substring($stage.Length+1);SHA256=(Get-FileHash -LiteralPath $binary.FullName -Algorithm SHA256).Hash;Status=[string]$signature.Status;Signer=$signature.SignerCertificate.Subject}
}
$manifest | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $stage 'SIGNED-MANIFEST.json') -Encoding utf8
Write-Output 'Signatures verified. Rebuild and sign the installer after signing its payload; then perform Windows launch and install tests. This script does not change Windows protection.'
