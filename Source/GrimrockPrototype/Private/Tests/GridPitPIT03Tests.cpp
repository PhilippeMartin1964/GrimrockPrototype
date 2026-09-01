#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDungeonAsset.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPitTrapdoorActor.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace GridPIT03
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
				FName(*FString::Printf(TEXT("PIT03TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
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
		Level->Width = 5;
		Level->Height = 5;
		Level->CellSize = 200.0f;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		return Level;
	}

	FGridLevelObjectData MakeControlledPit(int32 X, int32 Y, FName TargetLevelId, bool bInitiallyOpen)
	{
		FGridLevelObjectData Pit;
		Pit.ObjectId = FGuid::NewGuid();
		Pit.Type = EGridLevelObjectType::Pit;
		Pit.ArchetypeId = TEXT("Pit_Stone_01");
		Pit.CellX = X;
		Pit.CellY = Y;
		Pit.Edge = EGridEdge::None;
		Pit.bInitiallyEnabled = true;
		Pit.Behavior.Pit.bInitiallyOpen = bInitiallyOpen;
		Pit.Behavior.Pit.bUseSameCellCoordinates = true;
		Pit.Behavior.Transition.bIsTransition = true;
		Pit.Behavior.Transition.TargetLevelId = TargetLevelId;
		Pit.Behavior.Transition.TargetFacing = EGridEdge::North;
		Pit.Behavior.Transition.bRequireUseAction = false;
		return Pit;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPIT03ControlledStateTest, "Grimrock.Pit.PIT03.ControlledStateAndLinks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPIT03ControlledStateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPIT03;

	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("PIT03 world exists"), TestWorld.World))
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
	const FName UpperId(TEXT("PIT03_Upper"));
	const FName LowerId(TEXT("PIT03_Lower"));

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

	FGridLevelObjectData PitA = MakeControlledPit(2, 2, LowerId, false);
	FGridLevelObjectData PitB = MakeControlledPit(3, 2, LowerId, false);
	FGridLevelObjectData Button;
	Button.ObjectId = FGuid::NewGuid();
	Button.Type = EGridLevelObjectType::Button;
	Button.CellX = 1;
	Button.CellY = 2;
	Button.Edge = EGridEdge::East;
	Button.bInitiallyEnabled = true;

	const FGuid PitAId = PitA.ObjectId;
	const FGuid PitBId = PitB.ObjectId;
	const FGuid ButtonId = Button.ObjectId;
	Upper->Objects = { Button, PitA, PitB };

	FGridObjectLink OpenPitA;
	OpenPitA.SourceObjectId = ButtonId;
	OpenPitA.TargetObjectId = PitAId;
	OpenPitA.SourceEvent = EGridObjectEvent::Activated;
	OpenPitA.Command = EGridObjectCommand::Open;

	FGridObjectLink ChainPitB;
	ChainPitB.SourceObjectId = PitAId;
	ChainPitB.TargetObjectId = PitBId;
	ChainPitB.SourceEvent = EGridObjectEvent::Opened;
	ChainPitB.Command = EGridObjectCommand::Open;
	Upper->Links = { OpenPitA, ChainPitB };

	Runtime->DungeonAsset = Dungeon;
	Runtime->CurrentDungeonLevelId = UpperId;
	Runtime->LevelAsset = Upper;
	Party->SetGridStart(Runtime, 1, 1, EGridEdge::North);

	UGridActivationComponent* Activation = Runtime->FindComponentByClass<UGridActivationComponent>();
	if (!TestNotNull(TEXT("Activation component exists"), Activation))
	{
		return false;
	}
	Activation->Initialize(Runtime);
	Activation->RebuildIndexes();

	TestFalse(TEXT("Pit A starts closed"), Runtime->IsPitOpen(PitAId));
	TestFalse(TEXT("Pit B starts closed"), Runtime->IsPitOpen(PitBId));

	FGridObjectTransitionParams Transition;
	TestFalse(TEXT("Closed Pit A is not a fall-through cell"), Runtime->FindOpenPitAtCell(2, 2, Transition));

	UGridItemDefinitionAsset* StoneDefinition = NewObject<UGridItemDefinitionAsset>(Runtime);
	StoneDefinition->ItemDefinitionId = TEXT("Item_PIT03_Stone");
	StoneDefinition->Weight = 1.5f;

	FGridItemInstance Stone;
	Stone.RuntimeObjectId = FGuid::NewGuid();
	Stone.ItemDefinitionId = StoneDefinition->ItemDefinitionId;
	Stone.Quantity = 1;

	TestTrue(TEXT("World Item can rest on a closed trapdoor"),
		Runtime->TryDropItemInstanceAtCell(Stone, StoneDefinition, 2, 2, EGridEdge::None, FVector(12.0f, 8.0f, 0.0f)));
	TestEqual(TEXT("Closed trapdoor keeps item weight on upper level"), Runtime->GetWorldItemWeightAtCell(2, 2, false), StoneDefinition->Weight);

	TestTrue(TEXT("Button Activated -> Pit A Open executes"), Runtime->ExecuteLinksFromRuntimeObject(ButtonId, EGridObjectEvent::Activated));
	TestTrue(TEXT("Pit A is open after command"), Runtime->IsPitOpen(PitAId));
	TestTrue(TEXT("Pit A Opened event chained to Pit B Open"), Runtime->IsPitOpen(PitBId));
	TestTrue(TEXT("Open Pit A is now a fall-through cell"), Runtime->FindOpenPitAtCell(2, 2, Transition));
	TestEqual(TEXT("Opening under a World Item removes its upper-level weight"), Runtime->GetWorldItemWeightAtCell(2, 2, false), 0.0f);

	const FGridLevelRuntimeState* LowerState = Runtime->DungeonRuntimeState.LevelStates.Find(LowerId);
	if (!TestNotNull(TEXT("Lower level received falling item state"), LowerState))
	{
		return false;
	}
	TestTrue(TEXT("Opening the trapdoor routed the resting item through PIT02"), LowerState->PendingInboundItems.Contains(Stone.RuntimeObjectId));

	const FGridLevelRuntimeState* UpperState = Runtime->DungeonRuntimeState.LevelStates.Find(UpperId);
	if (!TestNotNull(TEXT("Upper runtime state exists"), UpperState))
	{
		return false;
	}
	const FGridRuntimePitState* PitAState = UpperState->Pits.Find(PitAId);
	TestTrue(TEXT("Pit A runtime state persists Open"), PitAState && PitAState->bIsOpen);

	TestTrue(TEXT("Close command API succeeds"), Runtime->SetPitOpen(PitAId, false, false));
	TestFalse(TEXT("Pit A is closed"), Runtime->IsPitOpen(PitAId));
	TestTrue(TEXT("Toggle reopens Pit A"), Runtime->TogglePit(PitAId, false));
	TestTrue(TEXT("Pit A is open after Toggle"), Runtime->IsPitOpen(PitAId));

	TestTrue(TEXT("Travel to lower level succeeds"), Runtime->TravelToDungeonLevel(LowerId, 1, 1, EGridEdge::North, Party));
	TestEqual(TEXT("Fallen item materialized normally below"), Runtime->GetWorldItemWeightAtCell(2, 2, false), StoneDefinition->Weight);
	TestTrue(TEXT("Travel back to upper level succeeds"), Runtime->TravelToDungeonLevel(UpperId, 1, 1, EGridEdge::North, Party));
	TestTrue(TEXT("Pit A open state survives level round-trip"), Runtime->IsPitOpen(PitAId));
	TestTrue(TEXT("Pit B open state survives level round-trip"), Runtime->IsPitOpen(PitBId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPIT03PresentationActorTest, "Grimrock.Pit.PIT03.PresentationActorState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPIT03PresentationActorTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace GridPIT03;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Presentation test world exists"), TestWorld.World))
	{
		return false;
	}

	AGridPitTrapdoorActor* Actor = TestWorld.World->SpawnActor<AGridPitTrapdoorActor>();
	if (!TestNotNull(TEXT("Pit trapdoor actor exists"), Actor))
	{
		return false;
	}

	Actor->SetPitOpenVisualState(false, false);
	TestFalse(TEXT("Presentation actor accepts Closed state"), Actor->IsPitOpenVisualState());
	Actor->SetPitOpenVisualState(true, false);
	TestTrue(TEXT("Presentation actor accepts Open state"), Actor->IsPitOpenVisualState());
	return true;
}

#endif
