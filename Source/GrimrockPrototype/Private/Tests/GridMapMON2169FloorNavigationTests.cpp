#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Core/GridDungeonAsset.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Map/GridMapExplorationState.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UI/GridMapWidget.h"
#include "UObject/UnrealType.h"

namespace GridMapMON2169Tests
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
				FName(*FString::Printf(TEXT("MapMON2169World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
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

	void AddEntry(
		UGridDungeonAsset* Dungeon,
		FName LevelId,
		UGridLevelAsset* Level,
		int32 FloorZ,
		const FIntPoint& TileXY = FIntPoint::ZeroValue,
		bool bEnabled = true)
	{
		FGridDungeonLevelEntry Entry;
		Entry.LevelId = LevelId;
		Entry.LevelAsset = Level;
		Entry.LogicalPosition = FIntVector(TileXY.X, TileXY.Y, FloorZ);
		Entry.bEnabled = bEnabled;
		Dungeon->Levels.Add(Entry);
	}

	void AddExploredCell(FGridDungeonRuntimeState& DungeonState, FName LevelId, const FIntPoint& Cell)
	{
		FGridLevelRuntimeState& State = DungeonState.LevelStates.Add(LevelId);
		State.LevelId = LevelId;
		bool bNewlyExplored = false;
		State.MapExploration.TryMarkExplored(Cell, bNewlyExplored);
	}

	struct FFixture
	{
		FTestWorld TestWorld;
		AGridLevelRuntimeActor* Runtime = nullptr;
		AGrimrockPartyPawn* Party = nullptr;
		UGridDungeonAsset* Dungeon = nullptr;
		UGridMapWidget* Widget = nullptr;

		bool Build(FAutomationTestBase& Test)
		{
			if (!Test.TestNotNull(TEXT("Transient world exists"), TestWorld.World))
			{
				return false;
			}

			Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
			Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
			if (!Test.TestNotNull(TEXT("Runtime actor exists"), Runtime) || !Test.TestNotNull(TEXT("Party pawn exists"), Party))
			{
				return false;
			}

			Dungeon = NewObject<UGridDungeonAsset>(Runtime);
			UGridLevelAsset* Lower = MakeOpenLevel(Dungeon);
			UGridLevelAsset* Current = MakeOpenLevel(Dungeon);
			UGridLevelAsset* Upper = MakeOpenLevel(Dungeon);
			UGridLevelAsset* Disabled = MakeOpenLevel(Dungeon);

			AddEntry(Dungeon, TEXT("Lower"), Lower, -3);
			AddEntry(Dungeon, TEXT("Current"), Current, 0);
			AddEntry(Dungeon, TEXT("Upper"), Upper, 2, FIntPoint(1, 0));
			AddEntry(Dungeon, TEXT("Disabled"), Disabled, 9, FIntPoint::ZeroValue, false);

			AddExploredCell(Runtime->DungeonRuntimeState, TEXT("Lower"), FIntPoint(3, 3));
			AddExploredCell(Runtime->DungeonRuntimeState, TEXT("Current"), FIntPoint(10, 10));
			AddExploredCell(Runtime->DungeonRuntimeState, TEXT("Upper"), FIntPoint(5, 6));

			Runtime->DungeonAsset = Dungeon;
			Runtime->LevelAsset = Current;
			Runtime->CurrentDungeonLevelId = TEXT("Current");

			Party->LevelRuntimeActor = Runtime;
			Party->CurrentCellX = 10;
			Party->CurrentCellY = 10;
			Party->Facing = EGridEdge::North;

			Widget = NewObject<UGridMapWidget>(Party);
			Widget->InitializeMapWidget(Party);
			return Test.TestNotNull(TEXT("Map widget exists"), Widget);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2169TraversalTest, "Grimrock.Map.MON21_6_9.FloorNavigation.SkipsMissingZAndStopsAtBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2169TraversalTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON2169Tests;

	FFixture Fixture;
	if (!Fixture.Build(*this))
	{
		return false;
	}

	TestEqual(TEXT("Map opens on the party floor"), Fixture.Widget->GetSelectedFloorZ(), 0);
	TestTrue(TEXT("Higher available floor exists"), Fixture.Widget->CanNavigateFloorUp());
	TestTrue(TEXT("Lower available floor exists"), Fixture.Widget->CanNavigateFloorDown());

	TestTrue(TEXT("Level Up skips missing Z=1 and reaches Z=2"), Fixture.Widget->NavigateFloorUp());
	TestEqual(TEXT("Selected floor is Z=2"), Fixture.Widget->GetSelectedFloorZ(), 2);
	TestFalse(TEXT("No Level Up beyond highest enabled floor"), Fixture.Widget->CanNavigateFloorUp());
	TestFalse(TEXT("Level Up at upper bound is rejected"), Fixture.Widget->NavigateFloorUp());
	TestEqual(TEXT("Rejected Level Up preserves selection"), Fixture.Widget->GetSelectedFloorZ(), 2);

	TestTrue(TEXT("Level Down returns directly from Z=2 to Z=0"), Fixture.Widget->NavigateFloorDown());
	TestEqual(TEXT("Selected floor returns to Z=0"), Fixture.Widget->GetSelectedFloorZ(), 0);
	TestTrue(TEXT("Level Down skips missing Z values and reaches Z=-3"), Fixture.Widget->NavigateFloorDown());
	TestEqual(TEXT("Selected floor is Z=-3"), Fixture.Widget->GetSelectedFloorZ(), -3);
	TestFalse(TEXT("No Level Down beyond lowest enabled floor"), Fixture.Widget->CanNavigateFloorDown());
	TestTrue(TEXT("From the lowest floor, the next enabled floor upward is still Z=0"), Fixture.Widget->CanNavigateFloorUp());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2169ProjectionTest, "Grimrock.Map.MON21_6_9.FloorNavigation.OtherFloorProjectionHidesParty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2169ProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON2169Tests;

	FFixture Fixture;
	if (!Fixture.Build(*this))
	{
		return false;
	}

	TestTrue(TEXT("Navigate to upper floor"), Fixture.Widget->NavigateFloorUp());
	const FGridMapFloorView& UpperView = Fixture.Widget->GetFloorView();
	TestEqual(TEXT("Read model follows selected upper floor"), UpperView.SelectedFloorZ, 2);
	TestFalse(TEXT("Party marker is hidden on a non-current floor"), UpperView.bHasPartyMarker);
	TestEqual(TEXT("Only upper explored geometry is projected"), UpperView.Cells.Num(), 1);
	TestTrue(TEXT("Upper tile global coordinates use stride 32"),
		UpperView.Cells.Num() == 1 && UpperView.Cells[0].MapCell == FIntPoint(37, 6));

	TestTrue(TEXT("Navigate back down to current floor"), Fixture.Widget->NavigateFloorDown());
	const FGridMapFloorView& CurrentView = Fixture.Widget->GetFloorView();
	TestTrue(TEXT("Party marker reappears on current floor"), CurrentView.bHasPartyMarker);
	TestTrue(TEXT("Party marker is on the canonical current cell"), CurrentView.PartyMapCell == FIntPoint(10, 10));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2169RefreshContractTest, "Grimrock.Map.MON21_6_9.FloorNavigation.RefreshPreservesSelectionOpenResetsToParty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2169RefreshContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON2169Tests;

	FFixture Fixture;
	if (!Fixture.Build(*this))
	{
		return false;
	}

	TestTrue(TEXT("Navigate to upper floor"), Fixture.Widget->NavigateFloorUp());
	TestEqual(TEXT("Upper floor selected"), Fixture.Widget->GetSelectedFloorZ(), 2);

	TestTrue(TEXT("Event-style RefreshMap rebuilds current selection"), Fixture.Widget->RefreshMap());
	TestEqual(TEXT("RefreshMap preserves browsed floor"), Fixture.Widget->GetSelectedFloorZ(), 2);
	TestFalse(TEXT("Refresh on browsed floor keeps party marker hidden"), Fixture.Widget->GetFloorView().bHasPartyMarker);

	TestTrue(TEXT("Opening/reset operation selects party floor"), Fixture.Widget->SelectPartyFloor());
	TestEqual(TEXT("Party floor is restored to Z=0"), Fixture.Widget->GetSelectedFloorZ(), 0);
	TestTrue(TEXT("Party marker returns after selecting party floor"), Fixture.Widget->GetFloorView().bHasPartyMarker);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2169LevelLabelTest, "Grimrock.Map.MON21_6_9.FloorNavigation.LogicalZLabel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2169LevelLabelTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON2169Tests;

	FFixture Fixture;
	if (!Fixture.Build(*this))
	{
		return false;
	}

	Fixture.Widget->Text_FloorLabel = NewObject<UTextBlock>(Fixture.Widget);
	TestTrue(TEXT("Selecting party floor refreshes the label"), Fixture.Widget->SelectPartyFloor());
	TestEqual(TEXT("Current floor label uses canonical logical Z"),
		Fixture.Widget->Text_FloorLabel->GetText().ToString(), FString(TEXT("Niveau 0")));

	TestTrue(TEXT("Navigate to upper floor"), Fixture.Widget->NavigateFloorUp());
	TestEqual(TEXT("Upper floor label uses its logical Z, independent of tile names"),
		Fixture.Widget->Text_FloorLabel->GetText().ToString(), FString(TEXT("Niveau 2")));

	TestTrue(TEXT("Navigate down twice to lower floor"), Fixture.Widget->NavigateFloorDown() && Fixture.Widget->NavigateFloorDown());
	TestEqual(TEXT("Negative logical Z is displayed directly"),
		Fixture.Widget->Text_FloorLabel->GetText().ToString(), FString(TEXT("Niveau -3")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2169UIContractTest, "Grimrock.Map.MON21_6_9.FloorNavigation.OptionalUMGAndTransientState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2169UIContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* MapClass = UGridMapWidget::StaticClass();
	TestNotNull(TEXT("Level Up callable exists"), MapClass->FindFunctionByName(TEXT("NavigateFloorUp")));
	TestNotNull(TEXT("Level Down callable exists"), MapClass->FindFunctionByName(TEXT("NavigateFloorDown")));
	TestNotNull(TEXT("Party floor selection callable exists"), MapClass->FindFunctionByName(TEXT("SelectPartyFloor")));
	TestNotNull(TEXT("CanNavigateFloorUp query exists"), MapClass->FindFunctionByName(TEXT("CanNavigateFloorUp")));
	TestNotNull(TEXT("CanNavigateFloorDown query exists"), MapClass->FindFunctionByName(TEXT("CanNavigateFloorDown")));

	const FObjectPropertyBase* UpProperty = CastField<FObjectPropertyBase>(MapClass->FindPropertyByName(TEXT("Button_LevelUp")));
	const FObjectPropertyBase* DownProperty = CastField<FObjectPropertyBase>(MapClass->FindPropertyByName(TEXT("Button_LevelDown")));
	const FObjectPropertyBase* LabelProperty = CastField<FObjectPropertyBase>(MapClass->FindPropertyByName(TEXT("Text_FloorLabel")));
	TestTrue(TEXT("Optional Level Up binding is a UButton"), UpProperty && UpProperty->PropertyClass == UButton::StaticClass());
	TestTrue(TEXT("Optional Level Down binding is a UButton"), DownProperty && DownProperty->PropertyClass == UButton::StaticClass());
	TestTrue(TEXT("Optional floor label binding is a UTextBlock"), LabelProperty && LabelProperty->PropertyClass == UTextBlock::StaticClass());

	const FProperty* SelectedFloorProperty = MapClass->FindPropertyByName(TEXT("SelectedFloorZ"));
	TestNotNull(TEXT("SelectedFloorZ exists as widget-only state"), SelectedFloorProperty);
	TestTrue(TEXT("SelectedFloorZ is transient"), SelectedFloorProperty && SelectedFloorProperty->HasAnyPropertyFlags(CPF_Transient));
	TestFalse(TEXT("SelectedFloorZ is never SaveGame state"), SelectedFloorProperty && SelectedFloorProperty->HasAnyPropertyFlags(CPF_SaveGame));
	TestEqual(TEXT("Floor navigation does not change exact-match SaveGame v23"), UGrimrockPartySaveGame::CurrentSaveVersion, 23);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
