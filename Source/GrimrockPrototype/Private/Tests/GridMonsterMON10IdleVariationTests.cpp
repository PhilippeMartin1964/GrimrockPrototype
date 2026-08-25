#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Animation/AnimSequence.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterIdleVariationComponent.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "UObject/UnrealType.h"

namespace
{
	struct FGridMON10IdleTestWorld
	{
		UWorld* World = nullptr;

		FGridMON10IdleTestWorld()
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
				FName(*FString::Printf(TEXT("MON10IdleTestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&Values);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridMON10IdleTestWorld()
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

	UGridLevelAsset* ConfigureMON10IdleFloor(AGridLevelRuntimeActor* Runtime, int32 Width = 6, int32 Height = 6)
	{
		if (!Runtime)
		{
			return nullptr;
		}

		UGridLevelAsset* Level = NewObject<UGridLevelAsset>(Runtime);
		Level->Width = Width;
		Level->Height = Height;
		Level->EnsureCellCount();
		for (FGridLevelCellData& Cell : Level->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		Runtime->LevelAsset = Level;
		Runtime->CurrentDungeonLevelId = TEXT("MON10IdleLevel");
		return Level;
	}

	FGridMonsterIdleVariationDefinition MakeMON10IdleVariation(UObject* Outer, FName VariationId, FName AnimationName, float ExpectedDuration = 0.25f)
	{
		FGridMonsterIdleVariationDefinition Variation;
		Variation.VariationId = VariationId;
		Variation.Animation = NewObject<UAnimSequence>(Outer, AnimationName);
		Variation.PlayRate = 1.0f;
		Variation.ExpectedDuration = ExpectedDuration;
		return Variation;
	}

	UGridMonsterDefinitionAsset* MakeMON10IdleDefinition(UObject* Outer, FName MonsterId = TEXT("MON10_IdleRat"), int32 VariationCount = 3)
	{
		UGridMonsterDefinitionAsset* Definition = NewObject<UGridMonsterDefinitionAsset>(Outer);
		Definition->MonsterId = MonsterId;
		Definition->DisplayName = FText::FromString(TEXT("Rat Idle MON10"));
		Definition->CategoryId = TEXT("Vermin");
		Definition->MaxHealth = 10;
		Definition->ActionPointsPerTurn = 2;
		Definition->bEnableIdleVariations = true;
		Definition->IdleVariationMinDelay = 0.1f;
		Definition->IdleVariationMaxDelay = 0.1f;
		Definition->IdleVariationSlotName = TEXT("DefaultSlot");

		static const FName VariationIds[] = { TEXT("Idle_Sniff"), TEXT("Idle_Scratch"), TEXT("Idle_LookAround") };
		static const FName AnimationNames[] = { TEXT("MON10IdleSniff"), TEXT("MON10IdleScratch"), TEXT("MON10IdleLookAround") };
		for (int32 Index = 0; Index < FMath::Min(VariationCount, 3); ++Index)
		{
			Definition->IdleVariations.Add(MakeMON10IdleVariation(Definition, VariationIds[Index], AnimationNames[Index]));
		}
		return Definition;
	}

	AGridMonsterActor* SpawnMON10IdleMonster(UWorld* World, AGridLevelRuntimeActor* Runtime, UGridMonsterDefinitionAsset* Definition,
		const FGuid& PersistenceId, FIntPoint Cell, FName ActorName, bool bAddMovement = false)
	{
		if (!World || !Definition)
		{
			return nullptr;
		}

		FActorSpawnParameters Params;
		Params.Name = ActorName;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AGridMonsterActor* Monster = World->SpawnActor<AGridMonsterActor>(AGridMonsterActor::StaticClass(), FTransform::Identity, Params);
		if (!Monster || !Monster->InitializeMonster(Definition, PersistenceId, Cell, EGridEdge::North))
		{
			return nullptr;
		}

		if (Runtime)
		{
			Monster->SetActorLocation(Runtime->GetCellCenterWorld(Cell.X, Cell.Y));
		}
		if (bAddMovement)
		{
			UGridMonsterMovementComponent* Movement = NewObject<UGridMonsterMovementComponent>(Monster, TEXT("MON10IdleTestMovement"));
			Movement->bAutoInitialize = false;
			Movement->bInferCellFromActorLocation = false;
			Monster->AddInstanceComponent(Movement);
			Movement->RegisterComponent();
		}

		Monster->IdleVariationComponent->bNativePlaybackEnabled = false;
		Monster->IdleVariationComponent->InitializeIdleVariations();
		Monster->IdleVariationComponent->RefreshIdleVariationScheduling();
		return Monster;
	}

	FGridRuntimeMonsterState MakeMON10IdleSavedState(const AGridMonsterActor* Monster, EGridMonsterState State, bool bDead)
	{
		FGridRuntimeMonsterState Saved;
		Saved.PersistenceId = Monster->ResolvePersistenceId();
		Saved.SpawnObjectId = Saved.PersistenceId;
		Saved.MonsterDefinitionId = Monster->MonsterDefinition->MonsterId;
		Saved.DungeonLevelId = TEXT("MON10IdleLevel");
		Saved.CellX = Monster->CurrentCell.X;
		Saved.CellY = Monster->CurrentCell.Y;
		Saved.Facing = EGridEdge::North;
		Saved.MonsterState = State;
		Saved.CurrentHealth = bDead ? 0 : 10;
		Saved.bMonsterEnabled = true;
		Saved.bIsDead = bDead;
		return Saved;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10IdleVariationDefinitionValidationTest,
	"Grimrock.Monsters.MON10.IdleVariations.IdleVariationDefinitionValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10IdleVariationDefinitionValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UObject* Outer = GetTransientPackage();
	FGridMonsterIdleVariationDefinition Variation = MakeMON10IdleVariation(Outer, TEXT("Idle_Sniff"), TEXT("MON10ValidationAnimation"));
	TestTrue(TEXT("A complete variation is valid"), Variation.IsValidDefinition());

	Variation.VariationId = NAME_None;
	TestFalse(TEXT("A None id is invalid"), Variation.IsValidDefinition());
	Variation = MakeMON10IdleVariation(Outer, TEXT("Idle_Sniff"), TEXT("MON10ValidationAnimation2"));
	Variation.Animation.Reset();
	TestFalse(TEXT("An explicit variation needs animation"), Variation.IsValidDefinition());
	Variation = MakeMON10IdleVariation(Outer, TEXT("Idle_Sniff"), TEXT("MON10ValidationAnimation3"));
	Variation.PlayRate = 0.0f;
	TestFalse(TEXT("Zero play rate is invalid"), Variation.IsValidDefinition());
	Variation.PlayRate = -1.0f;
	TestFalse(TEXT("Negative play rate is invalid"), Variation.IsValidDefinition());
	Variation = MakeMON10IdleVariation(Outer, TEXT("Idle_Sniff"), TEXT("MON10ValidationAnimation4"));
	Variation.ExpectedDuration = 0.0f;
	TestFalse(TEXT("Zero expected duration is invalid"), Variation.IsValidDefinition());
	Variation.ExpectedDuration = -1.0f;
	TestFalse(TEXT("Negative expected duration is invalid"), Variation.IsValidDefinition());

	UGridMonsterDefinitionAsset* Definition = MakeMON10IdleDefinition(Outer);
	TestTrue(TEXT("The configured definition is valid"), Definition->IsValidDefinition());
	Definition->IdleVariationMinDelay = 0.0f;
	TestFalse(TEXT("A zero minimum delay is invalid"), Definition->IsValidDefinition());
	Definition->IdleVariationMinDelay = 2.0f;
	Definition->IdleVariationMaxDelay = 1.0f;
	TestFalse(TEXT("An inverted delay is invalid"), Definition->IsValidDefinition());
	Definition->IdleVariationMinDelay = 0.1f;
	Definition->IdleVariationMaxDelay = 0.1f;
	Definition->IdleVariationBlendOutTime = -0.1f;
	TestFalse(TEXT("A negative blend is invalid"), Definition->IsValidDefinition());
	Definition->IdleVariationBlendOutTime = 0.15f;
	const FGridMonsterIdleVariationDefinition Duplicate = Definition->IdleVariations[0];
	Definition->IdleVariations.Add(Duplicate);
	TestFalse(TEXT("A duplicate VariationId is invalid"), Definition->IsValidDefinition());

	Definition->IdleVariations.Reset();
	Definition->IdleVariationSlotName = NAME_None;
	TestTrue(TEXT("An enabled empty list is allowed"), Definition->IsValidDefinition());
	Definition->bEnableIdleVariations = false;
	TestTrue(TEXT("A disabled system is allowed"), Definition->IsValidDefinition());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10IdleVariationDeterministicSelectionTest,
	"Grimrock.Monsters.MON10.IdleVariations.IdleVariationDeterministicSelection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10IdleVariationDeterministicSelectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGuid PersistenceId(1, 2, 3, 4);
	const int32 Seed = FGridMonsterIdleVariationSelector::BuildPresentationSeed(PersistenceId, TEXT("MON10_IdleRat"), 7);
	TestEqual(
		TEXT("The same inputs produce the same seed"), FGridMonsterIdleVariationSelector::BuildPresentationSeed(PersistenceId, TEXT("MON10_IdleRat"), 7), Seed);
	const int32 Index = FGridMonsterIdleVariationSelector::SelectVariationIndex(Seed, 4, INDEX_NONE, true);
	TestEqual(TEXT("The same seed produces the same index"), FGridMonsterIdleVariationSelector::SelectVariationIndex(Seed, 4, INDEX_NONE, true), Index);
	TestTrue(TEXT("The index stays in range"), Index >= 0 && Index < 4);

	FRandomStream CombatRandomStream(1337);
	const int32 CombatSeedBefore = CombatRandomStream.GetCurrentSeed();
	for (int32 Occurrence = 1; Occurrence <= 32; ++Occurrence)
	{
		const int32 Selected = FGridMonsterIdleVariationSelector::SelectVariationIndex(
			FGridMonsterIdleVariationSelector::BuildPresentationSeed(PersistenceId, TEXT("MON10_IdleRat"), Occurrence), 5, INDEX_NONE, true);
		TestTrue(TEXT("Every index stays in range"), Selected >= 0 && Selected < 5);
	}
	TestEqual(TEXT("Selection does not consume CombatRandomStream"), CombatRandomStream.GetCurrentSeed(), CombatSeedBefore);

	UGridTurnManagerComponent* TurnManager = NewObject<UGridTurnManagerComponent>();
	const int32 EncounterSeedBefore = TurnManager->EncounterRandomSeed;
	FGridMonsterIdleVariationSelector::SelectVariationIndex(Seed, 3, INDEX_NONE, true);
	TestEqual(TEXT("EncounterRandomSeed is unchanged"), TurnManager->EncounterRandomSeed, EncounterSeedBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10IdleVariationNoImmediateRepeatTest, "Grimrock.Monsters.MON10.IdleVariations.IdleVariationNoImmediateRepeat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10IdleVariationNoImmediateRepeatTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const int32 Seed = 123456;
	for (int32 Previous = 0; Previous < 4; ++Previous)
	{
		const int32 Selected = FGridMonsterIdleVariationSelector::SelectVariationIndex(Seed, 4, Previous, true);
		TestNotEqual(TEXT("Avoidance excludes the previous variation"), Selected, Previous);
		TestEqual(TEXT("Avoidance remains deterministic"), FGridMonsterIdleVariationSelector::SelectVariationIndex(Seed, 4, Previous, true), Selected);
	}
	TestEqual(TEXT("A single variation remains selectable"), FGridMonsterIdleVariationSelector::SelectVariationIndex(Seed, 1, 0, true), 0);
	const int32 Normal = FGridMonsterIdleVariationSelector::SelectVariationIndex(Seed, 4, 2, false);
	TestTrue(TEXT("Disabled avoidance uses the normal range"), Normal >= 0 && Normal < 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10IdleVariationDeterministicDelayTest, "Grimrock.Monsters.MON10.IdleVariations.IdleVariationDeterministicDelay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10IdleVariationDeterministicDelayTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const int32 Seed = 654321;
	const float Delay = FGridMonsterIdleVariationSelector::SelectDelay(Seed, 5.0f, 12.0f);
	TestEqual(TEXT("The same seed produces the same delay"), FGridMonsterIdleVariationSelector::SelectDelay(Seed, 5.0f, 12.0f), Delay);
	TestTrue(TEXT("The delay stays in range"), Delay >= 5.0f && Delay <= 12.0f);
	TestEqual(TEXT("A fixed interval is exact"), FGridMonsterIdleVariationSelector::SelectDelay(Seed, 3.0f, 3.0f), 3.0f);
	TestTrue(TEXT("Invalid input has a safe positive fallback"), FGridMonsterIdleVariationSelector::SelectDelay(Seed, -1.0f, 0.0f) > 0.0f);

	FRandomStream CombatRandomStream(1337);
	const int32 Before = CombatRandomStream.GetCurrentSeed();
	FGridMonsterIdleVariationSelector::SelectDelay(Seed, 2.0f, 4.0f);
	TestEqual(TEXT("Delay does not consume gameplay RNG"), CombatRandomStream.GetCurrentSeed(), Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10IdleVariationSchedulingLifecycleTest,
	"Grimrock.Monsters.MON10.IdleVariations.IdleVariationSchedulingLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10IdleVariationSchedulingLifecycleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10IdleTestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	ConfigureMON10IdleFloor(Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON10IdleDefinition(Runtime);
	AGridMonsterActor* Monster =
		SpawnMON10IdleMonster(TestWorld.World, Runtime, Definition, FGuid(10, 4, 1, 1), FIntPoint(2, 2), TEXT("MON10IdleLifecycleRat"), true);
	if (!Runtime || !Monster)
	{
		return false;
	}

	UGridMonsterIdleVariationComponent* Component = Monster->IdleVariationComponent;
	TestTrue(TEXT("Living Idle schedules a delay"), Component->IsIdleVariationDelayScheduled());
	const FTimerHandle FirstTimer = Component->IdleVariationDelayTimerHandle;
	const int32 FirstOccurrence = Component->ScheduledOccurrenceNumber;
	Component->RefreshIdleVariationScheduling();
	Component->RefreshIdleVariationScheduling();
	TestTrue(TEXT("Refresh keeps the same timer"), FirstTimer == Component->IdleVariationDelayTimerHandle);
	TestEqual(TEXT("Refresh keeps one occurrence"), Component->ScheduledOccurrenceNumber, FirstOccurrence);

	Monster->SetMovementAnimationState(true, 0.1f);
	TestFalse(TEXT("Movement stops the delay"), Component->IsIdleVariationDelayScheduled());
	Monster->SetMovementAnimationState(true, 0.8f);
	TestFalse(TEXT("Move alpha updates do not reschedule"), Component->IsIdleVariationDelayScheduled());
	Monster->SetMovementAnimationState(false);
	TestTrue(TEXT("Movement end resumes scheduling"), Component->IsIdleVariationDelayScheduled());

	Monster->SetTurnAnimationState(1);
	TestFalse(TEXT("Turning stops the delay"), Component->IsIdleVariationDelayScheduled());
	Monster->SetTurnAnimationState(0);
	TestTrue(TEXT("Turning end resumes scheduling"), Component->IsIdleVariationDelayScheduled());

	const EGridMonsterState ActiveStates[] = { EGridMonsterState::Alert, EGridMonsterState::Pursuing, EGridMonsterState::Attacking, EGridMonsterState::Hurt };
	for (const EGridMonsterState State : ActiveStates)
	{
		Monster->SetMonsterState(State);
		TestFalse(TEXT("An active gameplay state stops Idle"), Component->IsIdleVariationDelayScheduled());
		Monster->SetMonsterState(EGridMonsterState::Idle);
		TestTrue(TEXT("Returning Idle schedules again"), Component->IsIdleVariationDelayScheduled());
	}

	Runtime->SetMonsterRuntimeLevelActive(Monster, false);
	TestFalse(TEXT("Runtime deactivation stops Idle"), Component->IsIdleVariationDelayScheduled());
	Runtime->SetMonsterRuntimeLevelActive(Monster, true);
	Monster->SetMonsterState(EGridMonsterState::Idle);
	TestTrue(TEXT("Living runtime reactivation resumes"), Component->IsIdleVariationDelayScheduled());

	Monster->MarkDead();
	TestFalse(TEXT("Death stops the delay"), Component->IsIdleVariationDelayScheduled());
	TestFalse(TEXT("Death leaves no active Idle"), Component->IsIdleVariationActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10IdleVariationRequestExactlyOnceTest, "Grimrock.Monsters.MON10.IdleVariations.IdleVariationRequestExactlyOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10IdleVariationRequestExactlyOnceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10IdleTestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	ConfigureMON10IdleFloor(Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON10IdleDefinition(Runtime, TEXT("RequestRat"), 1);
	AGridMonsterActor* Monster = SpawnMON10IdleMonster(TestWorld.World, Runtime, Definition, FGuid(20, 4, 1, 1), FIntPoint(2, 2), TEXT("MON10IdleRequestRat"));
	if (!Monster)
	{
		return false;
	}

	UGridMonsterIdleVariationComponent* Component = Monster->IdleVariationComponent;
	TestTrue(TEXT("Manual playback is accepted"), Component->PlayIdleVariationNow());
	TestEqual(TEXT("One request is counted"), Component->PlaybackRequestCount, 1);
	TestEqual(TEXT("One delegate is broadcast"), Component->PlaybackRequestBroadcastCount, 1);
	TestTrue(TEXT("The variation is active"), Component->IsIdleVariationActive());
	const FGridMonsterIdleVariationPlaybackRequest& Request = Component->LastPlaybackRequest;
	TestEqual(TEXT("Sequence starts at one"), Request.SequenceNumber, 1);
	TestEqual(TEXT("Occurrence starts at one"), Request.OccurrenceNumber, 1);
	TestEqual(TEXT("MonsterId is exposed"), Request.MonsterId, Definition->MonsterId);
	TestEqual(TEXT("VariationId is exposed"), Request.VariationId, Definition->IdleVariations[0].VariationId);
	TestEqual(TEXT("Variation index is exposed"), Request.VariationIndex, 0);
	TestNotNull(TEXT("Animation is resolved"), Request.Animation.Get());
	TestEqual(TEXT("Slot is exposed"), Request.SlotName, Definition->IdleVariationSlotName);
	TestEqual(TEXT("Fallback duration is used"), Request.EffectiveDuration, Definition->IdleVariations[0].ExpectedDuration);

	Component->RefreshIdleVariationScheduling();
	Component->RefreshIdleVariationScheduling();
	TestEqual(TEXT("Refresh does not duplicate requests"), Component->PlaybackRequestCount, 1);
	TestEqual(TEXT("Refresh does not duplicate delegates"), Component->PlaybackRequestBroadcastCount, 1);
	TestFalse(TEXT("A second manual play is refused"), Component->PlayIdleVariationNow());

	Component->HandleIdleVariationDurationTimer();
	TestFalse(TEXT("Natural end clears active state"), Component->IsIdleVariationActive());
	TestTrue(TEXT("Natural end schedules a new delay"), Component->IsIdleVariationDelayScheduled());
	TestEqual(TEXT("Natural end does not play immediately"), Component->PlaybackRequestCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10IdleVariationRestoreSilentTest, "Grimrock.Monsters.MON10.IdleVariations.IdleVariationRestoreSilent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10IdleVariationRestoreSilentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10IdleTestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	ConfigureMON10IdleFloor(Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON10IdleDefinition(Runtime);
	if (!Runtime)
	{
		return false;
	}

	struct FRestoreCase
	{
		EGridMonsterState State;
		bool bDead;
		FIntPoint Cell;
		FGuid Id;
		FName Name;
	};
	const FRestoreCase Cases[] = { { EGridMonsterState::Dead, true, FIntPoint(1, 1), FGuid(30, 4, 1, 1), TEXT("RestoreDead") },
		{ EGridMonsterState::Idle, false, FIntPoint(2, 1), FGuid(30, 4, 1, 2), TEXT("RestoreIdle") },
		{ EGridMonsterState::Alert, false, FIntPoint(3, 1), FGuid(30, 4, 1, 3), TEXT("RestoreAlert") } };

	for (const FRestoreCase& Case : Cases)
	{
		AGridMonsterActor* Monster = SpawnMON10IdleMonster(TestWorld.World, Runtime, Definition, Case.Id, Case.Cell, Case.Name);
		if (!Monster)
		{
			return false;
		}
		UGridMonsterIdleVariationComponent* Component = Monster->IdleVariationComponent;
		Component->PlayIdleVariationNow();
		TestEqual(TEXT("Fixture has transient history"), Component->PlaybackRequestCount, 1);

		const FGridRuntimeMonsterState Saved = MakeMON10IdleSavedState(Monster, Case.State, Case.bDead);
		TestTrue(TEXT("The MON9 state restores"), Monster->RestoreRuntimeMonsterState(Saved, Runtime));
		TestEqual(TEXT("Restore replays no request"), Component->PlaybackRequestCount, 0);
		TestEqual(TEXT("Restore replays no delegate"), Component->PlaybackRequestBroadcastCount, 0);
		TestNull(TEXT("Restore keeps no dynamic montage"), Component->ActiveIdleDynamicMontage.Get());
		TestFalse(TEXT("Restore keeps no active variation"), Component->IsIdleVariationActive());

		if (Case.bDead)
		{
			TestFalse(TEXT("A corpse has no delay"), Component->IsIdleVariationDelayScheduled());
		}
		else if (Case.State == EGridMonsterState::Idle)
		{
			TestTrue(TEXT("Living Idle gets a new delay only"), Component->IsIdleVariationDelayScheduled());
		}
		else
		{
			TestFalse(TEXT("Alert gets no Idle delay"), Component->IsIdleVariationDelayScheduled());
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10IdleVariationNoTickNoPersistenceAndIsolationTest,
	"Grimrock.Monsters.MON10.IdleVariations.IdleVariationNoTickNoPersistenceAndIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10IdleVariationNoTickNoPersistenceAndIsolationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10IdleTestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	ConfigureMON10IdleFloor(Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON10IdleDefinition(Runtime, TEXT("IsolationRat"), 1);
	AGridMonsterActor* Monster =
		SpawnMON10IdleMonster(TestWorld.World, Runtime, Definition, FGuid(40, 4, 1, 1), FIntPoint(2, 2), TEXT("MON10IdleIsolationRat"));
	if (!Monster)
	{
		return false;
	}

	UGridMonsterIdleVariationComponent* Component = Monster->IdleVariationComponent;
	TestFalse(TEXT("The component never ticks"), Component->PrimaryComponentTick.bCanEverTick);
	for (TFieldIterator<FProperty> Property(FGridMonsterIdleVariationPlaybackRequest::StaticStruct()); Property; ++Property)
	{
		TestFalse(*FString::Printf(TEXT("%s is not SaveGame data"), *Property->GetName()), Property->HasAnyPropertyFlags(CPF_SaveGame));
	}
	TestNull(TEXT("MON9 monster state has no Idle request"),
		FindFProperty<FProperty>(FGridRuntimeMonsterState::StaticStruct(), TEXT("LastIdleVariationPlaybackRequest")));
	TestNull(TEXT("MON9 level state has no Idle request"), FindFProperty<FProperty>(FGridLevelRuntimeState::StaticStruct(), TEXT("IdleVariation")));

	TestNotNull(TEXT("Legacy AttackSound remains reflected"), FindFProperty<FProperty>(FGridMonsterAttackDefinition::StaticStruct(), TEXT("AttackSound")));
	TestNotNull(TEXT("Legacy ImpactVFX remains reflected"), FindFProperty<FProperty>(FGridMonsterAttackDefinition::StaticStruct(), TEXT("ImpactVFX")));

	const EGridMonsterState StateBefore = Monster->MonsterState;
	const int32 HealthBefore = Monster->CurrentHealth;
	const int32 AudioBefore = Monster->AudioComponent->PlaybackRequestCount;
	const int32 VFXBefore = Monster->VFXComponent->SpawnRequestCount;
	Component->PlayIdleVariationNow();
	Component->StopIdleVariations();
	TestEqual(TEXT("Stopping Idle does not change combat state"), Monster->MonsterState, StateBefore);
	TestEqual(TEXT("Stopping Idle does not change health"), Monster->CurrentHealth, HealthBefore);
	TestEqual(TEXT("Idle does not request audio"), Monster->AudioComponent->PlaybackRequestCount, AudioBefore);
	TestEqual(TEXT("Idle does not request VFX"), Monster->VFXComponent->SpawnRequestCount, VFXBefore);
	TestFalse(TEXT("Only the owned Idle state is cleared"), Component->IsIdleVariationActive());
	TestNull(TEXT("Only the owned montage reference is cleared"), Component->ActiveIdleDynamicMontage.Get());
	return true;
}

#endif
