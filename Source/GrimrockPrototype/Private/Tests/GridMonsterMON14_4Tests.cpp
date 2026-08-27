#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterPatrolSubsystem.h"

namespace
{
	struct FGridMON144TestWorld
	{
		UWorld* World = nullptr;

		FGridMON144TestWorld()
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
				FName(*FString::Printf(TEXT("MON144TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridMON144TestWorld()
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

	struct FGridMON144Fixture
	{
		FGridMON144TestWorld TestWorld;
		AGridLevelRuntimeActor* Runtime = nullptr;
		UGridLevelAsset* Level = nullptr;
		AGrimrockPartyPawn* Party = nullptr;
		UGridTurnManagerComponent* TurnManager = nullptr;
		UGridMonsterPatrolSubsystem* Patrol = nullptr;

		bool Initialize(FIntPoint PartyCell = FIntPoint(1, 4))
		{
			if (!TestWorld.World)
			{
				return false;
			}

			Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
			Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
			if (!Runtime || !Party || !Party->PartyInventoryComponent)
			{
				return false;
			}

			Runtime->bApplyLevelStartOnBeginPlay = false;
			Level = NewObject<UGridLevelAsset>(Runtime);
			Level->Width = 10;
			Level->Height = 10;
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
			Runtime->LevelAsset = Level;

			Party->LevelRuntimeActor = Runtime;
			Party->CurrentCellX = PartyCell.X;
			Party->CurrentCellY = PartyCell.Y;
			Party->SetActorLocation(Runtime->GetCellCenterWorld(PartyCell.X, PartyCell.Y));

			FGridCharacterInventoryState Character;
			Character.CharacterId = FGuid::NewGuid();
			Character.DisplayName = FText::FromString(TEXT("MON14.4 Party"));
			Character.DerivedStats.MaxHealth = 20;
			Character.Resources.CurrentHealth = 20;
			Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { Character };

			TurnManager = NewObject<UGridTurnManagerComponent>(Runtime, TEXT("MON144TurnManager"));
			TurnManager->bAutoInitialize = false;
			Runtime->AddInstanceComponent(TurnManager);
			TurnManager->RegisterComponent();
			if (!TurnManager->InitializeTurnManager(Runtime, Party))
			{
				return false;
			}

			Patrol = TestWorld.World->GetSubsystem<UGridMonsterPatrolSubsystem>();
			if (!Patrol)
			{
				return false;
			}
			Patrol->RegisterRuntime(Runtime);
			return true;
		}

		UGridMonsterDefinitionAsset* MakeDefinition(FName MonsterId, int32 SightRange, int32 HearingRange, bool bSharesAggro, int32 AggroRange)
		{
			UGridMonsterDefinitionAsset* Definition = NewObject<UGridMonsterDefinitionAsset>(Runtime);
			Definition->MonsterId = MonsterId;
			Definition->DisplayName = FText::FromName(MonsterId);
			Definition->CategoryId = TEXT("MON14_4_Test");
			Definition->MaxHealth = 10;
			Definition->ActionPointsPerTurn = 2;
			Definition->SightRangeCells = SightRange;
			Definition->HearingRangeCells = HearingRange;
			Definition->AggroPropagationRange = AggroRange;
			Definition->bSharesAggroWithGroup = bSharesAggro;
			Definition->GridFootprint = FIntPoint(1, 1);
			Definition->MoveDuration = 1.0f;
			Definition->TurnDuration = 1.0f;
			Definition->DeathExpectedDuration = 1.0f;
			return Definition;
		}

		AGridMonsterActor* AddMonster(
			UGridMonsterDefinitionAsset* Definition, FIntPoint Cell, EGridEdge Facing, EGridMonsterState InitialState, FName EncounterGroupId)
		{
			if (!Definition || !Runtime || !Level || !TestWorld.World)
			{
				return nullptr;
			}

			const FGuid StableId = FGuid::NewGuid();
			FGridLevelObjectData Spawn;
			Spawn.ObjectId = StableId;
			Spawn.Type = EGridLevelObjectType::MonsterSpawn;
			Spawn.CellX = Cell.X;
			Spawn.CellY = Cell.Y;
			Spawn.Edge = EGridEdge::None;
			Spawn.InitialFacing = Facing;
			Spawn.InitialMonsterState = InitialState;
			Spawn.MonsterDefinitionAsset = Definition;
			Spawn.MonsterDefinitionId = Definition->MonsterId;
			Spawn.EncounterGroupId = EncounterGroupId;
			Spawn.bInitiallyEnabled = true;
			Level->Objects.Add(Spawn);

			FActorSpawnParameters Params;
			Params.Owner = Runtime;
			AGridMonsterActor* Monster = TestWorld.World->SpawnActor<AGridMonsterActor>(
				AGridMonsterActor::StaticClass(), Runtime->GetCellCenterWorld(Cell.X, Cell.Y), FRotator::ZeroRotator, Params);
			if (!Monster || !Monster->InitializeMonster(Definition, StableId, Cell, Facing, EncounterGroupId))
			{
				return nullptr;
			}

			UGridMonsterMovementComponent* Movement = NewObject<UGridMonsterMovementComponent>(Monster, TEXT("MON144Movement"));
			Movement->bAutoInitialize = false;
			Movement->bInferCellFromActorLocation = false;
			Monster->AddInstanceComponent(Movement);
			Movement->RegisterComponent();

			UGridMonsterBehaviorComponent* Behavior = NewObject<UGridMonsterBehaviorComponent>(Monster, TEXT("MON144Behavior"));
			Behavior->bAutoInitialize = false;
			Monster->AddInstanceComponent(Behavior);
			Behavior->RegisterComponent();

			if (!Movement->InitializeMovement(Runtime) || !Behavior->InitializeBehavior(Runtime, Party))
			{
				return nullptr;
			}
			return Monster;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON144HearingAlarmPropagationTest, "Grimrock.Monsters.MON14.4.HearingAlarmPropagation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON144HearingAlarmPropagationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON144Fixture Fixture;
	TestTrue(TEXT("Fixture initializes"), Fixture.Initialize());
	if (!Fixture.Patrol)
	{
		return false;
	}

	UGridMonsterDefinitionAsset* SourceDefinition = Fixture.MakeDefinition(TEXT("MON14_4_Guard"), 0, 3, true, 2);
	UGridMonsterDefinitionAsset* AllyDefinition = Fixture.MakeDefinition(TEXT("MON14_4_Guard"), 0, 0, false, 0);

	AGridMonsterActor* Source = Fixture.AddMonster(SourceDefinition, FIntPoint(1, 1), EGridEdge::East, EGridMonsterState::Idle, TEXT("Alarm_A"));
	AGridMonsterActor* Ally = Fixture.AddMonster(AllyDefinition, FIntPoint(2, 1), EGridEdge::West, EGridMonsterState::Dormant, TEXT("Alarm_A"));

	TestNotNull(TEXT("Source exists"), Source);
	TestNotNull(TEXT("Ally exists"), Ally);
	if (!Source || !Ally)
	{
		return false;
	}

	TestTrue(TEXT("Source hearing is processed"), Fixture.Patrol->ProcessMonsterNow(Source, TEXT("MON144Hearing")));

	UGridMonsterBehaviorComponent* SourceBehavior = Source->FindComponentByClass<UGridMonsterBehaviorComponent>();
	UGridMonsterBehaviorComponent* AllyBehavior = Ally->FindComponentByClass<UGridMonsterBehaviorComponent>();

	TestTrue(TEXT("Source hears party"), SourceBehavior && SourceBehavior->bCanHearParty);
	TestFalse(TEXT("Source does not see party"), SourceBehavior && SourceBehavior->bCanSeeParty);
	TestEqual(TEXT("Dormant ally wakes to Alert"), Ally->MonsterState, EGridMonsterState::Alert);
	TestTrue(TEXT("Ally receives last known party cell"), AllyBehavior && AllyBehavior->bHasLastKnownPartyCell);
	TestTrue(TEXT("Ally receives correct party cell"), AllyBehavior && AllyBehavior->LastKnownPartyCell == FIntPoint(1, 4));
	TestEqual(
		TEXT("Ally starts investigation"), Fixture.Patrol->GetMonsterActivity(Ally->ResolvePersistenceId()), EGridMonsterExplorationActivity::Investigating);
	TestFalse(TEXT("Alarm alone does not start combat"), Fixture.TurnManager->bCombatActive);

	const int32 RepeatedAlertCount = Fixture.Patrol->HandleExplorationAlert(Source, FIntPoint(1, 4), TEXT("MON144RepeatSameCell"));
	TestEqual(TEXT("Same-cell repeated alarm does not restart investigation"), RepeatedAlertCount, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON144AlarmFilteringTest, "Grimrock.Monsters.MON14.4.AlarmFiltering", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON144AlarmFilteringTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON144Fixture Fixture;
	TestTrue(TEXT("Fixture initializes"), Fixture.Initialize());
	if (!Fixture.Patrol)
	{
		return false;
	}

	UGridMonsterDefinitionAsset* SourceDefinition = Fixture.MakeDefinition(TEXT("MON14_4_FilterGuard"), 0, 3, true, 2);
	UGridMonsterDefinitionAsset* SameTypeDefinition = Fixture.MakeDefinition(TEXT("MON14_4_FilterGuard"), 0, 0, true, 2);
	UGridMonsterDefinitionAsset* OtherTypeDefinition = Fixture.MakeDefinition(TEXT("MON14_4_OtherMonster"), 0, 0, true, 2);

	AGridMonsterActor* Source = Fixture.AddMonster(SourceDefinition, FIntPoint(1, 1), EGridEdge::East, EGridMonsterState::Idle, TEXT("Alarm_Filter"));
	AGridMonsterActor* ValidAlly = Fixture.AddMonster(SameTypeDefinition, FIntPoint(2, 1), EGridEdge::West, EGridMonsterState::Dormant, TEXT("Alarm_Filter"));
	AGridMonsterActor* OutOfRange = Fixture.AddMonster(SameTypeDefinition, FIntPoint(5, 1), EGridEdge::West, EGridMonsterState::Dormant, TEXT("Alarm_Filter"));
	AGridMonsterActor* OtherGroup = Fixture.AddMonster(SameTypeDefinition, FIntPoint(1, 2), EGridEdge::South, EGridMonsterState::Dormant, TEXT("Alarm_Other"));
	AGridMonsterActor* OtherType = Fixture.AddMonster(OtherTypeDefinition, FIntPoint(2, 2), EGridEdge::South, EGridMonsterState::Dormant, TEXT("Alarm_Filter"));

	if (!Source || !ValidAlly || !OutOfRange || !OtherGroup || !OtherType)
	{
		AddError(TEXT("MON14.4 filter fixture failed to create all monsters."));
		return false;
	}

	Fixture.Patrol->ProcessMonsterNow(Source, TEXT("MON144Filtering"));

	TestEqual(TEXT("Eligible same-group ally is alerted"), ValidAlly->MonsterState, EGridMonsterState::Alert);
	TestEqual(TEXT("Out-of-range ally remains dormant"), OutOfRange->MonsterState, EGridMonsterState::Dormant);
	TestEqual(TEXT("Other encounter group remains dormant"), OtherGroup->MonsterState, EGridMonsterState::Dormant);
	TestEqual(TEXT("Different monster type remains dormant"), OtherType->MonsterState, EGridMonsterState::Dormant);
	TestFalse(TEXT("Filtering scenario does not start combat"), Fixture.TurnManager->bCombatActive);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON144SharingDisabledTest, "Grimrock.Monsters.MON14.4.SharingDisabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON144SharingDisabledTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON144Fixture Fixture;
	TestTrue(TEXT("Fixture initializes"), Fixture.Initialize());
	if (!Fixture.Patrol)
	{
		return false;
	}

	UGridMonsterDefinitionAsset* SourceDefinition = Fixture.MakeDefinition(TEXT("MON14_4_SoloGuard"), 0, 3, false, 8);
	UGridMonsterDefinitionAsset* AllyDefinition = Fixture.MakeDefinition(TEXT("MON14_4_SoloGuard"), 0, 0, false, 0);

	AGridMonsterActor* Source = Fixture.AddMonster(SourceDefinition, FIntPoint(1, 1), EGridEdge::East, EGridMonsterState::Idle, TEXT("Alarm_Disabled"));
	AGridMonsterActor* Ally = Fixture.AddMonster(AllyDefinition, FIntPoint(2, 1), EGridEdge::West, EGridMonsterState::Dormant, TEXT("Alarm_Disabled"));
	if (!Source || !Ally)
	{
		return false;
	}

	Fixture.Patrol->ProcessMonsterNow(Source, TEXT("MON144SharingDisabled"));

	UGridMonsterBehaviorComponent* AllyBehavior = Ally->FindComponentByClass<UGridMonsterBehaviorComponent>();
	TestEqual(TEXT("Ally remains dormant when sharing is disabled"), Ally->MonsterState, EGridMonsterState::Dormant);
	TestFalse(TEXT("Ally receives no external last-known cell"), AllyBehavior && AllyBehavior->bHasLastKnownPartyCell);
	TestFalse(TEXT("No combat starts"), Fixture.TurnManager->bCombatActive);
	return true;
}

#endif
