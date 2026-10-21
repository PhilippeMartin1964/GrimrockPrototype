#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridItemDefinitionAsset.h"

namespace GridEditorITEMSTACK01
{
    constexpr const TCHAR* PalettePath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.DA_ObjectPalette_Default");

    struct FEditorFixture
    {
        UWorld* World = nullptr;
        AGridLevelEditorActor* Editor = nullptr;
        UGridLevelAsset* Level = nullptr;
        UGridObjectPaletteAsset* Palette = nullptr;

        FEditorFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
                .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::EditorPreview, false,
                FName(*FString::Printf(TEXT("ITEMSTACK01_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (!World)
            {
                return;
            }
            if (GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::EditorPreview);
                Context.SetCurrentWorld(World);
            }
            Editor = World->SpawnActor<AGridLevelEditorActor>();
            if (!Editor)
            {
                return;
            }
            Level = NewObject<UGridLevelAsset>(Editor);
            Level->Width = 3;
            Level->Height = 3;
            Level->EnsureCellCount();
            for (FGridLevelCellData& Cell : Level->Cells)
            {
                Cell.CellType = EGridCellType::Floor;
                Cell.bBlocksOccupancy = false;
            }
            Editor->LevelAsset = Level;
            Palette = LoadObject<UGridObjectPaletteAsset>(nullptr, PalettePath);
            Editor->ObjectPalette = Palette;
            Editor->SelectedCellX = 1;
            Editor->SelectedCellY = 1;
            Editor->SelectedEdge = EGridEdge::None;
            Editor->PlacementPolicy = EGridEditorObjectPlacementPolicy::ReplaceSameSlotOnly;
        }

        ~FEditorFixture()
        {
            if (!World)
            {
                return;
            }
            World->DestroyWorld(false);
            if (GEngine)
            {
                GEngine->DestroyWorldContext(World);
            }
        }

        bool IsReady() const
        {
            return World && Editor && Level && Palette;
        }

        bool Place(const TCHAR* EntryId)
        {
            if (!IsReady() || !Editor->ApplyPaletteEntry(FName(EntryId)))
            {
                return false;
            }
            const int32 Before = Level->LooseItemInstances.Num();
            Editor->PlaceSelectedObject();
            return Level->LooseItemInstances.Num() == Before + 1;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorITEMSTACK01AdditiveTest,
    "Grimrock.Editor.ITEMSTACK01.MultipleCollectiblesSameCell",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorITEMSTACK01AdditiveTest::RunTest(const FString&)
{
    GridEditorITEMSTACK01::FEditorFixture Fixture;
    if (!TestTrue(TEXT("Production palette and editor world exist"), Fixture.IsReady()))
    {
        return false;
    }

    // A logical object and its connector must survive painting more collectibles.
    FGridLogicObjectInstance& Logic = Fixture.Level->LogicObjects.AddDefaulted_GetRef();
    Logic.InstanceId = FGuid::NewGuid();
    Logic.Type = EGridLevelObjectType::Logic;
    Logic.CellX = 1;
    Logic.CellY = 1;
    const FGuid LogicId = Logic.InstanceId;

    if (!TestTrue(TEXT("First dagger placed"), Fixture.Place(TEXT("Weapon_Dagger_01"))))
    {
        return false;
    }
    const FGuid DaggerId = Fixture.Level->LooseItemInstances[0].InstanceId;
    FGridObjectLink& Link = Fixture.Level->Links.AddDefaulted_GetRef();
    Link.SourceObjectId = LogicId;
    Link.TargetObjectId = DaggerId;

    TestTrue(TEXT("Bow shares the same cell"), Fixture.Place(TEXT("Weapon_ShortBow_01")));
    TestTrue(TEXT("Shield shares the same cell"), Fixture.Place(TEXT("Weapon_RoundShield_01")));
    TestTrue(TEXT("Another dagger shares the same cell"), Fixture.Place(TEXT("Weapon_Dagger_01")));
    if (!TestEqual(TEXT("Four collectibles preserved"), Fixture.Level->LooseItemInstances.Num(), 4))
    {
        return false;
    }

    TSet<FGuid> UniqueIds;
    for (const FGridLooseItemInstance& Item : Fixture.Level->LooseItemInstances)
    {
        TestEqual(TEXT("Cell X unchanged"), Item.CellX, 1);
        TestEqual(TEXT("Cell Y unchanged"), Item.CellY, 1);
        TestEqual(TEXT("Floor surface retained"), Item.SurfaceSide, EGridEdge::None);
        TestNotNull(TEXT("Canonical definition retained"), Item.ItemDefinition.Get());
        TestTrue(TEXT("Unique persistent identity"), Item.InstanceId.IsValid() && !UniqueIds.Contains(Item.InstanceId));
        UniqueIds.Add(Item.InstanceId);
    }
    TestEqual(TEXT("First dagger identity retained"), Fixture.Level->LooseItemInstances[0].InstanceId, DaggerId);
    TestNotNull(TEXT("Logical object is retained"), Fixture.Level->FindLogicObjectInstanceById(LogicId));
    TestEqual(TEXT("Connector is retained"), Fixture.Level->Links.Num(), 1);
    if (Fixture.Level->Links.Num() == 1)
    {
        TestEqual(TEXT("Connector still references the original dagger"), Fixture.Level->Links[0].TargetObjectId, DaggerId);
    }
    TestEqual(TEXT("Four items plus one logic object share the grid cell"), Fixture.Level->GetTypedPlacementIdsAtCell(1, 1).Num(), 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorITEMSTACK01ReplaceTest,
    "Grimrock.Editor.ITEMSTACK01.ExplicitRemoveAllStillWorks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorITEMSTACK01ReplaceTest::RunTest(const FString&)
{
    GridEditorITEMSTACK01::FEditorFixture Fixture;
    if (!TestTrue(TEXT("Production palette and editor world exist"), Fixture.IsReady()))
    {
        return false;
    }

    Fixture.Editor->SelectedCellX = 0;
    Fixture.Editor->SelectedCellY = 0;
    if (!TestTrue(TEXT("Remote dagger placed"), Fixture.Place(TEXT("Weapon_Dagger_01"))))
    {
        return false;
    }
    const FGuid RemoteId = Fixture.Level->LooseItemInstances[0].InstanceId;

    Fixture.Editor->SelectedCellX = 1;
    Fixture.Editor->SelectedCellY = 1;
    if (!TestTrue(TEXT("Initial target dagger placed"), Fixture.Place(TEXT("Weapon_Dagger_01"))))
    {
        return false;
    }
    const FGuid OldTargetId = Fixture.Level->LooseItemInstances[1].InstanceId;

    Fixture.Editor->PlacementPolicy = EGridEditorObjectPlacementPolicy::RemoveAllAtSlot;
    TestTrue(TEXT("Select replacement bow"), Fixture.Editor->ApplyPaletteEntry(FName(TEXT("Weapon_ShortBow_01"))));
    Fixture.Editor->PlaceSelectedObject();

    TestEqual(TEXT("Only remote dagger and new bow remain"), Fixture.Level->LooseItemInstances.Num(), 2);
    TestNotNull(TEXT("Remote item retained"), Fixture.Level->FindLooseItemInstanceById(RemoteId));
    TestTrue(TEXT("Old target item deliberately removed"), Fixture.Level->FindLooseItemInstanceById(OldTargetId) == nullptr);
    TestEqual(TEXT("One target object remains"), Fixture.Level->GetTypedPlacementIdsAtCell(1, 1).Num(), 1);

    const FGridLooseItemInstance* Replacement = Fixture.Level->LooseItemInstances.FindByPredicate(
        [](const FGridLooseItemInstance& Item) { return Item.CellX == 1 && Item.CellY == 1; });
    if (TestNotNull(TEXT("Bow replacement exists"), Replacement) &&
        TestNotNull(TEXT("Replacement definition exists"), Replacement->ItemDefinition.Get()))
    {
        TestEqual(TEXT("Replacement identity"), Replacement->ItemDefinition->ItemDefinitionId, FName(TEXT("Weapon_ShortBow_01")));
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
