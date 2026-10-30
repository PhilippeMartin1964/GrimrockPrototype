#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "RPG/RPGStartingHealthPotionAuthoring.h"
#include "Runtime/Combat/GridWeaponVisualResolver.h"
#include "Runtime/Combat/GridWorldItemVisualResolver.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

namespace ARMEMENT01154
{
    constexpr const TCHAR* Root = TEXT("/Game/GrimrockPrototype/Core/DataAssets");
    constexpr const TCHAR* PotionPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Potion_Healing_01.DA_Item_Potion_Healing_01");
    constexpr const TCHAR* PalettePath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default");
    FName PotionId() { return FName(TEXT("Item_Potion_Healing_01")); }

    FString Csv(const FString& Value)
    {
        FString Escaped = Value;
        Escaped.ReplaceInline(TEXT("\""), TEXT("\"\""));
        return FString::Printf(TEXT("\"%s\""), *Escaped);
    }

    bool SupportsExplorationSelfConsume(const UGridItemDefinitionAsset& Item,
        const FGridCombatActionDefinition& Action)
    {
        return (Item.ItemType == EGridItemType::Potion || Item.ItemType == EGridItemType::Food) &&
            Action.TargetingPolicy == EGridCombatTargetingPolicy::Self &&
            Action.ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
            Action.ResourceCosts.SourceItemQuantityCost == 1 &&
            Action.ResourceCosts.ManaCost == 0 && Action.EffectProfile.IsValid() &&
            Action.StatusApplications.IsEmpty() && Action.StatusRemovals.IsEmpty() &&
            Action.ArmorEffects.IsEmpty() && Action.MovementEffects.IsEmpty() &&
            Action.SurfaceEffects.IsEmpty() && Action.SurfaceConversions.IsEmpty() &&
            !Action.TrapEffect.bPlaceTrap;
    }

