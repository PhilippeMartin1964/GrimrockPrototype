#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"

namespace ARMCLEAN064X12
{
struct FEditorWorld
{
    UWorld* World = nullptr;
    FEditorWorld()
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).RequiresHitProxies(false)
            .CreatePhysicsScene(false).CreateNavigation(false)
            .CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
        World = UWorld::CreateWorld(EWorldType::EditorPreview, false,
            FName(*FString::Printf(TEXT("ARM_CLEAN064X12_%s"),
                *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
            nullptr, true, ERHIFeatureLevel::Num, &Values);
        if (World && GEngine)
        {
            FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
            Context.SetCurrentWorld(World);
        }
    }
    ~FEditorWorld()
    {
        if (World)
        {
            World->DestroyWorld(false);
            if (GEngine) {GEngine->DestroyWorldContext(World);}
        }
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorClean064X12PalettePlacement,
    "Grimrock.RPG.ARM.CLEAN06_4X12.CanonicalPalettePlacementAndEdit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorClean064X12PalettePlacement::RunTest(const FString&)
{
    TStrongObjectPtr<UGridObjectPaletteAsset> Palette(LoadObject<UGridObjectPaletteAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default")));
    if (!TestNotNull(TEXT("Real direct palette"), Palette.Get())) {return false;}
    const bool bPaletteWasDirty = Palette->GetOutermost()->IsDirty();

    const FGridObjectPaletteEntry* Arm = nullptr;
    const FGridObjectPaletteEntry* NonArm = nullptr;
    for (const FGridObjectPaletteEntry& Entry : Palette->Entries)
    {
        if (!Entry.IsDirectItemEntry() || Entry.CanonicalItemId.IsNone()) {continue;}
        if (Entry.CanonicalDefaultItemLevel == 0 && !NonArm) {NonArm = &Entry;}
        if (Entry.CanonicalDefaultItemLevel > 0 && !Arm) {Arm = &Entry;}
    }
    if (!TestNotNull(TEXT("One real nonARM palette item"), NonArm) ||
        !TestNotNull(TEXT("One real ARM palette item"), Arm)) {return false;}

    ARMCLEAN064X12::FEditorWorld TestWorld;
    if (!TestNotNull(TEXT("Transient editor world"), TestWorld.World)) {return false;}
    AGridLevelEditorActor* Editor = TestWorld.World->SpawnActor<AGridLevelEditorActor>();
    if (!TestNotNull(TEXT("Grid editor exists"), Editor)) {return false;}
    UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Editor);
    Level->Width = 2;
    Level->Height = 2;
    Level->EnsureCellCount();
    for (FGridLevelCellData& Cell : Level->Cells)
    {
        Cell.CellType = EGridCellType::Floor;
        Cell.bBlocksOccupancy = false;
    }
    Editor->LevelAsset = Level;
    Editor->ObjectPalette = Palette.Get();
    Editor->SelectedCellX = 1;
    Editor->SelectedCellY = 1;
    Editor->SelectedEdge = EGridEdge::None;

    const auto Place = [&](const FGridObjectPaletteEntry* Entry) -> bool
    {
        if (!Editor->ApplyPaletteEntry(Entry->EntryId)) {return false;}
        const int32 Before = Level->LooseItemInstances.Num();
        Editor->PlaceSelectedObject();
        if (Level->LooseItemInstances.Num() != Before + 1) {return false;}
        const FGridLooseItemInstance& Item = Level->LooseItemInstances.Last();
        return Item.InstanceId.IsValid() &&
            Item.CanonicalItemId == Entry->CanonicalItemId &&
            FGridLooseItemInstance::StaticStruct()->FindPropertyByName(TEXT("ItemDefinition")) == nullptr &&
            Item.ItemLevel == Entry->CanonicalDefaultItemLevel &&
            Item.Quantity == 1 &&
            Item.CellX == 1 && Item.CellY == 1;
    };

    if (!TestTrue(TEXT("Real N0 direct entry creates physical canonical loose item"),
            Place(NonArm)) ||
        !TestTrue(TEXT("Real ARM entry writes authored N-level without N1 inference"),
            Place(Arm)))
        {return false;}

    const FGuid ArmGuid = Level->LooseItemInstances.Last().InstanceId;
    const int32 AuthoredLevel = Level->LooseItemInstances.Last().ItemLevel;
    // Editing label/notes on the selected ARM must not overwrite an authored
    // explicit N-level with the palette default.
    Level->LooseItemInstances.Last().ItemLevel = 20;
    if (!TestTrue(TEXT("Selected ARM can be edited"), Editor->ApplyEditedSelectedObject()) ||
        !TestEqual(TEXT("Selected ARM keeps its manually authored physical level"),
            Level->LooseItemInstances.Last().ItemLevel, 20) ||
        !TestTrue(TEXT("Selected item GUID never changed"),
            Level->LooseItemInstances.Last().InstanceId == ArmGuid) ||
        !TestEqual(TEXT("Selected ARM ItemId never changed"),
            Level->LooseItemInstances.Last().CanonicalItemId, Arm->CanonicalItemId))
        {return false;}

    TStrongObjectPtr<UGridObjectPaletteAsset> Invalid(NewObject<UGridObjectPaletteAsset>(Editor));
    FGridObjectPaletteEntry Bad = *Arm;
    Bad.EntryId = TEXT("ARM_CLEAN06_4X12_INVALID");
    UGridItemDefinitionAsset* ContradictorySource = NewObject<UGridItemDefinitionAsset>(Invalid.Get());
    ContradictorySource->ItemDefinitionId = Arm->CanonicalItemId;
    Bad.DefaultItemDefinition = ContradictorySource;
    Bad.CanonicalItemId = TEXT("Item_Arrows_01");
    Invalid->Entries.Add(Bad);
    Editor->ObjectPalette = Invalid.Get();
    if (!TestFalse(TEXT("Contradictory canonical entry cannot be selected"),
        Editor->ApplyPaletteEntry(Bad.EntryId)))
        {return false;}

    AddInfo(FString::Printf(TEXT(
        "[ARM-CLEAN06.4X12] 2 real direct entries inserted, GUID/level persisted; "
        "ARM authored N20 preserved on edit (palette default was N%d); "
        "no native palette/level package write."), AuthoredLevel));
    return TestEqual(TEXT("Two independent loose items were placed"),
            Level->LooseItemInstances.Num(), 2) &&
        TestEqual(TEXT("Real palette remains unmodified"),
            Palette->GetOutermost()->IsDirty(), bPaletteWasDirty);
}
#endif
