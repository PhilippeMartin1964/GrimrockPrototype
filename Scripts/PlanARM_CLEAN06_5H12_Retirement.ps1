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

$RequiredRoles = [ordered]@{
    historical_item_definition = 48
    historical_catalogue_source = 2
    save_readable_fixture = 2
    historical_throwing_knife_redirector = 1
    historical_action_icon_palette = 1
    unclassified_keep_and_investigate = 0
}
$ProtectedReadables = @(
    '/Game/GrimrockPrototype/Core/DataAssets/Tests/Readables/DA_BookPage_Test.DA_BookPage_Test',
    '/Game/GrimrockPrototype/Core/DataAssets/Tests/Readables/DA_Scroll_Test.DA_Scroll_Test'
)

function Assert-ArmH12Evidence($Evidence) {
    if ($Evidence.schema -cne 'ARM-CLEAN06.5A/retirement-evidence/v1' -or
        $Evidence.deletion_authorized -ne $false -or
        $Evidence.world_pointer_preflight -ne $true -or
        $Evidence.instance_cache_evidence -ne $true -or
        $Evidence.semantic_cache_evidence -ne $true -or
        $Evidence.instance_schema_projection -ne $true -or
        $Evidence.instance_cache_schema_retired -ne $true -or
        $Evidence.weight_cache_evidence -ne $false) {
        throw 'Not a fully executed, non-destructive post-cutover report.'
    }
    $Native = $Evidence.native
    if ([int]$Native.canonical_count -ne 48 -or [int]$Native.armament_count -ne 22 -or
        [int]$Native.resolved_item_levels -ne 466 -or
        [int]$Native.legacy_definition_count -ne 48 -or
        [int]$Native.candidate_asset_count -ne 54 -or
        [int]$Native.external_referencer_edges -ne 0 -or
        [int]$Native.unclassified_candidate_count -ne 0 -or
        $Native.native_root_dirty -ne $false -or
        @($Native.legacy_ids_without_canonical).Count -ne 0 -or
        @($Native.canonical_ids_without_legacy_source).Count -ne 0) {
        throw 'Native canonical identity or candidate inventory differs.'
    }
    foreach ($Role in @($RequiredRoles.Keys)) {
        if ([int]$Native.candidate_role_counts.$Role -ne $RequiredRoles[$Role]) {
            throw ('Unexpected retirement role: ' + $Role)
        }
    }
    $Candidates = @($Native.candidates)
    if ($Candidates.Count -ne 54) { throw 'Exactly 54 classified candidates required.' }
    $Seen = @{}
    foreach ($Candidate in $Candidates) {
        $Package = [string]$Candidate.package
        $Role = [string]$Candidate.retirement_role
        if ([string]::IsNullOrWhiteSpace($Package) -or $Seen.ContainsKey($Package) -or
            -not $RequiredRoles.Contains($Role) -or
            $Candidate.referencer_query -ne $true -or
            @($Candidate.external_referencers).Count -ne 0) {
            throw 'Candidate has duplicate path, missing role or unverified graph.'
        }
        $Seen[$Package] = $true
        if ($Role -eq 'historical_item_definition' -and
            ($Candidate.canonical_id_exists -ne $true -or
             [string]::IsNullOrWhiteSpace([string]$Candidate.item_id) -or
             [string]$Candidate.class -cne '/Script/GrimrockPrototype.GridItemDefinitionAsset')) {
            throw 'Historical item definition is not an exact canonical identity.'
        }
        if ($Role -eq 'save_readable_fixture' -and
            -not ($ProtectedReadables -ccontains [string]$Candidate.object)) {
            throw 'Unrecognized save-readable fixture.'
        }
    }
    if ([int]$Evidence.save_count -ne 7 -or [int]$Evidence.decode_failures -ne 0 -or
        @($Evidence.saves).Count -ne 7) {
        throw 'Seven completely decoded v27 saves are required.'
    }
    $PhysicalRows = 0
    $ReadablePointers = 0
    $ReferencedReadables = @()
    foreach ($Save in @($Evidence.saves)) {
        if ($Save.decoded -ne $true -or [int]$Save.version -ne 27 -or
            $Save.canonical_world_valid -ne $true -or
            $Save.memory_roundtrip_equal -ne $true -or
            $Save.cache_normalized_roundtrip_equal -ne $true -or
            [int]$Save.cache_invalid_identities -ne 0 -or
            [int]$Save.cache_projection_failures -ne 0 -or
            @($Save.unknown_item_ids).Count -ne 0 -or
            @($Save.durable_save_projection).Count -ne 10) {
            throw 'A v27 SaveGame failed canonical identity or full in-memory parity.'
        }
        $PhysicalRows += [int]$Save.item_instances_and_world_rows
        $ReadablePointers += [int]$Save.readable_content_pointers
        foreach ($Reference in @($Save.decoded_references)) {
            if ($Reference.field -cne 'ReadableContentAsset' -or
                $Reference.identity_conflict -ne $false -or
                -not ($ProtectedReadables -ccontains [string]$Reference.object)) {
                throw 'Unknown or conflicting durable narrative asset pointer.'
            }
            $ReferencedReadables += [string]$Reference.object
        }
    }
    if ($PhysicalRows -ne 145 -or $ReadablePointers -ne 2 -or
        $ReferencedReadables.Count -ne 2 -or
        @($ReferencedReadables | Sort-Object -Unique).Count -ne 2) {
        throw 'Observed physical-row or protected Readable evidence has changed.'
    }
    return [pscustomobject]@{
        historical_definitions = @($Candidates | Where-Object {
            $_.retirement_role -eq 'historical_item_definition'
        } | ForEach-Object { $_.package } | Sort-Object)
        protected_and_deferred = @($Candidates | Where-Object {
            $_.retirement_role -ne 'historical_item_definition'
        } | ForEach-Object { [pscustomobject]@{
            package = $_.package
            role = $_.retirement_role
        } } | Sort-Object package)
        physical_rows = $PhysicalRows
        readable_pointers = $ReadablePointers
    }
}

