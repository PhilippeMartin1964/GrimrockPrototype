#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDungeonAsset.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/Map/GridMapExplorationState.h"
#include "Runtime/Map/GridMapReadModel.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UI/GridMapWidget.h"
#include "UObject/UnrealType.h"

namespace GridMapMON21612Tests
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

	UGridWorldObjectDefinitionAsset* MakeDefinition(
		UObject* Outer,
		FName Id,
		EGridLevelObjectType SupportedType,
		EGridMapSymbolStyle MapSymbolStyle = EGridMapSymbolStyle::None)
	{
		UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>(Outer);
		Definition->DefinitionId = Id;
		Definition->SupportedType = SupportedType;
		Definition->MapSymbolStyle = MapSymbolStyle;
		return Definition;
	}

	FGridWorldObjectInstance MakeObject(
		FName DefinitionId,
		EGridLevelObjectType InstanceType,
		const FIntPoint& Cell)
	{
		FGridWorldObjectInstance Object;
		Object.InstanceId = FGuid::NewGuid();
		Object.WorldObjectDefinitionId = DefinitionId;
		Object.Type = InstanceType;
		Object.CellX = Cell.X;
		Object.CellY = Cell.Y;
		return Object;
	}

	bool HasTileSymbol(const FGridMapTileView& View, const FIntPoint& Cell, EGridMapSymbolKind Kind)
	{
		return View.Symbols.ContainsByPredicate(
			[Cell, Kind](const FGridMapSymbolView& Symbol)
			{
				return Symbol.LocalCell == Cell && Symbol.Kind == Kind;
			});
	}

	bool HasFloorSymbol(const FGridMapFloorView& View, const FIntPoint& Cell, EGridMapSymbolKind Kind)
	{
		return View.Symbols.ContainsByPredicate(
			[Cell, Kind](const FGridMapFloorSymbolView& Symbol)
			{
				return Symbol.MapCell == Cell && Symbol.Kind == Kind;
			});
	}

	void AddDungeonEntry(
		UGridDungeonAsset* Dungeon,
		FName LevelId,
		UGridLevelAsset* Level,
		const FIntVector& LogicalPosition)
	{
		FGridDungeonLevelEntry Entry;
		Entry.LevelId = LevelId;
		Entry.LevelAsset = Level;
		Entry.LogicalPosition = LogicalPosition;
		Entry.bEnabled = true;
		Dungeon->Levels.Add(Entry);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON21612FilteringTest, "Grimrock.Map.MON21_6_12.Symbols.ExploredOnlyAndNoIdentityLeak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON21612FilteringTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON21612Tests;

	UGridLevelAsset* Level = MakeOpenLevel(GetTransientPackage());
	Level->GetCellMutable(3, 3).CellType = EGridCellType::StairsUp;
	Level->GetCellMutable(4, 4).CellType = EGridCellType::StairsDown;

	UGridWorldObjectDefinitionAsset* RelocationDefinition =
		MakeDefinition(Level, TEXT("MON21612_Relocation"), EGridLevelObjectType::Relocation, EGridMapSymbolStyle::Relocation);

	const FGridWorldObjectInstance ExploredRelocation =
		MakeObject(RelocationDefinition->DefinitionId, EGridLevelObjectType::Decoration, FIntPoint(5, 5));
	const FGridWorldObjectInstance HiddenRelocation =
		MakeObject(RelocationDefinition->DefinitionId, EGridLevelObjectType::Relocation, FIntPoint(6, 6));

	UGridWorldObjectDefinitionAsset* NoSymbolDefinition =
		MakeDefinition(Level, TEXT("MON21612_NoMapSymbol"), EGridLevelObjectType::Relocation);
	const FGridWorldObjectInstance NoSymbolRelocation =
		MakeObject(NoSymbolDefinition->DefinitionId, EGridLevelObjectType::Relocation, FIntPoint(7, 7));

	UGridWorldObjectDefinitionAsset* RemovedPoiDefinition =
		MakeDefinition(Level, TEXT("MON21612_RemovedPOI"), EGridLevelObjectType::Decoration, EGridMapSymbolStyle::PointOfInterest);
	const FGridWorldObjectInstance RemovedPoi =
		MakeObject(RemovedPoiDefinition->DefinitionId, EGridLevelObjectType::Decoration, FIntPoint(8, 7));

	Level->WorldObjectInstances.Add(ExploredRelocation);
	Level->WorldObjectInstances.Add(HiddenRelocation);
	Level->WorldObjectInstances.Add(NoSymbolRelocation);
	Level->WorldObjectInstances.Add(RemovedPoi);

	FGridLevelRuntimeState State;
	State.LevelId = TEXT("Symbols");
	bool bNew = false;
	State.MapExploration.TryMarkExplored(FIntPoint(3, 3), bNew);
	State.MapExploration.TryMarkExplored(FIntPoint(4, 4), bNew);
	State.MapExploration.TryMarkExplored(FIntPoint(5, 5), bNew);
	State.MapExploration.TryMarkExplored(FIntPoint(7, 7), bNew);
	State.MapExploration.TryMarkExplored(FIntPoint(8, 7), bNew);
	FGridRuntimeObjectPresenceState& RemovedPresence = State.ObjectPresence.Add(RemovedPoi.InstanceId);
	RemovedPresence.ObjectId = RemovedPoi.InstanceId;
	RemovedPresence.bRemovedFromInitialPlacement = true;

	const TArray<TObjectPtr<UGridWorldObjectDefinitionAsset>> Definitions = {
		RelocationDefinition,
		NoSymbolDefinition,
		RemovedPoiDefinition
	};
	FGridMapTileView View;
	TestTrue(TEXT("Symbol read model builds"),
		FGridMapReadModelBuilder::BuildTileView(State.LevelId, *Level, State, Definitions, nullptr, View));

	TestTrue(TEXT("Explored StairsUp cell exposes a StairsUp symbol"),
		HasTileSymbol(View, FIntPoint(3, 3), EGridMapSymbolKind::StairsUp));
	TestTrue(TEXT("Explored StairsDown cell exposes a StairsDown symbol"),
		HasTileSymbol(View, FIntPoint(4, 4), EGridMapSymbolKind::StairsDown));
	TestTrue(TEXT("Explicit definition MapSymbolStyle exposes explored Relocation"),
		HasTileSymbol(View, FIntPoint(5, 5), EGridMapSymbolKind::Relocation));
	TestFalse(TEXT("Unexplored Relocation is never exposed"),
		HasTileSymbol(View, FIntPoint(6, 6), EGridMapSymbolKind::Relocation));
	TestFalse(TEXT("Explored world object without explicit MapSymbolStyle remains absent"),
		HasTileSymbol(View, FIntPoint(7, 7), EGridMapSymbolKind::Relocation));
	TestFalse(TEXT("Removed PointOfInterest does not remain on the Map"),
		HasTileSymbol(View, FIntPoint(8, 7), EGridMapSymbolKind::PointOfInterest));

	UScriptStruct* SymbolStruct = FGridMapSymbolView::StaticStruct();
	TestNull(TEXT("Map symbol exposes no ObjectId"), SymbolStruct->FindPropertyByName(TEXT("ObjectId")));
	TestNull(TEXT("Map symbol exposes no WorldObjectDefinitionId"), SymbolStruct->FindPropertyByName(TEXT("WorldObjectDefinitionId")));
	TestNull(TEXT("Map symbol exposes no LogicId"), SymbolStruct->FindPropertyByName(TEXT("LogicId")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapSTAIRS01OrientationTest, "Grimrock.Map.MAP_STAIRS01.Orientation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapSTAIRS01OrientationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON21612Tests;

	UGridLevelAsset* Level = MakeOpenLevel(GetTransientPackage());
	UGridWorldObjectDefinitionAsset* StairsUpDefinition =
		MakeDefinition(Level, TEXT("MAP_STAIRS01_Up"), EGridLevelObjectType::Relocation, EGridMapSymbolStyle::StairsUp);
	UGridWorldObjectDefinitionAsset* StairsDownDefinition =
		MakeDefinition(Level, TEXT("MAP_STAIRS01_Down"), EGridLevelObjectType::Relocation, EGridMapSymbolStyle::StairsDown);

	auto MakeOrientedStair = [](FName DefinitionId, const FIntPoint& Cell, float Yaw)
	{
		FGridWorldObjectInstance Stair =
			MakeObject(DefinitionId, EGridLevelObjectType::Relocation, Cell);
		if (!FMath::IsNearlyZero(Yaw))
		{
			Stair.bHasLocalTransformOverride = true;
			Stair.LocalTransformOverride = FTransform(FRotator(0.0f, Yaw, 0.0f));
		}
		return Stair;
	};

	Level->WorldObjectInstances.Add(MakeOrientedStair(StairsUpDefinition->DefinitionId, FIntPoint(10, 10), 0.0f));
	Level->WorldObjectInstances.Add(MakeOrientedStair(StairsDownDefinition->DefinitionId, FIntPoint(11, 10), 90.0f));
	Level->WorldObjectInstances.Add(MakeOrientedStair(StairsUpDefinition->DefinitionId, FIntPoint(12, 10), 180.0f));
	Level->WorldObjectInstances.Add(MakeOrientedStair(StairsDownDefinition->DefinitionId, FIntPoint(13, 10), 270.0f));

	FGridLevelRuntimeState State;
	State.LevelId = TEXT("StairOrientation");
	bool bNew = false;
	for (int32 X = 10; X <= 13; ++X)
	{
		State.MapExploration.TryMarkExplored(FIntPoint(X, 10), bNew);
	}

	const TArray<TObjectPtr<UGridWorldObjectDefinitionAsset>> Definitions = { StairsUpDefinition, StairsDownDefinition };
	FGridMapTileView TileView;
	TestTrue(TEXT("Directional stairs tile view builds"),
		FGridMapReadModelBuilder::BuildTileView(State.LevelId, *Level, State, Definitions, nullptr, TileView));

	auto TestFacing = [this, &TileView](const TCHAR* What, const FIntPoint& Cell, EGridMapSymbolKind Kind, EGridEdge Expected)
	{
		const FGridMapSymbolView* Symbol = TileView.Symbols.FindByPredicate(
			[Cell, Kind](const FGridMapSymbolView& Candidate)
			{
				return Candidate.LocalCell == Cell && Candidate.Kind == Kind;
			});
		return TestTrue(What, Symbol && Symbol->Facing == Expected);
	};

	TestFacing(TEXT("0 deg stair keeps South-to-North default"), FIntPoint(10, 10), EGridMapSymbolKind::StairsUp, EGridEdge::North);
	TestFacing(TEXT("90 deg stair faces East"), FIntPoint(11, 10), EGridMapSymbolKind::StairsDown, EGridEdge::East);
	TestFacing(TEXT("180 deg stair faces South"), FIntPoint(12, 10), EGridMapSymbolKind::StairsUp, EGridEdge::South);
	TestFacing(TEXT("270 deg stair faces West for East-to-West travel"), FIntPoint(13, 10), EGridMapSymbolKind::StairsDown, EGridEdge::West);

	UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(GetTransientPackage());
	AddDungeonEntry(Dungeon, State.LevelId, Level, FIntVector::ZeroValue);
	FGridDungeonRuntimeState DungeonState;
	DungeonState.LevelStates.Add(State.LevelId) = State;

	FGridMapFloorView FloorView;
	TestTrue(TEXT("Directional stairs floor view builds"),
		FGridMapReadModelBuilder::BuildFloorView(
			*Dungeon,
			DungeonState,
			Definitions,
			State.LevelId,
			FIntPoint(10, 10),
			EGridEdge::North,
			0,
			nullptr,
			FloorView));

	const FGridMapFloorSymbolView* WestFacingFloorStair = FloorView.Symbols.FindByPredicate(
		[](const FGridMapFloorSymbolView& Symbol)
		{
			return Symbol.MapCell == FIntPoint(13, 10) && Symbol.Kind == EGridMapSymbolKind::StairsDown;
		});
	TestTrue(TEXT("Floor projection preserves West-facing stair orientation"),
		WestFacingFloorStair && WestFacingFloorStair->Facing == EGridEdge::West);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON21612PitTest, "Grimrock.Map.MON21_6_12.Symbols.MAP_PIT01.PitPersistsAcrossState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON21612PitTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON21612Tests;

	UGridLevelAsset* Level = MakeOpenLevel(GetTransientPackage());
	UGridWorldObjectDefinitionAsset* PitDefinition =
		MakeDefinition(Level, TEXT("MON21612_Pit"), EGridLevelObjectType::Pit, EGridMapSymbolStyle::Pit);

	FGridWorldObjectInstance Pit =
		MakeObject(PitDefinition->DefinitionId, EGridLevelObjectType::Decoration, FIntPoint(8, 8));
	Pit.InstanceConfig.Pit.bInitiallyOpen = false;
	Level->WorldObjectInstances.Add(Pit);

	FGridLevelRuntimeState State;
	State.LevelId = TEXT("PitSymbols");
	bool bNew = false;
	State.MapExploration.TryMarkExplored(FIntPoint(8, 8), bNew);

	const TArray<TObjectPtr<UGridWorldObjectDefinitionAsset>> Definitions = { PitDefinition };

	FGridMapTileView ClosedView;
	TestTrue(TEXT("Closed pit tile view builds"),
		FGridMapReadModelBuilder::BuildTileView(State.LevelId, *Level, State, Definitions, nullptr, ClosedView));
	TestTrue(TEXT("Explored closed pit keeps its Pit symbol"),
		HasTileSymbol(ClosedView, FIntPoint(8, 8), EGridMapSymbolKind::Pit));

	FGridRuntimePitState& RuntimePit = State.Pits.Add(Pit.InstanceId);
	RuntimePit.ObjectId = Pit.InstanceId;
	RuntimePit.bIsOpen = true;

	FGridMapTileView OpenView;
	TestTrue(TEXT("Open pit tile view builds"),
		FGridMapReadModelBuilder::BuildTileView(State.LevelId, *Level, State, Definitions, nullptr, OpenView));
	TestTrue(TEXT("Runtime-open pit keeps its Pit symbol"),
		HasTileSymbol(OpenView, FIntPoint(8, 8), EGridMapSymbolKind::Pit));

	RuntimePit.bIsOpen = false;
	FGridMapTileView ReclosedView;
	TestTrue(TEXT("Reclosed pit tile view builds"),
		FGridMapReadModelBuilder::BuildTileView(State.LevelId, *Level, State, Definitions, nullptr, ReclosedView));
	TestTrue(TEXT("Reclosed pit still keeps its known Pit symbol"),
		HasTileSymbol(ReclosedView, FIntPoint(8, 8), EGridMapSymbolKind::Pit));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON21612FloorProjectionTest, "Grimrock.Map.MON21_6_12.Symbols.MultiTileGlobalProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON21612FloorProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON21612Tests;

	UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(GetTransientPackage());
	UGridLevelAsset* West = MakeOpenLevel(Dungeon);
	UGridLevelAsset* East = MakeOpenLevel(Dungeon);

	West->GetCellMutable(31, 1).CellType = EGridCellType::StairsUp;
	UGridWorldObjectDefinitionAsset* RelocationDefinition =
		MakeDefinition(Dungeon, TEXT("MON21612_FloorRelocation"), EGridLevelObjectType::Relocation, EGridMapSymbolStyle::Relocation);
	East->WorldObjectInstances.Add(
		MakeObject(RelocationDefinition->DefinitionId, EGridLevelObjectType::Relocation, FIntPoint(0, 1)));

	AddDungeonEntry(Dungeon, TEXT("West"), West, FIntVector(-1, 2, 0));
	AddDungeonEntry(Dungeon, TEXT("East"), East, FIntVector(0, 2, 0));

	FGridDungeonRuntimeState DungeonState;
	DungeonState.LevelStates.Add(TEXT("West")).LevelId = TEXT("West");
	DungeonState.LevelStates.Add(TEXT("East")).LevelId = TEXT("East");
	FGridLevelRuntimeState* WestState = DungeonState.LevelStates.Find(TEXT("West"));
	FGridLevelRuntimeState* EastState = DungeonState.LevelStates.Find(TEXT("East"));
	if (!TestNotNull(TEXT("West state exists"), WestState) || !TestNotNull(TEXT("East state exists"), EastState))
	{
		return false;
	}
	bool bNew = false;
	WestState->MapExploration.TryMarkExplored(FIntPoint(31, 1), bNew);
	EastState->MapExploration.TryMarkExplored(FIntPoint(0, 1), bNew);

	const TArray<TObjectPtr<UGridWorldObjectDefinitionAsset>> Definitions = { RelocationDefinition };
	FGridMapFloorView View;
	TestTrue(TEXT("Floor symbol projection builds"),
		FGridMapReadModelBuilder::BuildFloorView(
			*Dungeon,
			DungeonState,
			Definitions,
			TEXT("West"),
			FIntPoint(31, 1),
			EGridEdge::North,
			0,
			nullptr,
			View));

	TestTrue(TEXT("West tile symbol projects to global (-1,65)"),
		HasFloorSymbol(View, FIntPoint(-1, 65), EGridMapSymbolKind::StairsUp));
	TestTrue(TEXT("East tile symbol projects to global (0,65)"),
		HasFloorSymbol(View, FIntPoint(0, 65), EGridMapSymbolKind::Relocation));
	TestEqual(TEXT("Exactly two authorized symbols are projected"), View.Symbols.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON21612PresentationTest, "Grimrock.Map.MON21_6_12.Symbols.PresentationContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON21612PresentationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridMapWidget* Widget = NewObject<UGridMapWidget>(GetTransientPackage());
	if (!TestNotNull(TEXT("Map widget exists"), Widget))
	{
		return false;
	}

	TestTrue(TEXT("Symbol stroke is visible by default"), Widget->SymbolStrokeThickness > 0.0f);
	TestTrue(TEXT("Symbol scale stays cell-local"), Widget->SymbolScale >= 0.4f && Widget->SymbolScale <= 1.0f);
	TestTrue(TEXT("Symbols disappear below a readable cell size"), Widget->SymbolMinCellPixels > 0.0f);
	TestTrue(TEXT("Navigation symbol ink is visible"), Widget->NavigationSymbolColor.A > 0.0f);
	TestTrue(TEXT("Hazard symbol ink is visible"), Widget->HazardSymbolColor.A > 0.0f);

	UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>(GetTransientPackage());
	TestEqual(TEXT("Definitions opt out of Map symbols by default"), Definition->MapSymbolStyle, EGridMapSymbolStyle::None);
	Definition->MapSymbolStyle = EGridMapSymbolStyle::PointOfInterest;
	TestEqual(TEXT("PointOfInterest is an explicit presentation-only option"),
		Definition->MapSymbolStyle, EGridMapSymbolStyle::PointOfInterest);

	const FProperty* DefinitionMapSymbolProperty =
		UGridWorldObjectDefinitionAsset::StaticClass()->FindPropertyByName(TEXT("MapSymbolStyle"));
	TestNotNull(TEXT("Definition exposes presentation MapSymbolStyle metadata"), DefinitionMapSymbolProperty);
	TestFalse(TEXT("Definition MapSymbolStyle is not SaveGame state"),
		DefinitionMapSymbolProperty && DefinitionMapSymbolProperty->HasAnyPropertyFlags(CPF_SaveGame));

	for (UScriptStruct* Struct : { FGridMapSymbolView::StaticStruct(), FGridMapFloorSymbolView::StaticStruct() })
	{
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			TestFalse(*FString::Printf(TEXT("%s.%s is transient projection, never SaveGame"), *Struct->GetName(), *It->GetName()),
				It->HasAnyPropertyFlags(CPF_SaveGame));
		}
	}

	for (const TCHAR* PropertyName : {
		TEXT("NavigationSymbolColor"),
		TEXT("HazardSymbolColor"),
		TEXT("SymbolStrokeThickness"),
		TEXT("SymbolScale"),
		TEXT("SymbolMinCellPixels") })
	{
		const FProperty* Property = UGridMapWidget::StaticClass()->FindPropertyByName(PropertyName);
		TestNotNull(*FString::Printf(TEXT("%s exists"), PropertyName), Property);
		TestFalse(*FString::Printf(TEXT("%s is presentation config, never SaveGame"), PropertyName),
			Property && Property->HasAnyPropertyFlags(CPF_SaveGame));
	}

	TestEqual(TEXT("Map Symbols do not change exact-match SaveGame v23"), UGrimrockPartySaveGame::CurrentSaveVersion, 23);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
