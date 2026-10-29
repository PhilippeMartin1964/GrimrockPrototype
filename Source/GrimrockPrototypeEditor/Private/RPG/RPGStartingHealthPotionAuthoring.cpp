#include "RPG/RPGStartingHealthPotionAuthoring.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UI/UILocalizationText.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

namespace ARMEMENT0115
{
    constexpr const TCHAR* AssetName = TEXT("DA_Item_Potion_Healing_01");
    constexpr const TCHAR* ItemId = TEXT("Item_Potion_Healing_01");
    constexpr const TCHAR* PackageName = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Potion_Healing_01");
    constexpr const TCHAR* ObjectPath = TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Potion_Healing_01.DA_Item_Potion_Healing_01");
    constexpr const TCHAR* NameKey = TEXT("Item.StartingPotion.Healing01.Name");
    constexpr const TCHAR* DescriptionKey = TEXT("Item.StartingPotion.Healing01.Description");

    bool LocalizedPresentationAvailable(const UGridItemDefinitionAsset& Item)
    {
        return !Item.DisplayName.IsEmpty() && !Item.Description.IsEmpty();
    }
}

void FRPGStartingHealthPotionAuthoring::Configure(UGridItemDefinitionAsset& Item)
{
    // Presentation is authored from the canonical ST_UI StringTable, never
    // from a hardcoded player-facing literal in runtime C++.
    Item.ItemDefinitionId = ARMEMENT0115::ItemId;
    Item.DisplayName = FUILocalizationText::Get(ARMEMENT0115::NameKey);
    Item.Description = FUILocalizationText::Get(ARMEMENT0115::DescriptionKey);
    Item.ItemType = EGridItemType::Potion;
    Item.Weight = 0.2f;
    Item.bStackable = true;
    Item.MaxStackSize = 10;
    Item.HandUsage = EGridItemHandUsage::NotHandHeld;
    Item.ItemTags = { TEXT("QuickItem.Potion"), TEXT("QuickItem.Potion.Positive") };
    Item.bProvidesQuickItemCombatAction = true;
    Item.QuickItemActionIdOverride = NAME_None;

    FGridCombatActionDefinition Action;
    Action.ActionId = FGridCombatHotbarBinding::MakeQuickItemActionId(Item.ItemDefinitionId);
    Action.DisplayName = Item.DisplayName;
    Action.Description = Item.Description;
    Action.ActionType = EGridCombatActionType::Ability;
    Action.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
    Action.TargetingPolicy = EGridCombatTargetingPolicy::Self;
    Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
    Action.ActionPointCost = ActionPointCost;
    Action.RangeCells = 0;
    Action.ResourceCosts.SourceItemQuantityCost = 1;
    Action.EffectProfile.RestoreHealth = HealAmount;
    Item.QuickItemCombatAction = Action;
}

