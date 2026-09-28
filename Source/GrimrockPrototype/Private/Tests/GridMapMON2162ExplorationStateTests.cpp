#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/Map/GridMapExplorationState.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2162DefaultsTest, "Grimrock.Map.MON21_6_2.ExplorationState.DefaultsAndBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2162DefaultsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridMapExplorationState State;
	TestEqual(TEXT("Canonical map tile width is 32"), FGridMapExplorationState::GridSize, 32);
	TestEqual(TEXT("Canonical map tile contains 1024 cells"), FGridMapExplorationState::CellCount, 1024);
	TestTrue(TEXT("Fresh exploration state is structurally valid"), State.IsStructurallyValid());
	TestEqual(TEXT("Fresh exploration state allocates no cell storage"), State.GetStorageCellCount(), 0);
	TestEqual(TEXT("Fresh exploration state has zero explored cells"), State.GetExploredCellCount(), 0);
	TestFalse(TEXT("Fresh valid cell is unknown"), State.IsExplored(FIntPoint(0, 0)));
	TestFalse(TEXT("Negative X is outside the canonical tile"), FGridMapExplorationState::IsValidCell(FIntPoint(-1, 0)));
	TestFalse(TEXT("Negative Y is outside the canonical tile"), FGridMapExplorationState::IsValidCell(FIntPoint(0, -1)));
	TestFalse(TEXT("X=32 is outside the canonical tile"), FGridMapExplorationState::IsValidCell(FIntPoint(32, 0)));
	TestFalse(TEXT("Y=32 is outside the canonical tile"), FGridMapExplorationState::IsValidCell(FIntPoint(0, 32)));
	TestTrue(TEXT("(31,31) is the last canonical cell"), FGridMapExplorationState::IsValidCell(FIntPoint(31, 31)));

	bool bNewlyExplored = true;
	TestFalse(TEXT("Invalid discovery is rejected"), State.TryMarkExplored(FIntPoint(32, 31), bNewlyExplored));
	TestFalse(TEXT("Rejected discovery never reports a new cell"), bNewlyExplored);
	TestEqual(TEXT("Rejected discovery does not allocate storage"), State.GetStorageCellCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2162MutationTest, "Grimrock.Map.MON21_6_2.ExplorationState.MutationIsIdempotent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2162MutationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridMapExplorationState State;
	bool bNewlyExplored = false;

	TestTrue(TEXT("First discovery succeeds"), State.TryMarkExplored(FIntPoint(0, 0), bNewlyExplored));
	TestTrue(TEXT("First discovery is reported as new"), bNewlyExplored);
	TestEqual(TEXT("First discovery lazily materializes exactly 1024 cells"), State.GetStorageCellCount(), 1024);
	TestTrue(TEXT("Discovered origin is readable"), State.IsExplored(FIntPoint(0, 0)));
	TestEqual(TEXT("One cell is explored"), State.GetExploredCellCount(), 1);

	bNewlyExplored = true;
	TestTrue(TEXT("Repeated discovery remains a successful idempotent operation"), State.TryMarkExplored(FIntPoint(0, 0), bNewlyExplored));
	TestFalse(TEXT("Repeated discovery is not reported as new"), bNewlyExplored);
	TestEqual(TEXT("Repeated discovery does not duplicate state"), State.GetExploredCellCount(), 1);

	TestTrue(TEXT("Opposite corner discovery succeeds"), State.TryMarkExplored(FIntPoint(31, 31), bNewlyExplored));
	TestTrue(TEXT("Opposite corner is a new discovery"), bNewlyExplored);
	TestTrue(TEXT("Opposite corner is readable independently"), State.IsExplored(FIntPoint(31, 31)));
	TestFalse(TEXT("Unmarked neighbour remains unknown"), State.IsExplored(FIntPoint(30, 31)));
	TestEqual(TEXT("Two independent cells are explored"), State.GetExploredCellCount(), 2);
	TestTrue(TEXT("Materialized state remains structurally valid"), State.IsStructurallyValid());

	State.Reset();
	TestEqual(TEXT("Reset returns to zero explored cells"), State.GetExploredCellCount(), 0);
	TestEqual(TEXT("Reset releases lazy storage"), State.GetStorageCellCount(), 0);
	TestFalse(TEXT("Reset cell is unknown again"), State.IsExplored(FIntPoint(0, 0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2162PerLevelAuthorityTest, "Grimrock.Map.MON21_6_2.ExplorationState.PerLevelAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2162PerLevelAuthorityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridDungeonRuntimeState DungeonState;
	DungeonState.LevelStates.Add(TEXT("MapTile_A")).LevelId = TEXT("MapTile_A");
	DungeonState.LevelStates.Add(TEXT("MapTile_B")).LevelId = TEXT("MapTile_B");

	FGridLevelRuntimeState* FirstLevel = DungeonState.LevelStates.Find(TEXT("MapTile_A"));
	FGridLevelRuntimeState* SecondLevel = DungeonState.LevelStates.Find(TEXT("MapTile_B"));
	TestNotNull(TEXT("Level A runtime state exists"), FirstLevel);
	TestNotNull(TEXT("Level B runtime state exists"), SecondLevel);
	if (!FirstLevel || !SecondLevel)
	{
		return false;
	}

	bool bNewlyExplored = false;
	TestTrue(TEXT("Level A discovery succeeds"), FirstLevel->MapExploration.TryMarkExplored(FIntPoint(4, 7), bNewlyExplored));
	TestTrue(TEXT("Level A cell is explored"), FirstLevel->MapExploration.IsExplored(FIntPoint(4, 7)));
	TestFalse(TEXT("Level B keeps an independent exploration authority"), SecondLevel->MapExploration.IsExplored(FIntPoint(4, 7)));

	const FGridDungeonRuntimeState SessionCopy = DungeonState;
	const FGridLevelRuntimeState* CopiedFirst = SessionCopy.LevelStates.Find(TEXT("MapTile_A"));
	const FGridLevelRuntimeState* CopiedSecond = SessionCopy.LevelStates.Find(TEXT("MapTile_B"));
	TestNotNull(TEXT("Session copy keeps Level A"), CopiedFirst);
	TestNotNull(TEXT("Session copy keeps Level B"), CopiedSecond);
	TestTrue(TEXT("Session copy keeps Level A exploration"), CopiedFirst && CopiedFirst->MapExploration.IsExplored(FIntPoint(4, 7)));
	TestFalse(TEXT("Session copy keeps Level B unexplored"), CopiedSecond && CopiedSecond->MapExploration.IsExplored(FIntPoint(4, 7)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2162SaveBoundaryTest, "Grimrock.Map.MON21_6_2.ExplorationState.SaveBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2162SaveBoundaryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UScriptStruct* LevelStateStruct = FGridLevelRuntimeState::StaticStruct();
	const FProperty* ExplorationProperty = LevelStateStruct ? LevelStateStruct->FindPropertyByName(TEXT("MapExploration")) : nullptr;
	TestNotNull(TEXT("FGridLevelRuntimeState owns MapExploration"), ExplorationProperty);
	TestTrue(TEXT("MapExploration remains the same authority and is SaveGame-persistent from MON21.6.5"),
		ExplorationProperty && ExplorationProperty->HasAnyPropertyFlags(CPF_SaveGame));

	UScriptStruct* ExplorationStruct = FGridMapExplorationState::StaticStruct();
	const FProperty* CellsProperty = ExplorationStruct ? ExplorationStruct->FindPropertyByName(TEXT("ExploredCells")) : nullptr;
	TestNotNull(TEXT("Exploration state owns one cell-state array"), CellsProperty);
	TestTrue(TEXT("Cell-state array is SaveGame-persistent from MON21.6.5"),
		CellsProperty && CellsProperty->HasAnyPropertyFlags(CPF_SaveGame));

	TestTrue(TEXT("Current exact-match schema is at or beyond the MON21.6.2 v22 baseline"), UGrimrockPartySaveGame::CurrentSaveVersion >= 22);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
