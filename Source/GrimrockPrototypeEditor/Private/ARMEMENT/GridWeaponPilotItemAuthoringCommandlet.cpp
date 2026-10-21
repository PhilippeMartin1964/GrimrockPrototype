#include "ARMEMENT/GridWeaponPilotItemAuthoringCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

namespace ARMEMENT0166
{
    static constexpr const TCHAR* RpgCataloguePath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue");
    static constexpr const TCHAR* VisualCataloguePath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue");
    static constexpr const TCHAR* ContentFolder =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons");

    struct FItemSpec
    {
        const TCHAR* AssetName;
        const TCHAR* ItemId;
        const TCHAR* ArchetypeId;
        const TCHAR* VisualId;
        EGridItemType Type;
        EGridItemHandUsage Usage;
        EGridEquipmentSlot Slot;
        const TCHAR* AttackId;
        EGridCombatActionType AttackType;
    };

    static const FItemSpec Specs[] =
    {
        {TEXT("DA_Weapon_Dagger_01"), TEXT("Weapon_Dagger_01"), TEXT("ARM001"), TEXT("VIS_Dagger_01"),
         EGridItemType::Weapon, EGridItemHandUsage::OneHanded, EGridEquipmentSlot::MainHand,
         TEXT("Attack_Dagger_01"), EGridCombatActionType::MeleeAttack},
        {TEXT("DA_Weapon_ShortBow_01"), TEXT("Weapon_ShortBow_01"), TEXT("ARM045"), TEXT("VIS_ShortBow_01"),
         EGridItemType::Weapon, EGridItemHandUsage::TwoHanded, EGridEquipmentSlot::MainHand,
         TEXT("Attack_ShortBow_01"), EGridCombatActionType::RangedAttack},
        {TEXT("DA_Weapon_RoundShield_01"), TEXT("Weapon_RoundShield_01"), TEXT("ARM075"), TEXT("VIS_RoundShield_01"),
         EGridItemType::Shield, EGridItemHandUsage::NotHandHeld, EGridEquipmentSlot::OffHand,
         nullptr, EGridCombatActionType::None}
    };

    FString PackagePath(const FItemSpec& Spec)
    {
        return FString::Printf(TEXT("%s/%s"), ContentFolder, Spec.AssetName);
    }

    FString ObjectPath(const FItemSpec& Spec)
    {
        return PackagePath(Spec) + TEXT(".") + Spec.AssetName;
    }

    // Constructs a complete transient item; no existing package is modified.
    bool BuildCandidate(const FItemSpec& Spec, UGridWeaponCatalogueAsset* Rpg,
        UGridWeaponVisualCatalogueAsset* Visuals, UGridItemDefinitionAsset* Item)
    {
        const FName Archetype(Spec.ArchetypeId);
        const FGridWeaponLevelProfile* Profile = Rpg->FindProfile(Archetype, 1);
        const FGridWeaponVisualProfile* Look = Visuals->FindProfileForArchetype(Archetype);
        if (!Profile || !Profile->IsValidProfile() || !Look || !Look->IsValidProfile() ||
            Look->VisualId != FName(Spec.VisualId) || Look->BaseIcon.IsNull() || Look->WorldMesh.IsNull())
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.6] Invalid source profile for %s"), Spec.ArchetypeId);
            return false;
        }
        if (!Look->BaseIcon.LoadSynchronous() || !Look->WorldMesh.LoadSynchronous())
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.6] Visual resources cannot load: %s"), Spec.ArchetypeId);
            return false;
        }
        if (!FMath::IsFinite(Profile->WeightKg) || Profile->WeightKg < 0.0f)
        {
            return false;
        }

        Item->ItemDefinitionId = FName(Spec.ItemId);
        Item->DisplayName = Profile->DisplayName;
        Item->ItemType = Spec.Type;
        Item->Weight = Profile->WeightKg;
        Item->bStackable = false;
        Item->MaxStackSize = 1;
        Item->HandUsage = Spec.Usage;
        Item->CompatibleEquipmentSlots = { Spec.Slot };
        if (Archetype == FName(TEXT("ARM001")))
        {
            Item->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::OffHand);
        }
        Item->WeaponArchetypeId = Archetype;
        Item->DefaultWeaponItemLevel = 1;
        Item->WeaponCatalogue = Rpg;
        Item->bUseWeaponCatalogue = true;
        Item->WeaponVisualCatalogue = Visuals;
        Item->bUseWeaponVisualCatalogue = true;
        // Explicit Legacy fields keep older inventory/held-item displays useful.
        Item->Icon = Look->BaseIcon;
        Item->WorldMesh = Look->WorldMesh;
        Item->EquippedMesh = Look->EquippedMesh.IsNull() ? Look->WorldMesh : Look->EquippedMesh;
        Item->bCombatThrowWeapon = false;
        Item->bProvidesQuickItemCombatAction = false;

        if (Spec.AttackId)
        {
            FGridAttackDefinition Attack;
            if (!Profile->DamageDice.bEnabled || !Profile->TryBuildAttackDefinition(Attack) ||
                Profile->ActionPointCost < 1 || Profile->ActionPointCost > 6 ||
                Profile->RangeCells < 1 || Profile->RangeCells > 32)
            {
                UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.6] Invalid action source for %s"), Spec.ArchetypeId);
                return false;
            }

            FGridCombatActionDefinition Action;
            Action.ActionId = FName(Spec.AttackId);
            Action.DisplayName = Profile->DisplayName;
            Action.Icon = Look->BaseIcon;
            Action.ActionType = Spec.AttackType;
            Action.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
            Action.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
            Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
            Action.ActionPointCost = Profile->ActionPointCost;
            Action.RangeCells = Profile->RangeCells;
            Action.bRequiresLineOfSight = (Spec.AttackType == EGridCombatActionType::RangedAttack);
            Action.OffensiveProfile.AttackId = Action.ActionId;
            Action.OffensiveProfile.AttackDefinition = Attack;
            Action.OffensiveProfile.DamageScalingAttribute = Profile->ScalingAttribute;
            Action.OffensiveProfile.RangeCells = Profile->RangeCells;
            if (!Action.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.6] Invalid constructed attack: %s"), Spec.AttackId);
                return false;
            }
            Item->CombatActions.Add(Action);
        }
        else
        {
            // Passive shield: level-1 catalogue armor. Future equipment level
            // scaling is a separate feature, NOT inferred or fabricated here.
            if (Profile->DamageDice.bEnabled || Profile->PhysicalArmor < 0)
            {
                UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.6] Shield is not passive in RPG catalogue"));
                return false;
            }
            Item->EquipmentStatBonus.ArmorBonus = Profile->PhysicalArmor;
        }
        return Item->IsValidDefinition() &&
            Item->CanEquipToSlot(Spec.Slot) &&
            (Spec.AttackId ? Item->CanProvideAttackFromSlot(Spec.Slot) : Item->CombatActions.IsEmpty());
    }
}

