#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectPaletteAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Runtime/GridEditorPreviewComponent.h"
#include "Runtime/GridEditorPreviewObjectActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"

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
    // Even a legacy BP with RemoveAllAtSlot selected must not delete loose items.
    Fixture.Editor->PlacementPolicy = EGridEditorObjectPlacementPolicy::RemoveAllAtSlot;
    TestTrue(TEXT("Another dagger shares the same cell regardless of legacy policy"), Fixture.Place(TEXT("Weapon_Dagger_01")));
    if (!TestEqual(TEXT("Four collectibles preserved"), Fixture.Level->LooseItemInstances.Num(), 4))
    {
        return false;
    }

    TSet<FGuid> UniqueIds;
    TArray<FVector2D> UsedPositions;
    const FVector2D CellCenter((1.5f) * Fixture.Level->CellSize, (1.5f) * Fixture.Level->CellSize);
    for (const FGridLooseItemInstance& Item : Fixture.Level->LooseItemInstances)
    {
        TestEqual(TEXT("Cell X unchanged"), Item.CellX, 1);
        TestEqual(TEXT("Cell Y unchanged"), Item.CellY, 1);
        TestEqual(TEXT("Floor surface retained"), Item.SurfaceSide, EGridEdge::None);
        TestNotNull(TEXT("Canonical definition retained"), Item.ItemDefinition.Get());
        TestTrue(TEXT("Unique persistent identity"), Item.InstanceId.IsValid() && !UniqueIds.Contains(Item.InstanceId));
        UniqueIds.Add(Item.InstanceId);
        const FVector2D Offset(Item.LocalOffset.X, Item.LocalOffset.Y);
        for (const FVector2D& PriorPosition : UsedPositions)
        {
            TestTrue(TEXT("Each collectible gets a distinct visible floor position"),
                !Offset.Equals(PriorPosition, KINDA_SMALL_NUMBER));
        }
        UsedPositions.Add(Offset);
        TestTrue(TEXT("Automatic floor position is kept inside the cell"),
            FMath::Abs(Offset.X) <= Fixture.Level->CellSize * 0.25f &&
            FMath::Abs(Offset.Y) <= Fixture.Level->CellSize * 0.25f);
        FVector WorldLocation = FVector::ZeroVector;
        if (TestTrue(TEXT("Individual collectible can be located by id"),
            Fixture.Editor->TryGetObjectWorldLocationById(Item.InstanceId, WorldLocation)))
        {
            TestTrue(TEXT("Editor selection position agrees with authored floor offset"),
                FVector2D::DistSquared(FVector2D(WorldLocation.X, WorldLocation.Y),
                    CellCenter + Offset) < 0.01f);
        }
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorITEMSTACK01ExplicitRemovalTest,
    "Grimrock.Editor.ITEMSTACK01.ExplicitRemoveCommandStillWorks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorITEMSTACK01ExplicitRemovalTest::RunTest(const FString&)
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
    if (!TestTrue(TEXT("Bow added to the same cell"), Fixture.Place(TEXT("Weapon_ShortBow_01"))))
    {
        return false;
    }
    const FGuid OldTargetId = Fixture.Level->LooseItemInstances[1].InstanceId;

    // Explicit Remove is the supported destructive action; painting never deletes loot.
    Fixture.Editor->RemoveObjectsAtSelection();
    TestEqual(TEXT("Only remote dagger remains"), Fixture.Level->LooseItemInstances.Num(), 1);
    TestNotNull(TEXT("Remote item retained"), Fixture.Level->FindLooseItemInstanceById(RemoteId));
    TestTrue(TEXT("Target dagger removed"), Fixture.Level->FindLooseItemInstanceById(OldTargetId) == nullptr);
    TestEqual(TEXT("No target items remain"), Fixture.Level->GetTypedPlacementIdsAtCell(1, 1).Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorITEMSTACK01PreviewSelectionTest,
    "Grimrock.Editor.ITEMSTACK01.WorldPreviewsAndIndividualSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorITEMSTACK01PreviewSelectionTest::RunTest(const FString&)
{
    GridEditorITEMSTACK01::FEditorFixture Fixture;
    if (!TestTrue(TEXT("Production palette and editor world exist"), Fixture.IsReady()))
    {
        return false;
    }
    if (!TestTrue(TEXT("Dagger placed"), Fixture.Place(TEXT("Weapon_Dagger_01"))) ||
        !TestTrue(TEXT("Bow placed"), Fixture.Place(TEXT("Weapon_ShortBow_01"))) ||
        !TestTrue(TEXT("Shield placed"), Fixture.Place(TEXT("Weapon_RoundShield_01"))))
    {
        return false;
    }

    // Exercise the real runtime preview component on a separate test actor.
    AGridLevelRuntimeActor* Runtime = Fixture.World->SpawnActor<AGridLevelRuntimeActor>();
    if (!TestNotNull(TEXT("Preview runtime actor"), Runtime))
    {
        return false;
    }
    Runtime->LevelAsset = Fixture.Level;
    UGridEditorPreviewComponent* PreviewComponent = Runtime->FindComponentByClass<UGridEditorPreviewComponent>();
    if (!TestNotNull(TEXT("Preview component exists"), PreviewComponent))
    {
        return false;
    }
    PreviewComponent->Initialize(Runtime);
    PreviewComponent->RebuildPreviewObjects();

    for (const FGridLooseItemInstance& Item : Fixture.Level->LooseItemInstances)
    {
        AGridEditorPreviewObjectActor* Preview = nullptr;
        for (TActorIterator<AGridEditorPreviewObjectActor> It(Fixture.World); It; ++It)
        {
            if ((*It)->ObjectId == Item.InstanceId)
            {
                Preview = *It;
                break;
            }
        }
        if (!TestNotNull(TEXT("Every placed item has its own world preview"), Preview))
        {
            continue;
        }
        const FVector Expected = Runtime->GetCellCenterWorld(Item.CellX, Item.CellY, 12.f) + Item.LocalOffset;
        TestTrue(TEXT("Preview actor preserves authored per-item offset"),
            Preview->GetActorLocation().Equals(Expected, 0.01f));
        TestNotNull(TEXT("Preview mesh exists"), Preview->MeshComponent->GetStaticMesh());
        TestTrue(TEXT("The correct item is independently hoverable"),
            Fixture.Editor->UpdateHoveredObjectFromWorldPoint(Preview->GetActorLocation()));
        TestEqual(TEXT("Hover selects the exact item GUID"), Fixture.Editor->HoveredObjectId, Item.InstanceId);
    }

    PreviewComponent->ClearPreviewObjects();
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
