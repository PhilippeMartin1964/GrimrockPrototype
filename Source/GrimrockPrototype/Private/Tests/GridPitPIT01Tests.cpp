#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDungeonAsset.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace
{
	struct FGridPIT01TestWorld
	{
		UWorld* World = nullptr;

		FGridPIT01TestWorld()
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
				FName(*FString::Printf(TEXT("PIT01TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridPIT01TestWorld()
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

	UGridLevelAsset* MakePitFloor(UObject* Outer, int32 Width = 4, int32 Height = 4)
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Outer);
		Level->Width = Width;
		Level->Height = Height;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		return Level;
	}

	FGridLevelObjectData MakeStaticPit(int32 X, int32 Y, FName TargetLevelId, bool bOpen = true)
	{
		FGridLevelObjectData Pit;
		Pit.ObjectId = FGuid::NewGuid();
		Pit.Type = EGridLevelObjectType::Pit;
		Pit.CellX = X;
		Pit.CellY = Y;
		Pit.Edge = EGridEdge::None;
		Pit.bInitiallyEnabled = true;
		Pit.ArchetypeId = TEXT("Pit_Stone_01");
		Pit.Behavior.Pit.bInitiallyOpen = bOpen;
		Pit.Behavior.Pit.bUseSameCellCoordinates = true;
		Pit.Behavior.Transition.bIsTransition = true;
		Pit.Behavior.Transition.TargetLevelId = TargetLevelId;
		Pit.Behavior.Transition.TargetCellX = 0;
		Pit.Behavior.Transition.TargetCellY = 0;
		Pit.Behavior.Transition.TargetFacing = EGridEdge::East;
		Pit.Behavior.Transition.bRequireUseAction = false;
		return Pit;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPIT01DataContractTest, "Grimrock.Pit.PIT01.DataContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPIT01DataContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridObjectArchetypeAsset* Archetype = NewObject<UGridObjectArchetypeAsset>();
	Archetype->SupportedType = EGridLevelObjectType::Pit;
	Archetype->PlacementKind = EGridObjectPlacementKind::Floor;
	Archetype->DefaultBehavior.Pit.bInitiallyOpen = true;
	Archetype->DefaultBehavior.Pit.bUseSameCellCoordinates = true;
	Archetype->DefaultBehavior.Transition.bIsTransition = true;
	Archetype->DefaultBehavior.Transition.bRequireUseAction = false;

	TestTrue(TEXT("Pit is a center/floor placed gameplay object"), Archetype->SupportsCenterPlacement());
	TestTrue(TEXT("Pit uses a runtime actor for its visual mesh"), Archetype->RequiresRuntimeActorClass());
	TestTrue(TEXT("PIT01 defaults to open"), Archetype->DefaultBehavior.Pit.bInitiallyOpen);
	TestTrue(TEXT("PIT01 defaults to same-cell destination coordinates"), Archetype->DefaultBehavior.Pit.bUseSameCellCoordinates);
	TestTrue(TEXT("PIT01 is an automatic transition"), !Archetype->DefaultBehavior.Transition.bRequireUseAction);

	UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>();
	UGridLevelAsset* ListedUpper = MakePitFloor(Dungeon);
	UGridLevelAsset* ListedLower = MakePitFloor(Dungeon);

	FGridDungeonLevelEntry ListedUpperEntry;
	ListedUpperEntry.LevelId = TEXT("ListedUpper");
	ListedUpperEntry.LevelAsset = ListedUpper;
	ListedUpperEntry.LogicalPosition = FIntVector::ZeroValue;
	ListedUpperEntry.bEnabled = true;

	FGridDungeonLevelEntry ListedLowerEntry;
	ListedLowerEntry.LevelId = TEXT("ListedLower");
	ListedLowerEntry.LevelAsset = ListedLower;
	ListedLowerEntry.LogicalPosition = FIntVector::ZeroValue;
	ListedLowerEntry.bEnabled = true;

	Dungeon->Levels = { ListedUpperEntry, ListedLowerEntry };
	const FGridDungeonLevelEntry* ListFallback = Dungeon->FindLevelBelow(ListedUpperEntry.LevelId);
	TestNotNull(TEXT("Level-list fallback resolves a lower level when LogicalPosition.Z is not authored"), ListFallback);
	if (ListFallback)
	{
		TestEqual(TEXT("Level-list fallback selects the next enabled dungeon level"), ListFallback->LevelId, ListedLowerEntry.LevelId);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridPIT01FallLifecycleTest, "Grimrock.Pit.PIT01.RuntimeFallLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridPIT01FallLifecycleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridPIT01TestWorld TestWorld;
	if (!TestNotNull(TEXT("PIT01 world exists"), TestWorld.World))
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
	UGridLevelAsset* Upper = MakePitFloor(Dungeon);
	UGridLevelAsset* Lower = MakePitFloor(Dungeon);
	const FName UpperId(TEXT("PIT01_Upper"));
	const FName LowerId(TEXT("PIT01_Lower"));

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

	// Standard Pit authoring: no manual target, no generic transition flag and no arrival facing.
	// Also emulate stale placed data from an earlier prototype revision: stored Type and ObjectId are not trusted
	// when the archetype itself authoritatively identifies a Pit.
	Upper->Objects.Add(MakeStaticPit(2, 2, NAME_None));
	Upper->Objects[0].Type = EGridLevelObjectType::Decoration;
	Upper->Objects[0].ObjectId = FGuid();
	Upper->Objects[0].Behavior.Pit.bInitiallyOpen = false; // no MovingMesh cover => static hole must still be Open
	Upper->Objects[0].Behavior.Transition.bIsTransition = false;
	Upper->Objects[0].Behavior.Transition.TargetFacing = EGridEdge::None;

	UGridObjectArchetypeAsset* PitArchetype = NewObject<UGridObjectArchetypeAsset>(Runtime);
	PitArchetype->ArchetypeId = TEXT("Pit_Stone_01");
	PitArchetype->SupportedType = EGridLevelObjectType::Pit;
	PitArchetype->PlacementKind = EGridObjectPlacementKind::Floor;
	PitArchetype->MovingMesh = nullptr;
	Runtime->ObjectArchetypes.Add(PitArchetype);

	Runtime->DungeonAsset = Dungeon;
	Runtime->CurrentDungeonLevelId = UpperId;
	Runtime->LevelAsset = Upper;
	Party->SetGridStart(Runtime, 1, 2, EGridEdge::East);
	Party->bNativeMovementAudioPlaybackEnabled = false;
	Party->PitFallDuration = 0.10f;

	FGridObjectTransitionParams PitTransition;
	TestTrue(TEXT("Static hole resolves as Open even with stale stored Type/ObjectId and false authored state"), Runtime->FindOpenPitAtCell(2, 2, PitTransition));
	TestEqual(TEXT("Same-cell X overrides authored transition X"), PitTransition.TargetCellX, 2);
	TestEqual(TEXT("Same-cell Y overrides authored transition Y"), PitTransition.TargetCellY, 2);
	TestEqual(TEXT("Pit automatically resolves the lower level"), PitTransition.TargetLevelId, LowerId);

	FGridObjectTransitionParams GenericTransition;
	TestFalse(TEXT("Generic stair transition path ignores the Pit archetype"), Runtime->FindTransitionAtCell(2, 2, false, GenericTransition));

	Lower->Objects.Add(MakeStaticPit(2, 2, UpperId));
	AddExpectedError(TEXT("Pit fall rejected: destination"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("PIT01 rejects chained arrival onto another open pit"), Runtime->TryBeginPitFallAtCell(2, 2, Party));
	Lower->Objects.Reset();

	// Real player path: start next to the Pit, move one grid cell onto it, and let movement completion
	// invoke TryBeginPitFallAtCell automatically.
	TestTrue(TEXT("Party can start a normal move onto the Pit"), Party->TryStartMove(EGridEdge::East));
	Party->UpdateMove(10.0f);
	TestEqual(TEXT("Party logical X reaches Pit cell"), Party->CurrentCellX, 2);
	TestEqual(TEXT("Party logical Y reaches Pit cell"), Party->CurrentCellY, 2);
	TestTrue(TEXT("Walking onto the Pit automatically starts the fall"), Party->IsPitFalling());

	Party->UpdatePitFall(1.0f);
	TestFalse(TEXT("Fall presentation completes"), Party->IsPitFalling());
	TestEqual(TEXT("Runtime switched to lower dungeon level"), Runtime->CurrentDungeonLevelId, LowerId);
	TestTrue(TEXT("Runtime now owns lower LevelAsset"), Runtime->LevelAsset == Lower);
	TestEqual(TEXT("Party landed on same X"), Party->CurrentCellX, 2);
	TestEqual(TEXT("Party landed on same Y"), Party->CurrentCellY, 2);
	TestEqual(TEXT("Pit with no authored arrival facing preserves party facing"), Party->Facing, EGridEdge::East);
	return true;
}

#endif
