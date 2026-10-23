param(
 [Parameter(Mandatory=$true)][string]$StageDirectory,
 [Parameter(Mandatory=$true)][string]$ReportPath,
 [switch]$RequireAllSigned
)
$ErrorActionPreference='Stop'
$root=(Resolve-Path -LiteralPath $StageDirectory).Path
$files=@(Get-ChildItem -LiteralPath $root -Recurse -File | Where-Object Extension -in '.exe','.dll')
if(!$files.Count){throw 'No EXE/DLL files found'}
$rows=foreach($file in $files){
 $signature=Get-AuthenticodeSignature -LiteralPath $file.FullName
 [pscustomobject]@{Path=$file.FullName.Substring($root.Length+1);SHA256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash;Status=[string]$signature.Status;Signer=$signature.SignerCertificate.Subject;Product=$file.VersionInfo.ProductName;Version=$file.VersionInfo.ProductVersion}
}
$rows | Export-Csv -LiteralPath $ReportPath -NoTypeInformation -Encoding utf8
$invalid=@($rows | Where-Object Status -ne 'Valid')
Write-Output ("{0} binaries; {1} not verified as signed. Report: {2}" -f $rows.Count,$invalid.Count,$ReportPath)
if($RequireAllSigned -and $invalid.Count){throw 'Signing gate failed: not all payload signatures are valid. No files were changed.'}
