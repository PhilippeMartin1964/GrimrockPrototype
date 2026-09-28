#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Map/GridMapRevealService.h"

namespace GridMapMON2163Tests
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
			Cell.bBlocksOccupancy = false;
			Cell.NorthWall = EGridWallType::None;
			Cell.EastWall = EGridWallType::None;
			Cell.SouthWall = EGridWallType::None;
			Cell.WestWall = EGridWallType::None;
		}
		return Level;
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
				FName(*FString::Printf(TEXT("MapMON2163World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2163RadiusTest, "Grimrock.Map.MON21_6_3.Reveal.RadiusAndOccupancy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2163RadiusTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON2163Tests;

	UGridLevelAsset* Level = MakeOpenLevel(GetTransientPackage());
	Level->GetCellMutable(10, 11).bBlocksOccupancy = true;

	FGridMapExplorationState Exploration;
	TestEqual(TEXT("Open 1.25-cell reveal discovers center plus four cardinals"),
		FGridMapRevealService::RevealAroundCell(*Level, nullptr, FIntPoint(10, 10), Exploration), 5);
	TestTrue(TEXT("North is visible even when occupancy-blocked"), Exploration.IsExplored(FIntPoint(10, 11)));
	TestTrue(TEXT("East is revealed"), Exploration.IsExplored(FIntPoint(11, 10)));
	TestTrue(TEXT("South is revealed"), Exploration.IsExplored(FIntPoint(10, 9)));
	TestTrue(TEXT("West is revealed"), Exploration.IsExplored(FIntPoint(9, 10)));
	TestFalse(TEXT("North-east diagonal remains unknown at radius 1.25"), Exploration.IsExplored(FIntPoint(11, 11)));
	TestFalse(TEXT("South-west diagonal remains unknown at radius 1.25"), Exploration.IsExplored(FIntPoint(9, 9)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2163WallsTest, "Grimrock.Map.MON21_6_3.Reveal.StructuralWallsBlockBothSides",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2163WallsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON2163Tests;

	UGridLevelAsset* Level = MakeOpenLevel(GetTransientPackage());
	FGridMapExplorationState Exploration;

	Level->GetCellMutable(10, 10).NorthWall = EGridWallType::Solid;
	TestEqual(TEXT("Direct solid wall removes one cardinal reveal"),
		FGridMapRevealService::RevealAroundCell(*Level, nullptr, FIntPoint(10, 10), Exploration), 4);
	TestFalse(TEXT("Cell behind direct wall remains unknown"), Exploration.IsExplored(FIntPoint(10, 11)));

	Exploration.Reset();
	Level->GetCellMutable(10, 10).NorthWall = EGridWallType::None;
	Level->GetCellMutable(10, 11).SouthWall = EGridWallType::Solid;
	TestEqual(TEXT("Reverse solid wall also removes one cardinal reveal"),
		FGridMapRevealService::RevealAroundCell(*Level, nullptr, FIntPoint(10, 10), Exploration), 4);
	TestFalse(TEXT("Cell behind reverse wall remains unknown"), Exploration.IsExplored(FIntPoint(10, 11)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2163DoorTest, "Grimrock.Map.MON21_6_3.Reveal.DoorTopology",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2163DoorTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON2163Tests;

	FTestWorld TestWorld;
	TestNotNull(TEXT("Transient map world exists"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	TestNotNull(TEXT("Runtime actor exists"), Runtime);
	if (!Runtime)
	{
		return false;
	}

	UGridLevelAsset* Level = MakeOpenLevel(Runtime);
	FGridWorldObjectInstance Door;
	Door.InstanceId = FGuid::NewGuid();
	Door.Type = EGridLevelObjectType::Door;
	Door.CellX = 10;
	Door.CellY = 10;
	Door.WallSide = EGridEdge::North;
	Level->WorldObjectInstances.Add(Door);
	Runtime->LevelAsset = Level;

	UGridDoorSystemComponent* DoorSystem = Runtime->FindComponentByClass<UGridDoorSystemComponent>();
	TestNotNull(TEXT("Door system exists"), DoorSystem);
	if (!DoorSystem)
	{
		return false;
	}
	DoorSystem->Initialize(Runtime);
	DoorSystem->RebuildIndexes();

	FGridMapExplorationState Exploration;
	DoorSystem->SetDoorPassageBlocked(10, 10, EGridEdge::North, true);
	TestEqual(TEXT("Closed door blocks the north reveal"),
		FGridMapRevealService::RevealAroundCell(*Level, DoorSystem, FIntPoint(10, 10), Exploration), 4);
	TestFalse(TEXT("Cell behind closed door remains unknown"), Exploration.IsExplored(FIntPoint(10, 11)));

	Exploration.Reset();
	DoorSystem->SetDoorPassageBlocked(10, 10, EGridEdge::North, false);
	TestEqual(TEXT("Open door restores the north reveal"),
		FGridMapRevealService::RevealAroundCell(*Level, DoorSystem, FIntPoint(10, 10), Exploration), 5);
	TestTrue(TEXT("Cell behind open door becomes explored"), Exploration.IsExplored(FIntPoint(10, 11)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2163RuntimeHookTest, "Grimrock.Map.MON21_6_3.Reveal.RuntimeCellChangeHook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2163RuntimeHookTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON2163Tests;

	FTestWorld TestWorld;
	TestNotNull(TEXT("Transient runtime-hook world exists"), TestWorld.World);
	if (!TestWorld.World)
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	TestNotNull(TEXT("Runtime actor exists"), Runtime);
	if (!Runtime)
	{
		return false;
	}
	Runtime->LevelAsset = MakeOpenLevel(Runtime);

	Runtime->HandlePartyCellChanged(12, 12, 12, 12);
	const FGridLevelRuntimeState* InitialState = Runtime->FindRuntimeStateForCurrentLevel();
	TestNotNull(TEXT("Initial same-cell notification creates map state"), InitialState);
	TestEqual(TEXT("Initial party position reveals five cells"), InitialState ? InitialState->MapExploration.GetExploredCellCount() : 0, 5);

	Runtime->HandlePartyCellChanged(12, 12, 13, 12);
	const FGridLevelRuntimeState* MovedState = Runtime->FindRuntimeStateForCurrentLevel();
	TestEqual(TEXT("One eastward move extends exploration without erasing prior cells"),
		MovedState ? MovedState->MapExploration.GetExploredCellCount() : 0, 8);
	TestTrue(TEXT("New forward-east fringe is revealed"), MovedState && MovedState->MapExploration.IsExplored(FIntPoint(14, 12)));
	TestTrue(TEXT("Previous west fringe remains explored"), MovedState && MovedState->MapExploration.IsExplored(FIntPoint(11, 12)));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
