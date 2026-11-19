#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"
#include "../EditorTools/Widgets/GridEditorCanonicalPaletteReadModel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorClean064X14SlateReadModel,
    "Grimrock.RPG.ARM.CLEAN06_4X14.CanonicalSlatePaletteReadModel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorClean064X14SlateReadModel::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    TStrongObjectPtr<UGridObjectPaletteAsset> Palette(LoadObject<UGridObjectPaletteAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default")));
    if (!TestNotNull(TEXT("Native registry"), Root.Get()) ||
        !TestNotNull(TEXT("Native palette"), Palette.Get())) {return false;}
    const bool bRootDirty = Root->GetOutermost()->IsDirty();
    const bool bPaletteDirty = Palette->GetOutermost()->IsDirty();
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Valid canonical registry"), Registry->RegisterItemSystem(Root.Get(), Errors)))
        {return false;}
    int32 Count = 0;
    int32 OldPointers = 0;
    const FGridObjectPaletteEntry* Arm = nullptr;
    for (const FGridObjectPaletteEntry& Entry : Palette->Entries)
    {
        if (!Entry.IsDirectItemEntry()) {continue;}
        ++Count;
        const FGridEditorCanonicalPaletteReadView View =
            FGridEditorCanonicalPaletteReadModel::Resolve(Entry, Registry.Get());
        FGridResolvedItem Expected;
        if (!TestTrue(TEXT("Canonical identity resolves at authored physical level"),
                Registry->ResolveItem(Entry.CanonicalItemId, Entry.CanonicalDefaultItemLevel, Expected)))
            return false;
        const UGridItemDefinitionAsset* OldSource = Entry.DefaultItemDefinition.Get();
        if (OldSource) ++OldPointers;
        const FString ExpectedLabel = Expected.DisplayName.IsEmpty()
            ? Entry.CanonicalItemId.ToString() : Expected.DisplayName.ToString();
        if (!TestTrue(TEXT("Real direct entry uses canonical projection"),
                View.bCanonicalSource && View.bResolved) ||
            !TestEqual(TEXT("ItemId unchanged"), View.ItemId, Entry.CanonicalItemId) ||
            !TestEqual(TEXT("Visible label uses canonical resolution"), View.DisplayName.ToString(), ExpectedLabel) ||
            !TestEqual(TEXT("Search description uses canonical resolution"),
                View.Description.ToString(), Expected.Description.ToString()) ||
            !TestTrue(TEXT("Tile icon uses canonical resolution"),
                View.Icon.ToSoftObjectPath() == Expected.ItemIcon.ToSoftObjectPath()) ||
            !TestTrue(TEXT("Historical presentation parity while pointer exists"), !OldSource ||
                (View.DisplayName.ToString() == Entry.GetEffectiveDisplayName().ToString() &&
                 View.Description.ToString() == OldSource->Description.ToString() &&
                 View.Icon.ToSoftObjectPath() == OldSource->Icon.ToSoftObjectPath())))
            {return false;}
        if (Entry.CanonicalDefaultItemLevel > 0 && !Arm) {Arm = &Entry;}
    }
    if (!TestEqual(TEXT("33 real direct entries"), Count, 33) ||
        !TestTrue(TEXT("Complete native palette state"), OldPointers == 0) ||
        !TestNotNull(TEXT("At least one ARM entry exists"), Arm))
        {return false;}

    // Keep genuine synthetic conflicting and legacy-only fixtures post-conversion.
    const FGridEditorCanonicalPaletteReadView ArmView =
        FGridEditorCanonicalPaletteReadModel::Resolve(*Arm, Registry.Get());
    TStrongObjectPtr<UGridItemDefinitionAsset> Fixture(NewObject<UGridItemDefinitionAsset>());
    Fixture->ItemDefinitionId = Arm->CanonicalItemId;
    Fixture->DisplayName = ArmView.DisplayName;
    Fixture->Description = ArmView.Description;
    Fixture->Icon = ArmView.Icon;
    FGridObjectPaletteEntry Bad = *Arm;
    Bad.DefaultItemDefinition = Fixture.Get();
    Bad.CanonicalItemId = TEXT("Item_Arrows_01");
    FGridEditorCanonicalPaletteReadView Invalid =
        FGridEditorCanonicalPaletteReadModel::Resolve(Bad, Registry.Get());
    if (!TestTrue(TEXT("Known conflicting canonical source is recognized"),
        Invalid.bCanonicalSource) ||
        !TestFalse(TEXT("Conflicting source cannot use legacy projection"), Invalid.bResolved) ||
        !TestTrue(TEXT("Conflicting source has no legacy icon"), Invalid.Icon.IsNull()) ||
        !TestTrue(TEXT("Conflicting source has no legacy description"), Invalid.Description.IsEmpty()))
        {return false;}

    Bad = *Arm;
    Bad.DefaultItemDefinition = Fixture.Get();
    const FGridEditorCanonicalPaletteReadView MatchingOld =
        FGridEditorCanonicalPaletteReadModel::Resolve(Bad, Registry.Get());
    if (!TestFalse(TEXT("Even agreeing historical pointer cannot supply presentation"), MatchingOld.bResolved) ||
        !TestTrue(TEXT("No old icon or description"), MatchingOld.Icon.IsNull() && MatchingOld.Description.IsEmpty()))
        return false;
    FGridObjectPaletteEntry Legacy = *Arm;
    Legacy.CanonicalItemId = NAME_None;
    Legacy.DefaultItemDefinition = Fixture.Get();
    const FGridEditorCanonicalPaletteReadView Old =
        FGridEditorCanonicalPaletteReadModel::Resolve(Legacy, Registry.Get());
    const FGridEditorCanonicalPaletteReadView NoRegistry =
        FGridEditorCanonicalPaletteReadModel::Resolve(*Arm, nullptr);
    return TestFalse(TEXT("Old-only entry has no canonical identity"), Old.bCanonicalSource) &&
        TestFalse(TEXT("Legacy-only tile cannot resolve"), Old.bResolved) &&
        TestTrue(TEXT("Legacy-only tile has no historical presentation"), Old.Icon.IsNull() &&
            Old.Description.IsEmpty() && Old.DisplayName.ToString() == Legacy.EntryId.ToString()) &&
        TestFalse(TEXT("Missing registry cannot resolve"), NoRegistry.bResolved) &&
        TestEqual(TEXT("No native palette mutation"), Palette->GetOutermost()->IsDirty(), bPaletteDirty) &&
        TestEqual(TEXT("No registry mutation"), Root->GetOutermost()->IsDirty(), bRootDirty);
}
#endif