UGridWeaponPilotItemAuthoringCommandlet::UGridWeaponPilotItemAuthoringCommandlet()
{
    IsEditor = true;
    IsClient = false;
    IsServer = false;
    LogToConsole = true;
}

int32 UGridWeaponPilotItemAuthoringCommandlet::Main(const FString& Params)
{
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    UGridWeaponCatalogueAsset* Rpg = LoadObject<UGridWeaponCatalogueAsset>(nullptr, ARMEMENT0166::RpgCataloguePath);
    UGridWeaponVisualCatalogueAsset* Visuals = LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, ARMEMENT0166::VisualCataloguePath);
    if (!IsValid(Rpg) || !IsValid(Visuals) || !Rpg->IsValidCatalogue() || !Visuals->IsValidCatalogue())
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.6] Missing/invalid RPG or visual catalogue"));
        return 1;
    }

    // Preflight ALL three output paths, including files not registered in memory.
    for (const ARMEMENT0166::FItemSpec& Spec : ARMEMENT0166::Specs)
    {
        const FString PackageName = ARMEMENT0166::PackagePath(Spec);
        const FString Filename = FPackageName::LongPackageNameToFilename(
            PackageName, FPackageName::GetAssetPackageExtension());
        if (FPaths::FileExists(Filename) || FindPackage(nullptr, *PackageName) ||
            FindObject<UObject>(nullptr, *ARMEMENT0166::ObjectPath(Spec)))
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.6] Output already exists; refusing overwrite: %s"), *PackageName);
            return 1;
        }
    }

    TArray<UGridItemDefinitionAsset*> Candidates;
    for (const ARMEMENT0166::FItemSpec& Spec : ARMEMENT0166::Specs)
    {
        UGridItemDefinitionAsset* Candidate = NewObject<UGridItemDefinitionAsset>(GetTransientPackage());
        if (!ARMEMENT0166::BuildCandidate(Spec, Rpg, Visuals, Candidate))
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.6] Preflight failed for %s"), Spec.AssetName);
            return 1;
        }
        Candidates.Add(Candidate);
        UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.6.6] READY %s Id=%s Archetype=%s Weight=%.3fkg Actions=%d"),
            Spec.AssetName, Spec.ItemId, Spec.ArchetypeId, Candidate->Weight, Candidate->CombatActions.Num());
    }
    if (!bApply)
    {
        UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.6.6] DRY RUN OK: 3 candidates, zero writes"));
        return 0;
    }

    // Actual authoring occurs only after complete source and path validation.
    for (int32 Index = 0; Index < Candidates.Num(); ++Index)
    {
        const ARMEMENT0166::FItemSpec& Spec = ARMEMENT0166::Specs[Index];
        const FString PackageName = ARMEMENT0166::PackagePath(Spec);
        UPackage* Package = CreatePackage(*PackageName);
        if (!Package)
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.6] Failed creating package %s"), *PackageName);
            return 1;
        }
        UGridItemDefinitionAsset* Asset = NewObject<UGridItemDefinitionAsset>(Package, FName(Spec.AssetName),
            RF_Public | RF_Standalone | RF_Transactional);
        if (!Asset || !ARMEMENT0166::BuildCandidate(Spec, Rpg, Visuals, Asset))
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.6] Construction failed: %s"), Spec.AssetName);
            return 1;
        }
        FAssetRegistryModule::AssetCreated(Asset);
        Asset->MarkPackageDirty();
        const FString Filename = FPackageName::LongPackageNameToFilename(
            PackageName, FPackageName::GetAssetPackageExtension());
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        SaveArgs.SaveFlags = SAVE_NoError;
        if (!UPackage::SavePackage(Package, Asset, *Filename, SaveArgs))
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.6.6] Save failed: %s"), *Filename);
            return 1;
        }
        UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.6.6] SAVED %s"), *PackageName);
    }
    UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.6.6] SUCCESS: three new definitions; existing assets untouched"));
    return 0;
}
