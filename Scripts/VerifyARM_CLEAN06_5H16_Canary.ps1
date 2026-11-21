[CmdletBinding()]
param(
    [string]$ExpectedHead,
    [string]$FreshEvidencePath,
    [string]$CopyReceiptPath,
    [string]$PaletteReceiptPath,
    [string]$OutputPath,
    [string]$EngineRoot='D:\UE_5.5',
    [switch]$SelfTest
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'ARM_CLEAN06_4X66_Rollback.ps1')
$Root=Split-Path -Parent $PSScriptRoot
$Relative='Content/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons.uasset'
$Package='/Game/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons'
$Source=Join-Path $Root $Relative.Replace('/','\')
$Live=Join-Path $Root 'Saved\SaveGames'

function Get-H16Hash([string]$Path) {
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}
function Assert-H16Evidence($Data,[bool]$Absent) {
    $PaletteCount=if ($Absent) {0} else {1}
    $ExpectedCount=53+$PaletteCount
    if ($Data.schema -cne 'ARM-CLEAN06.5A/retirement-evidence/v1' -or
        $Data.deletion_authorized -ne $false -or
        $Data.world_pointer_preflight -ne $true -or
        $Data.instance_cache_evidence -ne $true -or
        $Data.semantic_cache_evidence -ne $true -or
        $Data.instance_schema_projection -ne $true -or
        $Data.instance_cache_schema_retired -ne $true -or
        $Data.weight_cache_evidence -ne $false -or
        [int]$Data.save_count -ne 7 -or [int]$Data.decode_failures -ne 0 -or
        @($Data.saves).Count -ne 7) {
        throw 'Incomplete private save evidence.'
    }
    $N=$Data.native
    if ([int]$N.canonical_count -ne 48 -or [int]$N.armament_count -ne 22 -or
        [int]$N.resolved_item_levels -ne 466 -or
        [int]$N.legacy_definition_count -ne 48 -or
        [int]$N.candidate_asset_count -ne $ExpectedCount -or
        @($N.candidates).Count -ne $ExpectedCount -or
        [int]$N.unclassified_candidate_count -ne 0 -or
        [int]$N.external_referencer_edges -ne 0 -or
        @($N.legacy_ids_without_canonical).Count -ne 0 -or
        @($N.canonical_ids_without_legacy_source).Count -ne 0) {
        throw 'Unexpected canonical inventory or native packages.'
    }
    $Roles=@{
        historical_item_definition=48;historical_catalogue_source=2
        save_readable_fixture=2;historical_throwing_knife_redirector=1
        historical_action_icon_palette=$PaletteCount;unclassified_keep_and_investigate=0
    }
    foreach ($Key in @($Roles.Keys)) {
        if ([int]$N.candidate_role_counts.$Key -ne $Roles[$Key] -or
            @($N.candidates | Where-Object { $_.retirement_role -ceq $Key }).Count -ne $Roles[$Key]) {
            throw ('Changed candidate category: '+$Key)
        }
    }
    $Palette=@($N.candidates | Where-Object { $_.package -ceq $Package })
    if ($Palette.Count -ne $PaletteCount -or
        ($Palette.Count -eq 1 -and
        ($Palette[0].referencer_query -ne $true -or
        @($Palette[0].all_referencers).Count -ne 0 -or
        @($Palette[0].hard_referencers).Count -ne 0 -or
        @($Palette[0].soft_referencers).Count -ne 0))) {
        throw 'Icon palette still referenced or unexpectedly present.'
    }
    foreach ($Candidate in @($N.candidates)) {
        if ($Candidate.referencer_query -ne $true -or
            @($Candidate.external_referencers).Count -ne 0) {
            throw 'AssetRegistry graph incomplete or externally referenced.'
        }
    }
    $Readables=@($N.candidates | Where-Object {
        $_.retirement_role -ceq 'save_readable_fixture'
    })
    $Allowed=@(
        '/Game/GrimrockPrototype/Core/DataAssets/Tests/Readables/DA_Scroll_Test',
        '/Game/GrimrockPrototype/Core/DataAssets/Tests/Readables/DA_BookPage_Test'
    )
    if ($Readables.Count -ne 2 -or
        @($Readables | Where-Object { $Allowed -cnotcontains $_.package }).Count -ne 0) {
        throw 'Persisted Readable fixtures must be preserved.'
    }
    $Pointers=0
    foreach ($Save in @($Data.saves)) {
        if ($Save.decoded -ne $true -or [int]$Save.version -ne 27 -or
            $Save.canonical_world_valid -ne $true -or
            $Save.memory_roundtrip_equal -ne $true -or
            $Save.cache_normalized_roundtrip_equal -ne $true -or
            [int]$Save.cache_invalid_identities -ne 0 -or
            [int]$Save.cache_projection_failures -ne 0 -or
            @($Save.unknown_item_ids).Count -ne 0) {
            throw 'Saved-game canonical identity or memory parity failure.'
        }
        $Pointers += [int]$Save.readable_content_pointers
    }
    if ($Pointers -ne 2) {throw 'Durable Readable references changed.'}
}
function Assert-H16ExternalFile([string]$Path) {
    if ([string]::IsNullOrWhiteSpace($Path)) {throw 'Missing private path.'}
    $Full=[IO.Path]::GetFullPath($Path)
    $Project=[IO.Path]::GetFullPath($Root).TrimEnd('\','/')
    if ($Full.Equals($Project,[StringComparison]::OrdinalIgnoreCase) -or
        $Full.StartsWith($Project+'\',[StringComparison]::OrdinalIgnoreCase) -or
        -not (Test-Path -LiteralPath $Full -PathType Leaf)) {
        throw 'Private evidence file missing or located inside repository.'
    }
    Assert-ArmX66OrdinaryPath (Split-Path -Parent $Full)
    $Full
}
function Get-H16SaveInventory([string]$Folder) {
    Assert-ArmX66OrdinaryPath $Folder
    if (-not (Test-Path -LiteralPath $Folder -PathType Container)) {
        throw 'Saved games inventory directory absent.'
    }
    $Rows=@()
    $Files=@(Get-ChildItem -LiteralPath $Folder -Force | Sort-Object Name)
    if ($Files.Count -ne 7) {throw 'Expected exactly seven private saved games.'}
    foreach ($File in $Files) {
        if ($File.PSIsContainer -or $File.Name -cnotmatch '^[A-Za-z0-9_-]+\.sav$') {
            throw 'Unknown SaveGames entry.'
        }
        Assert-ArmX66OrdinaryPath $File.FullName
        $Rows += [pscustomobject]@{
            name=$File.Name;bytes=[int64]$File.Length;sha256=Get-H16Hash $File.FullName
        }
    }
    ConvertTo-Json -InputObject $Rows -Depth 5 -Compress
}
if ($SelfTest) {
    $Roles=@{
        historical_item_definition=48;historical_catalogue_source=2
        save_readable_fixture=2;historical_throwing_knife_redirector=1
        historical_action_icon_palette=1;unclassified_keep_and_investigate=0
    }
    $Candidates=@()
    foreach ($Role in @($Roles.Keys)) {
        for ($I=0; $I -lt $Roles[$Role]; ++$I) {
            $Path='/Game/Fixture/'+$Role+'_'+$I
            if ($Role -eq 'historical_action_icon_palette') {$Path=$Package}
            if ($Role -eq 'save_readable_fixture') {
                $Path=@(
                    '/Game/GrimrockPrototype/Core/DataAssets/Tests/Readables/DA_Scroll_Test',
                    '/Game/GrimrockPrototype/Core/DataAssets/Tests/Readables/DA_BookPage_Test'
                )[$I]
            }
            $Candidates += [pscustomobject]@{
                package=$Path;retirement_role=$Role;referencer_query=$true
                all_referencers=@();hard_referencers=@();soft_referencers=@()
                external_referencers=@()
            }
        }
    }
    $Saves=@(1..7 | ForEach-Object {
        [pscustomobject]@{
            decoded=$true;version=27;canonical_world_valid=$true
            memory_roundtrip_equal=$true;cache_normalized_roundtrip_equal=$true
            cache_invalid_identities=0;cache_projection_failures=0
            unknown_item_ids=@();readable_content_pointers=$(if ($_ -eq 1) {2} else {0})
        }
    })
    $N=[pscustomobject]@{
        canonical_count=48;armament_count=22;resolved_item_levels=466
        legacy_definition_count=48;candidate_asset_count=54;unclassified_candidate_count=0
        external_referencer_edges=0;legacy_ids_without_canonical=@()
        canonical_ids_without_legacy_source=@()
        candidate_role_counts=[pscustomobject]$Roles;candidates=$Candidates
    }
    $E=[pscustomobject]@{
        schema='ARM-CLEAN06.5A/retirement-evidence/v1'
        deletion_authorized=$false;world_pointer_preflight=$true
        instance_cache_evidence=$true;semantic_cache_evidence=$true
        instance_schema_projection=$true;instance_cache_schema_retired=$true
        weight_cache_evidence=$false;save_count=7;decode_failures=0
        saves=$Saves;native=$N
    }
    Assert-H16Evidence $E $false
    $N.candidates=@($Candidates | Where-Object { $_.package -cne $Package })
    $N.candidate_role_counts.historical_action_icon_palette=0
    $N.candidate_asset_count=53
    Assert-H16Evidence $E $true
    $N.candidates[0].external_referencers=@('BadExternal')
    $Denied=$false
    try {Assert-H16Evidence $E $true} catch {$Denied=$true}
    if (-not $Denied) {throw 'Unsafe external reference accepted.'}
    $N.candidates[0].external_referencers=@()
    $Saves[0].memory_roundtrip_equal=$false
    $Denied=$false
    try {Assert-H16Evidence $E $true} catch {$Denied=$true}
    if (-not $Denied) {throw 'Corrupted save accepted.'}
    Write-Host '[OK] H16 54/53 canary assertions and negative fixtures.'
    return
}
if ($ExpectedHead -cnotmatch '^[0-9a-f]{40}$') {
    throw 'Expected exact master SHA.'
}
$Git=(Get-Command git.exe -CommandType Application -ErrorAction Stop | Select-Object -First 1).Source
$Current=@(& $Git -C $Root rev-parse HEAD)
$Branch=@(& $Git -C $Root branch --show-current)
$Remote=@(& $Git -C $Root ls-remote origin refs/heads/master)
if ($Current.Count -ne 1 -or $Current[0] -cne $ExpectedHead -or
    $Branch.Count -ne 1 -or $Branch[0] -cne 'master' -or
    $Remote.Count -ne 1 -or ($Remote[0] -split '\s+')[0] -cne $ExpectedHead) {
    throw 'Origin/master and local master are not on exact transaction HEAD.'
}
if (@(Get-Process -Name UnrealEditor,UnrealEditor-Cmd,GrimrockPrototype,
    GrimrockPrototype-Win64-Shipping -ErrorAction SilentlyContinue).Count -gt 0) {
    throw 'Close Unreal Editor and game before running postflight.'
}
$Work=@(& $Git -C $Root diff --name-status --no-renames)
$Index=@(& $Git -C $Root diff --cached --name-status --no-renames)
$Untracked=@(& $Git -C $Root ls-files --others --exclude-standard)
if ($Work.Count -ne 1 -or $Work[0] -cne ('D'+[char]9+$Relative) -or
    $Index.Count -ne 0 -or $Untracked.Count -ne 0 -or
    (Test-Path -LiteralPath $Source)) {
    throw 'Only the UNSTAGED deletion of the icon palette is allowed.'
}
$PreFile=Assert-H16ExternalFile $FreshEvidencePath
$CopyFile=Assert-H16ExternalFile $CopyReceiptPath
$PaletteFile=Assert-H16ExternalFile $PaletteReceiptPath
$Output=[IO.Path]::GetFullPath($OutputPath)
$Project=[IO.Path]::GetFullPath($Root).TrimEnd('\','/')
if ($Output.StartsWith($Project+'\',[StringComparison]::OrdinalIgnoreCase) -or
    (Test-Path -LiteralPath $Output) -or
    [IO.Path]::GetExtension($Output) -cne '.json') {
    throw 'Output must be a NEW JSON outside the repository.'
}
Assert-ArmX66OrdinaryPath (Split-Path -Parent $Output)
$Copies=Join-Path (Split-Path -Parent $CopyFile) 'AuditCopies'
if ((Split-Path -Parent $PreFile) -cne (Split-Path -Parent $CopyFile)) {
    throw 'Fresh pre-audit and save-copy receipt must originate from the same private audit run.'
}
$Pre=Get-Content -LiteralPath $PreFile -Raw -Encoding UTF8 | ConvertFrom-Json
$Copy=Get-Content -LiteralPath $CopyFile -Raw -Encoding UTF8 | ConvertFrom-Json
$Palette=Get-Content -LiteralPath $PaletteFile -Raw -Encoding UTF8 | ConvertFrom-Json
if ($Copy.schema -cne 'ARM-CLEAN06.5A/source-copy-receipt/v1' -or
    $Copy.head -cne $ExpectedHead -or
    $Palette.schema -cne 'ARM-CLEAN06.5H.14/native-palette-copy/v1' -or
    $Palette.package -cne $Package -or
    $Palette.deletion_authorized -ne $false -or
    [int64]$Palette.bytes -ne 4340 -or
    $Palette.sha256 -cne '3ac913df8d755a38db741c7c40aa1707990a5f0869554f6982521f0d6ccb2749') {
    throw 'Transaction is not tied to fresh private audit and protected H14 palette.'
}
& $Git -C $Root merge-base --is-ancestor $Palette.head $ExpectedHead
if ($LASTEXITCODE -ne 0) {throw 'Native backup receipt is not an ancestor of master.'}
$Backup=Join-Path (Split-Path -Parent $PaletteFile) 'DA_EquippedWeaponActionIcons.uasset'
Assert-ArmX66OrdinaryPath $Backup
if (-not (Test-Path -LiteralPath $Backup -PathType Leaf) -or
    (Get-Item -LiteralPath $Backup).Length -ne [int64]$Palette.bytes -or
    (Get-H16Hash $Backup) -cne $Palette.sha256) {
    throw 'Private native palette binary does not match H14 verified snapshot.'
}
Assert-H16Evidence $Pre $false
$Original=Get-H16SaveInventory $Live
if (@($Copy.files).Count -ne 7) {throw 'Fresh private copy receipt is incomplete.'}
$Recorded=@($Copy.files | Sort-Object name | ForEach-Object {
    [pscustomobject]@{
        name=[string]$_.name
        bytes=[int64]$_.bytes
        sha256=[string]$_.sha256
    }
})
if ((ConvertTo-Json -InputObject $Recorded -Depth 5 -Compress) -cne $Original) {
    throw 'Current SaveGames differ from the original source-copy receipt.'
}
if ((Get-H16SaveInventory $Copies) -cne $Original) {
    throw 'Private audit copies are not byte identical to live saves.'
}
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$ProjectFile=Join-Path $Root 'GrimrockPrototype.uproject'
$Post=Join-Path (Split-Path -Parent $Output) ('h16-post-'+[guid]::NewGuid().ToString('N')+'.json')
$Log=[IO.Path]::ChangeExtension($Post,'.log')
$Args=@($ProjectFile,'-run=GridArmClean065RetirementAudit','-ReadOnly',
    '-WorldPointerPreflight','-InstanceCacheEvidence','-SemanticCacheEvidence',
    '-InstanceSchemaProjection',"-Copies=$Copies","-Output=$Post",
    '-Unattended','-NoP4','-NoSplash','-NoSound','-NullRHI',"-abslog=$Log")
& $Editor @Args
$Code=$LASTEXITCODE
if ($Code -ne 0 -or -not (Test-Path -LiteralPath $Post -PathType Leaf)) {
    throw 'Native postflight commandlet failed; do not commit deletion.'
}
$After=Get-Content -LiteralPath $Post -Raw -Encoding UTF8 | ConvertFrom-Json
Assert-H16Evidence $After $true
if ((Get-H16SaveInventory $Live) -cne $Original -or
    (Get-H16SaveInventory $Copies) -cne $Original -or
    @(& $Git -C $Root diff --name-status --no-renames).Count -ne 1 -or
    @(& $Git -C $Root diff --cached --name-status --no-renames).Count -ne 0 -or
    @(& $Git -C $Root ls-files --others --exclude-standard).Count -ne 0) {
    throw 'SaveGames or Git checkout changed during native postflight.'
}
[ordered]@{
    schema='ARM-CLEAN06.5H.16/post-retirement-read-only/v1'
    before_head=$ExpectedHead;icon_removed_from_registry=$true
    candidates_before=54;candidates_after=53
    preserved_legacy_definitions=48;preserved_readables=2
    seven_save_roundtrips_valid=$true;save_files_byte_identical=$true
    private_palette_backup_unchanged=$true
    fresh_pre_evidence_sha256=Get-H16Hash $PreFile
    post_evidence_sha256=Get-H16Hash $Post
    deletion_authorized=$false
    native_editor_deleted_only_palette=$true
    commit_and_push_not_performed=$true
} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $Output -Encoding UTF8
Write-Host ('[OK] Native canary 54 -> 53 verified: '+$Output)
Write-Host '[STOP] Do NOT commit or push until postflight receipt is reviewed.'
