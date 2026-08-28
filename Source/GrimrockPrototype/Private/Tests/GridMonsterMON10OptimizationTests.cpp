#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDirectionUtils.h"
#include "Core/GridLevelAsset.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridCombatDiagnostics.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterAudioComponent.h"
#include "Runtime/Monsters/GridMonsterBalanceTypes.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterIdleVariationComponent.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterVFXComponent.h"
#include "UObject/UnrealType.h"

namespace
{
	struct FGridMON10OptimizationTestWorld
	{
		UWorld* World = nullptr;

		FGridMON10OptimizationTestWorld()
		{
			const UWorld::InitializationValues InitializationValues = UWorld::InitializationValues()
																		  .AllowAudioPlayback(false)
																		  .RequiresHitProxies(false)
																		  .CreatePhysicsScene(false)
																		  .CreateNavigation(false)
																		  .CreateAISystem(false)
																		  .ShouldSimulatePhysics(false)
																		  .SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				FName(*FString::Printf(TEXT("MON10OptimizationTestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true,
				ERHIFeatureLevel::Num, &InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridMON10OptimizationTestWorld()
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

	void ConfigureOptimizationFloor(AGridLevelRuntimeActor* Runtime, FName LevelId = TEXT("MON10OptimizationLevel"))
	{
		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
		Level->Width = 6;
		Level->Height = 6;
		Level->StartCellX = 5;
		Level->StartCellY = 5;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		Runtime->LevelAsset = Level;
		Runtime->CurrentDungeonLevelId = LevelId;
	}

	UGridMonsterDefinitionAsset* MakeOptimizationDefinition(UObject* Outer)
	{
		UGridMonsterDefinitionAsset* Definition = NewObject<UGridMonsterDefinitionAsset>(Outer);
		Definition->MonsterId = TEXT("MON10_OptimizationRat");
		Definition->DisplayName = FText::FromString(TEXT("Rat optimisation"));
		Definition->CategoryId = TEXT("Vermin");
		Definition->DangerLevel = 2;
		Definition->MaxHealth = 12;
		Definition->PhysicalArmor = 1;
		Definition->MagicalArmor = 2;
		Definition->Initiative = 11;
		Definition->Accuracy = 4;
		Definition->Evasion = 3;
		Definition->ActionPointsPerTurn = 2;
		Definition->SightRangeCells = 8;
		Definition->HearingRangeCells = 4;
		Definition->ExperienceReward = 15;

		FGridMonsterAttackDefinition Bite;
		Bite.AttackId = TEXT("Attack_Bite");
		Bite.MinDamage = 1;
		Bite.MaxDamage = 4;
		Bite.DamageBonus = 1;
		Bite.AccuracyBonus = 2;
		Bite.ActionPointCost = 1;
		Definition->Attacks.Add(Bite);
		return Definition;
	}

	AGrimrockPartyPawn* MakeOptimizationParty(UWorld* World, AGridLevelRuntimeActor* Runtime)
	{
		AGrimrockPartyPawn* Party = World->SpawnActor<AGrimrockPartyPawn>();
		if (!Party || !Party->PartyInventoryComponent)
		{
			return Party;
		}

		Party->LevelRuntimeActor = Runtime;
		Party->CurrentCellX = 4;
		Party->CurrentCellY = 4;
		Party->Facing = EGridEdge::North;
		Party->MoveDuration = 0.0f;
		Party->TurnDuration = 0.0f;
		Party->InputBufferMaxAge = 0.0f;
		Party->SetActorLocation(Runtime->GetCellCenterWorld(Party->CurrentCellX, Party->CurrentCellY, Party->EyeHeight));
		Party->SetActorRotation(FRotator(0.0f, GridDirectionUtils::ToYaw(Party->Facing), 0.0f));

		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid(9, 9, 9, 9);
		Character.DisplayName = FText::FromString(TEXT("Elias"));
		Character.DerivedStats.MaxHealth = 30;
		Character.Resources.CurrentHealth = 30;
		Character.DerivedStats.Evasion = 0;
		Character.DerivedStats.Initiative = 100;
		Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { Character };
		return Party;
	}

	AGridMonsterActor* MakeOptimizationMonster(
		UWorld* World, AGridLevelRuntimeActor* Runtime, UGridMonsterDefinitionAsset* Definition, const FGuid& PersistenceId)
	{
		AGridMonsterActor* Monster = World->SpawnActor<AGridMonsterActor>();
		if (!Monster || !Monster->InitializeMonster(Definition, PersistenceId, FIntPoint(1, 1), EGridEdge::North))
		{
			return nullptr;
		}
		Monster->SetActorLocation(Runtime->GetCellCenterWorld(1, 1));

		UGridMonsterMovementComponent* Movement = NewObject<UGridMonsterMovementComponent>(Monster, TEXT("MON10OptimizationMovement"));
		Movement->bAutoInitialize = false;
		Movement->bInferCellFromActorLocation = false;
		Monster->AddInstanceComponent(Movement);
		Movement->RegisterComponent();

		UGridMonsterBehaviorComponent* Behavior = NewObject<UGridMonsterBehaviorComponent>(Monster, TEXT("MON10OptimizationBehavior"));
		Behavior->bAutoInitialize = false;
		Monster->AddInstanceComponent(Behavior);
		Behavior->RegisterComponent();
		return Monster;
	}

	bool EqualAttackResults(const FGridAttackResult& Left, const FGridAttackResult& Right)
	{
		return Left.bHit == Right.bHit && Left.bCriticalHit == Right.bCriticalHit && Left.NaturalAttackRoll == Right.NaturalAttackRoll &&
			Left.AttackRoll == Right.AttackRoll && Left.RawDamage == Right.RawDamage && Left.HealthDamage == Right.HealthDamage &&
			Left.TargetHealthAfter == Right.TargetHealthAfter;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10OptimizationEncounterSeedDeterministicTest,
	"Grimrock.Monsters.MON10.Optimization.EncounterSeedDeterministicAndOrderIndependent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10OptimizationEncounterSeedDeterministicTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGuid First(1, 2, 3, 4);
	const FGuid Second(5, 6, 7, 8);
	const int32 Forward = FGridEncounterSeedBuilder::BuildEncounterSeed(1337, TEXT("Dungeon_A"), { First, Second });
	const int32 Reverse = FGridEncounterSeedBuilder::BuildEncounterSeed(1337, TEXT("Dungeon_A"), { Second, First });
	const int32 WithDuplicatesAndInvalid = FGridEncounterSeedBuilder::BuildEncounterSeed(1337, TEXT("Dungeon_A"), { FGuid(), First, Second, First, FGuid() });

	TestEqual(TEXT("Equal inputs produce an equal seed"), Forward, Reverse);
	TestEqual(TEXT("Order does not influence the seed"), Forward, WithDuplicatesAndInvalid);

	FRandomStream UnrelatedStream(9876);
	const int32 StreamState = UnrelatedStream.GetCurrentSeed();
	FGridEncounterSeedBuilder::BuildEncounterSeed(1337, TEXT("Dungeon_A"), { First, Second });
	TestEqual(TEXT("The builder consumes no random stream"), UnrelatedStream.GetCurrentSeed(), StreamState);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10OptimizationEncounterSeedDistinguishesTest,
	"Grimrock.Monsters.MON10.Optimization.EncounterSeedDistinguishesEncounters", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10OptimizationEncounterSeedDistinguishesTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGuid First(10, 20, 30, 40);
	const FGuid Second(11, 21, 31, 41);
	const int32 Baseline = FGridEncounterSeedBuilder::BuildEncounterSeed(1337, TEXT("Dungeon_A"), { First });
	TestNotEqual(TEXT("Base seed distinguishes encounters"), Baseline, FGridEncounterSeedBuilder::BuildEncounterSeed(1338, TEXT("Dungeon_A"), { First }));
	TestNotEqual(TEXT("Dungeon level distinguishes encounters"), Baseline, FGridEncounterSeedBuilder::BuildEncounterSeed(1337, TEXT("Dungeon_B"), { First }));
	TestNotEqual(
		TEXT("Participants distinguish encounters"), Baseline, FGridEncounterSeedBuilder::BuildEncounterSeed(1337, TEXT("Dungeon_A"), { First, Second }));
	TestEqual(TEXT("Reconstruction remains stable"), Baseline, FGridEncounterSeedBuilder::BuildEncounterSeed(1337, TEXT("Dungeon_A"), { First }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10OptimizationEncounterSeedLifecycleTest, "Grimrock.Monsters.MON10.Optimization.EncounterSeedLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10OptimizationEncounterSeedLifecycleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10OptimizationTestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	ConfigureOptimizationFloor(Runtime);
	AGrimrockPartyPawn* Party = MakeOptimizationParty(TestWorld.World, Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeOptimizationDefinition(Runtime);
	const FGuid ParticipantId(100, 200, 300, 400);
	AGridMonsterActor* Monster = MakeOptimizationMonster(TestWorld.World, Runtime, Definition, ParticipantId);
	UGridTurnManagerComponent* TurnManager = NewObject<UGridTurnManagerComponent>(Runtime);
	if (!Runtime || !Party || !Monster || !TurnManager || !TurnManager->InitializeTurnManager(Runtime, Party))
	{
		return false;
	}
	TurnManager->CombatStartSafetyPadding = 0.0f;

	const int32 SeedBeforeRejectedStart = TurnManager->ActiveEncounterRandomSeed;
	const int32 StreamBeforeRejectedStart = TurnManager->CombatRandomStream.GetCurrentSeed();
	TestFalse(TEXT("An empty encounter is rejected"), TurnManager->StartCombatInternal({}));
	TestEqual(TEXT("Rejected start preserves the active seed"), TurnManager->ActiveEncounterRandomSeed, SeedBeforeRejectedStart);
	TestEqual(TEXT("Rejected start consumes no gameplay RNG"), TurnManager->CombatRandomStream.GetCurrentSeed(), StreamBeforeRejectedStart);

	const int32 ExpectedSeed =
		FGridEncounterSeedBuilder::BuildEncounterSeed(TurnManager->EncounterRandomSeed, Runtime->CurrentDungeonLevelId, { ParticipantId });
	TestTrue(TEXT("The valid encounter starts"), TurnManager->StartCombatInternal({ Monster }));
	TestEqual(TEXT("The active seed is derived before gameplay"), TurnManager->ActiveEncounterRandomSeed, ExpectedSeed);
	TestEqual(TEXT("The gameplay stream is initialized from it"), TurnManager->CombatRandomStream.GetInitialSeed(), ExpectedSeed);
	const uint32 FirstValue = TurnManager->CombatRandomStream.GetUnsignedInt();

	TurnManager->AbortCombat();
	TestTrue(TEXT("The same encounter restarts"), TurnManager->StartCombatInternal({ Monster }));
	TestEqual(TEXT("Restart rebuilds the same active seed"), TurnManager->ActiveEncounterRandomSeed, ExpectedSeed);
	TestEqual(TEXT("Restart reproduces the random sequence"), TurnManager->CombatRandomStream.GetUnsignedInt(), FirstValue);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10OptimizationBalanceSnapshotTest, "Grimrock.Monsters.MON10.Optimization.MonsterBalanceSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10OptimizationBalanceSnapshotTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridMonsterDefinitionAsset* Definition = MakeOptimizationDefinition(GetTransientPackage());
	FGridMonsterAttackDefinition Claw;
	Claw.AttackId = TEXT("Attack_Claw");
	Claw.MinDamage = 4;
	Claw.MaxDamage = 8;
	Claw.DamageBonus = -1;
	Claw.ActionPointCost = 2;
	Definition->Attacks.Add(Claw);

	FGridMonsterBalanceSnapshot Snapshot;
	TestTrue(TEXT("A definition produces a snapshot"), Definition->BuildBalanceSnapshot(Snapshot));
	TestEqual(TEXT("Monster id is copied"), Snapshot.MonsterId, Definition->MonsterId);
	TestEqual(TEXT("Health is copied"), Snapshot.MaxHealth, 12);
	TestEqual(TEXT("Armor is copied"), Snapshot.PhysicalArmor, 1);
	TestEqual(TEXT("Attack count is copied"), Snapshot.AttackCount, 2);
	TestEqual(TEXT("Damage bonus is included in the minimum"), Snapshot.MinimumBaseDamage, 2);
	TestEqual(TEXT("Damage bonus is included in the maximum"), Snapshot.MaximumBaseDamage, 7);
	TestEqual(TEXT("Raw midpoint average is correct"), Snapshot.AverageBaseDamage, 4.25f);
	TestEqual(TEXT("Minimum AP cost is correct"), Snapshot.MinimumAttackActionPointCost, 1);
	TestEqual(TEXT("Maximum AP cost is correct"), Snapshot.MaximumAttackActionPointCost, 2);

	Definition->Attacks.Reset();
	TestTrue(TEXT("Zero attacks is supported"), Definition->BuildBalanceSnapshot(Snapshot));
	TestEqual(TEXT("Zero attacks has zero average"), Snapshot.AverageBaseDamage, 0.0f);
	TestFalse(TEXT("A null definition is rejected"), FGridMonsterBalanceAnalyzer::BuildSnapshot(nullptr, Snapshot));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10OptimizationMetricsLifecycleTest, "Grimrock.Monsters.MON10.Optimization.CombatRuntimeMetricsLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10OptimizationMetricsLifecycleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10OptimizationTestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	ConfigureOptimizationFloor(Runtime);
	AGrimrockPartyPawn* Party = MakeOptimizationParty(TestWorld.World, Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeOptimizationDefinition(Runtime);
	AGridMonsterActor* Monster = MakeOptimizationMonster(TestWorld.World, Runtime, Definition, FGuid(101, 201, 301, 401));
	UGridTurnManagerComponent* TurnManager = NewObject<UGridTurnManagerComponent>(Runtime);
	if (!Runtime || !Party || !Monster || !TurnManager || !TurnManager->InitializeTurnManager(Runtime, Party))
	{
		return false;
	}

	TurnManager->bCollectRuntimeMetrics = true;
	TurnManager->CombatStartSafetyPadding = 0.0f;
	TestFalse(TEXT("Rejected starts are reported separately"), TurnManager->StartCombatInternal({}));
	TestTrue(TEXT("A valid encounter starts"), TurnManager->StartCombatInternal({ Monster }));
	TestEqual(TEXT("Successful start count"), TurnManager->RuntimeMetrics.SuccessfulCombatStarts, 1);
	TestEqual(TEXT("Rejected start count"), TurnManager->RuntimeMetrics.RejectedCombatStarts, 1);
	TestEqual(TEXT("Participant peak"), TurnManager->RuntimeMetrics.PeakCombatMonsterCount, 1);
	TestTrue(TEXT("The first round is counted"), TurnManager->RuntimeMetrics.RoundsStarted >= 1);

	TestTrue(TEXT("The active player turn can end"), TurnManager->EndActivePlayerTurn());
	TestTrue(TEXT("Monster turns are counted"), TurnManager->RuntimeMetrics.MonsterTurnsStarted >= 1);
	TestTrue(TEXT("Actions started are counted"), TurnManager->RuntimeMetrics.ActionsStarted >= 1);
	if (TurnManager->bHasActiveAction)
	{
		TurnManager->CompleteActiveAction(false);
	}
	TestTrue(TEXT("Actions completed are counted"), TurnManager->RuntimeMetrics.ActionsCompleted >= 1);
	TestTrue(TEXT("Planned action peak is tracked"), TurnManager->RuntimeMetrics.PeakPendingActionCount >= 1);

	TurnManager->CurrentMonster = Monster;
	TurnManager->CurrentCombatComponent = Monster->CombatComponent;
	Monster->CombatComponent->InitializeCombat(Party);
	TurnManager->ActiveAttackDefinition = Definition->Attacks[0];
	TurnManager->ActiveAction = FGridCombatAction();
	TurnManager->ActiveAction.Type = EGridCombatActionType::MeleeAttack;
	TurnManager->ActiveAction.AttackId = Definition->Attacks[0].AttackId;
	TurnManager->ActiveAction.TargetCharacterIndex = 0;
	TurnManager->bHasActiveAction = true;
	TurnManager->bActiveAttackImpactCommitted = false;
	TurnManager->CombatRandomStream.Initialize(TurnManager->ActiveEncounterRandomSeed);
	TurnManager->CommitActiveAttackImpact();
	TestEqual(TEXT("Resolved attacks are counted"), TurnManager->RuntimeMetrics.AttacksResolved, 1);

	TurnManager->ResetRuntimeMetrics();
	const FGridCombatRuntimeMetrics Reset = TurnManager->GetRuntimeMetrics();
	TestEqual(TEXT("Reset clears successful starts"), Reset.SuccessfulCombatStarts, 0);
	TestEqual(TEXT("Reset clears actions"), Reset.ActionsStarted, 0);
	TestEqual(TEXT("Reset clears timings"), Reset.MaximumTurnPlanningMilliseconds, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10OptimizationMetricsNoGameplayInfluenceTest,
	"Grimrock.Monsters.MON10.Optimization.CombatRuntimeMetricsNoGameplayInfluence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10OptimizationMetricsNoGameplayInfluenceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const int32 ActiveSeed = FGridEncounterSeedBuilder::BuildEncounterSeed(1337, TEXT("Dungeon_NoInfluence"), { FGuid(7, 8, 9, 10) });
	FRandomStream MetricsDisabledStream(ActiveSeed);
	FRandomStream MetricsEnabledStream(ActiveSeed);

	const int32 TargetWithoutMetrics = MetricsDisabledStream.RandRange(0, 3);
	FGridCombatRuntimeMetrics Metrics;
	++Metrics.SuccessfulCombatStarts;
	++Metrics.RoundsStarted;
	++Metrics.MonsterTurnsStarted;
	const int32 TargetWithMetrics = MetricsEnabledStream.RandRange(0, 3);
	TestEqual(TEXT("Target selection is unchanged"), TargetWithMetrics, TargetWithoutMetrics);

	FGridAttackSourceStats Source;
	Source.Accuracy = 5;
	FGridAttackTargetStats Target;
	Target.CurrentHealth = 20;
	Target.Evasion = 2;
	FGridAttackDefinition Attack;
	Attack.MinDamage = 2;
	Attack.MaxDamage = 6;
	const FGridAttackResult WithoutMetrics = FGridCombatResolver::ResolveAttack(Source, Target, Attack, MetricsDisabledStream);
	++Metrics.ActionsStarted;
	++Metrics.AttacksResolved;
	const FGridAttackResult WithMetrics = FGridCombatResolver::ResolveAttack(Source, Target, Attack, MetricsEnabledStream);
	TestTrue(TEXT("Rolls and damage are unchanged"), EqualAttackResults(WithoutMetrics, WithMetrics));
	TestEqual(TEXT("Active seed is unchanged"), MetricsEnabledStream.GetInitialSeed(), MetricsDisabledStream.GetInitialSeed());

	FGridTurnPhaseStateMachine DisabledPhases;
	FGridTurnPhaseStateMachine EnabledPhases;
	TestEqual(TEXT("Start phase result is identical"), EnabledPhases.StartCombat(), DisabledPhases.StartCombat());
	++Metrics.ActionsCompleted;
	TestEqual(TEXT("Round phase result is identical"), EnabledPhases.BeginRound(), DisabledPhases.BeginRound());
	TestEqual(TEXT("Phases remain identical"), EnabledPhases.GetPhase(), DisabledPhases.GetPhase());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10OptimizationInvariantsTest, "Grimrock.Monsters.MON10.Optimization.OptimizationInvariants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10OptimizationInvariantsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestFalse(TEXT("Monster actor never ticks"), GetDefault<AGridMonsterActor>()->PrimaryActorTick.bCanEverTick);
	TestFalse(TEXT("Audio never ticks"), NewObject<UGridMonsterAudioComponent>()->PrimaryComponentTick.bCanEverTick);
	TestFalse(TEXT("VFX never ticks"), NewObject<UGridMonsterVFXComponent>()->PrimaryComponentTick.bCanEverTick);
	TestFalse(TEXT("Idle variations never tick"), NewObject<UGridMonsterIdleVariationComponent>()->PrimaryComponentTick.bCanEverTick);

	for (TFieldIterator<FProperty> Property(FGridCombatRuntimeMetrics::StaticStruct()); Property; ++Property)
	{
		TestFalse(*FString::Printf(TEXT("%s is not SaveGame data"), *Property->GetName()), Property->HasAnyPropertyFlags(CPF_SaveGame));
	}

	const FProperty* ActiveSeedProperty = FindFProperty<FProperty>(UGridTurnManagerComponent::StaticClass(), TEXT("ActiveEncounterRandomSeed"));
	TestNotNull(TEXT("Active encounter seed is reflected"), ActiveSeedProperty);
	if (ActiveSeedProperty)
	{
		TestTrue(TEXT("Active encounter seed is transient"), ActiveSeedProperty->HasAnyPropertyFlags(CPF_Transient));
		TestFalse(TEXT("Active encounter seed is not saved"), ActiveSeedProperty->HasAnyPropertyFlags(CPF_SaveGame));
	}
	TestNotNull(
		TEXT("Historical base seed remains reflected"), FindFProperty<FProperty>(UGridTurnManagerComponent::StaticClass(), TEXT("EncounterRandomSeed")));
	TestEqual(TEXT("Historical base seed default remains 1337"), GetDefault<UGridTurnManagerComponent>()->EncounterRandomSeed, 1337);
	TestFalse(TEXT("Phase logs are silent by default"), GetDefault<UGridTurnManagerComponent>()->bLogPhaseChanges);
	TestNull(TEXT("Legacy AttackSound is removed"), FindFProperty<FProperty>(FGridMonsterAttackDefinition::StaticStruct(), TEXT("AttackSound")));
	TestNull(TEXT("Legacy ImpactVFX is removed"), FindFProperty<FProperty>(FGridMonsterAttackDefinition::StaticStruct(), TEXT("ImpactVFX")));
	TestNull(TEXT("MON9 monster state has no metrics"), FindFProperty<FProperty>(FGridRuntimeMonsterState::StaticStruct(), TEXT("RuntimeMetrics")));
	TestNull(TEXT("MON9 level state has no active seed"), FindFProperty<FProperty>(FGridLevelRuntimeState::StaticStruct(), TEXT("ActiveEncounterRandomSeed")));
	return true;
}

#endif
