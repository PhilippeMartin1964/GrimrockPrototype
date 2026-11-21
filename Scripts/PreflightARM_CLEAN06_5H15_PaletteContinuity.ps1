[CmdletBinding()]
param(
    [string]$ExpectedHead,
    [string]$EvidencePath,
    [string]$PaletteReceiptPath,
    [string]$OutputPath,
    [switch]$SelfTest
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'ARM_CLEAN06_4X66_Rollback.ps1')
$RepoRoot = Split-Path -Parent $PSScriptRoot
$PalettePackage = '/Game/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons'
$PaletteRelative = 'Content/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons.uasset'
$ReadableBase = '/Game/GrimrockPrototype/Core/DataAssets/Tests/Readables/'
$ExpectedRoles = [ordered]@{
    historical_item_definition = 48
    historical_catalogue_source = 2
    save_readable_fixture = 2
    historical_throwing_knife_redirector = 1
    historical_action_icon_palette = 1
    unclassified_keep_and_investigate = 0
}

function Get-ArmH15Hash([string]$Path) {
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}
function Assert-ArmH15Binary([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw 'Expected native palette binary is missing.'
    }
    Assert-ArmX66OrdinaryPath $Path
    $Info = Get-Item -LiteralPath $Path -Force -ErrorAction Stop
    if ($Info.Length -le 200 -or
        ($Info.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'Invalid native palette (missing, symlink or tiny LFS pointer).'
    }
    $Stream = [IO.File]::OpenRead($Path)
    try {
        $Bytes = New-Object byte[] 64
        $Count = $Stream.Read($Bytes, 0, $Bytes.Length)
        if ([Text.Encoding]::ASCII.GetString($Bytes, 0, $Count).StartsWith(
            'version https://git-lfs.github.com/spec/v1',
            [StringComparison]::Ordinal)) {
            throw 'Refusing Git LFS pointer in place of Unreal palette.'
        }
    } finally { $Stream.Dispose() }
    [pscustomobject]@{ bytes = [int64]$Info.Length; sha256 = Get-ArmH15Hash $Path }
}
function Assert-ArmH15Evidence($Evidence) {
    if ($Evidence.schema -cne 'ARM-CLEAN06.5A/retirement-evidence/v1' -or
        $Evidence.deletion_authorized -ne $false -or
        $Evidence.world_pointer_preflight -ne $true -or
        $Evidence.instance_cache_evidence -ne $true -or
        $Evidence.semantic_cache_evidence -ne $true -or
        $Evidence.instance_schema_projection -ne $true -or
        $Evidence.instance_cache_schema_retired -ne $true -or
        $Evidence.weight_cache_evidence -ne $false -or
        [int]$Evidence.save_count -ne 7 -or
        [int]$Evidence.decode_failures -ne 0 -or @($Evidence.saves).Count -ne 7) {
        throw 'Canary requires full v27 read-only evidence for seven saves.'
    }
    foreach ($Save in @($Evidence.saves)) {
        if ($Save.decoded -ne $true -or [int]$Save.version -ne 27 -or
            $Save.canonical_world_valid -ne $true -or
            $Save.memory_roundtrip_equal -ne $true -or
            $Save.cache_normalized_roundtrip_equal -ne $true -or
            [int]$Save.cache_invalid_identities -ne 0 -or
            [int]$Save.cache_projection_failures -ne 0 -or
            @($Save.unknown_item_ids).Count -ne 0) {
            throw 'One historical save did not preserve canonical round-trip parity.'
        }
    }
    $Native = $Evidence.native
    if ([int]$Native.canonical_count -ne 48 -or
        [int]$Native.armament_count -ne 22 -or
        [int]$Native.resolved_item_levels -ne 466 -or
        [int]$Native.legacy_definition_count -ne 48 -or
        [int]$Native.candidate_asset_count -ne 54 -or
        [int]$Native.unclassified_candidate_count -ne 0 -or
        [int]$Native.external_referencer_edges -ne 0 -or
        @($Native.legacy_ids_without_canonical).Count -ne 0 -or
        @($Native.canonical_ids_without_legacy_source).Count -ne 0 -or
        @($Native.candidates).Count -ne 54) {
        throw 'Canonical or native candidate counts disagree with 5H.14 receipt.'
    }
    foreach ($Role in @($ExpectedRoles.Keys)) {
        if ([int]$Native.candidate_role_counts.$Role -ne $ExpectedRoles[$Role]) {
            throw ('Changed retirement candidate role: ' + $Role)
        }
    }
    $Palette = @($Native.candidates | Where-Object {
        $_.package -ceq $PalettePackage -and
        $_.retirement_role -ceq 'historical_action_icon_palette'
    })
    if ($Palette.Count -ne 1) { throw 'Expected exactly one legacy icon palette.' }
    if ($Palette[0].referencer_query -ne $true -or
        @($Palette[0].all_referencers).Count -ne 0 -or
        @($Palette[0].hard_referencers).Count -ne 0 -or
        @($Palette[0].soft_referencers).Count -ne 0 -or
        @($Palette[0].external_referencers).Count -ne 0) {
        throw 'Icon palette still has registered referrers or graph query failed.'
    }
    $RoleCounts = @{}
    foreach ($Candidate in @($Native.candidates)) {
        if ([string]::IsNullOrWhiteSpace([string]$Candidate.package) -or
            -not $ExpectedRoles.Contains([string]$Candidate.retirement_role) -or
            $Candidate.referencer_query -ne $true -or
            @($Candidate.external_referencers).Count -ne 0) {
            throw 'Incomplete or externally referenced candidate.'
        }
        $K = [string]$Candidate.retirement_role
        if (-not $RoleCounts.ContainsKey($K)) { $RoleCounts[$K] = 0 }
        $RoleCounts[$K]++
    }
    foreach ($Role in @($ExpectedRoles.Keys)) {
        if ($RoleCounts.ContainsKey($Role)) {
            if ($RoleCounts[$Role] -ne $ExpectedRoles[$Role]) {
                throw ('Candidate array contradicts role counts: ' + $Role)
            }
        } elseif ($ExpectedRoles[$Role] -ne 0) {
            throw ('Missing role: ' + $Role)
        }
    }
    $Readables = @($Native.candidates | Where-Object {
        $_.retirement_role -ceq 'save_readable_fixture'
    })
    $Allowed = @(
        ($ReadableBase + 'DA_BookPage_Test'),
        ($ReadableBase + 'DA_Scroll_Test')
    )
    foreach ($Readable in $Readables) {
        if ($Allowed -cnotcontains [string]$Readable.package) {
            throw 'Unknown readable: do not retire it.'
        }
    }
    $Pointers = 0
    $PointerPaths = @()
    foreach ($Save in @($Evidence.saves)) {
        $Pointers += [int]$Save.readable_content_pointers
        foreach ($Ref in @($Save.decoded_references)) {
            if ($Ref.field -cne 'ReadableContentAsset' -or
                $Ref.identity_conflict -ne $false -or
                $Ref.object -cnotin @(
                    ($ReadableBase + 'DA_BookPage_Test.DA_BookPage_Test'),
                    ($ReadableBase + 'DA_Scroll_Test.DA_Scroll_Test')
                )) {
                throw 'Unknown narrative pointer, retained asset required.'
            }
            $PointerPaths += [string]$Ref.object
        }
    }
    if ($Pointers -ne 2 -or $PointerPaths.Count -ne 2 -or
        @($PointerPaths | Sort-Object -Unique).Count -ne 2) {
        throw 'Expected two distinct durable Readable references.'
    }
}
function Assert-ArmH15Receipt($Receipt, $Source, $Backup) {
    if ($Receipt.schema -cne 'ARM-CLEAN06.5H.14/native-palette-copy/v1' -or
        $Receipt.head -cnotmatch '^[0-9a-f]{40}$' -or
        $Receipt.package -cne $PalettePackage -or
        $Receipt.relative_path -cne $PaletteRelative -or
        $Receipt.deletion_authorized -ne $false -or
        $Receipt.source_unchanged -ne $true -or
        $Receipt.backup_byte_identical -ne $true -or
        $Receipt.sha256 -cnotmatch '^[0-9a-f]{64}$' -or
        [int64]$Receipt.bytes -le 200) {
        throw 'Palette backup receipt has an invalid or unsafe contract.'
    }
    foreach ($Info in @($Source, $Backup)) {
        if ([int64]$Info.bytes -ne [int64]$Receipt.bytes -or
            $Info.sha256 -cne [string]$Receipt.sha256) {
            throw 'Snapshot receipt, source and local backup hashes diverge.'
        }
    }
}

if ($SelfTest) {
    $Tmp = Join-Path ([IO.Path]::GetTempPath()) ('ArmH15-'+[guid]::NewGuid().ToString('N'))
    [void](New-Item -ItemType Directory -Path $Tmp -ErrorAction Stop)
    try {
        $Src = Join-Path $Tmp 'source.uasset'
        $Dest = Join-Path $Tmp 'backup.uasset'
        [IO.File]::WriteAllBytes($Src, [byte[]]@(0..255))
        Copy-Item -LiteralPath $Src -Destination $Dest -ErrorAction Stop
        $SrcState = Assert-ArmH15Binary $Src
        $BackupState = Assert-ArmH15Binary $Dest
        $Receipt = [pscustomobject]@{
            schema = 'ARM-CLEAN06.5H.14/native-palette-copy/v1'
            head = ('a' * 40); package = $PalettePackage
            relative_path = $PaletteRelative
            deletion_authorized = $false
            source_unchanged = $true; backup_byte_identical = $true
            bytes = $SrcState.bytes; sha256 = $SrcState.sha256
        }
        Assert-ArmH15Receipt $Receipt $SrcState $BackupState
        [IO.File]::WriteAllBytes($Dest, [byte[]]@(1..255))
        $Rejected = $false
        try { Assert-ArmH15Receipt $Receipt $SrcState (Assert-ArmH15Binary $Dest) }
        catch { $Rejected = $true }
        if (-not $Rejected) { throw 'Modified private palette copy accepted.' }

        $Candidates = @()
        foreach ($Role in @($ExpectedRoles.Keys)) {
            for ($i = 0; $i -lt $ExpectedRoles[$Role]; ++$i) {
                $P = '/Game/Fixture/' + $Role + '_' + $i
                if ($Role -ceq 'historical_action_icon_palette') { $P = $PalettePackage }
                if ($Role -ceq 'save_readable_fixture') {
                    $P = $ReadableBase + $(if ($i -eq 0) { 'DA_BookPage_Test' } else { 'DA_Scroll_Test' })
                }
                $Candidates += [pscustomobject]@{
                    package = $P; retirement_role = $Role; referencer_query = $true
                    all_referencers = @(); hard_referencers = @()
                    soft_referencers = @(); external_referencers = @()
                }
            }
        }
        $Saves = @(1..7 | ForEach-Object {
            $FixtureIndex = $_
            $Refs = @()
            if ($FixtureIndex -eq 1) {
                $Refs = @(
                    [pscustomobject]@{
                        field = 'ReadableContentAsset'
                        object = ($ReadableBase + 'DA_BookPage_Test.DA_BookPage_Test')
                        identity_conflict = $false
                    },
                    [pscustomobject]@{
                        field = 'ReadableContentAsset'
                        object = ($ReadableBase + 'DA_Scroll_Test.DA_Scroll_Test')
                        identity_conflict = $false
                    }
                )
            }
            [pscustomobject]@{
                decoded = $true; version = 27; canonical_world_valid = $true
                memory_roundtrip_equal = $true; cache_normalized_roundtrip_equal = $true
                cache_invalid_identities = 0; cache_projection_failures = 0
                unknown_item_ids = @()
                readable_content_pointers = $(if ($FixtureIndex -eq 1) { 2 } else { 0 })
                decoded_references = $Refs
            }
        })
        $Native = [pscustomobject]@{
            canonical_count = 48; armament_count = 22; resolved_item_levels = 466
            legacy_definition_count = 48; candidate_asset_count = 54
            unclassified_candidate_count = 0; external_referencer_edges = 0
            legacy_ids_without_canonical = @(); canonical_ids_without_legacy_source = @()
            candidate_role_counts = [pscustomobject]$ExpectedRoles; candidates = $Candidates
        }
        $Evidence = [pscustomobject]@{
            schema = 'ARM-CLEAN06.5A/retirement-evidence/v1'
            deletion_authorized = $false; world_pointer_preflight = $true
            instance_cache_evidence = $true; semantic_cache_evidence = $true
            instance_schema_projection = $true; instance_cache_schema_retired = $true
            weight_cache_evidence = $false
            save_count = 7; decode_failures = 0; saves = $Saves; native = $Native
        }
        Assert-ArmH15Evidence $Evidence
        $Candidates[-1].all_referencers = @('Injected.External')
        $Rejected = $false
        try { Assert-ArmH15Evidence $Evidence } catch { $Rejected = $true }
        if (-not $Rejected) { throw 'Unrecognized icon palette reference accepted.' }
    } finally {
        Remove-Item -LiteralPath $Tmp -Force -Recurse
    }
    Write-Host '[OK] H15 read-only receipt/graph evidence and negative tests.'
    return
}

if ($ExpectedHead -cnotmatch '^[0-9a-f]{40}$' -or
    [string]::IsNullOrWhiteSpace($EvidencePath) -or
    [string]::IsNullOrWhiteSpace($PaletteReceiptPath) -or
    [string]::IsNullOrWhiteSpace($OutputPath)) {
    throw 'Expected complete HEAD, -EvidencePath, -PaletteReceiptPath and -OutputPath.'
}
$Project = [IO.Path]::GetFullPath($RepoRoot).TrimEnd('\','/')
$EvidenceFull = [IO.Path]::GetFullPath($EvidencePath)
$ReceiptFull = [IO.Path]::GetFullPath($PaletteReceiptPath)
$OutputFull = [IO.Path]::GetFullPath($OutputPath)
$Folder = Split-Path -Parent $ReceiptFull
$BackupFull = Join-Path $Folder 'DA_EquippedWeaponActionIcons.uasset'
$SourceFull = Join-Path $Project $PaletteRelative.Replace('/', '\')
Assert-ArmX66OrdinaryPath $Folder
Assert-ArmX66OrdinaryPath (Split-Path -Parent $OutputFull)
foreach ($Path in @($EvidenceFull, $ReceiptFull, $BackupFull, $OutputFull)) {
    if ($Path.Equals($Project, [StringComparison]::OrdinalIgnoreCase) -or
        $Path.StartsWith($Project + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Evidence, copies and generated receipt must remain outside repository.'
    }
}
if ((Test-Path -LiteralPath $OutputFull) -or
    -not (Test-Path -LiteralPath $EvidenceFull -PathType Leaf) -or
    -not (Test-Path -LiteralPath $ReceiptFull -PathType Leaf)) {
    throw 'Need existing private inputs and a NEW, external output path.'
}
$Git = (Get-Command git.exe -CommandType Application -ErrorAction Stop |
    Select-Object -First 1).Source
function Invoke-ArmH15Git([string[]]$Arguments) {
    $Output = @(& $Git -C $RepoRoot @Arguments)
    if ($LASTEXITCODE -ne 0) { throw 'Git verification failed.' }
    $Output
}
$Local = @(Invoke-ArmH15Git @('rev-parse','HEAD'))
$Remote = @(Invoke-ArmH15Git @('ls-remote','origin','refs/heads/master'))
$Branch = @(Invoke-ArmH15Git @('branch','--show-current'))
$Dirty = @(Invoke-ArmH15Git @('status','--porcelain','--untracked-files=all'))
if ($Local.Count -ne 1 -or $Local[0] -cne $ExpectedHead -or
    $Branch.Count -ne 1 -or $Branch[0] -cne 'master' -or
    $Remote.Count -ne 1 -or ($Remote[0] -split '\s+')[0] -cne $ExpectedHead -or
    $Dirty.Count -ne 0) { throw 'Require clean master exactly equal to origin.' }
if (@(Get-Process -Name UnrealEditor,UnrealEditor-Cmd,GrimrockPrototype,
    GrimrockPrototype-Win64-Shipping -ErrorAction SilentlyContinue).Count -ne 0) {
    throw 'Close UE and the game before canary continuity preflight.'
}
$Evidence = Get-Content -LiteralPath $EvidenceFull -Raw -Encoding UTF8 | ConvertFrom-Json
$Receipt = Get-Content -LiteralPath $ReceiptFull -Raw -Encoding UTF8 | ConvertFrom-Json
Assert-ArmH15Evidence $Evidence
$Before = Assert-ArmH15Binary $SourceFull
$Copy = Assert-ArmH15Binary $BackupFull
Assert-ArmH15Receipt $Receipt $Before $Copy
# The H14 receipt can be an ancestor, not the current HEAD. Validate the
# SHA format in the receipt BEFORE passing it to Git as an argument.
& $Git -C $RepoRoot merge-base --is-ancestor $Receipt.head $ExpectedHead
if ($LASTEXITCODE -ne 0) { throw 'H14 backup receipt is not an ancestor of current master.' }
$After = Assert-ArmH15Binary $SourceFull
if ($After.sha256 -cne $Before.sha256 -or $After.bytes -ne $Before.bytes) {
    throw 'Source palette changed during preflight.'
}
if (@(Invoke-ArmH15Git @('status','--porcelain','--untracked-files=all')).Count -ne 0) {
    throw 'Checkout changed during read-only native preflight.'
}
$Output = [ordered]@{
    schema = 'ARM-CLEAN06.5H.15/palette-continuity/v1'
    head = $ExpectedHead
    backup_created_at_head = [string]$Receipt.head
    palette_package = $PalettePackage
    palette_bytes = [int64]$Before.bytes
    palette_sha256 = $Before.sha256
    private_receipt_sha256 = Get-ArmH15Hash $ReceiptFull
    private_evidence_sha256 = Get-ArmH15Hash $EvidenceFull
    snapshot_still_byte_identical = $true
    asset_registry_preflight_zero_referrers = $true
    seven_saves_roundtrip_valid = $true
    evidence_has_no_attested_producer_head = $true
    deletion_authorized = $false
    requires_fresh_transaction_sha_audit = $true
    next_step = 'Native Unreal Editor-only deletion transaction, then PIE, Save/Continue, Cook and rollback checks.'
}
$Output | ConvertTo-Json -Depth 5 |
    Set-Content -LiteralPath $OutputFull -Encoding UTF8 -ErrorAction Stop
Write-Host ('[OK] H15 private palette continuity preflight: ' + $OutputFull)
Write-Host ('[OK] Native SHA256: ' + $Before.sha256)
Write-Host '[BLOCKED] No native deletion or SaveGame mutation authorized.'
