# ARM-CLEAN06.2E.3 — dot-sourced after the native UE census has been validated.
# Parameters are supplied by PreflightARM_CLEAN06_2_HandSemantics.ps1:
# $WorktreePath, $OutDir, $Report, $ExpectedHead, $Stage.
# Never change the source or publish arbitrary UE asset names/classes.
$ManifestPath=Join-Path $WorktreePath 'Config\Armement\WeaponProductionManifest.csv'
if (-not (Test-Path -LiteralPath $ManifestPath -PathType Leaf)) {
    throw 'Production manifest missing.'
}
$Manifest=@(Import-Csv -LiteralPath $ManifestPath -Encoding UTF8)
$ByManifestPackage=@{}
$ManifestItemIds=@{}
$ManifestArmIds=@{}
foreach ($Entry in $Manifest) {
    $Pkg=([string]$Entry.ItemPath -split '\.')[0]
    $ItemId=[string]$Entry.ItemDefinitionId
    $Arm=[string]$Entry.ArchetypeId
    if ($Pkg -notlike '/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_*' -or
        [string]::IsNullOrWhiteSpace($ItemId) -or
        $Arm -notmatch '^ARM(00[1-9]|0[1-7][0-9]|08[0-2])$' -or
        $ByManifestPackage.ContainsKey($Pkg) -or
        $ManifestItemIds.ContainsKey($ItemId) -or
        $ManifestArmIds.ContainsKey($Arm)) {
        throw 'Production manifest contains invalid or duplicate keys.'
    }
    $ByManifestPackage[$Pkg]=$Entry
    $ManifestItemIds[$ItemId]=$true
    $ManifestArmIds[$Arm]=$true
}
if ($Manifest.Count -ne 22 -or $ByManifestPackage.Count -ne 22) {
    throw 'Production manifest must contain exactly 22 distinct rows.'
}

$AllSourceArms=@{}
foreach ($Row in @($Report.source_archetype_rows)) {
    $Arm=[string]$Row.archetype_id
    if ([string]::IsNullOrWhiteSpace($Arm) -or $AllSourceArms.ContainsKey($Arm)) {
        throw 'Native ARM source inventory invalid.'
    }
    $AllSourceArms[$Arm]=$true
}
$ByPackage=@{}
foreach ($Row in @($Report.concrete_weapon_rows)) {
    $Pkg=[string]$Row.asset_package
    if ([string]::IsNullOrWhiteSpace($Pkg) -or $ByPackage.ContainsKey($Pkg)) {
        throw 'Native package inventory duplicated or invalid.'
    }
    $ByPackage[$Pkg]=$Row
}
if ($AllSourceArms.Count -ne 82 -or $ByPackage.Count -ne 24) {
    throw 'Native ARM and package inventory incomplete.'
}
foreach ($Pkg in $ByManifestPackage.Keys) {
    if (-not $ByPackage.ContainsKey($Pkg)) {
        throw 'Manifest package absent from native UE registry.'
    }
}

