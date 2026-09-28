#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/Button.h"
#include "Core/GridDungeonAsset.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Map/GridMapExplorationState.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UI/GridMapWidget.h"
#include "UObject/UnrealType.h"

namespace GridMapMON21610Tests
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
				FName(*FString::Printf(TEXT("MapMON21610World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
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

	void AddEntry(UGridDungeonAsset* Dungeon, FName LevelId, UGridLevelAsset* Level, int32 Z)
	{
		FGridDungeonLevelEntry Entry;
		Entry.LevelId = LevelId;
		Entry.LevelAsset = Level;
		Entry.LogicalPosition = FIntVector(0, 0, Z);
		Entry.bEnabled = true;
		Dungeon->Levels.Add(Entry);
	}

	void AddExplored(FGridDungeonRuntimeState& DungeonState, FName LevelId, const FIntPoint& Cell)
	{
		FGridLevelRuntimeState& State = DungeonState.LevelStates.Add(LevelId);
		State.LevelId = LevelId;
		bool bNew = false;
		State.MapExploration.TryMarkExplored(Cell, bNew);
	}

	struct FFixture
	{
		FTestWorld TestWorld;
		AGridLevelRuntimeActor* Runtime = nullptr;
		AGrimrockPartyPawn* Party = nullptr;
		UGridMapWidget* Widget = nullptr;

		bool Build(FAutomationTestBase& Test)
		{
			if (!Test.TestNotNull(TEXT("Transient world exists"), TestWorld.World))
			{
				return false;
			}

			Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
			Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
			if (!Test.TestNotNull(TEXT("Runtime exists"), Runtime) || !Test.TestNotNull(TEXT("Party exists"), Party))
			{
				return false;
			}

			UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(Runtime);
			UGridLevelAsset* Current = MakeOpenLevel(Dungeon);
			UGridLevelAsset* Upper = MakeOpenLevel(Dungeon);
			AddEntry(Dungeon, TEXT("Current"), Current, 0);
			AddEntry(Dungeon, TEXT("Upper"), Upper, 2);
			AddExplored(Runtime->DungeonRuntimeState, TEXT("Current"), FIntPoint(10, 10));
			AddExplored(Runtime->DungeonRuntimeState, TEXT("Current"), FIntPoint(11, 10));
			AddExplored(Runtime->DungeonRuntimeState, TEXT("Upper"), FIntPoint(4, 4));

			Runtime->DungeonAsset = Dungeon;
			Runtime->LevelAsset = Current;
			Runtime->CurrentDungeonLevelId = TEXT("Current");

			Party->LevelRuntimeActor = Runtime;
			Party->CurrentCellX = 10;
			Party->CurrentCellY = 10;
			Party->Facing = EGridEdge::North;

			Widget = NewObject<UGridMapWidget>(Party);
			Widget->InitializeMapWidget(Party);
			return Test.TestTrue(TEXT("Map widget has renderable data"), Widget && Widget->HasRenderableMap());
		}
	};

	bool ReadPrivateBool(UGridMapWidget* Widget, const TCHAR* PropertyName, bool& OutValue)
	{
		const FBoolProperty* Property = CastField<FBoolProperty>(UGridMapWidget::StaticClass()->FindPropertyByName(PropertyName));
		if (!Property || !Widget)
		{
			return false;
		}
		OutValue = Property->GetPropertyValue_InContainer(Widget);
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON21610ZoomTest, "Grimrock.Map.MON21_6_10.View.ZoomClampsWithoutReadModelRebuild",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON21610ZoomTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON21610Tests;

	FFixture Fixture;
	if (!Fixture.Build(*this))
	{
		return false;
	}

	const FGridMapFloorCellView* const CellsBefore = Fixture.Widget->GetFloorView().Cells.GetData();
	const int32 CountBefore = Fixture.Widget->GetFloorView().Cells.Num();
	Fixture.Widget->MinZoomScale = 0.5f;
	Fixture.Widget->MaxZoomScale = 2.0f;
	Fixture.Widget->ZoomStep = 0.25f;

	TestTrue(TEXT("Positive wheel delta zooms in"), Fixture.Widget->AdjustZoom(1.0f));
	TestEqual(TEXT("Zoom increments by configured step"), Fixture.Widget->GetZoomScale(), 1.25f);
	TestTrue(TEXT("Large positive delta clamps to max"), Fixture.Widget->AdjustZoom(100.0f));
	TestEqual(TEXT("Zoom is clamped to max"), Fixture.Widget->GetZoomScale(), 2.0f);
	TestTrue(TEXT("Large negative delta clamps to min"), Fixture.Widget->AdjustZoom(-100.0f));
	TestEqual(TEXT("Zoom is clamped to min"), Fixture.Widget->GetZoomScale(), 0.5f);
	TestFalse(TEXT("Further zoom-out at min does nothing"), Fixture.Widget->AdjustZoom(-1.0f));

	TestEqual(TEXT("Zoom never changes projected cell count"), Fixture.Widget->GetFloorView().Cells.Num(), CountBefore);
	TestTrue(TEXT("Zoom does not rebuild the FloorView cell buffer"), Fixture.Widget->GetFloorView().Cells.GetData() == CellsBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON21610PanTest, "Grimrock.Map.MON21_6_10.View.PanIsTransientAndFloorNavigationResetsIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON21610PanTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON21610Tests;

	FFixture Fixture;
	if (!Fixture.Build(*this))
	{
		return false;
	}

	const FGridMapFloorCellView* const CellsBefore = Fixture.Widget->GetFloorView().Cells.GetData();
	Fixture.Widget->PanMapByPixels(FVector2D(48.0, -24.0));
	TestTrue(TEXT("Pan offset accumulates in UI pixels"), Fixture.Widget->GetPanOffsetPixels().Equals(FVector2D(48.0, -24.0)));
	TestTrue(TEXT("Pan does not rebuild the FloorView cell buffer"), Fixture.Widget->GetFloorView().Cells.GetData() == CellsBefore);

	TestTrue(TEXT("Navigate to another floor"), Fixture.Widget->NavigateFloorUp());
	TestTrue(TEXT("Floor navigation resets pan"), Fixture.Widget->GetPanOffsetPixels().IsNearlyZero());
	TestEqual(TEXT("Floor navigation keeps the current zoom"), Fixture.Widget->GetZoomScale(), 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON21610RecenterTest, "Grimrock.Map.MON21_6_10.View.RecenterReturnsToPartyFloorAndPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON21610RecenterTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON21610Tests;

	FFixture Fixture;
	if (!Fixture.Build(*this))
	{
		return false;
	}

	Fixture.Widget->AdjustZoom(2.0f);
	const float ZoomBefore = Fixture.Widget->GetZoomScale();
	Fixture.Widget->PanMapByPixels(FVector2D(30.0, 15.0));
	TestTrue(TEXT("Navigate away from party floor"), Fixture.Widget->NavigateFloorUp());
	Fixture.Widget->PanMapByPixels(FVector2D(-20.0, 10.0));

	TestTrue(TEXT("Recenter succeeds"), Fixture.Widget->RecenterMap());
	TestEqual(TEXT("Recenter restores party logical floor"), Fixture.Widget->GetSelectedFloorZ(), 0);
	TestTrue(TEXT("Recenter restores party marker"), Fixture.Widget->GetFloorView().bHasPartyMarker);
	TestTrue(TEXT("Recenter clears pan"), Fixture.Widget->GetPanOffsetPixels().IsNearlyZero());
	TestEqual(TEXT("Recenter preserves zoom"), Fixture.Widget->GetZoomScale(), ZoomBefore);

	bool bCenterOnParty = false;
	TestTrue(TEXT("Center-on-party state can be inspected"), ReadPrivateBool(Fixture.Widget, TEXT("bCenterViewOnParty"), bCenterOnParty));
	TestTrue(TEXT("Recenter enables party-centered rendering"), bCenterOnParty);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON21610ContractTest, "Grimrock.Map.MON21_6_10.View.OptionalUMGAndTransientContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON21610ContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* MapClass = UGridMapWidget::StaticClass();
	TestNotNull(TEXT("RecenterMap callable exists"), MapClass->FindFunctionByName(TEXT("RecenterMap")));
	TestNotNull(TEXT("AdjustZoom callable exists"), MapClass->FindFunctionByName(TEXT("AdjustZoom")));
	TestNotNull(TEXT("PanMapByPixels callable exists"), MapClass->FindFunctionByName(TEXT("PanMapByPixels")));

	const FObjectPropertyBase* RecenterProperty = CastField<FObjectPropertyBase>(MapClass->FindPropertyByName(TEXT("Button_Recenter")));
	TestTrue(TEXT("Optional recenter binding is a UButton"), RecenterProperty && RecenterProperty->PropertyClass == UButton::StaticClass());

	for (const TCHAR* PropertyName : { TEXT("ZoomScale"), TEXT("PanOffsetPixels"), TEXT("bCenterViewOnParty"), TEXT("bIsPanning") })
	{
		const FProperty* Property = MapClass->FindPropertyByName(PropertyName);
		TestNotNull(*FString::Printf(TEXT("%s exists"), PropertyName), Property);
		TestTrue(*FString::Printf(TEXT("%s is transient"), PropertyName), Property && Property->HasAnyPropertyFlags(CPF_Transient));
		TestFalse(*FString::Printf(TEXT("%s is never SaveGame"), PropertyName), Property && Property->HasAnyPropertyFlags(CPF_SaveGame));
	}

	TestEqual(TEXT("View navigation does not change exact-match SaveGame v23"), UGrimrockPartySaveGame::CurrentSaveVersion, 23);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
