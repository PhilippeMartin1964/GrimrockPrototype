[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$ExpectedHead,
    [string]$EngineRoot='D:\UE_5.5',
    [string]$StateRoot='D:\GrimrockNativeSnapshots',
    [switch]$SelfTest
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'ARM_CLEAN06_4X66_Rollback.ps1')
$RepoRoot=Split-Path -Parent $PSScriptRoot
# Resolve the native executable explicitly: PowerShell function names are
# case-insensitive, and a wrapper named Git would shadow a bare git call.
$GitExecutable=(Get-Command git.exe -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
function Invoke-ArmRetirementGit([string[]]$Arguments) {
    $Result=@(& $GitExecutable -C $RepoRoot @Arguments)
    if ($LASTEXITCODE -ne 0) { throw 'Git read failed.' }
    $Result
}
function CheckClean {
    if ((Invoke-ArmRetirementGit @('rev-parse','HEAD')) -cne $ExpectedHead -or (Invoke-ArmRetirementGit @('branch','--show-current')) -cne 'master' -or
        @(Invoke-ArmRetirementGit @('status','--porcelain','--untracked-files=all')).Count -ne 0) { throw 'Expected clean master at exact SHA.' }
    if ((@(Invoke-ArmRetirementGit @('ls-remote','origin','refs/heads/master'))[0] -split '\s+')[0] -cne $ExpectedHead) { throw 'Origin master changed.' }
}
function CheckClosed {
    if (@(Get-Process -Name UnrealEditor,UnrealEditor-Cmd,GrimrockPrototype,GrimrockPrototype-Win64-Shipping -ErrorAction SilentlyContinue).Count -ne 0) {
        throw 'Close UE and the game before collecting personal save copies.'
    }
}
function Inventory([string]$Directory) {
    Assert-ArmX66OrdinaryPath $Directory
    if (-not (Test-Path -LiteralPath $Directory -PathType Container)) { throw 'Save directory absent.' }
    foreach ($File in @(Get-ChildItem -LiteralPath $Directory -Force | Sort-Object Name)) {
        Assert-ArmX66OrdinaryPath $File.FullName
        if ($File.PSIsContainer -or $File.Name -notmatch '^[a-zA-Z0-9_-]+\.sav$') { throw 'Unexpected save directory entry; audit refused.' }
        [pscustomobject]@{ name=$File.Name; bytes=$File.Length; sha256=(Get-ArmX66FileState $File.FullName) }
    }
}
# The same script and functions run in ValidateUE, on disposable bytes
# only: no real SaveGames, native assets, Unreal or network access.
if ($SelfTest) {
    $Actual=@(Invoke-ArmRetirementGit @('rev-parse','HEAD'))
    if ($Actual.Count -ne 1 -or $Actual[0] -cne $ExpectedHead -or $Actual[0] -cnotmatch '^[0-9a-f]{40}$') {
        throw 'Retirement audit native Git self-test: HEAD mismatch.'
    }
    [void]@(Invoke-ArmRetirementGit @('status','--porcelain','--untracked-files=all'))
    $Fixture=Join-Path ([IO.Path]::GetTempPath()) ('ArmRetirementTest-'+[guid]::NewGuid().ToString('N'))
    if (Test-Path -LiteralPath $Fixture) { throw 'Fixture collision.' }
    [void](New-Item -ItemType Directory -Path $Fixture)
    try {
        $From=Join-Path $Fixture 'Source'
        $To=Join-Path $Fixture 'Copies'
        [void](New-Item -ItemType Directory -Path $From)
        [void](New-Item -ItemType Directory -Path $To)
        [IO.File]::WriteAllBytes((Join-Path $From 'Fixture.sav'),[byte[]]@(1,2,3,4))
        $Rows=@(Inventory $From)
        if ($Rows.Count -ne 1 -or $Rows[0].bytes -ne 4 -or $Rows[0].sha256 -cnotmatch '^[0-9a-f]{64}$') {
            throw 'Retirement audit fixture inventory mismatch.'
        }
        foreach ($Row in $Rows) { Copy-Item -LiteralPath (Join-Path $From $Row.name) -Destination (Join-Path $To $Row.name) }
        $Expected=ConvertTo-Json -InputObject $Rows -Compress
        if ((ConvertTo-Json -InputObject @(Inventory $To) -Compress) -cne $Expected) { throw 'Fixture copy mismatch.' }
        [IO.File]::WriteAllBytes((Join-Path $To 'Fixture.sav'),[byte[]]@(1,2,3,5))
        if ((ConvertTo-Json -InputObject @(Inventory $To) -Compress) -ceq $Expected) { throw 'Fixture modified bytes not detected.' }
        if ((ConvertTo-Json -InputObject @(Inventory $From) -Compress) -cne $Expected) { throw 'Fixture source changed.' }
    } finally {
        Remove-Item -LiteralPath $Fixture -Recurse -Force
    }
    Write-Host '[OK] Retirement audit native Git and copy-inventory self-test.'
    return
}
$Live=Join-Path $RepoRoot 'Saved\SaveGames'
$StateRoot=[IO.Path]::GetFullPath($StateRoot).TrimEnd('\','/')
$Project=[IO.Path]::GetFullPath($RepoRoot).TrimEnd('\','/')
if ($StateRoot -ieq $Project -or $StateRoot.StartsWith($Project+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Private state must be outside project.' }
CheckClean
CheckClosed
Assert-ArmX66OrdinaryPath $StateRoot
$Before=@(Inventory $Live)
if ($Before.Count -eq 0) { throw 'No live saves to audit.' }
$Run=Join-Path $StateRoot ('ARM_CLEAN06_5A_'+$ExpectedHead.Substring(0,12)+'_'+[guid]::NewGuid().ToString('N'))
$Copies=Join-Path $Run 'AuditCopies'
if (Test-Path -LiteralPath $Run) { throw 'Private run directory collision.' }
[void](New-Item -ItemType Directory -Path $Copies -Force)
foreach ($File in $Before) { Copy-Item -LiteralPath (Join-Path $Live $File.name) -Destination (Join-Path $Copies $File.name) }
$InventoryJson=ConvertTo-Json -InputObject $Before -Compress
if ((ConvertTo-Json -InputObject @(Inventory $Copies) -Compress) -cne $InventoryJson -or
    (ConvertTo-Json -InputObject @(Inventory $Live) -Compress) -cne $InventoryJson) { throw 'Save copy mismatch.' }
[pscustomobject]@{schema='ARM-CLEAN06.5A/source-copy-receipt/v1'; head=$ExpectedHead; files=$Before} |
    ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $Run 'copy-receipt.json') -Encoding UTF8
$Build=Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$ProjectFile=Join-Path $RepoRoot 'GrimrockPrototype.uproject'
& $Build GrimrockPrototypeEditor Win64 Development "-Project=$ProjectFile" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw 'Build failed before read-only audit.' }
CheckClean
CheckClosed
$Report=Join-Path $Run 'retirement-evidence.json'
$Log=Join-Path $Run 'audit.log'
# Full post-cutover evidence: world identities and in-memory SaveGame
# round-trip parity. These modes never rewrite the live saves or copies.
$EvidenceModes=@('-WorldPointerPreflight','-InstanceCacheEvidence','-SemanticCacheEvidence','-InstanceSchemaProjection')
& $Editor $ProjectFile '-run=GridArmClean065RetirementAudit' '-ReadOnly' @EvidenceModes "-Copies=$Copies" "-Output=$Report" '-Unattended' '-NoP4' '-NoSplash' '-NoSound' '-NullRHI' "-abslog=$Log"
$UEExit=$LASTEXITCODE
CheckClean
if ((ConvertTo-Json -InputObject @(Inventory $Live) -Compress) -cne $InventoryJson -or
    (ConvertTo-Json -InputObject @(Inventory $Copies) -Compress) -cne $InventoryJson) { throw 'Save inventory changed; preserve private evidence.' }
Write-Host '[OK] All live saves and audit copies are byte-identical; clean master.'
if (-not (Test-Path -LiteralPath $Report -PathType Leaf)) { throw 'Native audit did not produce a report.' }
$Result=Get-Content -LiteralPath $Report -Raw -Encoding UTF8 | ConvertFrom-Json
if ($Result.schema -cne 'ARM-CLEAN06.5A/retirement-evidence/v1' -or $Result.deletion_authorized -or
    [int]$Result.save_count -ne $Before.Count) { throw 'Unexpected evidence report.' }
# False compatibility counters in the retired C++ schema are insufficient
# evidence. Require the active, non-mutating proof modes and every per-save
# projection/round-trip check to have actually run and passed.
if (-not $Result.world_pointer_preflight -or -not $Result.instance_cache_evidence -or
    -not $Result.semantic_cache_evidence -or -not $Result.instance_schema_projection -or
    -not $Result.instance_cache_schema_retired -or $Result.weight_cache_evidence) {
    throw 'Required post-cutover evidence modes were not executed.'
}
$ExpectedRoles=@{
    historical_item_definition=48
    historical_catalogue_source=2
    save_readable_fixture=2
    historical_throwing_knife_redirector=1
    unclassified_keep_and_investigate=0
}
$LegacyIconPalettes=[int]$Result.native.candidate_role_counts.historical_action_icon_palette
if (-not $Result.native.candidate_role_counts -or
    $LegacyIconPalettes -notin @(0,1) -or
    [int]$Result.native.unclassified_candidate_count -ne 0 -or
    [int]$Result.native.candidate_asset_count -ne (53 + $LegacyIconPalettes)) {
    throw 'Native candidate disposition was not proven.'
}
foreach ($Role in $ExpectedRoles.Keys) {
    if ([int]$Result.native.candidate_role_counts.$Role -ne $ExpectedRoles[$Role]) {
        throw ('Unexpected native retirement candidate role: '+$Role)
    }
}
foreach ($Save in @($Result.saves)) {
    if (-not $Save.decoded -or -not $Save.canonical_world_valid -or
        -not $Save.memory_roundtrip_equal -or -not $Save.cache_normalized_roundtrip_equal -or
        $Save.cache_invalid_identities -ne 0 -or $Save.cache_projection_failures -ne 0) {
        throw ('Private save evidence incomplete: '+$Save.filename)
    }
}
Write-Host ('[ARM-CLEAN06.5A] REPORT '+$Report)
Write-Host ('Canonical='+$Result.native.canonical_count+' Legacy='+$Result.native.legacy_definition_count+' ExternalEdges='+$Result.native.external_referencer_edges)
Write-Host ('Saves='+$Result.save_count+' DecodeFailures='+$Result.decode_failures)
if ($UEExit -ne 0 -or $Result.decode_failures -ne 0) { throw 'Read-only report contains incomplete decoding evidence; retain it for review.' }
Write-Host '[OK] Full canonical world/instance and memory round-trip evidence passed. No deletion authorized.'
