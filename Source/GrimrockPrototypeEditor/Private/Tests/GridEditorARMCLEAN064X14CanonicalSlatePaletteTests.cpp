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
    const FGridObjectPaletteEntry* Arm = nullptr;
    for (const FGridObjectPaletteEntry& Entry : Palette->Entries)
    {
        if (!Entry.IsDirectItemEntry()) {continue;}
        ++Count;
        const FGridEditorCanonicalPaletteReadView View =
            FGridEditorCanonicalPaletteReadModel::Resolve(Entry, Registry.Get());
        if (!TestTrue(TEXT("Real direct entry uses canonical projection"),
                View.bCanonicalSource && View.bResolved) ||
            !TestEqual(TEXT("ItemId unchanged"), View.ItemId, Entry.CanonicalItemId) ||
            !TestEqual(TEXT("No visible label change"), View.DisplayName.ToString(),
                Entry.GetEffectiveDisplayName().ToString()) ||
            !TestEqual(TEXT("No visible search-description change"),
                View.Description.ToString(), Entry.DefaultItemDefinition->Description.ToString()) ||
            !TestTrue(TEXT("No tile icon change"),
                View.Icon.ToSoftObjectPath() == Entry.DefaultItemDefinition->Icon.ToSoftObjectPath()))
            {return false;}
        if (Entry.CanonicalDefaultItemLevel > 0 && !Arm) {Arm = &Entry;}
    }
    if (!TestEqual(TEXT("33 real direct entries"), Count, 33) ||
        !TestNotNull(TEXT("At least one ARM entry exists"), Arm))
        {return false;}

    FGridObjectPaletteEntry Bad = *Arm;
    Bad.CanonicalItemId = TEXT("Item_Arrows_01");
    FGridEditorCanonicalPaletteReadView Invalid =
        FGridEditorCanonicalPaletteReadModel::Resolve(Bad, Registry.Get());
    if (!TestTrue(TEXT("Known conflicting canonical source is recognized"),
        Invalid.bCanonicalSource) ||
        !TestFalse(TEXT("Conflicting source cannot use legacy projection"), Invalid.bResolved) ||
        !TestTrue(TEXT("Conflicting source has no legacy icon"), Invalid.Icon.IsNull()) ||
        !TestTrue(TEXT("Conflicting source has no legacy description"), Invalid.Description.IsEmpty()))
        {return false;}

    FGridObjectPaletteEntry Legacy = *Arm;
    Legacy.CanonicalItemId = NAME_None;
    const FGridEditorCanonicalPaletteReadView Old =
        FGridEditorCanonicalPaletteReadModel::Resolve(Legacy, nullptr);
    return TestFalse(TEXT("Synthetic fixture not considered canonical"), Old.bCanonicalSource) &&
        TestTrue(TEXT("Synthetic fixture retains historical icon and text"),
            Old.bResolved &&
            Old.Icon.ToSoftObjectPath() == Legacy.DefaultItemDefinition->Icon.ToSoftObjectPath() &&
            Old.DisplayName.ToString() == Legacy.GetEffectiveDisplayName().ToString()) &&
        TestEqual(TEXT("No native palette mutation"), Palette->GetOutermost()->IsDirty(), bPaletteDirty) &&
        TestEqual(TEXT("No registry mutation"), Root->GetOutermost()->IsDirty(), bRootDirty);
}
#endif
