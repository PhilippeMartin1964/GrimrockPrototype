#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDungeonAsset.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/Map/GridMapExplorationState.h"
#include "Runtime/Map/GridMapReadModel.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace GridMapMON2167Tests
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

	void AddEntry(UGridDungeonAsset* Dungeon, FName LevelId, UGridLevelAsset* Level, const FIntVector& LogicalPosition, bool bEnabled = true)
	{
		FGridDungeonLevelEntry Entry;
		Entry.LevelId = LevelId;
		Entry.LevelAsset = Level;
		Entry.LogicalPosition = LogicalPosition;
		Entry.bEnabled = bEnabled;
		Dungeon->Levels.Add(Entry);
	}

	FGridLevelRuntimeState& AddExploredState(FGridDungeonRuntimeState& DungeonState, FName LevelId, const FIntPoint& Cell)
	{
		FGridLevelRuntimeState& State = DungeonState.LevelStates.Add(LevelId);
		State.LevelId = LevelId;
		bool bNewlyExplored = false;
		State.MapExploration.TryMarkExplored(Cell, bNewlyExplored);
		return State;
	}

	const FGridMapFloorCellView* FindCell(const FGridMapFloorView& View, const FIntPoint& MapCell)
	{
		return View.Cells.FindByPredicate(
			[MapCell](const FGridMapFloorCellView& Cell)
			{
				return Cell.MapCell == MapCell;
			});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2167CoordinatesTest, "Grimrock.Map.MON21_6_7.FloorProjection.GlobalCoordinatesAndPartyMarker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2167CoordinatesTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON2167Tests;

	UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(GetTransientPackage());
	UGridLevelAsset* West = MakeOpenLevel(Dungeon);
	UGridLevelAsset* East = MakeOpenLevel(Dungeon);
	AddEntry(Dungeon, TEXT("West"), West, FIntVector(-1, 2, 0));
	AddEntry(Dungeon, TEXT("East"), East, FIntVector(0, 2, 0));

	FGridDungeonRuntimeState DungeonState;
	AddExploredState(DungeonState, TEXT("West"), FIntPoint(31, 1));
	AddExploredState(DungeonState, TEXT("East"), FIntPoint(0, 1));

	const TArray<TObjectPtr<UGridWorldObjectDefinitionAsset>> Definitions;
	FGridMapFloorView View;
	TestTrue(TEXT("Same-floor tiles compose into one floor view"),
		FGridMapReadModelBuilder::BuildFloorView(
			*Dungeon, DungeonState, Definitions, TEXT("West"), FIntPoint(31, 1), EGridEdge::East, 0, nullptr, View));

	TestEqual(TEXT("Two explored cells from two tiles are projected"), View.Cells.Num(), 2);
	TestNotNull(TEXT("Negative tile X projects local 31 to global -1"), FindCell(View, FIntPoint(-1, 65)));
	TestNotNull(TEXT("Adjacent tile projects local 0 to global 0"), FindCell(View, FIntPoint(0, 65)));
	TestTrue(TEXT("Adjacent tile cells are seamless in global X"), FindCell(View, FIntPoint(-1, 65)) && FindCell(View, FIntPoint(0, 65)));
	TestTrue(TEXT("Party marker is visible on the current floor"), View.bHasPartyMarker);
	TestTrue(TEXT("Party marker uses current tile global coordinates"), View.PartyMapCell == FIntPoint(-1, 65));
	TestEqual(TEXT("Party facing is projected without duplication"), View.PartyFacing, EGridEdge::East);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2167FloorsTest, "Grimrock.Map.MON21_6_7.FloorProjection.AvailableFloorsAndSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2167FloorsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON2167Tests;

	UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(GetTransientPackage());
	UGridLevelAsset* Upper = MakeOpenLevel(Dungeon);
	UGridLevelAsset* Middle = MakeOpenLevel(Dungeon);
	UGridLevelAsset* Lower = MakeOpenLevel(Dungeon);
	UGridLevelAsset* UnknownMiddle = MakeOpenLevel(Dungeon);
	UGridLevelAsset* Disabled = MakeOpenLevel(Dungeon);

	AddEntry(Dungeon, TEXT("Upper"), Upper, FIntVector(0, 0, 2));
	AddEntry(Dungeon, TEXT("Middle"), Middle, FIntVector(0, 0, 0));
	AddEntry(Dungeon, TEXT("UnknownMiddle"), UnknownMiddle, FIntVector(1, 0, 0));
	AddEntry(Dungeon, TEXT("Lower"), Lower, FIntVector(0, 0, -3));
	AddEntry(Dungeon, TEXT("Disabled"), Disabled, FIntVector(0, 0, 1), false);

	FGridDungeonRuntimeState DungeonState;
	AddExploredState(DungeonState, TEXT("Upper"), FIntPoint(1, 1));
	AddExploredState(DungeonState, TEXT("Middle"), FIntPoint(2, 2));
	AddExploredState(DungeonState, TEXT("Lower"), FIntPoint(3, 3));
	// UnknownMiddle intentionally has no exploration state.

	TArray<int32> FloorZs;
	FGridMapReadModelBuilder::GetAvailableFloorZs(*Dungeon, FloorZs);
	TestEqual(TEXT("Only enabled floors are listed"), FloorZs.Num(), 3);
	TestEqual(TEXT("Available floors are sorted ascending: lower"), FloorZs.IsValidIndex(0) ? FloorZs[0] : 999, -3);
	TestEqual(TEXT("Available floors are sorted ascending: middle"), FloorZs.IsValidIndex(1) ? FloorZs[1] : 999, 0);
	TestEqual(TEXT("Available floors are sorted ascending: upper"), FloorZs.IsValidIndex(2) ? FloorZs[2] : 999, 2);

	const TArray<TObjectPtr<UGridWorldObjectDefinitionAsset>> Definitions;
	FGridMapFloorView MiddleView;
	TestTrue(TEXT("Middle floor builds"),
		FGridMapReadModelBuilder::BuildFloorView(
			*Dungeon, DungeonState, Definitions, TEXT("Middle"), FIntPoint(2, 2), EGridEdge::North, 0, nullptr, MiddleView));
	TestEqual(TEXT("Only explored geometry from the selected floor is exposed"), MiddleView.Cells.Num(), 1);
	TestNotNull(TEXT("Explored middle cell is present"), FindCell(MiddleView, FIntPoint(2, 2)));
	TestTrue(TEXT("Party marker appears on selected current floor"), MiddleView.bHasPartyMarker);

	FGridMapFloorView UpperView;
	TestTrue(TEXT("Upper floor builds"),
		FGridMapReadModelBuilder::BuildFloorView(
			*Dungeon, DungeonState, Definitions, TEXT("Middle"), FIntPoint(2, 2), EGridEdge::North, 2, nullptr, UpperView));
	TestEqual(TEXT("Upper floor exposes only upper explored geometry"), UpperView.Cells.Num(), 1);
	TestFalse(TEXT("Party marker is hidden when viewing another floor"), UpperView.bHasPartyMarker);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2167SeamTest, "Grimrock.Map.MON21_6_7.FloorProjection.TechnicalSeamAndBoundaryDedup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2167SeamTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON2167Tests;

	UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(GetTransientPackage());
	UGridLevelAsset* Left = MakeOpenLevel(Dungeon);
	UGridLevelAsset* Right = MakeOpenLevel(Dungeon);
	AddEntry(Dungeon, TEXT("Left"), Left, FIntVector(0, 0, 0));
	AddEntry(Dungeon, TEXT("Right"), Right, FIntVector(1, 0, 0));

	FGridDungeonRuntimeState DungeonState;
	AddExploredState(DungeonState, TEXT("Left"), FIntPoint(31, 5));
	AddExploredState(DungeonState, TEXT("Right"), FIntPoint(0, 5));

	const TArray<TObjectPtr<UGridWorldObjectDefinitionAsset>> Definitions;
	FGridMapFloorView OpenSeam;
	TestTrue(TEXT("Open adjacent tiles build"),
		FGridMapReadModelBuilder::BuildFloorView(
			*Dungeon, DungeonState, Definitions, TEXT("Left"), FIntPoint(31, 5), EGridEdge::East, 0, nullptr, OpenSeam));
	TestEqual(TEXT("Pure LevelAsset boundary is not drawn as a technical seam"), OpenSeam.Boundaries.Num(), 0);

	Left->GetCellMutable(31, 5).EastWall = EGridWallType::Solid;
	Right->GetCellMutable(0, 5).WestWall = EGridWallType::Solid;

	FGridMapFloorView WalledSeam;
	TestTrue(TEXT("Walled adjacent tiles build"),
		FGridMapReadModelBuilder::BuildFloorView(
			*Dungeon, DungeonState, Definitions, TEXT("Left"), FIntPoint(31, 5), EGridEdge::East, 0, nullptr, WalledSeam));
	TestEqual(TEXT("Same physical wall authored from both tiles is emitted once"), WalledSeam.Boundaries.Num(), 1);
	TestTrue(TEXT("Deduplicated seam wall is represented from deterministic left tile side"),
		WalledSeam.Boundaries.Num() == 1 && WalledSeam.Boundaries[0].MapCell == FIntPoint(31, 5));
	TestTrue(TEXT("Deduplicated seam wall keeps east orientation"),
		WalledSeam.Boundaries.Num() == 1 && WalledSeam.Boundaries[0].Edge == EGridEdge::East);
	TestTrue(TEXT("Deduplicated seam boundary is a wall"),
		WalledSeam.Boundaries.Num() == 1 && WalledSeam.Boundaries[0].Kind == EGridMapBoundaryKind::Wall);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2167ContractTest, "Grimrock.Map.MON21_6_7.FloorProjection.TransientAndInvalidSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2167ContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON2167Tests;

	TestEqual(TEXT("Floor projection does not change exact-match SaveGame v23"), UGrimrockPartySaveGame::CurrentSaveVersion, 23);
	for (TFieldIterator<FProperty> It(FGridMapFloorView::StaticStruct()); It; ++It)
	{
		TestFalse(*FString::Printf(TEXT("Floor view property '%s' is not SaveGame state"), *It->GetName()), It->HasAnyPropertyFlags(CPF_SaveGame));
	}

	UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(GetTransientPackage());
	UGridLevelAsset* Level = MakeOpenLevel(Dungeon);
	AddEntry(Dungeon, TEXT("OnlyFloor"), Level, FIntVector(0, 0, 4));

	FGridDungeonRuntimeState DungeonState;
	AddExploredState(DungeonState, TEXT("OnlyFloor"), FIntPoint(0, 0));
	const TArray<TObjectPtr<UGridWorldObjectDefinitionAsset>> Definitions;

	FGridMapFloorView View;
	View.SelectedFloorZ = 123;
	View.bHasPartyMarker = true;
	TestFalse(TEXT("Selecting a Z with no enabled dungeon floor is rejected"),
		FGridMapReadModelBuilder::BuildFloorView(
			*Dungeon, DungeonState, Definitions, TEXT("OnlyFloor"), FIntPoint(0, 0), EGridEdge::North, 99, nullptr, View));
	TestEqual(TEXT("Failed build leaves the output reset"), View.Cells.Num(), 0);
	TestFalse(TEXT("Failed build does not leak a stale party marker"), View.bHasPartyMarker);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
