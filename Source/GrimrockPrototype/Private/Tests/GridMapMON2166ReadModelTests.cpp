#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridDoorActor.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridSecretDoorActor.h"
#include "Runtime/Map/GridMapReadModel.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace GridMapMON2166Tests
{
	UGridLevelAsset* MakeOpenLevel(UObject* Outer)
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Outer);
		Level->Width = FGridMapExplorationState::GridSize;
		Level->Height = FGridMapExplorationState::GridSize;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.NorthWall = EGridWallType::None;
			Cell.EastWall = EGridWallType::None;
			Cell.SouthWall = EGridWallType::None;
			Cell.WestWall = EGridWallType::None;
		}
		return Level;
	}

	UGridWorldObjectDefinitionAsset* MakeDoorDefinition(UObject* Outer, FName Id, bool bSecret)
	{
		UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>(Outer);
		Definition->DefinitionId = Id;
		Definition->SupportedType = EGridLevelObjectType::Door;
		Definition->RuntimeActorClass = bSecret ? AGridSecretDoorActor::StaticClass() : AGridDoorActor::StaticClass();
		return Definition;
	}

	FGridWorldObjectInstance AddDoor(UGridLevelAsset* Level, FName DefinitionId, bool bInitiallyOpen = false)
	{
		FGridWorldObjectInstance Door;
		Door.InstanceId = FGuid::NewGuid();
		Door.Type = EGridLevelObjectType::Door;
		Door.WorldObjectDefinitionId = DefinitionId;
		Door.CellX = 10;
		Door.CellY = 10;
		Door.WallSide = EGridEdge::North;
		Door.InstanceConfig.bDoorInitiallyOpen = bInitiallyOpen;
		Level->WorldObjectInstances.Add(Door);
		return Door;
	}

	const FGridMapBoundaryView* FindBoundary(const FGridMapTileView& View, EGridMapBoundaryKind Kind)
	{
		return View.Boundaries.FindByPredicate(
			[Kind](const FGridMapBoundaryView& Boundary)
			{
				return Boundary.Kind == Kind;
			});
	}

	struct FTestWorld
	{
		UWorld* World = nullptr;

		FTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false)
				.RequiresHitProxies(false)
				.CreatePhysicsScene(false)
				.CreateNavigation(false)
				.CreateAISystem(false)
				.ShouldSimulatePhysics(false)
				.SetTransactional(false);

			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("MapMON2166World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FTestWorld()
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
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2166FilteringTest, "Grimrock.Map.MON21_6_6.ReadModel.FiltersUnknownGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2166FilteringTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON2166Tests;

	UGridLevelAsset* Level = MakeOpenLevel(GetTransientPackage());
	Level->GetCellMutable(10, 11).CellType = EGridCellType::StairsUp;
	Level->GetCellMutable(10, 11).SouthWall = EGridWallType::Solid;

	FGridLevelRuntimeState State;
	State.LevelId = TEXT("Tile_A");
	bool bNewlyExplored = false;
	TestTrue(TEXT("Only the observed cell is marked explored"), State.MapExploration.TryMarkExplored(FIntPoint(10, 10), bNewlyExplored));

	FGridMapTileView View;
	const TArray<TObjectPtr<UGridWorldObjectDefinitionAsset>> Definitions;
	TestTrue(TEXT("Read model builds for canonical tile"),
		FGridMapReadModelBuilder::BuildTileView(State.LevelId, *Level, State, Definitions, nullptr, View));
	TestEqual(TEXT("Exactly one explored cell is exposed"), View.Cells.Num(), 1);
	TestTrue(TEXT("The exposed cell is the observed cell"), View.Cells[0].LocalCell == FIntPoint(10, 10));
	TestEqual(TEXT("Unknown neighbour cell type is not exposed"), View.Cells[0].CellType, EGridCellType::Floor);
	TestEqual(TEXT("Only the shared known wall is exposed"), View.Boundaries.Num(), 1);
	TestEqual(TEXT("Shared boundary is normalized to Wall"), View.Boundaries[0].Kind, EGridMapBoundaryKind::Wall);
	TestTrue(TEXT("Boundary is observable from the explored cell"), View.Boundaries[0].LocalCell == FIntPoint(10, 10));
	TestEqual(TEXT("Observable boundary is north of the explored cell"), View.Boundaries[0].Edge, EGridEdge::North);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2166DoorStateTest, "Grimrock.Map.MON21_6_6.ReadModel.DoorStatePrefersLiveRuntime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2166DoorStateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON2166Tests;

	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Transient world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	if (!TestNotNull(TEXT("Runtime actor exists"), Runtime))
	{
		return false;
	}

	UGridLevelAsset* Level = MakeOpenLevel(Runtime);
	const FName DefinitionId(TEXT("MON2166_NormalDoor"));
	UGridWorldObjectDefinitionAsset* Definition = MakeDoorDefinition(Runtime, DefinitionId, false);
	const FGridWorldObjectInstance Door = AddDoor(Level, DefinitionId);

	FGridLevelRuntimeState State;
	State.LevelId = TEXT("Tile_Door");
	bool bNewlyExplored = false;
	State.MapExploration.TryMarkExplored(FIntPoint(10, 10), bNewlyExplored);
	FGridRuntimeDoorState& PersistedDoor = State.Doors.Add(Door.InstanceId);
	PersistedDoor.ObjectId = Door.InstanceId;
	PersistedDoor.bIsOpen = false;
	PersistedDoor.bBlocksMovement = true;

	TArray<TObjectPtr<UGridWorldObjectDefinitionAsset>> Definitions = { Definition };
	FGridMapTileView PersistedView;
	TestTrue(TEXT("Persisted fallback builds without live door system"),
		FGridMapReadModelBuilder::BuildTileView(State.LevelId, *Level, State, Definitions, nullptr, PersistedView));
	const FGridMapBoundaryView* PersistedBoundary = FindBoundary(PersistedView, EGridMapBoundaryKind::Door);
	TestNotNull(TEXT("Normal door is projected as Door"), PersistedBoundary);
	TestFalse(TEXT("Persisted blocked door is closed"), PersistedBoundary && PersistedBoundary->bDoorOpen);

	Runtime->LevelAsset = Level;
	UGridDoorSystemComponent* DoorSystem = Runtime->FindComponentByClass<UGridDoorSystemComponent>();
	if (!TestNotNull(TEXT("Live door system exists"), DoorSystem))
	{
		return false;
	}
	DoorSystem->Initialize(Runtime);
	DoorSystem->RebuildIndexes();
	DoorSystem->SetDoorPassageBlocked(Door.CellX, Door.CellY, Door.WallSide, false);

	FGridMapTileView LiveView;
	TestTrue(TEXT("Live read model builds"),
		FGridMapReadModelBuilder::BuildTileView(State.LevelId, *Level, State, Definitions, DoorSystem, LiveView));
	const FGridMapBoundaryView* LiveBoundary = FindBoundary(LiveView, EGridMapBoundaryKind::Door);
	TestNotNull(TEXT("Live normal door remains Door"), LiveBoundary);
	TestTrue(TEXT("Live door state overrides stale persisted closed state"), LiveBoundary && LiveBoundary->bDoorOpen);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2166SecretTest, "Grimrock.Map.MON21_6_6.ReadModel.SecretMetadataIsFiltered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2166SecretTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON2166Tests;

	UGridLevelAsset* Level = MakeOpenLevel(GetTransientPackage());
	const FName DefinitionId(TEXT("MON2166_DataDrivenSecret"));
	UGridWorldObjectDefinitionAsset* Definition = MakeDoorDefinition(Level, DefinitionId, true);
	const FGridWorldObjectInstance Door = AddDoor(Level, DefinitionId);

	FGridLevelRuntimeState State;
	State.LevelId = TEXT("Tile_Secret");
	bool bNewlyExplored = false;
	State.MapExploration.TryMarkExplored(FIntPoint(10, 10), bNewlyExplored);
	FGridRuntimeDoorState& PersistedDoor = State.Doors.Add(Door.InstanceId);
	PersistedDoor.ObjectId = Door.InstanceId;
	PersistedDoor.bIsOpen = false;
	PersistedDoor.bBlocksMovement = true;

	TArray<TObjectPtr<UGridWorldObjectDefinitionAsset>> Definitions = { Definition };
	FGridMapTileView HiddenView;
	TestTrue(TEXT("Hidden secret read model builds"),
		FGridMapReadModelBuilder::BuildTileView(State.LevelId, *Level, State, Definitions, nullptr, HiddenView));
	TestEqual(TEXT("Hidden secret exposes exactly one boundary"), HiddenView.Boundaries.Num(), 1);
	TestEqual(TEXT("Undiscovered secret is indistinguishable from a normal wall"), HiddenView.Boundaries[0].Kind, EGridMapBoundaryKind::Wall);
	TestFalse(TEXT("Hidden secret exposes no open-door state"), HiddenView.Boundaries[0].bDoorOpen);

	UScriptStruct* BoundaryStruct = FGridMapBoundaryView::StaticStruct();
	TestNull(TEXT("Boundary view exposes no ObjectId"), BoundaryStruct->FindPropertyByName(TEXT("ObjectId")));
	TestNull(TEXT("Boundary view exposes no WorldObjectDefinitionId"), BoundaryStruct->FindPropertyByName(TEXT("WorldObjectDefinitionId")));
	TestNull(TEXT("Boundary view exposes no hidden-secret boolean"), BoundaryStruct->FindPropertyByName(TEXT("bIsSecret")));

	bool bNewlyDiscovered = false;
	TestTrue(TEXT("Secret discovery is accepted"), State.MapExploration.TryMarkSecretDiscovered(Door.InstanceId, bNewlyDiscovered));
	FGridMapTileView DiscoveredView;
	TestTrue(TEXT("Discovered secret read model builds"),
		FGridMapReadModelBuilder::BuildTileView(State.LevelId, *Level, State, Definitions, nullptr, DiscoveredView));
	TestEqual(TEXT("Discovered secret exposes exactly one boundary"), DiscoveredView.Boundaries.Num(), 1);
	TestEqual(TEXT("Discovered secret is explicitly projected"), DiscoveredView.Boundaries[0].Kind, EGridMapBoundaryKind::SecretDoor);
	TestFalse(TEXT("Closed discovered secret remains known while closed"), DiscoveredView.Boundaries[0].bDoorOpen);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2166TransientTest, "Grimrock.Map.MON21_6_6.ReadModel.TransientContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2166TransientTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(TEXT("Read model does not change exact-match SaveGame v24"), UGrimrockPartySaveGame::CurrentSaveVersion, 24);

	for (TFieldIterator<FProperty> It(FGridMapTileView::StaticStruct()); It; ++It)
	{
		TestFalse(*FString::Printf(TEXT("Tile view property '%s' is not SaveGame state"), *It->GetName()), It->HasAnyPropertyFlags(CPF_SaveGame));
	}
	for (TFieldIterator<FProperty> It(FGridMapCellView::StaticStruct()); It; ++It)
	{
		TestFalse(*FString::Printf(TEXT("Cell view property '%s' is not SaveGame state"), *It->GetName()), It->HasAnyPropertyFlags(CPF_SaveGame));
	}
	for (TFieldIterator<FProperty> It(FGridMapBoundaryView::StaticStruct()); It; ++It)
	{
		TestFalse(*FString::Printf(TEXT("Boundary view property '%s' is not SaveGame state"), *It->GetName()), It->HasAnyPropertyFlags(CPF_SaveGame));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
