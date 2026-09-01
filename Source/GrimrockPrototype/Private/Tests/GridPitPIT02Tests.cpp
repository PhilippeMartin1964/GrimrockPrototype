#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDungeonAsset.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace GridPIT02
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
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("PIT02TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
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

	UGridLevelAsset* MakeFloor(UObject* Outer)
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Outer);
		Level->Width = 4;
		Level->Height = 4;
		Level->CellSize = 200.0f;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		return Level;
	}

	FGridLevelObjectData MakePit(int32 X, int32 Y, FName TargetLevelId)
	{
		FGridLevelObjectData Pit;
		Pit.ObjectId = FGuid::NewGuid();
		Pit.Type = EGridLevelObjectType::Pit;
		Pit.ArchetypeId = TEXT("Pit_Stone_01");
		Pit.CellX = X;
		Pit.CellY = Y;
		Pit.Edge = EGridEdge::None;
		Pit.bInitiallyEnabled = true;
		Pit.Behavior.Pit.bInitiallyOpen = true;
		Pit.Behavior.Pit.bUseSameCellCoordinates = true;
		Pit.Behavior.Transition.bIsTransition = true;
		Pit.Behavior.Transition.TargetLevelId = TargetLevelId;
		Pit.Behavior.Transition.TargetFacing = EGridEdge::North;
		Pit.Behavior.Transition.bRequireUseAction = false;
		return Pit;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPIT02WorldItemsTest, "Grimrock.Pit.PIT02.WorldItemsFallingThroughPit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPIT02WorldItemsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPIT02;

	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("PIT02 world exists"), TestWorld.World))
	{
		return false;
	}

	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
	if (!TestNotNull(TEXT("Runtime exists"), Runtime) || !TestNotNull(TEXT("Party exists"), Party))
	{
		return false;
	}

	UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(Runtime);
	UGridLevelAsset* Upper = MakeFloor(Dungeon);
	UGridLevelAsset* Lower = MakeFloor(Dungeon);
	const FName UpperId(TEXT("PIT02_Upper"));
	const FName LowerId(TEXT("PIT02_Lower"));

	FGridDungeonLevelEntry UpperEntry;
	UpperEntry.LevelId = UpperId;
	UpperEntry.DisplayName = FText::FromString(TEXT("Upper"));
	UpperEntry.LevelAsset = Upper;
	UpperEntry.LogicalPosition = FIntVector(0, 0, 1);
	UpperEntry.bEnabled = true;

	FGridDungeonLevelEntry LowerEntry;
	LowerEntry.LevelId = LowerId;
	LowerEntry.DisplayName = FText::FromString(TEXT("Lower"));
	LowerEntry.LevelAsset = Lower;
	LowerEntry.LogicalPosition = FIntVector(0, 0, 0);
	LowerEntry.bEnabled = true;

	Dungeon->DefaultLevelId = UpperId;
	Dungeon->Levels = { UpperEntry, LowerEntry };
	Upper->Objects.Add(MakePit(2, 2, LowerId));
	Lower->GetCellMutable(2, 2).CellType = EGridCellType::Empty;
	Lower->GetCellMutable(2, 2).bBlocksOccupancy = false;

	Runtime->DungeonAsset = Dungeon;
	Runtime->CurrentDungeonLevelId = UpperId;
	Runtime->LevelAsset = Upper;
	Party->SetGridStart(Runtime, 1, 2, EGridEdge::East);

	UGridItemDefinitionAsset* StoneDefinition = NewObject<UGridItemDefinitionAsset>(Runtime);
	StoneDefinition->ItemDefinitionId = TEXT("Item_PIT02_Stone");
	StoneDefinition->Weight = 1.25f;

	UGridObjectArchetypeAsset* StoneArchetype = NewObject<UGridObjectArchetypeAsset>(Runtime);
	StoneArchetype->ArchetypeId = TEXT("Item_PIT02_Stone");
	StoneArchetype->SupportedType = EGridLevelObjectType::Item;
	StoneArchetype->DefaultBehavior.Item.ItemDefinitionAsset = StoneDefinition;
	Runtime->ObjectArchetypes.Add(StoneArchetype);

	FGridItemInstance Stone;
	Stone.RuntimeObjectId = FGuid::NewGuid();
	Stone.ItemDefinitionId = StoneDefinition->ItemDefinitionId;
	Stone.Quantity = 1;
	Stone.bLightsEnabled = false;

	const FVector SourceLocalOffset(24.0f, -16.0f, 0.0f);
	TestTrue(TEXT("Dropping a normal World Item on an open pit routes it away from the source level"),
		Runtime->TryDropItemInstanceAtCell(Stone, StoneDefinition, 2, 2, EGridEdge::None, SourceLocalOffset));
	TestEqual(TEXT("No World Item remains on the upper pit cell"), Runtime->GetWorldItemWeightAtCell(2, 2, false), 0.0f);

	const FGridLevelRuntimeState* LowerState = Runtime->DungeonRuntimeState.LevelStates.Find(LowerId);
	if (!TestNotNull(TEXT("Lower level received a runtime state container"), LowerState))
	{
		return false;
	}
	TestFalse(TEXT("Queuing a falling item does not mark the lower level as visited"), LowerState->bHasBeenVisited);
	TestTrue(TEXT("The falling item is queued by its persistent RuntimeObjectId"), LowerState->PendingInboundItems.Contains(Stone.RuntimeObjectId));

	const FGridPendingWorldItemState* Pending = LowerState->PendingInboundItems.Find(Stone.RuntimeObjectId);
	if (!TestNotNull(TEXT("Pending item data exists"), Pending))
	{
		return false;
	}
	TestEqual(TEXT("Pending item resolves nearest usable destination X"), Pending->ItemState.CellX, 2);
	TestEqual(TEXT("Pending item resolves nearest usable destination Y"), Pending->ItemState.CellY, 1);
	TestEqual(TEXT("Pending item keeps its definition"), Pending->ItemState.ItemDefinitionId, StoneDefinition->ItemDefinitionId);
	TestTrue(TEXT("Pending item keeps a resolvable definition asset"), Pending->ItemDefinitionAsset.Get() == StoneDefinition);

	TestTrue(TEXT("Party can travel to the lower level"), Runtime->TravelToDungeonLevel(LowerId, 1, 1, EGridEdge::North, Party));
	TestEqual(TEXT("The queued item materializes at the resolved fallback cell on the lower level"),
		Runtime->GetWorldItemWeightAtCell(2, 1, false), StoneDefinition->Weight);

	const FGridLevelRuntimeState* LowerAfterArrival = Runtime->DungeonRuntimeState.LevelStates.Find(LowerId);
	if (!TestNotNull(TEXT("Lower runtime state still exists"), LowerAfterArrival))
	{
		return false;
	}
	TestFalse(TEXT("Pending queue is consumed after materialization"), LowerAfterArrival->PendingInboundItems.Contains(Stone.RuntimeObjectId));

	TestTrue(TEXT("Capturing the active lower level succeeds"), Runtime->CaptureCurrentLevelRuntimeState());
	const FGridLevelRuntimeState* CapturedLower = Runtime->DungeonRuntimeState.LevelStates.Find(LowerId);
	if (!TestNotNull(TEXT("Captured lower state exists"), CapturedLower))
	{
		return false;
	}
	TestTrue(TEXT("The fallen item now uses ordinary World Item persistence"), CapturedLower->Items.Contains(Stone.RuntimeObjectId));
	TestTrue(TEXT("The lower level becomes visited only when actually active/captured"), CapturedLower->bHasBeenVisited);
	return true;
}

#endif
