#include "RPG/RPGStartingDisarmToolAuthoring.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UI/UILocalizationText.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

namespace ARMEMENT0116
{
    constexpr const TCHAR* AssetName = TEXT("DA_Item_DisarmTools_01");
    constexpr const TCHAR* ItemId = TEXT("Item_DisarmTools_01");
    constexpr const TCHAR* PackageName =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_DisarmTools_01");
    constexpr const TCHAR* ObjectPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_DisarmTools_01.DA_Item_DisarmTools_01");
    constexpr const TCHAR* NameKey = TEXT("Item.StartingTool.Disarm01.Name");
    constexpr const TCHAR* DescriptionKey = TEXT("Item.StartingTool.Disarm01.Description");
}

void FRPGStartingDisarmToolAuthoring::Configure(UGridItemDefinitionAsset& Item)
{
    Item.ItemDefinitionId = ARMEMENT0116::ItemId;
    Item.DisplayName = FUILocalizationText::Get(ARMEMENT0116::NameKey);
    Item.Description = FUILocalizationText::Get(ARMEMENT0116::DescriptionKey);
    Item.ItemType = EGridItemType::Misc;
    Item.Weight = 0.5f; // Provisional physical mass, not a disarm skill bonus.
    Item.bStackable = false;
    Item.MaxStackSize = 1;
    Item.HandUsage = EGridItemHandUsage::NotHandHeld;
    Item.ItemTags = { TEXT("Tool.Disarm") };
    Item.bProvidesQuickItemCombatAction = false;
    Item.QuickItemActionIdOverride = NAME_None;
    Item.QuickItemCombatAction = FGridCombatActionDefinition();
    Item.CombatActions.Reset();
}

bool FRPGStartingDisarmToolAuthoring::Matches(const UGridItemDefinitionAsset& Item)
{
    if (Item.ItemDefinitionId != FName(ARMEMENT0116::ItemId) ||
        Item.ItemType != EGridItemType::Misc ||
        !FMath::IsNearlyEqual(Item.Weight, 0.5f) ||
        Item.bStackable || Item.MaxStackSize != 1 ||
        Item.HandUsage != EGridItemHandUsage::NotHandHeld ||
        !Item.CompatibleEquipmentSlots.IsEmpty() ||
        !Item.CombatActions.IsEmpty() ||
        Item.bProvidesQuickItemCombatAction ||
        !Item.QuickItemActionIdOverride.IsNone() ||
        Item.bUseWeaponCatalogue || Item.bUseWeaponVisualCatalogue ||
        Item.bCombatThrowWeapon ||
        Item.ItemTags.Num() != 1 || !Item.ItemTags.Contains(TEXT("Tool.Disarm")))
    {
        return false;
    }

    const FText ExpectedName = FUILocalizationText::Get(ARMEMENT0116::NameKey);
    const FText ExpectedDescription = FUILocalizationText::Get(ARMEMENT0116::DescriptionKey);
    if (ExpectedName.IsEmpty() || ExpectedDescription.IsEmpty() ||
        !Item.DisplayName.EqualTo(ExpectedName) ||
        !Item.Description.EqualTo(ExpectedDescription) ||
        !Item.IsValidDefinition())
    {
        return false;
    }

    FGridCombatActionDefinition Action;
    return !Item.BuildQuickItemCombatActionDefinition(Action) &&
        !Item.BuildInventoryCombatActionDefinition(Action);
}

bool FRPGStartingDisarmToolAuthoring::Run(bool bApply, FString& OutError)
{
    OutError.Reset();
    UGridItemDefinitionAsset* Candidate =
        NewObject<UGridItemDefinitionAsset>(GetTransientPackage());
    if (!Candidate)
    {
        OutError = TEXT("Cannot allocate a transient disarm-tool definition.");
        return false;
    }
    Configure(*Candidate);
    if (!Matches(*Candidate))
    {
        OutError = TEXT("Disarm tool authoring preflight failed. Synchronize ST_UI from UI.csv.");
        return false;
    }

    const FString FileName = FPackageName::LongPackageNameToFilename(
        ARMEMENT0116::PackageName, FPackageName::GetAssetPackageExtension());
    if (FPaths::FileExists(FileName) || FPackageName::DoesPackageExist(ARMEMENT0116::PackageName))
    {
        UGridItemDefinitionAsset* Existing = LoadObject<UGridItemDefinitionAsset>(
            nullptr, ARMEMENT0116::ObjectPath);
        if (!IsValid(Existing) || !Matches(*Existing))
        {
            OutError = TEXT("Existing disarm tool conflicts with approved N1 physical contract. Refusing overwrite.");
            return false;
        }
        UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.16.1] PRESENT_SAME %s"),
            ARMEMENT0116::ObjectPath);
        return true;
    }

    if (!bApply)
    {
        UE_LOG(LogTemp, Display,
            TEXT("[ARMEMENT01.16.1] DRY_RUN READY %s Tag=Tool.Disarm (no executor, no kit grant)"),
            ARMEMENT0116::ObjectPath);
        return true;
    }

    UPackage* Package = CreatePackage(ARMEMENT0116::PackageName);
    UGridItemDefinitionAsset* Asset = Package
        ? NewObject<UGridItemDefinitionAsset>(Package, ARMEMENT0116::AssetName,
            RF_Public | RF_Standalone | RF_Transactional)
        : nullptr;
    if (!Asset)
    {
        OutError = TEXT("Unable to create the disarm tool package.");
        return false;
    }
    Configure(*Asset);
    if (!Matches(*Asset))
    {
        OutError = TEXT("Newly authored disarm tool does not satisfy its contract.");
        return false;
    }
    FAssetRegistryModule::AssetCreated(Asset);
    Asset->MarkPackageDirty();
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(FileName), true);
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    if (!UPackage::SavePackage(Package, Asset, *FileName, Args))
    {
        OutError = FString::Printf(TEXT("Could not save disarm tool package: %s"), *FileName);
        return false;
    }
    UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.16.1] SAVED %s"), ARMEMENT0116::ObjectPath);
    return true;
}
