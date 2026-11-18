#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorClean064X11RealPalette,
    "Grimrock.RPG.ARM.CLEAN06_4X11.RealCanonicalPaletteReadOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorClean064X11RealPalette::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    TStrongObjectPtr<UGridObjectPaletteAsset> Palette(LoadObject<UGridObjectPaletteAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default")));
    if (!TestNotNull(TEXT("Native canonical item registry"), Root.Get()) ||
        !TestNotNull(TEXT("Native editor palette"), Palette.Get()) ||
        !TestEqual(TEXT("Canonical family count"), Root->ItemDefinitions.Num(), 34))
    {
        return false;
    }
    const bool bRootDirty = Root->GetOutermost()->IsDirty();
    const bool bPaletteDirty = Palette->GetOutermost()->IsDirty();
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Single canonical registry initialized"),
        Registry->RegisterItemSystem(Root.Get(), Errors)))
    {
        for (const FString& Error : Errors) {AddError(Error);}
        return false;
    }

    int32 Direct = 0, Arms = 0, NonArms = 0, Old = 0;
    int32 LabelDiffs = 0, DescriptionDiffs = 0, IconDiffs = 0;
    TSet<FName> EntryIds;
    for (const FGridObjectPaletteEntry& Entry : Palette->Entries)
    {
        if (!Entry.IsDirectItemEntry())
        {
            continue; // Object archetypes and monster entries are outside X.11.
        }
        ++Direct;
        if (!TestTrue(TEXT("Direct entry has unique EntryId"),
            !Entry.EntryId.IsNone() && !EntryIds.Contains(Entry.EntryId)))
            {return false;}
        EntryIds.Add(Entry.EntryId);
        if (!TestTrue(TEXT("All real direct items have persisted ItemId"),
            !Entry.CanonicalItemId.IsNone()) ||
            !TestTrue(TEXT("Optional historical pointer agrees with persisted ItemId"),
                !Entry.DefaultItemDefinition ||
                Entry.DefaultItemDefinition->ItemDefinitionId == Entry.CanonicalItemId))
            {return false;}

        FGridResolvedItem Resolved;
        if (!TestTrue(TEXT("Real palette item and level resolve"),
            Registry->ResolveItem(Entry.CanonicalItemId,
                Entry.CanonicalDefaultItemLevel, Resolved)) ||
            !TestTrue(TEXT("Resolved identity is unchanged"),
                Resolved.IsValid() && Resolved.ItemId == Entry.CanonicalItemId &&
                Resolved.ItemLevel == Entry.CanonicalDefaultItemLevel))
            {return false;}
        if (Resolved.bIsArmament)
        {
            ++Arms;
            if (!TestTrue(TEXT("Real ARM palette default is physical N1-N20"),
                Entry.CanonicalDefaultItemLevel >= 1 &&
                Entry.CanonicalDefaultItemLevel <= 20)) {return false;}
        }
        else
        {
            ++NonArms;
            if (!TestEqual(TEXT("Real nonARM palette default is physical N0"),
                Entry.CanonicalDefaultItemLevel, 0)) {return false;}
        }
        // Measure parity, do not silently relabel or overwrite user-authored
        // data. Any differences require a dedicated mapping before editor cutover.
        if (Entry.DefaultItemDefinition)
        {
            ++Old;
            if (Entry.DefaultItemDefinition->DisplayName.ToString() != Resolved.DisplayName.ToString())
                {++LabelDiffs;}
            if (Entry.DefaultItemDefinition->Description.ToString() != Resolved.Description.ToString())
                {++DescriptionDiffs;}
            if (Entry.DefaultItemDefinition->Icon.ToSoftObjectPath() !=
                Resolved.ItemIcon.ToSoftObjectPath())
                {++IconDiffs;}
        }
    }
    if (!TestEqual(TEXT("All 33 direct entries inspected"), Direct, 33) ||
        !TestTrue(TEXT("Complete native pre/post state"), Old == 33 || Old == 0) ||
        !TestEqual(TEXT("Exactly 33 canonical physical levels"),
            Arms + NonArms, 33) ||
        !TestEqual(TEXT("Read-only palette access"),
            Palette->GetOutermost()->IsDirty(), bPaletteDirty) ||
        !TestEqual(TEXT("Read-only source registry access"),
            Root->GetOutermost()->IsDirty(), bRootDirty))
        {return false;}

    AddInfo(FString::Printf(TEXT(
        "[ARM-CLEAN06.4X11] 33 direct entries: %d ARM, %d nonARM; "
        "historical comparisons=%d: label=%d, description=%d, icon=%d differences; "
        "read-only (no binary write)."),
        Arms, NonArms, Old, LabelDiffs, DescriptionDiffs, IconDiffs));
    return !HasAnyErrors();
}
#endif
