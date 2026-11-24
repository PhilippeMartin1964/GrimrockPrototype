#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridObjectPaletteAsset.h"
#include "Runtime/Items/GridItemCatalogues.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"
#include "../EditorTools/Widgets/GridEditorPaletteReadModel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorClean064X14SlateReadModel,
    "Grimrock.RPG.ARM.CLEAN06_4X14.CanonicalSlatePaletteReadModel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorClean064X14SlateReadModel::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_ItemSystem.DA_ItemSystem")));
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
    const FGridObjectPaletteEntry* Arm = nullptr;
    for (const FGridObjectPaletteEntry& Entry : Palette->Entries)
    {
        if (!Entry.IsDirectItemEntry()) {continue;}
        ++Count;
        const FGridEditorPaletteReadView View =
            FGridEditorPaletteReadModel::Resolve(Entry, Registry.Get());
        FGridResolvedItem Expected;
        if (!TestTrue(TEXT("Canonical identity resolves at authored physical level"),
                Registry->ResolveItem(Entry.ItemId, Entry.DefaultItemLevel, Expected)))
            return false;
        const FString ExpectedLabel = Expected.DisplayName.IsEmpty()
            ? Entry.ItemId.ToString() : Expected.DisplayName.ToString();
        if (!TestTrue(TEXT("Real direct entry uses canonical projection"),
                View.bRegisteredSource && View.bResolved) ||
            !TestEqual(TEXT("ItemId unchanged"), View.ItemId, Entry.ItemId) ||
            !TestEqual(TEXT("Visible label uses canonical resolution"), View.DisplayName.ToString(), ExpectedLabel) ||
            !TestEqual(TEXT("Search description uses canonical resolution"),
                View.Description.ToString(), Expected.Description.ToString()) ||
            !TestTrue(TEXT("Tile icon uses canonical resolution"),
                View.Icon.ToSoftObjectPath() == Expected.ItemIcon.ToSoftObjectPath()) )
            {return false;}
        if (Entry.DefaultItemLevel > 0 && !Arm) {Arm = &Entry;}
    }
    if (!TestEqual(TEXT("33 real direct entries"), Count, 33) ||
        !TestNull(TEXT("Removed palette pointer schema"), FGridObjectPaletteEntry::StaticStruct()->FindPropertyByName(TEXT("DefaultItemDefinition"))) ||
        !TestNotNull(TEXT("At least one ARM entry exists"), Arm))
        {return false;}

    FGridObjectPaletteEntry Bad = *Arm;
    Bad.DefaultItemLevel = 0;
    const auto WrongLevel = FGridEditorPaletteReadModel::Resolve(Bad, Registry.Get());
    Bad = *Arm;
    Bad.ItemId = TEXT("ARM_CLEAN06_UNKNOWN");
    const auto Unknown = FGridEditorPaletteReadModel::Resolve(Bad, Registry.Get());
    Bad = *Arm;
    Bad.ItemId = NAME_None;
    const auto NoRegistry = FGridEditorPaletteReadModel::Resolve(*Arm, nullptr);
    return TestFalse(TEXT("ARM N0 cannot resolve"), WrongLevel.bResolved) &&
        TestFalse(TEXT("Unknown direct identity cannot resolve"), Unknown.bResolved) &&
        TestTrue(TEXT("Unknown identity has no icon or description"), Unknown.Icon.IsNull() && Unknown.Description.IsEmpty()) &&
        TestFalse(TEXT("Empty direct identity without world source is invalid"), Bad.IsValidEntry()) &&
        TestFalse(TEXT("Missing registry cannot resolve"), NoRegistry.bResolved) &&
        TestEqual(TEXT("No native palette mutation"), Palette->GetOutermost()->IsDirty(), bPaletteDirty) &&
        TestEqual(TEXT("No registry mutation"), Root->GetOutermost()->IsDirty(), bRootDirty);
}
#endif
