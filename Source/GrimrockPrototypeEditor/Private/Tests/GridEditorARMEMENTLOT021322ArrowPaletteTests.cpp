#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENTLOT021322Arrows,
    "Grimrock.RPG.ARMEMENT.LOT02.ArrowPaletteDirectCollectible",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENTLOT021322Arrows::RunTest(const FString&)
{
    UGridObjectPaletteAsset* Palette = LoadObject<UGridObjectPaletteAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default"));
    UGridItemDefinitionAsset* Arrows = LoadObject<UGridItemDefinitionAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Arrows_01.DA_Item_Arrows_01"));
    if (!TestNotNull(TEXT("Real palette"), Palette) ||
        !TestNotNull(TEXT("Real arrow stack"), Arrows)) return false;
    const bool WasPaletteDirty = Palette->GetOutermost()->IsDirty();
    const bool WasArrowDirty = Arrows->GetOutermost()->IsDirty();
    TArray<FGridWorldObjectDefinitionValidationMessage> Validation;
    const bool bPaletteValid = Palette->ValidatePalette(Validation);
    if (!bPaletteValid)
    {
        // Keep the authoritative palette contract strict, but expose the
        // actual invalid entries rather than only a generic test failure.
        for (const FGridWorldObjectDefinitionValidationMessage& Message : Validation)
        {
            if (Message.Severity == EGridWorldObjectDefinitionValidationSeverity::Error)
            {
                AddInfo(FString::Printf(TEXT("[LOT02.13.4.1] Palette validation: %s"), *Message.Message));
            }
        }
    }
    TestTrue(TEXT("Whole production palette valid"), bPaletteValid);
    const FName ArrowId(TEXT("Item_Arrows_01"));
    const FGridObjectPaletteEntry* Entry = Palette->FindEntryById(ArrowId);
    if (!TestNotNull(TEXT("Arrow is available in editor palette"), Entry)) return false;
    TestEqual(TEXT("Canonical item definition"), Entry->DefaultItemDefinition.Get(), Arrows);
    TestEqual(TEXT("Category Items"), Entry->PaletteCategory, FName(TEXT("Items")));
    TestNull(TEXT("Direct arrow entry must not duplicate the item icon"), Entry->Icon.Get());
    TestNull(TEXT("Direct arrow entry must not define a world object"), Entry->DefaultWorldObjectDefinition.Get());
    TestTrue(TEXT("Arrow direct-item entry is valid"), Entry->IsValidEntry());
    int32 Count = 0;
    for (const FGridObjectPaletteEntry& Candidate : Palette->Entries)
        if (Candidate.EntryId == ArrowId) ++Count;
    TestEqual(TEXT("No duplicate arrow entry"), Count, 1);

    const UWorld::InitializationValues Values = UWorld::InitializationValues()
        .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
        .SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false,
        FName(*FString::Printf(TEXT("LOT021322_%s"),
            *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
        nullptr, true, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Transient editor world created"), World)) return false;
    if (GEngine)
    {
        FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
        Context.SetCurrentWorld(World);
    }
    AGridLevelEditorActor* Editor = World->SpawnActor<AGridLevelEditorActor>();
    bool bPlacement = false;
    if (TestNotNull(TEXT("Real editor actor"), Editor))
    {
        UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Editor);
        Level->Width = 4;
        Level->Height = 4;
        Level->EnsureCellCount();
        for (FGridLevelCellData& Cell : Level->Cells)
        {
            Cell.CellType = EGridCellType::Floor;
            Cell.bBlocksOccupancy = false;
        }
        Editor->LevelAsset = Level;
        Editor->ObjectPalette = Palette;
        Editor->SelectedCellX = 1;
        Editor->SelectedCellY = 2;
        Editor->SelectedEdge = EGridEdge::None;
        if (TestTrue(TEXT("Select arrow in real grid editor"),
            Editor->ApplyPaletteEntry(ArrowId)))
        {
            TestEqual(TEXT("Item mode"), Editor->PaintObjectType, EGridLevelObjectType::Item);
            Editor->PlaceSelectedObject();
            bPlacement = Level->LooseItemInstances.Num() == 1 &&
                Level->LooseItemInstances[0].ItemDefinition.Get() == Arrows &&
                Level->LooseItemInstances[0].PaletteEntryId == ArrowId &&
                Level->LooseItemInstances[0].CellX == 1 &&
                Level->LooseItemInstances[0].CellY == 2 &&
                Level->WorldObjectInstances.IsEmpty();
        }
    }
    TestTrue(TEXT("Placement persists arrow item in transient level"), bPlacement);
    World->DestroyWorld(false);
    if (GEngine) GEngine->DestroyWorldContext(World);
    TestEqual(TEXT("Production palette not modified by test"),
        Palette->GetOutermost()->IsDirty(), WasPaletteDirty);
    TestEqual(TEXT("Arrow asset not modified by test"),
        Arrows->GetOutermost()->IsDirty(), WasArrowDirty);
    AddInfo(TEXT("[LOT02.13.4.1] Direct arrow entry/placement verified against canonical ItemDefinition; PIE acceptance is separate."));
    return true;
}
#endif
