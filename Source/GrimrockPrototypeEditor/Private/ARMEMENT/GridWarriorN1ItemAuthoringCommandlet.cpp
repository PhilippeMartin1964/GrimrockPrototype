#include "ARMEMENT/GridWarriorN1ItemAuthoringCommandlet.h"
#include "ARMEMENT/GridWarriorN1ItemAuthoringSpecs.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/AssetData.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

namespace ARMEMENT01102
{
    const TCHAR* const RpgPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue");
    const TCHAR* const VisualsPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponVisualCatalogue.DA_WeaponVisualCatalogue");
    const TCHAR* const ItemFolder = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons");

    FString MakePackagePath(const FWarriorItemSpec& Spec)
    {
        return FString::Printf(TEXT("%s/%s"), ItemFolder, Spec.AssetName);
    }

    // Build from real N1 catalogue data, not from guessed weapon statistics.
    bool BuildItem(const FWarriorItemSpec& Spec,
        UGridWeaponCatalogueAsset* Rpg, UGridWeaponVisualCatalogueAsset* Visuals,
        UGridItemDefinitionAsset* Item)
    {
        const FName ArchetypeId(Spec.ArchetypeId);
        const FGridWeaponLevelProfile* Profile = Rpg->FindProfile(ArchetypeId, 1);
        const FGridWeaponVisualProfile* Look = Visuals->FindProfileForArchetype(ArchetypeId);
        if (!Profile || !Profile->IsValidProfile() || !Profile->DamageDice.bEnabled)
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2] Invalid offensive N1 profile: %s"), Spec.ArchetypeId);
            return false;
        }

        EGridItemHandUsage Usage;
        if (Profile->HandUsage == TEXT("1 main"))
        {
            Usage = EGridItemHandUsage::OneHanded;
        }
        else if (Profile->HandUsage == TEXT("2 mains"))
        {
            Usage = EGridItemHandUsage::TwoHanded;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2] Unsupported source hand usage: %s"), Spec.ArchetypeId);
            return false;
        }
        // This is the approved two-handed Breaker weapon. Its real source
        // must agree; never silently turn it into a one-handed item.
        if (ArchetypeId == TEXT("ARM031") && Usage != EGridItemHandUsage::TwoHanded)
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2] ARM031 is not two-handed in source"));
            return false;
        }

        if (!Look || !Look->IsValidProfile() ||
            Look->BaseIcon.IsNull() || Look->WorldMesh.IsNull() ||
            !Look->BaseIcon.LoadSynchronous() || !Look->WorldMesh.LoadSynchronous())
        {
            UE_LOG(LogTemp, Display,
                TEXT("[ARMEMENT01.10.2] BLOCKED %s: real visual binding, icon and mesh must be authored first"),
                Spec.ArchetypeId);
            return false;
        }

        FGridAttackDefinition Attack;
        if (!Profile->TryBuildAttackDefinition(Attack) ||
            Profile->ActionPointCost < 1 || Profile->RangeCells < 1)
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2] Invalid attack data: %s"), Spec.ArchetypeId);
            return false;
        }

        Item->ItemDefinitionId = FName(Spec.ItemDefinitionId);
        Item->DisplayName = Profile->DisplayName;
        Item->ItemType = EGridItemType::Weapon;
        Item->Weight = Profile->WeightKg;
        Item->bStackable = false;
        Item->MaxStackSize = 1;
        Item->HandUsage = Usage;
        Item->CompatibleEquipmentSlots = { EGridEquipmentSlot::MainHand };
        Item->WeaponArchetypeId = ArchetypeId;
        Item->DefaultWeaponItemLevel = 1;
        Item->bUseWeaponCatalogue = true;
        Item->WeaponCatalogue = Rpg;
        Item->bUseWeaponVisualCatalogue = true;
        Item->WeaponVisualCatalogue = Visuals;
        Item->Icon = Look->BaseIcon;
        Item->WorldMesh = Look->WorldMesh;
        Item->EquippedMesh = Look->EquippedMesh.IsNull() ? Look->WorldMesh : Look->EquippedMesh;
        Item->bCombatThrowWeapon = false;
        Item->bProvidesQuickItemCombatAction = false;

        FGridCombatActionDefinition Action;
        Action.ActionId = FName(Spec.AttackId);
        Action.DisplayName = Profile->DisplayName;
        Action.Icon = Look->BaseIcon;
        Action.ActionType = EGridCombatActionType::MeleeAttack;
        Action.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
        Action.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
        Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
        Action.ActionPointCost = Profile->ActionPointCost;
        Action.RangeCells = Profile->RangeCells;
        Action.bRequiresLineOfSight = false;
        Action.OffensiveProfile.AttackId = Action.ActionId;
        Action.OffensiveProfile.AttackDefinition = Attack;
        Action.OffensiveProfile.DamageScalingAttribute = Profile->ScalingAttribute;
        Action.OffensiveProfile.RangeCells = Profile->RangeCells;
        if (!Action.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2] Invalid constructed melee action: %s"), Spec.ArchetypeId);
            return false;
        }
        Item->CombatActions.Add(Action);

        return Item->IsValidDefinition() &&
            Item->CanEquipToSlot(EGridEquipmentSlot::MainHand) &&
            Item->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand);
    }

    bool OutputIsFree(const FWarriorItemSpec& Spec, IAssetRegistry& Registry)
    {
        const FString PackagePath = MakePackagePath(Spec);
        const FString Filename = FPackageName::LongPackageNameToFilename(
            PackagePath, FPackageName::GetAssetPackageExtension());
        if (FPaths::FileExists(Filename) ||
            FindPackage(nullptr, *PackagePath) ||
            FindObject<UObject>(nullptr, *(PackagePath + TEXT(".") + Spec.AssetName)))
        {
            return false;
        }

        // Also reject a differently named item already bound to this archetype.
        TArray<FAssetData> Existing;
        Registry.GetAssetsByPath(FName(ItemFolder), Existing, true);
        for (const FAssetData& Data : Existing)
        {
            const UGridItemDefinitionAsset* Item = Cast<UGridItemDefinitionAsset>(Data.GetAsset());
            if (Item && Item->WeaponArchetypeId == FName(Spec.ArchetypeId))
            {
                return false;
            }
        }
        return true;
    }
}

