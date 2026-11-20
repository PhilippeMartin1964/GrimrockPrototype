param([Parameter(Mandatory=$true)][string]$StateRoot,[Parameter(Mandatory=$true)][string]$WorktreePath,[Parameter(Mandatory=$true)][string]$ExpectedHead)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'ARM_CLEAN06_4X66_Rollback.ps1')
$Baseline='be0f6ed88fede589893542258b6515ac9489130b'
$Actual=@(& git -C $WorktreePath rev-parse HEAD)
if ($LASTEXITCODE -ne 0 -or $Actual.Count -ne 1 -or $Actual[0] -cne $ExpectedHead) { throw 'Wrong diagnostic HEAD.' }
$Parent=@(& git -C $WorktreePath rev-parse HEAD^)
if ($LASTEXITCODE -ne 0 -or $Parent.Count -ne 1 -or $Parent[0] -cne $Baseline) { throw 'Wrong diagnostic baseline.' }
$Runs=Join-Path $StateRoot 'Runs'
Assert-ArmX66OrdinaryPath $Runs
$Matches=@(Get-ChildItem -LiteralPath $Runs -Directory -Filter ($Baseline.Substring(0,12)+'-*') | ForEach-Object {
    $Candidate=Join-Path $_.FullName 'InstanceCacheEvidence'
    if (Test-Path -LiteralPath (Join-Path $Candidate 'cache-proof.json') -PathType Leaf) { $Candidate }
})
if ($Matches.Count -ne 1) { throw 'Unique baseline evidence required.' }
$Dir=$Matches[0]
foreach ($Name in @('cache-proof.json','receipt.json','world-preflight.json')) { Assert-ArmX66OrdinaryPath (Join-Path $Dir $Name) }
$Proof=Get-Content -LiteralPath (Join-Path $Dir 'cache-proof.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$Receipt=Get-Content -LiteralPath (Join-Path $Dir 'receipt.json') -Raw -Encoding UTF8 | ConvertFrom-Json
$Report=Get-Content -LiteralPath (Join-Path $Dir 'world-preflight.json') -Raw -Encoding UTF8 | ConvertFrom-Json
if ($Proof.schema -cne 'ARM-CLEAN06.5G88/instance-cache-aggregate/v1' -or $Proof.head -cne $Baseline -or
    $Proof.parity -isnot [bool] -or $Proof.parity -or -not $Proof.source_unchanged -or -not $Proof.copies_unchanged -or
    $Receipt.head -cne $Baseline -or -not $Receipt.passed -or -not $Receipt.all_source_bytes_unchanged -or -not $Receipt.all_copies_unchanged -or
    $Report.schema -cne 'ARM-CLEAN06.5A/retirement-evidence/v1' -or -not $Report.instance_cache_evidence -or
    $Report.decode_failures -ne 0 -or $Report.save_count -le 0 -or @($Report.saves).Count -ne $Report.save_count -or
    @($Receipt.files).Count -ne $Report.save_count) { throw 'Baseline evidence envelope rejected.' }
$NameConflict=$false; $WeightConflict=$false; $InvalidIdentity=$false; $NormalizedRoundtrip=$false
foreach ($Row in @($Report.saves)) {
    if (-not $Row.decoded -or -not $Row.canonical_world_valid -or -not $Row.memory_roundtrip_equal -or
        $Row.cache_normalized_roundtrip_equal -isnot [bool]) { throw 'Baseline row rejected.' }
    foreach ($Key in @('cache_invalid_identities','cache_name_conflicts','cache_weight_conflicts')) {
        if (($Row.$Key -isnot [int] -and $Row.$Key -isnot [long]) -or $Row.$Key -lt 0) { throw 'Invalid diagnostic count.' }
    }
    if ($Row.cache_invalid_identities -gt 0) { $InvalidIdentity=$true }
    if ($Row.cache_name_conflicts -gt 0) { $NameConflict=$true }
    if ($Row.cache_weight_conflicts -gt 0) { $WeightConflict=$true }
    if ($Row.cache_invalid_identities -eq 0 -and $Row.cache_name_conflicts -eq 0 -and $Row.cache_weight_conflicts -eq 0 -and
        -not $Row.cache_normalized_roundtrip_equal) { $NormalizedRoundtrip=$true }
}
if (-not ($NameConflict -or $WeightConflict -or $InvalidIdentity -or $NormalizedRoundtrip)) { throw 'No refusal cause in baseline.' }
# Only fixed booleans. Never forward any private report row or raw exception.
[pscustomobject]@{NameConflict=$NameConflict;WeightConflict=$WeightConflict;InvalidIdentity=$InvalidIdentity;NormalizedRoundtrip=$NormalizedRoundtrip}