function Get-ArmH12SourceMentions([string]$Root) {
    # These are *source-text leads*, including possible comments and tests,
    # NOT a complete C++ call graph or a licence to delete unmentioned assets.
    $Needles = @(
        'LoadObject<UGridItemDefinitionAsset>',
        'DA_WeaponCatalogue',
        'DA_WeaponVisualCatalogue',
        'DA_EquippedWeaponActionIcons',
        'DA_Weapon_ThrowingKnife'
    )
    $Hits = New-Object 'System.Collections.Generic.List[object]'
    foreach ($Subdir in @('Source', 'Scripts')) {
        $Directory = Join-Path $Root $Subdir
        foreach ($File in @(Get-ChildItem -LiteralPath $Directory -Recurse -File |
            Where-Object { $_.Extension -in @('.cpp', '.h', '.ps1') } |
            Sort-Object FullName)) {
            if ($File.Name -eq 'PlanARM_CLEAN06_5H12_Retirement.ps1') { continue }
            $Matches = @(Select-String -LiteralPath $File.FullName -Pattern $Needles -SimpleMatch)
            if ($Matches.Count -eq 0) { continue }
            $Relative = $File.FullName.Substring($Root.TrimEnd('\', '/').Length + 1).Replace('\', '/')
            [void]$Hits.Add([pscustomobject]@{
                path = $Relative
                matching_lines = @($Matches | ForEach-Object { $_.LineNumber } | Sort-Object -Unique)
            })
        }
    }
    return @($Hits.ToArray())
}

if ($SelfTest) {
    $Candidates = @()
    foreach ($Role in @($RequiredRoles.Keys)) {
        for ($Index = 0; $Index -lt $RequiredRoles[$Role]; ++$Index) {
            $Object = '/Game/Fixture/' + $Role + '_' + $Index
            if ($Role -eq 'save_readable_fixture') { $Object = $ProtectedReadables[$Index] }
            $Candidates += [pscustomobject]@{
                package = '/Game/Fixture/' + $Role + '_' + $Index
                object = $Object
                retirement_role = $Role
                referencer_query = $true
                external_referencers = @()
                canonical_id_exists = $true
                item_id = 'Fixture_' + $Index
                class = '/Script/GrimrockPrototype.GridItemDefinitionAsset'
            }
        }
    }
    $Saves = @(1..7 | ForEach-Object {
        $FixtureIndex = $_
        $FixtureRefs = @()
        if ($FixtureIndex -eq 1) {
            $FixtureRefs = @($ProtectedReadables | ForEach-Object {
                [pscustomobject]@{field = 'ReadableContentAsset'; object = $_; identity_conflict = $false}
            })
        }
        [pscustomobject]@{
            decoded = $true; version = 27; canonical_world_valid = $true
            memory_roundtrip_equal = $true; cache_normalized_roundtrip_equal = $true
            cache_invalid_identities = 0; cache_projection_failures = 0
            unknown_item_ids = @(); durable_save_projection = @(1..10)
            item_instances_and_world_rows = $(if ($FixtureIndex -eq 1) {145} else {0})
            readable_content_pointers = $(if ($FixtureIndex -eq 1) {2} else {0})
            decoded_references = $FixtureRefs
        }
    })
    $Evidence = [pscustomobject]@{
        schema = 'ARM-CLEAN06.5A/retirement-evidence/v1'
        deletion_authorized = $false; world_pointer_preflight = $true
        instance_cache_evidence = $true; semantic_cache_evidence = $true
        instance_schema_projection = $true; instance_cache_schema_retired = $true
        weight_cache_evidence = $false; save_count = 7; decode_failures = 0
        native = [pscustomobject]@{
            canonical_count = 48; armament_count = 22; resolved_item_levels = 466
            legacy_definition_count = 48; candidate_asset_count = 54
            external_referencer_edges = 0; unclassified_candidate_count = 0
            native_root_dirty = $false; legacy_ids_without_canonical = @()
            canonical_ids_without_legacy_source = @(); candidate_role_counts = $RequiredRoles
            candidates = $Candidates
        }
        saves = $Saves
    }
    $Valid = Assert-ArmH12Evidence $Evidence
    if ($Valid.historical_definitions.Count -ne 48 -or
        $Valid.protected_and_deferred.Count -ne 6 -or $Valid.physical_rows -ne 145) {
        throw 'Valid retirement fixture rejected.'
    }
    $Evidence.saves[0].memory_roundtrip_equal = $false
    $Rejected = $false
    try { [void](Assert-ArmH12Evidence $Evidence) } catch { $Rejected = $true }
    if (-not $Rejected) { throw 'An altered SaveGame parity was accepted.' }
    $Evidence.saves[0].memory_roundtrip_equal = $true
    $Evidence.native.candidates[0].external_referencers = @('Outside')
    $Rejected = $false
    try { [void](Assert-ArmH12Evidence $Evidence) } catch { $Rejected = $true }
    if (-not $Rejected) { throw 'An external package referencer was accepted.' }
    Write-Host '[OK] 5H12 fully isolated evidence gate: positive and two negative fixtures.'
    return
}

if ([string]::IsNullOrWhiteSpace($EvidencePath) -or
    [string]::IsNullOrWhiteSpace($OutputPath) -or
    $ExpectedHead -cnotmatch '^[0-9a-f]{40}$') {
    throw 'Expected -EvidencePath, -OutputPath and 40-char -ExpectedHead.'
}
$Git = (Get-Command git.exe -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
$LocalHead = @(& $Git -C $RepoRoot rev-parse HEAD)
$Remote = @(& $Git -C $RepoRoot ls-remote origin refs/heads/master)
$Dirty = @(& $Git -C $RepoRoot status --porcelain --untracked-files=all)
if ($LASTEXITCODE -ne 0 -or $LocalHead.Count -ne 1 -or
    $LocalHead[0] -cne $ExpectedHead -or
    $Remote.Count -ne 1 -or
    ($Remote[0] -split '\s+')[0] -cne $ExpectedHead -or
    $Dirty.Count -ne 0) {
    throw 'Read-only planner requires clean master at the exact remote SHA.'
}
$InputFile = [IO.Path]::GetFullPath($EvidencePath)
$Destination = [IO.Path]::GetFullPath($OutputPath)
$Project = [IO.Path]::GetFullPath($RepoRoot).TrimEnd('\', '/')
if (-not [IO.File]::Exists($InputFile) -or [IO.File]::Exists($Destination) -or
    $InputFile -ieq $Destination -or
    $Destination.StartsWith($Project + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Existing private evidence and a new external output path are mandatory.'
}
$Evidence = Get-Content -LiteralPath $InputFile -Raw -Encoding UTF8 | ConvertFrom-Json
$Validated = Assert-ArmH12Evidence $Evidence
$SourceMentions = @(Get-ArmH12SourceMentions $Project)
$Plan = [ordered]@{
    schema = 'ARM-CLEAN06.5H.12/read-only-retirement-plan/v1'
    expected_head = $ExpectedHead
    evidence_modes_complete = $true
    files_decoded = 7
    physical_rows = $Validated.physical_rows
    candidate_definitions_not_approved_for_deletion = $Validated.historical_definitions
    protected_or_deferred_packages = $Validated.protected_and_deferred
    source_text_mentions_not_an_exhaustive_dependency_graph = $SourceMentions
    source_mention_file_count = $SourceMentions.Count
    deletion_authorized = $false
    next_step = 'Native editor backup, hard/soft/source-reference closure, PIE, Cook, Save/Continue before any delete.'
}
$Plan | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $Destination -Encoding UTF8
Write-Host ('[OK] Read-only plan : ' + $Destination)
Write-Host ('Native historical definitions : ' + $Validated.historical_definitions.Count)
Write-Host ('Static source mention files   : ' + $SourceMentions.Count)
Write-Host '[BLOCKED] No asset deletion authorized by this plan.'