UGridWarriorN1ItemAuthoringCommandlet::UGridWarriorN1ItemAuthoringCommandlet()
{
    IsEditor = true;
    IsClient = false;
    IsServer = false;
    LogToConsole = true;
}

int32 UGridWarriorN1ItemAuthoringCommandlet::Main(const FString& Params)
{
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    FString RequestedArchetype;
    FParse::Value(*Params, TEXT("Archetype="), RequestedArchetype);

    if (bApply && RequestedArchetype.IsEmpty())
    {
        UE_LOG(LogTemp, Error,
            TEXT("[ARMEMENT01.10.2] -Apply requires -Archetype=ARM014|ARM015|ARM025|ARM031"));
        return 1;
    }

    UGridWeaponCatalogueAsset* Rpg = LoadObject<UGridWeaponCatalogueAsset>(nullptr, ARMEMENT01102::RpgPath);
    UGridWeaponVisualCatalogueAsset* Visuals =
        LoadObject<UGridWeaponVisualCatalogueAsset>(nullptr, ARMEMENT01102::VisualsPath);
    if (!IsValid(Rpg) || !IsValid(Visuals) ||
        !Rpg->IsValidCatalogue() || !Rpg->IsValidClassAffinityData() || !Visuals->IsValidCatalogue())
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2] Missing/invalid authoritative catalogues"));
        return 1;
    }

    IAssetRegistry& Registry =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    TArray<FString> Paths = { FString(ARMEMENT01102::ItemFolder) };
    Registry.ScanPathsSynchronous(Paths, true);

    int32 Selected = 0;
    int32 Ready = 0;
    int32 Blocked = 0;
    for (const ARMEMENT01102::FWarriorItemSpec& Spec : ARMEMENT01102::WarriorItems)
    {
        if (!RequestedArchetype.IsEmpty() &&
            RequestedArchetype != Spec.ArchetypeId)
        {
            continue;
        }
        ++Selected;

        // Approved design does not automatically mean equipped/usable.
        const EGridWeaponKitAffinity Affinity =
            Rpg->GetKitRecommendationAffinity(FName(Spec.ArchetypeId), TEXT("Warrior"));
        if (Affinity != EGridWeaponKitAffinity::Primary &&
            Affinity != EGridWeaponKitAffinity::Secondary)
        {
            UE_LOG(LogTemp, Error,
                TEXT("[ARMEMENT01.10.2] Unapproved workbook affinity: %s"), Spec.ArchetypeId);
            return 1;
        }

        if (!ARMEMENT01102::OutputIsFree(Spec, Registry))
        {
            if (bApply)
            {
                UE_LOG(LogTemp, Error,
                    TEXT("[ARMEMENT01.10.2] Existing item or occupied target: refusing overwrite of %s"),
                    Spec.ArchetypeId);
                return 1;
            }
            UE_LOG(LogTemp, Display,
                TEXT("[ARMEMENT01.10.2] PRESENT %s: definition already exists; no writes"),
                Spec.ArchetypeId);
            continue;
        }

        UGridItemDefinitionAsset* Candidate = NewObject<UGridItemDefinitionAsset>(GetTransientPackage());
        if (!ARMEMENT01102::BuildItem(Spec, Rpg, Visuals, Candidate))
        {
            ++Blocked;
            continue;
        }
        ++Ready;
        UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.10.2] READY %s -> %s"),
            Spec.ArchetypeId, Spec.AssetName);

        if (!bApply)
        {
            continue;
        }

        // At most one package may be created per commandlet invocation.
        const FString PackagePath = ARMEMENT01102::MakePackagePath(Spec);
        UPackage* Package = CreatePackage(*PackagePath);
        if (!Package)
        {
            return 1;
        }
        UGridItemDefinitionAsset* Asset = NewObject<UGridItemDefinitionAsset>(
            Package, FName(Spec.AssetName), RF_Public | RF_Standalone | RF_Transactional);
        if (!Asset || !ARMEMENT01102::BuildItem(Spec, Rpg, Visuals, Asset))
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2] Package construction failed: %s"), *PackagePath);
            return 1;
        }

        FAssetRegistryModule::AssetCreated(Asset);
        Asset->MarkPackageDirty();
        const FString Filename = FPackageName::LongPackageNameToFilename(
            PackagePath, FPackageName::GetAssetPackageExtension());
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        SaveArgs.SaveFlags = SAVE_NoError;
        if (!UPackage::SavePackage(Package, Asset, *Filename, SaveArgs))
        {
            UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2] Save failed: %s"), *Filename);
            return 1;
        }
        UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.10.2] CREATED %s (one new package)"), *PackagePath);
    }

    if (Selected != (RequestedArchetype.IsEmpty() ? 4 : 1))
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2] Unknown archetype selector: %s"), *RequestedArchetype);
        return 1;
    }

    if (bApply && Blocked > 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[ARMEMENT01.10.2] BLOCKED: no item created; author missing art first"));
        return 2;
    }
    UE_LOG(LogTemp, Display,
        TEXT("[ARMEMENT01.10.2] %s: %d ready, %d blocked; %s"),
        bApply ? TEXT("APPLY") : TEXT("READ-ONLY PREFLIGHT"), Ready, Blocked,
        bApply ? TEXT("single requested asset") : TEXT("no packages written"));
    return 0;
}
