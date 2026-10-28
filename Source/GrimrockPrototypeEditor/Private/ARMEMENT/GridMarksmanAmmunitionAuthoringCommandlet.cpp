#include "ARMEMENT/GridMarksmanAmmunitionAuthoringCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

namespace MarksmanAmmo
{
    static constexpr const TCHAR* ArrowId = TEXT("Item_Arrows_01");
    static constexpr const TCHAR* ArrowAssetName = TEXT("DA_Item_Arrows_01");
    static constexpr const TCHAR* ArrowPackage = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Arrows_01");
    static constexpr const TCHAR* ArrowObject = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Arrows_01.DA_Item_Arrows_01");
    static constexpr const TCHAR* BowObject = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ShortBow_01.DA_Weapon_ShortBow_01");

    void ConfigureArrow(UGridItemDefinitionAsset& Item)
    {
        Item.ItemDefinitionId = FName(ArrowId);
        Item.DisplayName = FText::FromString(TEXT("Flèches"));
        Item.Description = FText::FromString(TEXT("Munitions pour les arcs. Une flèche est consommée à chaque tir réussi ou manqué."));
        Item.ItemType = EGridItemType::Misc;
        Item.Weight = 0.04f;
        Item.bStackable = true;
        Item.MaxStackSize = 64;
        Item.HandUsage = EGridItemHandUsage::NotHandHeld;
        Item.bCombatThrowWeapon = false;
    }

    bool IsExpectedArrow(const UGridItemDefinitionAsset& Item)
    {
        return Item.ItemDefinitionId == FName(ArrowId) && Item.bStackable &&
            Item.MaxStackSize == 64 && FMath::IsNearlyEqual(Item.Weight, 0.04f) &&
            !Item.bCombatThrowWeapon && !Item.bUseWeaponCatalogue &&
            Item.CombatActions.IsEmpty() && Item.IsValidDefinition();
    }

    bool Save(UGridItemDefinitionAsset* Item)
    {
        if (!IsValid(Item)) return false;
        UPackage* Package = Item->GetOutermost();
        const FString Filename = FPackageName::LongPackageNameToFilename(
            Package->GetName(), FPackageName::GetAssetPackageExtension());
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        Args.SaveFlags = SAVE_NoError;
        Item->MarkPackageDirty();
        return UPackage::SavePackage(Package, Item, *Filename, Args);
    }
}

UGridMarksmanAmmunitionAuthoringCommandlet::UGridMarksmanAmmunitionAuthoringCommandlet()
{
    IsEditor = true;
    IsClient = false;
    IsServer = false;
    LogToConsole = true;
}

