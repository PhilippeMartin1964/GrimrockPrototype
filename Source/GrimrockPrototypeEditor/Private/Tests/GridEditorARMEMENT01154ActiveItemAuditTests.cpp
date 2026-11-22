#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GridEditorCanonicalStarterTestWorld.h"
#include "Core/GridObjectPaletteAsset.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace ARMEMENT01154
{
    constexpr TCHAR PalettePath[] =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default");
    const FName PotionId(TEXT("Item_Potion_Healing_01"));

    FString Csv(FString Text)
    {
        Text.ReplaceInline(TEXT("\""), TEXT("\"\""));
        return FString::Printf(TEXT("\"%s\""), *Text);
    }

    bool SupportsExplorationSelfConsume(const FGridCombatActionDefinition& Action)
    {
        return Action.TargetingPolicy == EGridCombatTargetingPolicy::Self &&
            Action.ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
            Action.ResourceCosts.SourceItemQuantityCost == 1 &&
            Action.ResourceCosts.ManaCost == 0 &&
            Action.EffectProfile.IsValid() &&
            Action.StatusApplications.IsEmpty() && Action.StatusRemovals.IsEmpty() &&
            Action.ArmorEffects.IsEmpty() && Action.MovementEffects.IsEmpty() &&
            Action.SurfaceEffects.IsEmpty() && Action.SurfaceConversions.IsEmpty() &&
            !Action.TrapEffect.bPlaceTrap;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01154InventoryItemAudit,
    "Grimrock.RPG.ARMEMENT01.15.Closure.ActiveItemAudit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01154InventoryItemAudit::RunTest(const FString&)
{
    FGridEditorCanonicalStarterTestWorld Fixture;
    if (!TestTrue(TEXT("Canonical production registry configured"), Fixture.Ready()))
        return false;
    TArray<FName> Ids;
    Fixture.Registry->GetItemIds(Ids);
    if (!TestEqual(TEXT("Exactly 48 canonical identities"), Ids.Num(), 48))
        return false;

    FString Report = TEXT("ItemId,Level,Armament,IconLoaded,WorldMeshLoaded,QuickActionResolved,ExplorationSelfConsume,Status\n");
    int32 Ready = 0, NeedsReview = 0;
    int32 MissingIcon = 0, MissingMesh = 0, InvalidQuick = 0;
    bool bPotionScanned = false, bPotionIcon = false, bPotionMesh = false;
    bool bPotionQuick = false, bPotionExploration = false;

    for (FName Id : Ids)
    {
        FGridResolvedItem Item;
        const bool bResolved = Fixture.Registry->ResolveItem(Id, 0, Item) ||
            Fixture.Registry->ResolveItem(Id, 1, Item);
        if (!TestTrue(TEXT("Registered canonical identity resolves"), bResolved && Item.IsValid()))
            continue;
        const bool bIconLoaded = Item.ItemIcon.LoadSynchronous() != nullptr;
        const bool bMeshLoaded = Item.WorldMesh.LoadSynchronous() != nullptr;
        FGridCombatActionDefinition Action;
        bool bQuickResolved = false;
        bool bExplore = false;
        for (const FGridItemActionBinding& Binding : Item.AvailableActions)
        {
            if (Fixture.Registry->ResolveDetailedNonArmAction(Item.ItemId, Item.ItemLevel,
                Binding.ActionId, Action))
            {
                bQuickResolved = true;
                bExplore |= ARMEMENT01154::SupportsExplorationSelfConsume(Action);
            }
        }
        const bool bExpectsQuick = Item.Tags.Contains(TEXT("QuickItem.Potion")) ||
            Item.Tags.Contains(TEXT("QuickItem.Food"));
        const bool bReady = bIconLoaded && bMeshLoaded && (!bExpectsQuick || bQuickResolved);
        Ready += bReady ? 1 : 0;
        NeedsReview += bReady ? 0 : 1;
        MissingIcon += bIconLoaded ? 0 : 1;
        MissingMesh += bMeshLoaded ? 0 : 1;
        InvalidQuick += bExpectsQuick && !bQuickResolved ? 1 : 0;
        Report += FString::Printf(TEXT("%s,%d,%d,%d,%d,%d,%d,%s\n"),
            *ARMEMENT01154::Csv(Item.ItemId.ToString()), Item.ItemLevel,
            Item.bIsArmament ? 1 : 0, bIconLoaded ? 1 : 0, bMeshLoaded ? 1 : 0,
            bQuickResolved ? 1 : 0, bExplore ? 1 : 0,
            bReady ? TEXT("Ready") : TEXT("Review"));
        if (Item.ItemId == ARMEMENT01154::PotionId)
        {
            bPotionScanned = true;
            bPotionIcon = bIconLoaded;
            bPotionMesh = bMeshLoaded;
            bPotionQuick = bQuickResolved;
            bPotionExploration = bExplore;
        }
    }
    const FString Directory = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("Automation/ARMEMENT01"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString ReportPath = FPaths::Combine(
        Directory, TEXT("ARMEMENT01_15_ActiveItemAudit.csv"));
    TestTrue(TEXT("Canonical inventory CSV written under Saved/Automation"),
        FFileHelper::SaveStringToFile(Report, *ReportPath,
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
    AddInfo(FString::Printf(
        TEXT("[ARM-CLEAN06] Canonical identities=%d Ready=%d Review=%d MissingIcon=%d MissingWorldMesh=%d MissingQuick=%d"),
        Ids.Num(), Ready, NeedsReview, MissingIcon, MissingMesh, InvalidQuick));

    TestTrue(TEXT("Canonical potion discovered"), bPotionScanned);
    TestTrue(TEXT("Canonical healing icon loads"), bPotionIcon);
    TestTrue(TEXT("Canonical healing world mesh loads"), bPotionMesh);
    TestTrue(TEXT("Canonical healing action is bound"), bPotionQuick);
    TestTrue(TEXT("Healing supports exploration self-consume"), bPotionExploration);

    UGridObjectPaletteAsset* Palette =
        LoadObject<UGridObjectPaletteAsset>(nullptr, ARMEMENT01154::PalettePath);
    if (IsValid(Palette))
    {
        const bool bWasDirty = Palette->GetOutermost()->IsDirty();
        int32 References = 0;
        for (const FGridObjectPaletteEntry& Entry : Palette->Entries)
        {
            if (Entry.CanonicalItemId == ARMEMENT01154::PotionId)
            {
                ++References;
                TestTrue(TEXT("Potion palette is canonical and direct"),
                    Entry.IsValidEntry() && Entry.CanonicalItemId == ARMEMENT01154::PotionId);
            }
        }
        AddInfo(FString::Printf(TEXT("[ARM-CLEAN06] Potion palette entries=%d"), References));
        TestEqual(TEXT("Palette not mutated by canonical audit"),
            Palette->GetOutermost()->IsDirty(), bWasDirty);
    }
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
