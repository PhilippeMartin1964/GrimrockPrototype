[CmdletBinding()]
param(
    [string]$RepositoryRoot,
    [string]$WorktreePath,
    [string]$EngineRoot,
    [string]$RunRoot,
    [string]$ExpectedHead,
    [string]$BaselineHead,
    [string]$AgentStateRoot,
    [switch]$AfterRetirement,
    [switch]$InstanceCacheEvidence,
    [switch]$WeightCacheEvidence,
    [switch]$SelfTest
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'ARM_CLEAN06_4X66_Rollback.ps1')
function Get-WorldSaveInventory([string]$Directory) {
    Assert-ArmX66OrdinaryPath $Directory
    if (-not (Test-Path -LiteralPath $Directory -PathType Container)) { throw 'Save directory absent.' }
    foreach ($File in @(Get-ChildItem -LiteralPath $Directory -Force | Sort-Object Name)) {
        Assert-ArmX66OrdinaryPath $File.FullName
        if ($File.PSIsContainer -or $File.Name -notmatch '^[a-zA-Z0-9_-]+\.sav$' -or $File.Length -le 0) {
            throw 'Unexpected save directory entry.'
        }
        [pscustomobject]@{name=$File.Name;bytes=$File.Length;sha256=(Get-ArmX66FileState $File.FullName)}
    }
}
function Get-InventoryJson([string]$Directory) {
    ConvertTo-Json -InputObject @(Get-WorldSaveInventory $Directory) -Compress
}
function Copy-VerifiedWorldSaves([string]$Source,[string]$Copies) {
    $Rows=@(Get-WorldSaveInventory $Source)
    if ($Rows.Count -eq 0 -or (Test-Path -LiteralPath $Copies)) { throw 'No input saves or copy collision.' }
    Assert-ArmX66OrdinaryPath $Copies
    $Expected=ConvertTo-Json -InputObject $Rows -Compress
    [void](New-Item -ItemType Directory -Path $Copies -Force)
    foreach ($Row in $Rows) { Copy-Item -LiteralPath (Join-Path $Source $Row.name) -Destination (Join-Path $Copies $Row.name) }
    if ((Get-InventoryJson $Source) -cne $Expected -or (Get-InventoryJson $Copies) -cne $Expected) { throw 'Copy parity refused.' }
    $Expected
}
function Check-WorldSaveReport($Report,$Rows) {
    if ($Report.schema -cne 'ARM-CLEAN06.5A/retirement-evidence/v1' -or -not $Report.world_pointer_preflight -or
        $Report.deletion_authorized -or $Report.save_count -ne $Rows.Count -or $Report.decode_failures -ne 0 -or
        @($Report.saves).Count -ne $Rows.Count -or $Report.native.canonical_count -ne 48 -or
        $Report.native.resolved_item_levels -ne 466 -or $Report.native.external_referencer_edges -ne 0) { throw 'Native evidence refused.' }
    $AllowedReadables=@(
        '/Game/GrimrockPrototype/Core/DataAssets/Tests/Readables/DA_Scroll_Test.DA_Scroll_Test',
        '/Game/GrimrockPrototype/Core/DataAssets/Tests/Readables/DA_BookPage_Test.DA_BookPage_Test'
    )
    foreach ($Row in $Rows) {
        $Match=@($Report.saves | Where-Object { $_.filename -ceq $Row.name })
        if ($Match.Count -ne 1) { throw 'Save identity mismatch.' }
        $S=$Match[0]
        if (-not $S.decoded -or $S.version -ne 27 -or $S.bytes -ne $Row.bytes -or
            -not $S.canonical_world_valid -or -not $S.memory_roundtrip_equal -or
            @($S.unknown_item_ids).Count -ne 0 -or $S.projectile_definition_pointers -ne 0 -or
            $S.pending_definition_pointers -ne 0 -or $S.readable_ids_without_pointer -ne 0) { throw 'World pointer retirement preflight refused.' }
        if ($null -eq $S.world_row_projection) { throw 'Missing private physical projection.' }
        foreach ($Ref in @($S.decoded_references)) {
            if ($Ref.identity_conflict -or $Ref.field -cne 'ReadableContentAsset') { throw 'Decoded reference refused.' }
        }
        foreach ($Ref in @($S.raw_path_candidates)) {
            if ($Ref.object -cnotin $AllowedReadables) { throw 'Residual raw historical path.' }
        }
    }
}
function Compare-WorldSaveReports($Before,$After,$Rows) {
    Check-WorldSaveReport $Before $Rows
    Check-WorldSaveReport $After $Rows
    foreach ($Row in $Rows) {
        $A=@($Before.saves | Where-Object { $_.filename -ceq $Row.name })[0]
        $B=@($After.saves | Where-Object { $_.filename -ceq $Row.name })[0]
        foreach ($Key in @('item_instances_and_world_rows','instance_legacy_name_or_weight','readable_content_pointers','readable_ids_without_pointer')) {
            if ($A.$Key -ne $B.$Key) { throw 'Physical or narrative count changed across schema.' }
        }
        foreach ($Key in @('world_row_projection','decoded_references','raw_path_candidates')) {
            if ((ConvertTo-Json -InputObject @($A.$Key) -Depth 10 -Compress) -cne
                (ConvertTo-Json -InputObject @($B.$Key) -Depth 10 -Compress)) { throw 'Physical or reference evidence changed across schema.' }
        }
    }
}
if ($SelfTest) {
    $Root=Join-Path ([IO.Path]::GetTempPath()) ('Arm5G85-'+[guid]::NewGuid().ToString('N'))
    try {
        $Source=Join-Path $Root 'Source'; $Copies=Join-Path $Root 'AuditCopies'
        [void](New-Item -ItemType Directory -Path $Source -Force)
        [IO.File]::WriteAllBytes((Join-Path $Source 'Fixture.sav'),[byte[]]@(1,2,3,4))
        $Expected=Copy-VerifiedWorldSaves $Source $Copies
        $Rows=@(Get-WorldSaveInventory $Source)
        $Report=[pscustomobject]@{schema='ARM-CLEAN06.5A/retirement-evidence/v1';world_pointer_preflight=$true;deletion_authorized=$false;save_count=1;decode_failures=0;native=[pscustomobject]@{canonical_count=48;resolved_item_levels=466;external_referencer_edges=0};saves=@([pscustomobject]@{filename='Fixture.sav';decoded=$true;version=27;bytes=4;canonical_world_valid=$true;memory_roundtrip_equal=$true;unknown_item_ids=@();projectile_definition_pointers=0;pending_definition_pointers=0;readable_ids_without_pointer=0;decoded_references=@();raw_path_candidates=@();world_row_projection=@();item_instances_and_world_rows=0;instance_legacy_name_or_weight=0;readable_content_pointers=0})}
        Check-WorldSaveReport $Report $Rows
        $Same=ConvertFrom-Json (ConvertTo-Json $Report -Depth 10)
        Compare-WorldSaveReports $Report $Same $Rows
        $Same.saves[0].world_row_projection=@('changed')
        $Different=$false
        try { Compare-WorldSaveReports $Report $Same $Rows } catch { $Different=$true }
        if (-not $Different) { throw 'Fixture cross-schema comparison failed.' }
        $Report.saves[0].pending_definition_pointers=1
        $Rejected=$false
        try { Check-WorldSaveReport $Report $Rows } catch { $Rejected=$true }
        if (-not $Rejected) { throw 'Fixture nonzero pointer was accepted.' }
        [IO.File]::WriteAllBytes((Join-Path $Copies 'Fixture.sav'),[byte[]]@(1,2,3,5))
        if ((Get-InventoryJson $Copies) -ceq $Expected -or (Get-InventoryJson $Source) -cne $Expected) { throw 'Fixture hash guard failed.' }
    } finally {
        if (Test-Path -LiteralPath $Root) { Remove-Item -LiteralPath $Root -Recurse -Force }
    }
    Write-Host '[OK] 5G85 copy inventory and evidence gate self-test.'
    return
}
function Assert-Closed {
    if (@(Get-Process -Name UnrealEditor,UnrealEditor-Cmd,GrimrockPrototype,GrimrockPrototype-Win64-Shipping -ErrorAction SilentlyContinue).Count -ne 0) { throw 'UE or game open; evidence refused.' }
}
if ($ExpectedHead -cnotmatch '^[0-9a-f]{40}$') { throw 'Expected SHA required.' }
$RepositoryRoot=[IO.Path]::GetFullPath($RepositoryRoot).TrimEnd('\','/')
$WorktreePath=[IO.Path]::GetFullPath($WorktreePath).TrimEnd('\','/')
$RunRoot=[IO.Path]::GetFullPath($RunRoot).TrimEnd('\','/')
foreach ($Root in @($RepositoryRoot,$WorktreePath)) {
    if ($RunRoot -ieq $Root -or $RunRoot.StartsWith($Root+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Report must stay outside both checkouts.' }
}
if ($RepositoryRoot -ieq $WorktreePath -or $WorktreePath.StartsWith($RepositoryRoot+'\',[StringComparison]::OrdinalIgnoreCase) -or $RepositoryRoot.StartsWith($WorktreePath+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Distinct validation checkout required.' }
$Git=(Get-Command git.exe -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
function Assert-Head {
    $Head=@(& $Git -C $WorktreePath rev-parse HEAD)
    if ($LASTEXITCODE -ne 0 -or $Head.Count -ne 1 -or $Head[0] -cne $ExpectedHead) { throw 'Validation HEAD mismatch.' }
    $Origin=@(& $Git -C $WorktreePath ls-remote origin refs/heads/master)
    if ($LASTEXITCODE -ne 0 -or $Origin.Count -ne 1 -or ($Origin[0] -split '\s+')[0] -cne $ExpectedHead) { throw 'Remote master changed.' }
    $Status=@(& $Git -C $WorktreePath status --porcelain --untracked-files=all)
    if ($LASTEXITCODE -ne 0 -or $Status.Count -ne 0) { throw 'Validation checkout dirty.' }
}
Assert-Closed
Assert-Head
Assert-ArmX66OrdinaryPath $RunRoot
if (Test-Path -LiteralPath $RunRoot) { throw 'Private run directory already exists.' }
$Source=Join-Path $RepositoryRoot 'Saved\SaveGames'
$BeforeReport=$null
if ($AfterRetirement) {
    if ($BaselineHead -cnotmatch '^[0-9a-f]{40}$' -or [string]::IsNullOrWhiteSpace($AgentStateRoot)) { throw 'Exact baseline required.' }
    $Parent=@(& $Git -C $WorktreePath rev-parse HEAD^)
    if ($LASTEXITCODE -ne 0 -or $Parent.Count -ne 1 -or $Parent[0] -cne $BaselineHead) { throw 'Baseline not exact parent.' }
    $Runs=Join-Path $AgentStateRoot 'Runs'
    Assert-ArmX66OrdinaryPath $Runs
    $Candidates=@(Get-ChildItem -LiteralPath $Runs -Directory -Filter ($BaselineHead.Substring(0,12)+'-*') | ForEach-Object {
        Assert-ArmX66OrdinaryPath $_.FullName
        $Proof=Join-Path $_.FullName 'WorldSavePreflight'
        $Receipt=Join-Path $Proof 'receipt.json'
        if (Test-Path -LiteralPath $Receipt -PathType Leaf) { $Proof }
    })
    if ($Candidates.Count -ne 1) { throw 'Unique private baseline receipt required.' }
    $Baseline=$Candidates[0]
    $ReceiptPath=Join-Path $Baseline 'receipt.json'
    Assert-ArmX66OrdinaryPath $ReceiptPath
    $Receipt=Get-Content -LiteralPath $ReceiptPath -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($Receipt.schema -cne 'ARM-CLEAN06.5G85/world-read-only-receipt/v1' -or $Receipt.head -cne $BaselineHead -or
        -not $Receipt.passed -or -not $Receipt.all_source_bytes_unchanged -or -not $Receipt.all_copies_unchanged -or
        $Receipt.world_pointer_count -ne 0 -or @($Receipt.files).Count -eq 0) { throw 'Unproven baseline receipt.' }
    $Source=Join-Path $Baseline 'AuditCopies'
    $ExpectedBaseline=ConvertTo-Json -InputObject @($Receipt.files) -Compress
    if ((Get-InventoryJson $Source) -cne $ExpectedBaseline) { throw 'Baseline byte mismatch.' }
    $BeforePath=Join-Path $Baseline 'world-preflight.json'
    Assert-ArmX66OrdinaryPath $BeforePath
    $BeforeReport=Get-Content -LiteralPath $BeforePath -Raw -Encoding UTF8 | ConvertFrom-Json
}
$Copies=Join-Path $RunRoot 'AuditCopies'
$Expected=Copy-VerifiedWorldSaves $Source $Copies
if ($AfterRetirement -and $Expected -cne $ExpectedBaseline) { throw 'Cross-schema source bytes changed.' }
$Rows=@(Get-WorldSaveInventory $Copies)
$ReportPath=Join-Path $RunRoot 'world-preflight.json'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project=Join-Path $WorktreePath 'GrimrockPrototype.uproject'
if ($AfterRetirement -and $InstanceCacheEvidence) { throw 'Mixed evidence modes refused.' }
$Mode=@()
if ($InstanceCacheEvidence) { $Mode=@('-InstanceCacheEvidence') }
if ($WeightCacheEvidence) {
    if (-not $InstanceCacheEvidence) { throw 'Weight proof requires instance evidence.' }
    $Mode+= '-WeightCacheEvidence'
}
Assert-Closed
& $Editor $Project @Mode '-run=GridArmClean065RetirementAudit' '-ReadOnly' '-WorldPointerPreflight' "-Copies=$Copies" "-Output=$ReportPath" '-Unattended' '-NoP4' '-NoSplash' '-NoSound' '-NullRHI' "-abslog=$(Join-Path $RunRoot 'audit.log')" | Out-Host
$Code=$LASTEXITCODE
Assert-Closed
Assert-Head
if ((Get-InventoryJson $Source) -cne $Expected -or (Get-InventoryJson $Copies) -cne $Expected) { throw 'Source or private copy changed; no retirement authorization.' }
if ($Code -ne 0 -or -not (Test-Path -LiteralPath $ReportPath -PathType Leaf)) { throw 'Native preflight failed.' }
$AfterReport=Get-Content -LiteralPath $ReportPath -Raw -Encoding UTF8 | ConvertFrom-Json
Check-WorldSaveReport $AfterReport $Rows
if ($AfterRetirement) { Compare-WorldSaveReports $BeforeReport $AfterReport $Rows }
if ($InstanceCacheEvidence) {
    if (-not $AfterReport.instance_cache_evidence) { throw 'Missing cache evidence.' }
    if ($WeightCacheEvidence -and -not $AfterReport.weight_cache_evidence) { throw 'Missing weight provenance evidence.' }
    $Parity=$true
    $Unexplained=$false
    foreach ($S in @($AfterReport.saves)) {
        foreach ($Key in @('cache_instances','cache_invalid_identities','cache_name_conflicts','cache_weight_conflicts','cache_names','cache_weights')) {
            if (($S.$Key -isnot [int] -and $S.$Key -isnot [long]) -or $S.$Key -lt 0) { throw 'Invalid cache count.' }
        }
        $Explained=0
        if ($WeightCacheEvidence) {
            $Explained=$S.cache_weight_explained
            if (($Explained -isnot [int] -and $Explained -isnot [long]) -or $Explained -lt 0 -or $Explained -gt $S.cache_weight_conflicts) { throw 'Invalid weight evidence.' }
        }
        if ($S.cache_weight_conflicts -ne $Explained) { $Unexplained=$true }
        if ($S.cache_invalid_identities -ne 0 -or $S.cache_name_conflicts -ne 0 -or $S.cache_weight_conflicts -ne $Explained -or -not $S.cache_normalized_roundtrip_equal) { $Parity=$false }
    }
    [pscustomobject]@{schema='ARM-CLEAN06.5G88/instance-cache-aggregate/v1';head=$ExpectedHead;parity=$Parity;weight_evidence=[bool]$WeightCacheEvidence;unexplained_weight=$Unexplained;source_unchanged=$true;copies_unchanged=$true} |
        ConvertTo-Json -Compress | Set-Content -LiteralPath (Join-Path $RunRoot 'cache-proof.json') -Encoding UTF8
}
$Schema=if ($AfterRetirement) { 'ARM-CLEAN06.5G86/world-post-retirement-receipt/v1' } else { 'ARM-CLEAN06.5G85/world-read-only-receipt/v1' }
[pscustomobject]@{schema=$Schema;head=$ExpectedHead;baseline_head=$BaselineHead;files=$Rows;all_source_bytes_unchanged=$true;all_copies_unchanged=$true;world_pointer_count=0;passed=$true} |
    ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $RunRoot 'receipt.json') -Encoding UTF8
Write-Host '[OK] 5G85 native world save preflight. Originals and copies unchanged.'