$Counts=[ordered]@{
    manifest_expected=0; manifest_other=0; manifest_unloadable=0
    extra_expected=0; extra_other=0; extra_unloadable=0
    manifest_identity_mismatch=0
    join_opt_out=0; join_missing_arm=0
    join_unknown_arm=0; join_missing_profile=0
    unresolved_joined_hand=0; unresolved_joined_kind=0
    other_engine=0; other_project_native=0
    other_blueprint=0; other_editor=0; other_module=0
}
$RowsPrivate=New-Object 'System.Collections.Generic.List[object]'
foreach ($Pkg in @($ByPackage.Keys | Sort-Object)) {
    $Row=$ByPackage[$Pkg]
    $IsListed=$ByManifestPackage.ContainsKey($Pkg)
    $Class=[string]$Row.load_classification
    if ($Class -notin @('expected_class','other_class','unloadable')) {
        throw 'Unexpected native load classification.'
    }
    $Suffix=switch ($Class) {
        'expected_class' { 'expected' }
        'other_class' { 'other' }
        'unloadable' { 'unloadable' }
    }
    $Bucket=if ($IsListed) { 'manifest_'+$Suffix } else { 'extra_'+$Suffix }
    $Counts[$Bucket]=[int]$Counts[$Bucket]+1

    if ($Class -eq 'other_class') {
        $Module='other_module'
        $FullClass=[string]$Row.loaded_class
        if ($FullClass -match '^/Script/Engine\.') { $Module='other_engine' }
        elseif ($FullClass -match '^/Script/GrimrockPrototype\.') { $Module='other_project_native' }
        elseif ($FullClass -match '^/Script/GrimrockPrototypeEditor\.') { $Module='other_editor' }
        elseif ($FullClass -match '^/Game/') { $Module='other_blueprint' }
        $Counts[$Module]=[int]$Counts[$Module]+1
    }
    $ManifestRecord=if ($IsListed) { $ByManifestPackage[$Pkg] } else { $null }
    $IdentityMatch=$null
    if ($IsListed -and $Class -eq 'expected_class') {
        $IdentityMatch=(
            ([string]$Row.item_id -ceq [string]$ManifestRecord.ItemDefinitionId) -and
            ([string]$Row.source_archetype_id -ceq [string]$ManifestRecord.ArchetypeId)
        )
        if (-not $IdentityMatch) { $Counts['manifest_identity_mismatch']=[int]$Counts['manifest_identity_mismatch']+1 }
    }

    $JoinReason='not_applicable'
    if ($Class -eq 'expected_class') {
        if ([bool]$Row.joined_to_source_n1) {
            $JoinReason='joined'
            if ([string]$Row.provable_hand_policy -eq 'Unresolved') {
                $Counts['unresolved_joined_hand']=[int]$Counts['unresolved_joined_hand']+1
            }
            if ([string]$Row.provable_kind -eq 'Unresolved') {
                $Counts['unresolved_joined_kind']=[int]$Counts['unresolved_joined_kind']+1
            }
        }
        elseif (-not [bool]$Row.b_use_weapon_catalogue) {
            $JoinReason='legacy_opt_out'
            $Counts['join_opt_out']=[int]$Counts['join_opt_out']+1
        }
        elseif ([string]::IsNullOrWhiteSpace([string]$Row.source_archetype_id) -or
            [string]$Row.source_archetype_id -eq 'None') {
            $JoinReason='missing_archetype'
            $Counts['join_missing_arm']=[int]$Counts['join_missing_arm']+1
        }
        elseif (-not $AllSourceArms.ContainsKey([string]$Row.source_archetype_id)) {
            $JoinReason='unknown_archetype'
            $Counts['join_unknown_arm']=[int]$Counts['join_unknown_arm']+1
        }
        else {
            $JoinReason='missing_profile'
            $Counts['join_missing_profile']=[int]$Counts['join_missing_profile']+1
        }
    }
    $RowsPrivate.Add([pscustomobject]@{
        asset_package=$Pkg
        registry_class=[string]$Row.registry_class
        loaded_class=[string]$Row.loaded_class
        load_classification=$Class
        in_production_manifest=$IsListed
        manifest_item_id=$(if ($IsListed) { [string]$ManifestRecord.ItemDefinitionId } else { '' })
        manifest_archetype_id=$(if ($IsListed) { [string]$ManifestRecord.ArchetypeId } else { '' })
        manifest_build_kind=$(if ($IsListed) { [string]$ManifestRecord.BuildKind } else { '' })
        actual_item_id=[string]$Row.item_id
        actual_archetype_id=[string]$Row.source_archetype_id
        identity_matches_manifest=$IdentityMatch
        join_reason=$JoinReason
        hand_policy=[string]$Row.provable_hand_policy
        armament_kind=[string]$Row.provable_kind
    })
}
if (($Counts['manifest_expected']+$Counts['manifest_other']+$Counts['manifest_unloadable']) -ne 22 -or
    ($Counts['extra_expected']+$Counts['extra_other']+$Counts['extra_unloadable']) -ne 2 -or
    ($Counts['manifest_expected']+$Counts['extra_expected']) -ne [int]$Report.count_expected_class -or
    ($Counts['manifest_other']+$Counts['extra_other']) -ne [int]$Report.count_other_class -or
    ($Counts['manifest_unloadable']+$Counts['extra_unloadable']) -ne [int]$Report.count_unloadable -or
    ($Counts['join_opt_out']+$Counts['join_missing_arm']+$Counts['join_unknown_arm']+
        $Counts['join_missing_profile']) -ne
        ([int]$Report.count_expected_class-[int]$Report.concrete_joined) -or
    $Counts['unresolved_joined_hand'] -ne
        ([int]$Report.concrete_joined-[int]$Report.concrete_hand_policy_proven) -or
    $Counts['unresolved_joined_kind'] -ne
        ([int]$Report.concrete_joined-[int]$Report.concrete_armament_kind_proven) -or
    ($Counts['other_engine']+$Counts['other_project_native']+$Counts['other_editor']+
        $Counts['other_blueprint']+$Counts['other_module']) -ne [int]$Report.count_other_class) {
    throw 'Native/manifest reconciliation invariant failed.'
}
$PrivateReport=[ordered]@{
    schema='ARM-CLEAN06.2E.3/reconciliation/read-only/v1'
    commit_sha=$ExpectedHead
    counts=$Counts
    rows=$RowsPrivate.ToArray()
    binary_migration_authorized=$false
    any_uasset_saved=$false
}
$PrivateReport | ConvertTo-Json -Depth 8 |
    Set-Content -LiteralPath (Join-Path $OutDir 'reconciliation-private.json') -Encoding UTF8