int32 UGridMarksmanAmmunitionAuthoringCommandlet::Main(const FString& Params)
{
    const bool bApply = FParse::Param(*Params, TEXT("Apply"));
    UGridItemDefinitionAsset* Bow =
        LoadObject<UGridItemDefinitionAsset>(nullptr, MarksmanAmmo::BowObject);
    if (!IsValid(Bow) || !Bow->IsValidDefinition() ||
        Bow->ItemDefinitionId != FName(TEXT("Weapon_ShortBow_01")) ||
        Bow->WeaponArchetypeId != FName(TEXT("ARM045")) ||
        !Bow->bUseWeaponCatalogue || Bow->bCombatThrowWeapon ||
        !Bow->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand))
    {
        UE_LOG(LogTemp, Error, TEXT("[LOT02.5] ARM045 is missing or invalid"));
        return 1;
    }
    const FGridWeaponLevelProfile* N1 = nullptr;
    if (!Bow->TryGetWeaponCatalogueProfile(1, N1) || !N1 ||
        !N1->IsValidProfile() || N1->RangeCells <= 1)
    {
        UE_LOG(LogTemp, Error, TEXT("[LOT02.5] ARM045 is not a valid ranged N1 weapon"));
        return 1;
    }

    const FString ArrowFile = FPackageName::LongPackageNameToFilename(
        MarksmanAmmo::ArrowPackage, FPackageName::GetAssetPackageExtension());
    const bool bFileExists = FPaths::FileExists(ArrowFile);
    UGridItemDefinitionAsset* Existing = bFileExists ?
        LoadObject<UGridItemDefinitionAsset>(nullptr, MarksmanAmmo::ArrowObject) : nullptr;

    if (bFileExists &&
        IsValid(Existing) && Existing->GetPathName() == MarksmanAmmo::ArrowObject &&
        MarksmanAmmo::IsExpectedArrow(*Existing) &&
        Bow->RequiredAmmunitionItemDefinitionId == FName(MarksmanAmmo::ArrowId))
    {
        UE_LOG(LogTemp, Display, TEXT("[LOT02.5] PRESENT_SAME Bow=ARM045 Ammo=Item_Arrows_01"));
        return 0;
    }
    if (bFileExists || FindPackage(nullptr, MarksmanAmmo::ArrowPackage) ||
        !Bow->RequiredAmmunitionItemDefinitionId.IsNone() ||
        Bow->GetOutermost()->IsDirty())
    {
        UE_LOG(LogTemp, Error, TEXT("[LOT02.5] CONFLICT: arrow output exists, bow already configured or package dirty"));
        return 1;
    }

    UGridItemDefinitionAsset* CandidateArrow =
        NewObject<UGridItemDefinitionAsset>(GetTransientPackage());
    MarksmanAmmo::ConfigureArrow(*CandidateArrow);
    UGridItemDefinitionAsset* CandidateBow =
        DuplicateObject<UGridItemDefinitionAsset>(Bow, GetTransientPackage());
    if (!IsValid(CandidateBow))
    {
        UE_LOG(LogTemp, Error, TEXT("[LOT02.5] Failed duplicating bow for dry run"));
        return 1;
    }
    CandidateBow->RequiredAmmunitionItemDefinitionId = FName(MarksmanAmmo::ArrowId);
    if (!MarksmanAmmo::IsExpectedArrow(*CandidateArrow) || !CandidateBow->IsValidDefinition())
    {
        UE_LOG(LogTemp, Error, TEXT("[LOT02.5] Invalid proposed arrow or bow contract"));
        return 1;
    }
    UE_LOG(LogTemp, Display, TEXT("[LOT02.5] READY Bow=ARM045 Ammo=Item_Arrows_01 Stack=64 StarterCount=12"));
    if (!bApply)
    {
        UE_LOG(LogTemp, Display, TEXT("[LOT02.5] DRY_RUN no assets written"));
        return 0;
    }

    // Preflight both proposed definitions before the first package write.
    UPackage* Package = CreatePackage(MarksmanAmmo::ArrowPackage);
    if (!IsValid(Package))
    {
        UE_LOG(LogTemp, Error, TEXT("[LOT02.5] Cannot allocate arrow package"));
        return 1;
    }
    UGridItemDefinitionAsset* Arrow = NewObject<UGridItemDefinitionAsset>(
        Package, FName(MarksmanAmmo::ArrowAssetName), RF_Public | RF_Standalone | RF_Transactional);
    MarksmanAmmo::ConfigureArrow(*Arrow);
    if (!MarksmanAmmo::IsExpectedArrow(*Arrow))
    {
        UE_LOG(LogTemp, Error, TEXT("[LOT02.5] New arrow definition rejected"));
        return 1;
    }
    FAssetRegistryModule::AssetCreated(Arrow);
    if (!MarksmanAmmo::Save(Arrow))
    {
        UE_LOG(LogTemp, Error, TEXT("[LOT02.5] Arrow save failed"));
        return 1;
    }
    Bow->Modify();
    Bow->RequiredAmmunitionItemDefinitionId = FName(MarksmanAmmo::ArrowId);
    if (!Bow->IsValidDefinition() || !MarksmanAmmo::Save(Bow))
    {
        UE_LOG(LogTemp, Error, TEXT("[LOT02.5] Bow update failed; inspect new arrow and bow locally"));
        return 1;
    }
    UE_LOG(LogTemp, Display, TEXT("[LOT02.5] SAVED Bow=ARM045 Ammo=Item_Arrows_01"));
    return 0;
}
