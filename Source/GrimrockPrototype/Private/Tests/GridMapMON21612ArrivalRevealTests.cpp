#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDungeonAsset.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridGenericObjectActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Map/GridMapReadModel.h"

namespace GridMapMON21612ArrivalRevealTests
{
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

			World = UWorld::CreateWorld(
				EWorldType::Game,
				false,
				FName(*FString::Printf(TEXT("MapMON21612ArrivalWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
				nullptr,
				true,
				ERHIFeatureLevel::Num,
				&Values);

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

	UGridLevelAsset* MakeOpenLevel(UObject* Outer)
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Outer);
		Level->Width = 32;
		Level->Height = 32;
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

	bool HasSymbol(const FGridMapTileView& View, const FIntPoint& Cell, EGridMapSymbolKind Kind)
	{
		return View.Symbols.ContainsByPredicate(
			[Cell, Kind](const FGridMapSymbolView& Symbol)
			{
				return Symbol.LocalCell == Cell && Symbol.Kind == Kind;
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMapMON21612ArrivalRevealTest,
	"Grimrock.Map.MON21_6_12.ArrivalReveal.CrossLevelTravelRevealsDestination",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON21612ArrivalRevealTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON21612ArrivalRevealTests;

	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Transient world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	if (!TestNotNull(TEXT("Runtime actor exists"), Runtime) || !TestNotNull(TEXT("Party pawn exists"), Party))
	{
		return false;
	}

	UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(Runtime);
	UGridLevelAsset* Upper = MakeOpenLevel(Dungeon);
	UGridLevelAsset* Lower = MakeOpenLevel(Dungeon);

	const FName UpperId(TEXT("MON21612_Upper"));
	const FName LowerId(TEXT("MON21612_Lower"));
	const FIntPoint ArrivalCell(12, 12);

	FGridDungeonLevelEntry UpperEntry;
	UpperEntry.LevelId = UpperId;
	UpperEntry.LevelAsset = Upper;
	UpperEntry.LogicalPosition = FIntVector(0, 0, 1);
	UpperEntry.bEnabled = true;

	FGridDungeonLevelEntry LowerEntry;
	LowerEntry.LevelId = LowerId;
	LowerEntry.LevelAsset = Lower;
	LowerEntry.LogicalPosition = FIntVector(0, 0, 0);
	LowerEntry.bEnabled = true;

	Dungeon->DefaultLevelId = UpperId;
	Dungeon->Levels = { UpperEntry, LowerEntry };

	UGridWorldObjectDefinitionAsset* StairsUpDefinition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
	StairsUpDefinition->DefinitionId = TEXT("Stairs_Up");
	StairsUpDefinition->SupportedType = EGridLevelObjectType::Relocation;
	StairsUpDefinition->PlacementSurface = EGridObjectPlacementKind::Floor;
	StairsUpDefinition->RuntimeActorClass = AGridGenericObjectActor::StaticClass();
	StairsUpDefinition->MapSymbolStyle = EGridMapSymbolStyle::StairsUp;
	Runtime->WorldObjectDefinitions.Add(StairsUpDefinition);

	FGridWorldObjectInstance StairsUp;
	StairsUp.InstanceId = FGuid::NewGuid();
	StairsUp.Type = EGridLevelObjectType::Relocation;
	StairsUp.WorldObjectDefinitionId = StairsUpDefinition->DefinitionId;
	StairsUp.CellX = ArrivalCell.X;
	StairsUp.CellY = ArrivalCell.Y;
	StairsUp.InstanceConfig.bRelocationInitiallyEnabled = true;
	Lower->WorldObjectInstances.Add(StairsUp);

	Runtime->DungeonAsset = Dungeon;
	Runtime->CurrentDungeonLevelId = UpperId;
	Runtime->LevelAsset = Upper;
	Party->SetGridStart(Runtime, 4, 4, EGridEdge::South);

	TestFalse(TEXT("Destination level has no exploration state before travel"),
		Runtime->DungeonRuntimeState.LevelStates.Contains(LowerId));

	TestTrue(TEXT("Cross-level travel succeeds"),
		Runtime->TravelToDungeonLevel(LowerId, ArrivalCell.X, ArrivalCell.Y, EGridEdge::North, Party));

	TestEqual(TEXT("Runtime switched to target level"), Runtime->CurrentDungeonLevelId, LowerId);
	TestEqual(TEXT("Party arrived at target X"), Party->CurrentCellX, ArrivalCell.X);
	TestEqual(TEXT("Party arrived at target Y"), Party->CurrentCellY, ArrivalCell.Y);

	const FGridLevelRuntimeState* LowerState = Runtime->DungeonRuntimeState.LevelStates.Find(LowerId);
	if (!TestNotNull(TEXT("Arrival creates target exploration state"), LowerState))
	{
		return false;
	}

	TestTrue(TEXT("Arrival cell is immediately explored without an extra movement"),
		LowerState->MapExploration.IsExplored(ArrivalCell));
	TestTrue(TEXT("Open topology reveals north cardinal on arrival"),
		LowerState->MapExploration.IsExplored(ArrivalCell + FIntPoint(0, 1)));
	TestTrue(TEXT("Open topology reveals east cardinal on arrival"),
		LowerState->MapExploration.IsExplored(ArrivalCell + FIntPoint(1, 0)));
	TestTrue(TEXT("Open topology reveals south cardinal on arrival"),
		LowerState->MapExploration.IsExplored(ArrivalCell + FIntPoint(0, -1)));
	TestTrue(TEXT("Open topology reveals west cardinal on arrival"),
		LowerState->MapExploration.IsExplored(ArrivalCell + FIntPoint(-1, 0)));
	TestEqual(TEXT("Arrival reveal matches canonical radius: center plus four cardinals"),
		LowerState->MapExploration.GetExploredCellCount(), 5);

	FGridMapTileView TileView;
	TestTrue(TEXT("Target tile read model builds immediately after travel"),
		FGridMapReadModelBuilder::BuildTileView(
			LowerId,
			*Lower,
			*LowerState,
			Runtime->WorldObjectDefinitions,
			nullptr,
			TileView));
	TestTrue(TEXT("Stairs Up symbol is immediately visible on the revealed arrival cell"),
		HasSymbol(TileView, ArrivalCell, EGridMapSymbolKind::StairsUp));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
