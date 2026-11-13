# ARM-CLEAN06.2E.4 — native object classification and semantic blockers.
# Dot-sourced after E3 reconciliation, solely in the isolated UEValidation run.
# This script does not read/write any .uasset, .umap, SaveGame or source file.
# Inputs: $Report, $ByPackage, $ByManifestPackage, $Counts,
# $ExpectedHead, $OutDir. All identifiers remain in the PRIVATE JSON.
$E4Step='INIT'
$Details=[ordered]@{
    other_class_data_assets=0
    other_class_primary_data_assets=0
    other_class_blueprint_compiled=0
    other_class_non_data_assets=0
    other_class_redirectors=0
    other_class_non_redirectors=0
    legacy_opt_out_manifest=0
    legacy_opt_out_unlisted=0
    legacy_opt_out_with_combat_actions=0
    legacy_opt_out_throw_enabled=0
    legacy_opt_out_inventory_action=0
    unresolved_hand_not_handheld=0
    unresolved_hand_effective_auto=0
    unresolved_hand_one_hand_permission=0
    unresolved_hand_two_hand_permission=0
    unresolved_hand_other=0
    unresolved_kind_passive_weapon_focus_candidate=0
    unresolved_kind_offensive_shield_conflict=0
    unresolved_kind_other=0
    explicit_slot_mismatch_on_joined=0
    widened_equip_without_attack=0
    joined_attack_main=0
    joined_attack_off=0
    equippable_without_attack_main=0
    equippable_without_attack_off=0
    unresolved_not_handheld_with_hand_slot=0
    focus_passive_with_attack=0
    listed_focus_passive_profiles=0
    listed_focus_passive_unresolved_kind=0
}
$E4Step='INIT_FACTS'
$Facts=New-Object 'System.Collections.Generic.List[object]'
foreach ($Package in @($ByPackage.Keys | Sort-Object)) {
    $E4Step='ROW_START'
    $Row=$ByPackage[$Package]
    $Class=[string]$Row.load_classification
    $Listed=$ByManifestPackage.ContainsKey($Package)
    $HandReason='not_applicable'
    $KindReason='not_applicable'
    $ClassReason='expected_item_definition'
    $bIsDataAsset=[bool]$Row.is_data_asset
    $bIsPrimaryDataAsset=[bool]$Row.is_primary_data_asset
    $bBlueprintClass=[bool]$Row.class_compiled_from_blueprint
    $E4Step='ROW_CLASS'
    if ($Class -eq 'other_class') {
        # Class structure is evidence, not an instruction to convert.
        if ($bIsDataAsset) { $Details['other_class_data_assets']=[int]$Details['other_class_data_assets']+1 }
        else { $Details['other_class_non_data_assets']=[int]$Details['other_class_non_data_assets']+1 }
        if ($bIsPrimaryDataAsset) { $Details['other_class_primary_data_assets']=[int]$Details['other_class_primary_data_assets']+1 }
        if ([bool]$Row.is_object_redirector) {
            $Details['other_class_redirectors']=[int]$Details['other_class_redirectors']+1
        } else {
            $Details['other_class_non_redirectors']=[int]$Details['other_class_non_redirectors']+1
        }
        if ($bBlueprintClass) { $Details['other_class_blueprint_compiled']=[int]$Details['other_class_blueprint_compiled']+1 }
        if ($bIsPrimaryDataAsset -and -not $bIsDataAsset) {
            throw 'Inconsistent native class hierarchy.'
        }
        if ($bIsPrimaryDataAsset) { $ClassReason='primary_data_asset_other' }
        elseif ($bIsDataAsset) { $ClassReason='data_asset_other' }
        else { $ClassReason='non_data_asset_other' }
    }
    elseif ($Class -eq 'unloadable') {
        $ClassReason='unloadable'
    }
    elseif ($Class -ne 'expected_class') {
        throw 'Unknown UE class classification.'
    }
    elseif (-not $bIsPrimaryDataAsset -or -not $bIsDataAsset) {
        throw 'Expected UGridItemDefinitionAsset is not a primary data asset.'
    }

    $E4Step='ROW_OPTOUT'
    $IsJoined=($Class -eq 'expected_class' -and [bool]$Row.joined_to_source_n1)
    $IsOptOut=($Class -eq 'expected_class' -and -not [bool]$Row.b_use_weapon_catalogue)
    if ($IsOptOut) {
        if ($Listed) { $Details['legacy_opt_out_manifest']=[int]$Details['legacy_opt_out_manifest']+1 }
        else { $Details['legacy_opt_out_unlisted']=[int]$Details['legacy_opt_out_unlisted']+1 }
        if ([int]$Row.combat_action_count -gt 0) {
            $Details['legacy_opt_out_with_combat_actions']=[int]$Details['legacy_opt_out_with_combat_actions']+1
        }
        if ([bool]$Row.combat_throw_weapon) {
            $Details['legacy_opt_out_throw_enabled']=[int]$Details['legacy_opt_out_throw_enabled']+1
        }
        if ([bool]$Row.has_inventory_combat_action) {
            $Details['legacy_opt_out_inventory_action']=[int]$Details['legacy_opt_out_inventory_action']+1
        }
    }

    $E4Step='ROW_SLOTS'
    # Item-only fields are deliberately absent from "other_class" and
    # "unloadable" rows. Under StrictMode, even reading such a missing
    # property raises PropertyNotFoundException. Preserve true unknowns.
    $MainDeclared=$null
    $OffDeclared=$null
    $MainRuntime=$null
    $OffRuntime=$null
    $EffectiveHand=$null
    $DeclaredHand=$null
    $AttackMain=$null
    $AttackOff=$null
    $PhysicalThrow=$null
    $CombatThrow=$null
    $InventoryCombat=$null
    if ($Class -eq 'expected_class') {
        $MainDeclared=[bool]$Row.declared_slot_main
        $OffDeclared=[bool]$Row.declared_slot_off
        $MainRuntime=[bool]$Row.can_equip_main
        $OffRuntime=[bool]$Row.can_equip_off
        $EffectiveHand=[int]$Row.effective_hand_usage_enum
        $DeclaredHand=[int]$Row.declared_hand_usage_enum
        $AttackMain=[bool]$Row.can_attack_main
        $AttackOff=[bool]$Row.can_attack_off
        $PhysicalThrow=[bool]$Row.is_physically_throwable
        $CombatThrow=[bool]$Row.is_combat_throwable
        $InventoryCombat=[bool]$Row.has_inventory_combat_action
    }

    $E4Step='ROW_SEMANTICS'
    if ($Class -eq 'expected_class') {
        if (@($Row.local_combat_action_rows).Count -ne [int]$Row.combat_action_count) {
            throw 'E4_ACTION_COUNT_CONTRACT'
        }
        # An attack must satisfy BOTH the authored attack slot and runtime equip gate.
        if (($AttackMain -and -not $MainDeclared) -or ($AttackOff -and -not $OffDeclared)) {
            throw 'E4_ATTACK_SLOT_CONTRACT'
        }
        if (($AttackMain -and -not $MainRuntime) -or ($AttackOff -and -not $OffRuntime)) {
            throw 'E4_ATTACK_EQUIP_CONTRACT'
        }
        if ($MainRuntime -and -not $AttackMain) {
            $Details['equippable_without_attack_main']=[int]$Details['equippable_without_attack_main']+1
        }
        if ($OffRuntime -and -not $AttackOff) {
            $Details['equippable_without_attack_off']=[int]$Details['equippable_without_attack_off']+1
        }
    }
    if ($IsJoined) {
        if ($AttackMain) { $Details['joined_attack_main']=[int]$Details['joined_attack_main']+1 }
        if ($AttackOff) { $Details['joined_attack_off']=[int]$Details['joined_attack_off']+1 }
        if (($MainRuntime -and -not $MainDeclared) -or
            ($OffRuntime -and -not $OffDeclared)) {
            # CanEquipToSlot presently permits either hand for OneHanded and
            # TwoHanded regardless of the authored slot array. This is an
            # essential behavioural discrepancy, not a migration fix.
            $Details['explicit_slot_mismatch_on_joined']=[int]$Details['explicit_slot_mismatch_on_joined']+1
            if (($MainRuntime -and -not $MainDeclared -and -not $AttackMain) -or
                ($OffRuntime -and -not $OffDeclared -and -not $AttackOff)) {
                $Details['widened_equip_without_attack']=[int]$Details['widened_equip_without_attack']+1
            }
        }
        if ([string]$Row.provable_hand_policy -eq 'Unresolved') {
            if ($EffectiveHand -eq 1) {
                $HandReason='effective_not_handheld'
                $Details['unresolved_hand_not_handheld']=[int]$Details['unresolved_hand_not_handheld']+1
                if ($MainRuntime -or $OffRuntime) {
                    $Details['unresolved_not_handheld_with_hand_slot']=[int]$Details['unresolved_not_handheld_with_hand_slot']+1
                }
            }
            elseif ($EffectiveHand -eq 0) {
                $HandReason='effective_auto_unresolved'
                $Details['unresolved_hand_effective_auto']=[int]$Details['unresolved_hand_effective_auto']+1
            }
            elseif ($EffectiveHand -eq 2) {
                $HandReason='one_hand_permission_mismatch'
                $Details['unresolved_hand_one_hand_permission']=[int]$Details['unresolved_hand_one_hand_permission']+1
            }
            elseif ($EffectiveHand -eq 3) {
                $HandReason='two_hand_permission_mismatch'
                $Details['unresolved_hand_two_hand_permission']=[int]$Details['unresolved_hand_two_hand_permission']+1
            }
            else {
                $HandReason='unknown_hand_enum'
                $Details['unresolved_hand_other']=[int]$Details['unresolved_hand_other']+1
            }
        }
        if ([string]$Row.provable_kind -eq 'Unresolved') {
            if ([int]$Row.item_type_enum -eq 2 -and
                -not [bool]$Row.source_n1_has_damage_dice) {
                $KindReason='passive_weapon_focus_candidate_only'
                $Details['unresolved_kind_passive_weapon_focus_candidate']=[int]$Details['unresolved_kind_passive_weapon_focus_candidate']+1
            }
            elseif ([int]$Row.item_type_enum -eq 3 -and
                [bool]$Row.source_n1_has_damage_dice) {
                $KindReason='shield_with_damage_conflict'
                $Details['unresolved_kind_offensive_shield_conflict']=[int]$Details['unresolved_kind_offensive_shield_conflict']+1
            }
            else {
                $KindReason='other_type_dice_mismatch'
                $Details['unresolved_kind_other']=[int]$Details['unresolved_kind_other']+1
            }
        }
        if ($Listed -and
            [string]$ByManifestPackage[$Package].BuildKind -ceq 'FocusPassive') {
            $Details['listed_focus_passive_profiles']=[int]$Details['listed_focus_passive_profiles']+1
            if ([string]$Row.provable_kind -eq 'Unresolved') {
                $Details['listed_focus_passive_unresolved_kind']=[int]$Details['listed_focus_passive_unresolved_kind']+1
            }
            if ($AttackMain -or $AttackOff) {
                $Details['focus_passive_with_attack']=[int]$Details['focus_passive_with_attack']+1
            }
        }
    }
    $E4Step='ROW_FACT'
    $Facts.Add([pscustomobject]@{
        asset_package=$Package
        class_path=[string]$Row.loaded_class
        registry_class=[string]$Row.registry_class
        class_role=$ClassReason
        class_is_redirector=$(if ($Class -eq 'other_class') { [bool]$Row.is_object_redirector } else { $false })
        redirect_target_package=$(if ($Class -eq 'other_class') { [string]$Row.redirect_target_package } else { '' })
        class_is_data_asset=$bIsDataAsset
        class_is_primary_data_asset=$bIsPrimaryDataAsset
        class_compiled_from_blueprint=$bBlueprintClass
        in_manifest=$Listed
        source_item_id=[string]$Row.item_id
        archetype_id=[string]$Row.source_archetype_id
        manifest_build_kind=$(if ($Listed) { [string]$ByManifestPackage[$Package].BuildKind } else { '' })
        joined_to_arm=$IsJoined
        catalogue_opt_out=$IsOptOut
        declared_hand_usage_enum=$DeclaredHand
        effective_hand_usage_enum=$EffectiveHand
        declared_main_slot=$MainDeclared
        declared_off_slot=$OffDeclared
        runtime_main_slot=$MainRuntime
        runtime_off_slot=$OffRuntime
        can_attack_main=$AttackMain
        can_attack_off=$AttackOff
        physically_throwable=$PhysicalThrow
        combat_throwable=$CombatThrow
        has_inventory_combat_action=$InventoryCombat
        inventory_combat_action_id=$(if ($Class -eq 'expected_class') { [string]$Row.inventory_combat_action_id } else { '' })
        local_combat_action_rows=$(if ($Class -eq 'expected_class') { ,$Row.local_combat_action_rows } else { $null })
        hand_reason=$HandReason
        kind_reason=$KindReason
        combat_action_count=$(if ($Class -eq 'expected_class') { [int]$Row.combat_action_count } else { $null })
    })
}
# Split independent checks so the Windows agent can safely report a
# fixed diagnostic token without leaking names, classes or logs.
$E4Step='ASSERT'
if ($Facts.Count -ne 24) {
    throw 'E4_BAD_PACKAGE_COUNT'
}
if (($Details['other_class_data_assets']+
        $Details['other_class_non_data_assets']) -ne
        [int]$Report.count_other_class) {
    throw 'E4_CLASS_COUNTS'
}
if (($Details['other_class_redirectors']+
        $Details['other_class_non_redirectors']) -ne [int]$Report.count_other_class) {
    throw 'E4_REDIRECTOR_COUNTS'
}
if ($Details['widened_equip_without_attack'] -ne
        $Details['explicit_slot_mismatch_on_joined']) {
    throw 'E4_WIDENED_ATTACK_COUNTS'
}
if (($Details['legacy_opt_out_manifest']+
        $Details['legacy_opt_out_unlisted']) -ne
        [int]$Counts['join_opt_out']) {
    throw 'E4_OPTOUT_COUNTS'
}
if (($Details['unresolved_hand_not_handheld']+
        $Details['unresolved_hand_effective_auto']+
        $Details['unresolved_hand_one_hand_permission']+
        $Details['unresolved_hand_two_hand_permission']+
        $Details['unresolved_hand_other']) -ne
        [int]$Counts['unresolved_joined_hand']) {
    throw 'E4_HAND_COUNTS'
}
if (($Details['unresolved_kind_passive_weapon_focus_candidate']+
        $Details['unresolved_kind_offensive_shield_conflict']+
        $Details['unresolved_kind_other']) -ne
        [int]$Counts['unresolved_joined_kind']) {
    throw 'E4_KIND_COUNTS'
}
if ($Details['listed_focus_passive_unresolved_kind'] -gt
        $Details['listed_focus_passive_profiles']) {
    throw 'E4_FOCUS_COUNTS'
}
$E4Step='OUTPUT_MODEL'
$Qualification=[ordered]@{
    schema='ARM-CLEAN06.2E.4.4/qualification/read-only/v1'
    commit_sha=$ExpectedHead
    classification_counts=$Details
    exact_private_facts=$Facts.ToArray()
    semantic_equivalence_proven=$false
    binary_migration_authorized=$false
    any_uasset_saved=$false
}
$E4Step='OUTPUT_JSON'
$Qualification | ConvertTo-Json -Depth 8 |
    Set-Content -LiteralPath (Join-Path $OutDir 'qualification-private.json') -Encoding UTF8