bool FRPGStartingHealthPotionAuthoring::Matches(const UGridItemDefinitionAsset& Item)
{
    if (Item.ItemDefinitionId != FName(ARMEMENT0115::ItemId) ||
        Item.ItemType != EGridItemType::Potion ||
        !FMath::IsNearlyEqual(Item.Weight, 0.2f) ||
        !Item.bStackable || Item.MaxStackSize != 10 ||
        Item.HandUsage != EGridItemHandUsage::NotHandHeld ||
        !Item.bProvidesQuickItemCombatAction ||
        !Item.QuickItemActionIdOverride.IsNone() ||
        Item.bUseWeaponCatalogue || Item.bCombatThrowWeapon ||
        !ARMEMENT0115::LocalizedPresentationAvailable(Item) ||
        !Item.ItemTags.Contains(TEXT("QuickItem.Potion")) ||
        !Item.ItemTags.Contains(TEXT("QuickItem.Potion.Positive")) ||
        Item.ItemTags.Num() != 2 ||
        !Item.CombatActions.IsEmpty())
    {
        return false;
    }

    const FText ExpectedName = FUILocalizationText::Get(ARMEMENT0115::NameKey);
    const FText ExpectedDescription = FUILocalizationText::Get(ARMEMENT0115::DescriptionKey);
    if (ExpectedName.IsEmpty() || ExpectedDescription.IsEmpty() ||
        !Item.DisplayName.EqualTo(ExpectedName) || !Item.Description.EqualTo(ExpectedDescription))
    {
        return false;
    }

    FGridCombatActionDefinition Normalized;
    if (!Item.IsValidDefinition() || !Item.BuildQuickItemCombatActionDefinition(Normalized))
    {
        return false;
    }
    return Normalized.ActionId == FGridCombatHotbarBinding::MakeQuickItemActionId(Item.ItemDefinitionId) &&
        Normalized.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem &&
        Normalized.ActionType == EGridCombatActionType::Ability &&
        Normalized.TargetingPolicy == EGridCombatTargetingPolicy::Self &&
        Normalized.ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
        Normalized.ActionPointCost == ActionPointCost &&
        Normalized.RangeCells == 0 &&
        Normalized.ResourceCosts.SourceItemQuantityCost == 1 &&
        Normalized.EffectProfile.RestoreHealth == HealAmount &&
        Normalized.EffectProfile.RestoreMana == 0 &&
        Normalized.QuickItemScaling.RestoreHealthSkillRankScale == 0 &&
        Normalized.QuickItemScaling.RestoreManaSkillRankScale == 0;
}

bool FRPGStartingHealthPotionAuthoring::Run(bool bApply, FString& OutError)
{
    OutError.Reset();
    UGridItemDefinitionAsset* Candidate = NewObject<UGridItemDefinitionAsset>(GetTransientPackage());
    if (!Candidate)
    {
        OutError = TEXT("Unable to create a transient health potion definition.");
        return false;
    }
    Configure(*Candidate);
    if (!Matches(*Candidate))
    {
        OutError = TEXT("N1 healing potion preflight failed. Refresh ST_UI from Localization/Source/UI.csv and check QuickItem configuration.");
        return false;
    }

    const FString FileName = FPackageName::LongPackageNameToFilename(
        ARMEMENT0115::PackageName, FPackageName::GetAssetPackageExtension());
    if (FPaths::FileExists(FileName) || FPackageName::DoesPackageExist(ARMEMENT0115::PackageName))
    {
        UGridItemDefinitionAsset* Existing = LoadObject<UGridItemDefinitionAsset>(
            nullptr, ARMEMENT0115::ObjectPath);
        if (!IsValid(Existing) || !Matches(*Existing))
        {
            OutError = TEXT("Existing N1 potion package conflicts with the approved recipe; refusing overwrite.");
            return false;
        }
        UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.15] PRESENT_SAME %s"), ARMEMENT0115::ObjectPath);
        return true;
    }

    if (!bApply)
    {
        UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.15] DRY_RUN READY %s Heal=%d AP=%d"),
            ARMEMENT0115::ObjectPath, HealAmount, ActionPointCost);
        return true;
    }

    UPackage* Package = CreatePackage(ARMEMENT0115::PackageName);
    UGridItemDefinitionAsset* Asset = Package
        ? NewObject<UGridItemDefinitionAsset>(Package, ARMEMENT0115::AssetName,
            RF_Public | RF_Standalone | RF_Transactional)
        : nullptr;
    if (!Asset)
    {
        OutError = TEXT("Failed to create the N1 potion package.");
        return false;
    }
    Configure(*Asset);
    if (!Matches(*Asset))
    {
        OutError = TEXT("Authored potion is invalid before save.");
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
        OutError = FString::Printf(TEXT("Could not save N1 healing potion: %s"), *FileName);
        return false;
    }
    UE_LOG(LogTemp, Display, TEXT("[ARMEMENT01.15] SAVED %s"), ARMEMENT0115::ObjectPath);
    return true;
}