    FString Status(const UGridItemDefinitionAsset& Item, bool bValid,
        bool bIcon, bool bWorldMesh, bool bQuickBuilds, bool bExplorerSupported, bool bDuplicate)
    {
        TArray<FString> Issues;
        if (!bValid) Issues.Add(TEXT("InvalidDefinition"));
        if (!bIcon) Issues.Add(TEXT("MissingOrUnloadedIcon"));
        if (!bWorldMesh) Issues.Add(TEXT("MissingOrUnloadedWorldMesh"));
        if (bDuplicate) Issues.Add(TEXT("DuplicateItemId"));
        if (Item.bProvidesQuickItemCombatAction && !bQuickBuilds)
        {
            Issues.Add(TEXT("InvalidQuickItemAction"));
        }
        if ((Item.ItemType == EGridItemType::Potion || Item.ItemType == EGridItemType::Food) &&
            !bQuickBuilds)
        {
            Issues.Add(TEXT("ConsumeWithoutQuickItemExecutor"));
        }
        // This is an advisory policy: an Area, Ally or hostile QuickItem may
        // execute in combat. It is not safe to synthesize an exploration effect.
        if (bQuickBuilds && !bExplorerSupported)
        {
            Issues.Add(TEXT("QuickItemNeedsCombatOrTargetReview"));
        }
        return Issues.IsEmpty() ? TEXT("Ready") : FString::Join(Issues, TEXT("|"));
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01154InventoryItemAudit,
    "Grimrock.RPG.ARMEMENT01.15.Closure.ActiveItemAudit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01154InventoryItemAudit::RunTest(const FString&)
{
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
        TEXT("AssetRegistry")).Get();
    Registry.ScanPathsSynchronous({ FString(ARMEMENT01154::Root) }, true);
    TArray<FAssetData> Assets;
    Registry.GetAssetsByPath(FName(ARMEMENT01154::Root), Assets, true);
    Assets.Sort([](const FAssetData& Left, const FAssetData& Right)
    {
        return Left.GetSoftObjectPath().ToString() < Right.GetSoftObjectPath().ToString();
    });

    TArray<UGridItemDefinitionAsset*> Items;
    TMap<FName, int32> DefinitionsById;
    for (const FAssetData& Data : Assets)
    {
        // Scan every real item definition, not merely DA_Item_* filenames.
        // Non-item assets must not be synchronously loaded for this audit.
        if (Data.AssetClassPath != UGridItemDefinitionAsset::StaticClass()->GetClassPathName())
        {
            continue;
        }
        UGridItemDefinitionAsset* Item = Cast<UGridItemDefinitionAsset>(Data.GetAsset());
        if (!IsValid(Item))
        {
            AddError(FString::Printf(TEXT("[ARMEMENT01.15.4] Item package cannot be loaded: %s"),
                *Data.GetSoftObjectPath().ToString()));
            continue;
        }
        Items.Add(Item);
        DefinitionsById.FindOrAdd(Item->ItemDefinitionId) += 1;
    }

    FString Report = TEXT(
        "Asset,ItemDefinitionId,ItemType,DefinitionValid,IconReference,IconLoaded,"
        "WorldMeshReference,WorldMeshLoaded,QuickItemDeclared,QuickItemValid,"
        "ExplorationSelfConsume,Status\n");
    int32 Ready = 0;
    int32 NeedsReview = 0;
    int32 MissingIcon = 0;
    int32 MissingMesh = 0;
    int32 InvalidQuick = 0;
    bool bPotionScanned = false;
    bool bPotionIcon = false;
    bool bPotionMesh = false;
    bool bPotionQuick = false;
    bool bPotionExploration = false;

    for (UGridItemDefinitionAsset* Item : Items)
    {
        const bool bWasDirty = Item->GetOutermost()->IsDirty();
        const bool bValid = Item->IsValidDefinition();

        if (Item->bUseWeaponVisualCatalogue && !Item->WeaponVisualCatalogue.IsNull())
        {
            Item->WeaponVisualCatalogue.LoadSynchronous();
        }
        FGridResolvedWeaponVisual Visual;
        EGridWeaponVisualResolutionError VisualError = EGridWeaponVisualResolutionError::None;
        FGridWeaponVisualResolver::TryResolve(*Item, nullptr, Visual, VisualError);
        const bool bIconRef = !Visual.Icon.IsNull();
        const bool bIconLoaded = bIconRef && IsValid(Visual.Icon.LoadSynchronous());
        const bool bMeshRef = !Visual.WorldMesh.IsNull() || !Item->WorldMesh.IsNull();
        const bool bMeshLoaded = IsValid(FGridWorldItemVisualResolver::Resolve(*Item).Mesh);

        FGridCombatActionDefinition Action;
        const bool bQuickBuilds = Item->bProvidesQuickItemCombatAction &&
            Item->BuildQuickItemCombatActionDefinition(Action);
        const bool bExplorerSupported = bQuickBuilds &&
            ARMEMENT01154::SupportsExplorationSelfConsume(*Item, Action);
        const bool bDuplicate = DefinitionsById.FindRef(Item->ItemDefinitionId) > 1;
        const FString Status = ARMEMENT01154::Status(
            *Item, bValid, bIconLoaded, bMeshLoaded, bQuickBuilds, bExplorerSupported, bDuplicate);
        if (Status == TEXT("Ready")) ++Ready;
        else ++NeedsReview;
        if (!bIconLoaded) ++MissingIcon;
        if (!bMeshLoaded) ++MissingMesh;
        if (Item->bProvidesQuickItemCombatAction && !bQuickBuilds) ++InvalidQuick;

        Report += FString::Printf(TEXT("%s,%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%s\n"),
            *ARMEMENT01154::Csv(Item->GetPathName()),
            *ARMEMENT01154::Csv(Item->ItemDefinitionId.ToString()),
            *ARMEMENT01154::Csv(StaticEnum<EGridItemType>()->GetNameStringByValue(
                static_cast<int64>(Item->ItemType))),
            bValid ? 1 : 0, bIconRef ? 1 : 0, bIconLoaded ? 1 : 0,
            bMeshRef ? 1 : 0, bMeshLoaded ? 1 : 0,
            Item->bProvidesQuickItemCombatAction ? 1 : 0,
            bQuickBuilds ? 1 : 0, bExplorerSupported ? 1 : 0,
            *ARMEMENT01154::Csv(Status));

        if (Item->ItemDefinitionId == ARMEMENT01154::PotionId())
        {
            bPotionScanned = true;
            bPotionIcon = bIconLoaded;
            bPotionMesh = bMeshLoaded;
            bPotionQuick = bQuickBuilds;
            bPotionExploration = bExplorerSupported;
        }

        if (Status != TEXT("Ready"))
        {
            AddInfo(FString::Printf(TEXT("[ARMEMENT01.15.4] REVIEW %s: %s"),
                *Item->GetPathName(), *Status));
        }
        TestEqual(TEXT("Read-only audit never dirties an item package"),
            Item->GetOutermost()->IsDirty(), bWasDirty);
    }

    const FString Directory = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("Automation/ARMEMENT01"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString ReportPath = FPaths::Combine(
        Directory, TEXT("ARMEMENT01_15_ActiveItemAudit.csv"));
    TestTrue(TEXT("The full item audit report is written under Saved/Automation"),
        FFileHelper::SaveStringToFile(Report, *ReportPath,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));

    AddInfo(FString::Printf(
        TEXT("[ARMEMENT01.15.4] Item assets=%d Ready=%d Review=%d MissingIcon=%d MissingWorldMesh=%d InvalidQuickItem=%d Report=%s"),
        Items.Num(), Ready, NeedsReview, MissingIcon, MissingMesh, InvalidQuick, *ReportPath));

    // Only the approved N1 potion is a release gate for ARMEMENT01.15.
    // Pre-existing partial content is inventoried for future work, not failed.
    UGridItemDefinitionAsset* Potion = LoadObject<UGridItemDefinitionAsset>(
        nullptr, ARMEMENT01154::PotionPath);
    if (!TestNotNull(TEXT("One canonical N1 potion package exists"), Potion)) return false;
    TestEqual(TEXT("N1 potion has no duplicate ItemDefinitionId"),
        DefinitionsById.FindRef(ARMEMENT01154::PotionId()), 1);
    TestTrue(TEXT("The N1 potion is discovered by the general item audit"), bPotionScanned);
    TestTrue(TEXT("N1 potion retains the validated QuickItem gameplay definition"),
        FRPGStartingHealthPotionAuthoring::Matches(*Potion));
    TestTrue(TEXT("N1 potion has a loadable inventory icon"), bPotionIcon);
    TestTrue(TEXT("N1 potion has a loadable world mesh"), bPotionMesh);
    TestTrue(TEXT("N1 potion builds its QuickItem combat action"), bPotionQuick);
    TestTrue(TEXT("N1 potion is eligible for self-consumption outside combat"),
        bPotionExploration);

    // The palette is independently authored; do not change unrelated entries
    // or require a palette entry merely to grant the potion in a starting kit.
    UGridObjectPaletteAsset* Palette = LoadObject<UGridObjectPaletteAsset>(
        nullptr, ARMEMENT01154::PalettePath);
    if (IsValid(Palette))
    {
        const bool bPaletteDirty = Palette->GetOutermost()->IsDirty();
        int32 References = 0;
        for (const FGridObjectPaletteEntry& Entry : Palette->Entries)
        {
            if (IsValid(Entry.DefaultItemDefinition) &&
                Entry.DefaultItemDefinition->ItemDefinitionId == ARMEMENT01154::PotionId())
            {
                ++References;
                TestTrue(TEXT("Existing N1 potion palette entry uses the canonical direct collectible contract"),
                    Entry.IsValidEntry() && Entry.DefaultItemDefinition == Potion);
            }
        }
        AddInfo(FString::Printf(
            TEXT("[ARMEMENT01.15.4] Potion palette entries=%d; 0 is valid for inventory-only starter items"),
            References));
        TestEqual(TEXT("The palette is never modified by the audit"),
            Palette->GetOutermost()->IsDirty(), bPaletteDirty);
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
