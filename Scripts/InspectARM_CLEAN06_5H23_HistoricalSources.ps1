[CmdletBinding()]
param(
    [string]$EvidencePath,
    [string]$OutputPath,
    [string]$ExpectedHead,
    [switch]$SelfTest
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent $PSScriptRoot

function Get-ArmH23Stems([string]$Line) {
    @([regex]::Matches($Line, '(?<![A-Za-z0-9_])DA_(?:Item|Weapon)_[A-Za-z0-9_]+(?![A-Za-z0-9_])') |
        ForEach-Object { $_.Value } | Sort-Object -Unique)
}
function Get-ArmH23Kind([string]$RelPath) {
    if ($RelPath -match '^Source/GrimrockPrototypeEditor/.*/Tests/') { return 'editor_test' }
    if ($RelPath -match '^Source/GrimrockPrototype/.*/Tests/') { return 'game_test' }
    if ($RelPath -match '^Source/GrimrockPrototypeEditor/') { return 'editor_tool' }
    if ($RelPath -match '^Source/') { return 'game_source' }
    if ($RelPath -match '^Scripts/') { return 'script' }
    if ($RelPath -match '^Config/') { return 'config' }
    'other'
}
if ($SelfTest) {
    $Found = @(Get-ArmH23Stems 'DA_Item_RatTooth DA_Item_RatTooth_extra DA_Weapon_ThrowingKnife_01')
    if ($Found.Count -ne 3 -or $Found -cnotcontains 'DA_Item_RatTooth' -or
        $Found -cnotcontains 'DA_Item_RatTooth_extra' -or
        $Found -cnotcontains 'DA_Weapon_ThrowingKnife_01') {
        throw 'Static exact-stem parser self-test failed.'
    }
    if ((Get-ArmH23Kind 'Source/GrimrockPrototypeEditor/Private/Tests/X.cpp') -cne 'editor_test' -or
        (Get-ArmH23Kind 'Source/GrimrockPrototype/Private/Runtime/X.cpp') -cne 'game_source') {
        throw 'Source reader classification self-test failed.'
    }
    Write-Host '[OK] ARM-CLEAN06.5H.23 static source inventory self-test.'
    return
}
if ($ExpectedHead -cnotmatch '^[a-f0-9]{40}$' -or
    [string]::IsNullOrWhiteSpace($EvidencePath) -or
    [string]::IsNullOrWhiteSpace($OutputPath)) {
    throw 'ExpectedHead, an existing private evidence JSON and a new external OutputPath are required.'
}
$Git = (Get-Command git.exe -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
$LocalSha = @(& $Git -C $RepoRoot rev-parse HEAD)
$Branch = @(& $Git -C $RepoRoot branch --show-current)
$Remote = @(& $Git -C $RepoRoot ls-remote origin refs/heads/master)
$Dirty = @(& $Git -C $RepoRoot status --porcelain --untracked-files=all)
if ($LASTEXITCODE -ne 0 -or $LocalSha.Count -ne 1 -or $LocalSha[0] -cne $ExpectedHead -or
    $Branch.Count -ne 1 -or $Branch[0] -cne 'master' -or
    $Remote.Count -ne 1 -or ($Remote[0] -split '\s+')[0] -cne $ExpectedHead -or
    $Dirty.Count -ne 0) {
    throw 'Source inventory requires clean local and remote master at ExpectedHead.'
}
$InputFile = [IO.Path]::GetFullPath($EvidencePath)
$Destination = [IO.Path]::GetFullPath($OutputPath)
$RepoFull = [IO.Path]::GetFullPath($RepoRoot).TrimEnd('\','/')
if (-not [IO.File]::Exists($InputFile) -or [IO.File]::Exists($Destination) -or
    $InputFile -ieq $Destination -or
    $Destination.StartsWith($RepoFull + '\',[StringComparison]::OrdinalIgnoreCase) -or
    $Destination.StartsWith($RepoFull + '/', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Existing private evidence and a new output outside the repository are required.'
}
$Evidence = Get-Content -LiteralPath $InputFile -Encoding UTF8 -Raw | ConvertFrom-Json
if ($Evidence.schema -cne 'ARM-CLEAN06.5A/retirement-evidence/v1' -or
    $Evidence.deletion_authorized -ne $false -or
    $Evidence.world_pointer_preflight -ne $true -or
    $Evidence.instance_schema_projection -ne $true -or
    [int]$Evidence.save_count -ne 7 -or [int]$Evidence.decode_failures -ne 0 -or
    [int]$Evidence.native.canonical_count -ne 48 -or
    [int]$Evidence.native.legacy_definition_count -ne 48 -or
    [int]$Evidence.native.candidate_asset_count -ne 53 -or
    [int]$Evidence.native.external_referencer_edges -ne 0 -or
    [int]$Evidence.native.candidate_role_counts.historical_item_definition -ne 48 -or
    [int]$Evidence.native.candidate_role_counts.historical_action_icon_palette -ne 0) {
    throw 'Evidence does not describe the verified native 53/48 post-palette baseline.'
}
foreach ($Save in @($Evidence.saves)) {
    if ($Save.decoded -ne $true -or $Save.canonical_world_valid -ne $true -or
        $Save.memory_roundtrip_equal -ne $true -or $Save.cache_normalized_roundtrip_equal -ne $true -or
        @($Save.unknown_item_ids).Count -ne 0) {
        throw 'Seven canonical save read-only proofs are incomplete.'
    }
}
$Candidates = @($Evidence.native.candidates | Where-Object { $_.retirement_role -ceq 'historical_item_definition' })
if ($Candidates.Count -ne 48) { throw 'Exactly 48 historical definitions required.' }
$ByStem = @{}
foreach ($Candidate in $Candidates) {
    $Stem = ([string]$Candidate.package -split '/')[-1]
    if ([string]::IsNullOrWhiteSpace($Stem) -or $ByStem.ContainsKey($Stem) -or
        $Candidate.canonical_id_exists -ne $true -or
        @($Candidate.external_referencers).Count -ne 0) {
        throw 'Duplicate, unresolved or externally referenced historical definition.'
    }
    $ByStem[$Stem] = @()
}
$Scanned = 0
foreach ($Dir in @('Source','Scripts','Config')) {
    $DirPath = Join-Path $RepoRoot $Dir
    foreach ($File in @(Get-ChildItem -LiteralPath $DirPath -Recurse -File |
        Where-Object { $_.Extension -in @('.h','.cpp','.inl','.ps1','.ini','.cs') })) {
        ++$Scanned
        $Relative = $File.FullName.Substring($RepoFull.Length + 1).Replace('\','/')
        $Kind = Get-ArmH23Kind $Relative
        $LineIndex = 0
        foreach ($Line in [IO.File]::ReadLines($File.FullName)) {
            ++$LineIndex
            foreach ($Stem in @(Get-ArmH23Stems $Line)) {
                if (-not $ByStem.ContainsKey($Stem)) { continue }
                $ByStem[$Stem] += [pscustomobject]@{
                    path = $Relative
                    line = $LineIndex
                    role = $Kind
                }
            }
        }
    }
}
$Rows = @($Candidates | Sort-Object package | ForEach-Object {
    $Candidate = $_
    $Stem = ([string]$Candidate.package -split '/')[-1]
    [pscustomobject]@{
        package = $Candidate.package
        item_id = $Candidate.item_id
        code_mentions = @($ByStem[$Stem])
        mention_count = @($ByStem[$Stem]).Count
        zero_text_mentions_does_not_authorize_deletion = $true
    }
})
$Report = [ordered]@{
    schema = 'ARM-CLEAN06.5H.23/private-static-retirement-blockers/v1'
    expected_head = $ExpectedHead
    evidence_not_bound_to_this_head = $true
    scanned_source_files = $Scanned
    historical_definitions = $Rows
    old_catalogue_sources_protected = 2
    saved_readables_protected = 2
    throwing_knife_redirector_requires_editor_fixup = $true
    deletion_authorized = $false
    next_step = 'Editor-only consumer migration; fresh native snapshot/audit then controlled deletion transaction.'
}
$Json = $Report | ConvertTo-Json -Depth 15
[IO.File]::WriteAllText($Destination, $Json, ([Text.UTF8Encoding]::new($false)))
Write-Host ('[OK] Source inventory saved privately: ' + $Destination)
Write-Host ('Historicals=' + $Rows.Count + ' scanned source files=' + $Scanned)
Write-Host '[BLOCKED] No native asset deletion authorized by source-text evidence.'
