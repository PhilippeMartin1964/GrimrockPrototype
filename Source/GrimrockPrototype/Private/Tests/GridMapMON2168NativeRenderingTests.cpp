#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDungeonAsset.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Map/GridMapExplorationState.h"
#include "UI/GridMapWidget.h"
#include "UI/GrimrockDesignSurfaceWidget.h"
#include "UI/GrimrockMenuWidget.h"
#include "UObject/UnrealType.h"

namespace GridMapMON2168Tests
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
				FName(*FString::Printf(TEXT("MapMON2168World_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2168WidgetContractTest, "Grimrock.Map.MON21_6_8.NativeRendering.WidgetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2168WidgetContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestTrue(TEXT("UGridMapWidget derives from the common design surface"),
		UGridMapWidget::StaticClass()->IsChildOf(UGrimrockDesignSurfaceWidget::StaticClass()));
	TestNotNull(TEXT("Map widget exposes InitializeMapWidget"),
		UGridMapWidget::StaticClass()->FindFunctionByName(TEXT("InitializeMapWidget")));
	TestNotNull(TEXT("Map widget exposes RefreshMap"),
		UGridMapWidget::StaticClass()->FindFunctionByName(TEXT("RefreshMap")));
	TestNotNull(TEXT("Map widget exposes HasRenderableMap"),
		UGridMapWidget::StaticClass()->FindFunctionByName(TEXT("HasRenderableMap")));

	const FProperty* FloorViewProperty = UGridMapWidget::StaticClass()->FindPropertyByName(TEXT("FloorView"));
	TestNotNull(TEXT("Map widget keeps one transient floor read model"), FloorViewProperty);
	TestTrue(TEXT("Floor view is transient presentation state"),
		FloorViewProperty && FloorViewProperty->HasAnyPropertyFlags(CPF_Transient));
	TestFalse(TEXT("Floor view is never SaveGame state"),
		FloorViewProperty && FloorViewProperty->HasAnyPropertyFlags(CPF_SaveGame));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2168RefreshTest, "Grimrock.Map.MON21_6_8.NativeRendering.RefreshBuildsCurrentFloor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2168RefreshTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridMapMON2168Tests;

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
	UGridLevelAsset* Level = MakeOpenLevel(Dungeon);
	FGridDungeonLevelEntry Entry;
	Entry.LevelId = TEXT("MapVisible");
	Entry.LevelAsset = Level;
	Entry.LogicalPosition = FIntVector(-1, 2, 7);
	Entry.bEnabled = true;
	Dungeon->Levels.Add(Entry);

	Runtime->DungeonAsset = Dungeon;
	Runtime->LevelAsset = Level;
	Runtime->CurrentDungeonLevelId = Entry.LevelId;

	FGridLevelRuntimeState& LevelState = Runtime->DungeonRuntimeState.LevelStates.Add(Entry.LevelId);
	LevelState.LevelId = Entry.LevelId;
	bool bNewlyExplored = false;
	LevelState.MapExploration.TryMarkExplored(FIntPoint(31, 4), bNewlyExplored);

	Party->LevelRuntimeActor = Runtime;
	Party->CurrentCellX = 31;
	Party->CurrentCellY = 4;
	Party->Facing = EGridEdge::West;

	UGridMapWidget* MapWidget = NewObject<UGridMapWidget>(Party);
	MapWidget->InitializeMapWidget(Party);
	TestTrue(TEXT("Widget builds a renderable floor from runtime authorities"), MapWidget->HasRenderableMap());

	const FGridMapFloorView& View = MapWidget->GetFloorView();
	TestEqual(TEXT("Widget follows the active logical floor"), View.SelectedFloorZ, 7);
	TestEqual(TEXT("Exactly the explored cell is projected"), View.Cells.Num(), 1);
	TestTrue(TEXT("Global cell uses tile stride 32"), View.Cells.Num() == 1 && View.Cells[0].MapCell == FIntPoint(-1, 68));
	TestTrue(TEXT("Party marker is projected"), View.bHasPartyMarker);
	TestTrue(TEXT("Party marker shares the same global cell"), View.PartyMapCell == FIntPoint(-1, 68));
	TestEqual(TEXT("Party facing reaches the renderer read model"), View.PartyFacing, EGridEdge::West);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2168FailClosedTest, "Grimrock.Map.MON21_6_8.NativeRendering.RefreshFailsClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2168FailClosedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridMapWidget* Widget = NewObject<UGridMapWidget>(GetTransientPackage());
	TestFalse(TEXT("Widget without a party/runtime cannot build map data"), Widget->RefreshMap());
	TestFalse(TEXT("Widget remains non-renderable after failed refresh"), Widget->HasRenderableMap());
	TestEqual(TEXT("Failed refresh clears cell projection"), Widget->GetFloorView().Cells.Num(), 0);
	TestFalse(TEXT("Failed refresh clears party marker"), Widget->GetFloorView().bHasPartyMarker);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapMON2168StandaloneHookTest, "Grimrock.Map.MON21_6_8.NativeRendering.StandaloneWindowHook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapMON2168StandaloneHookTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* PartyClass = AGrimrockPartyPawn::StaticClass();
	UClass* MenuClass = UGrimrockMenuWidget::StaticClass();
	if (!TestNotNull(TEXT("Party class exists"), PartyClass) || !TestNotNull(TEXT("Legacy menu class exists"), MenuClass))
	{
		return false;
	}

	const FObjectPropertyBase* MapClassProperty =
		CastField<FObjectPropertyBase>(PartyClass->FindPropertyByName(TEXT("MapWidgetClass")));
	const FObjectPropertyBase* MapInstanceProperty =
		CastField<FObjectPropertyBase>(PartyClass->FindPropertyByName(TEXT("MapWidgetInstance")));

	TestNotNull(TEXT("Standalone map class hook exists on the party"), MapClassProperty);
	TestTrue(TEXT("Standalone map class hook targets UGridMapWidget"),
		MapClassProperty && MapClassProperty->PropertyClass == UGridMapWidget::StaticClass());
	TestNotNull(TEXT("Standalone map instance hook exists on the party"), MapInstanceProperty);
	TestTrue(TEXT("Standalone map instance hook targets UGridMapWidget"),
		MapInstanceProperty && MapInstanceProperty->PropertyClass == UGridMapWidget::StaticClass());

	TestNotNull(TEXT("Party exposes standalone ShowMapWidget"),
		PartyClass->FindFunctionByName(TEXT("ShowMapWidget")));
	TestNotNull(TEXT("Party exposes standalone HideMapWidget"),
		PartyClass->FindFunctionByName(TEXT("HideMapWidget")));
	TestNotNull(TEXT("Party exposes standalone map visibility query"),
		PartyClass->FindFunctionByName(TEXT("IsMapWidgetVisible")));

	TestNull(TEXT("Legacy menu no longer exposes GetMapWidget"),
		MenuClass->FindFunctionByName(TEXT("GetMapWidget")));
	TestNull(TEXT("Legacy menu no longer exposes RefreshMap"),
		MenuClass->FindFunctionByName(TEXT("RefreshMap")));
	TestNull(TEXT("Legacy menu no longer binds Page_Map"),
		MenuClass->FindPropertyByName(TEXT("Page_Map")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
