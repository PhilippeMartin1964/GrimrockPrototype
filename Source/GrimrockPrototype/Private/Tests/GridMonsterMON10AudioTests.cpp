#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDirectionUtils.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterAudioComponent.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Sound/SoundWave.h"
#include "UObject/UnrealType.h"

namespace
{
	struct FGridMON10AudioTestWorld
	{
		UWorld* World = nullptr;

		FGridMON10AudioTestWorld()
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
				FName(*FString::Printf(TEXT("MON10AudioTestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridMON10AudioTestWorld()
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

	UGridLevelAsset* ConfigureMON10AudioFloor(AGridLevelRuntimeActor* Runtime, int32 Width = 6, int32 Height = 6)
	{
		if (!Runtime)
		{
			return nullptr;
		}

		UGridLevelAsset* LevelAsset = NewObject<UGridLevelAsset>(Runtime);
		LevelAsset->Width = Width;
		LevelAsset->Height = Height;
		LevelAsset->StartCellX = Width - 1;
		LevelAsset->StartCellY = Height - 1;
		LevelAsset->StartFacing = EGridEdge::North;
		LevelAsset->EnsureCellCount();
		for (FGridLevelCellData& Cell : LevelAsset->Cells)
		{
			Cell.CellType = EGridCellType::Floor;
			Cell.bBlocksOccupancy = false;
		}
		Runtime->LevelAsset = LevelAsset;
		Runtime->CurrentDungeonLevelId = TEXT("MON10AudioLevel");
		return LevelAsset;
	}

	USoundWave* MakeMON10AudioSound(UObject* Outer, FName Name)
	{
		return NewObject<USoundWave>(Outer, Name);
	}

	FGridMonsterAudioEventDefinition MakeMON10AudioEvent(UObject* Outer, FName SoundName)
	{
		FGridMonsterAudioEventDefinition Definition;
		Definition.Sounds.Add(MakeMON10AudioSound(Outer, SoundName));
		Definition.PitchMin = 0.90f;
		Definition.PitchMax = 1.10f;
		return Definition;
	}

	UGridMonsterDefinitionAsset* MakeMON10AudioDefinition(UObject* Outer, FName MonsterId = TEXT("MON10_AudioRat"))
	{
		UGridMonsterDefinitionAsset* Definition = NewObject<UGridMonsterDefinitionAsset>(Outer);
		Definition->MonsterId = MonsterId;
		Definition->DisplayName = FText::FromString(TEXT("Rat audio MON10"));
		Definition->CategoryId = TEXT("Vermin");
		Definition->MaxHealth = 10;
		Definition->Accuracy = 100;
		Definition->ActionPointsPerTurn = 2;
		Definition->DeathExpectedDuration = 1.0f;
		Definition->IdleAudioMinDelay = 1.0f;
		Definition->IdleAudioMaxDelay = 1.0f;

		FGridMonsterAttackDefinition Attack;
		Attack.AttackId = TEXT("Attack_Bite");
		Attack.MinDamage = 2;
		Attack.MaxDamage = 2;
		Attack.DamageType = EGridDamageType::Physical;
		Attack.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
		Definition->Attacks.Add(Attack);
		return Definition;
	}

	AGridMonsterActor* SpawnMON10AudioMonster(UWorld* World, AGridLevelRuntimeActor* Runtime, UGridMonsterDefinitionAsset* Definition,
		const FGuid& PersistenceId, FIntPoint Cell, FName ActorName, bool bAddRuntimeComponents = false)
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

		if (bAddRuntimeComponents)
		{
			UGridMonsterMovementComponent* Movement = NewObject<UGridMonsterMovementComponent>(Monster, TEXT("MON10AudioTestMovement"));
			Movement->bAutoInitialize = false;
			Movement->bInferCellFromActorLocation = false;
			Monster->AddInstanceComponent(Movement);
			Movement->RegisterComponent();

			UGridMonsterBehaviorComponent* Behavior = NewObject<UGridMonsterBehaviorComponent>(Monster, TEXT("MON10AudioTestBehavior"));
			Behavior->bAutoInitialize = false;
			Monster->AddInstanceComponent(Behavior);
			Behavior->RegisterComponent();
		}

		if (Monster->AudioComponent)
		{
			Monster->AudioComponent->bNativePlaybackEnabled = false;
			Monster->AudioComponent->InitializeMonsterAudio();
		}
		return Monster;
	}

	AGrimrockPartyPawn* MakeMON10AudioParty(UWorld* World, AGridLevelRuntimeActor* Runtime, int32 Health = 20)
	{
		AGrimrockPartyPawn* Party = World ? World->SpawnActor<AGrimrockPartyPawn>() : nullptr;
		if (!Party || !Party->PartyInventoryComponent || !Runtime)
		{
			return Party;
		}

		Party->LevelRuntimeActor = Runtime;
		Party->CurrentCellX = 5;
		Party->CurrentCellY = 5;
		Party->Facing = EGridEdge::North;
		Party->SetActorLocation(Runtime->GetCellCenterWorld(Party->CurrentCellX, Party->CurrentCellY, Party->EyeHeight));
		Party->SetActorRotation(FRotator(0.0f, GridDirectionUtils::ToYaw(Party->Facing), 0.0f));

		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid::NewGuid();
		Character.DisplayName = FText::FromString(TEXT("Elias"));
		Character.DerivedStats.MaxHealth = Health;
		Character.Resources.CurrentHealth = Health;
		Character.DerivedStats.Evasion = 0;
		Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { Character };
		return Party;
	}

	int32 CountMON10AttackEntries(const UGridTurnManagerComponent* TurnManager)
	{
		int32 Count = 0;
		if (TurnManager)
		{
			for (const FGridCombatLogEntry& Entry : TurnManager->CombatLogEntries)
			{
				Count += Entry.Type == EGridCombatLogEntryType::AttackHit || Entry.Type == EGridCombatLogEntryType::AttackMiss ? 1 : 0;
			}
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10AudioDefinitionValidationTest, "Grimrock.Monsters.MON10.Audio.AudioDefinitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10AudioDefinitionValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMonsterAudioEventDefinition Audio;
	TestTrue(TEXT("An empty audio definition is valid"), Audio.IsValidDefinition());
	TestFalse(TEXT("An empty audio definition has no sound"), Audio.HasConfiguredSound());

	Audio.VolumeMultiplier = -1.0f;
	TestFalse(TEXT("Negative volume is invalid"), Audio.IsValidDefinition());
	Audio = FGridMonsterAudioEventDefinition();
	Audio.PitchMin = 0.0f;
	TestFalse(TEXT("Zero minimum pitch is invalid"), Audio.IsValidDefinition());
	Audio = FGridMonsterAudioEventDefinition();
	Audio.PitchMin = 1.1f;
	Audio.PitchMax = 1.0f;
	TestFalse(TEXT("An inverted pitch interval is invalid"), Audio.IsValidDefinition());
	Audio = FGridMonsterAudioEventDefinition();
	Audio.CooldownSeconds = -0.1f;
	TestFalse(TEXT("Negative cooldown is invalid"), Audio.IsValidDefinition());
	Audio = FGridMonsterAudioEventDefinition();
	Audio.Sounds.Add(nullptr);
	TestFalse(TEXT("An explicit empty sound slot is invalid"), Audio.IsValidDefinition());

	UGridMonsterDefinitionAsset* Definition = MakeMON10AudioDefinition(GetTransientPackage());
	TestTrue(TEXT("A monster with no audio remains valid"), Definition->IsValidDefinition());
	Definition->IdleAudioMinDelay = 0.0f;
	TestFalse(TEXT("A zero idle minimum delay is invalid"), Definition->IsValidDefinition());
	Definition->IdleAudioMinDelay = 2.0f;
	Definition->IdleAudioMaxDelay = 1.0f;
	TestFalse(TEXT("An inverted idle delay range is invalid"), Definition->IsValidDefinition());

	FGridMonsterAttackDefinition Attack;
	Attack.AttackId = TEXT("Attack_CurrentAudio");
	Attack.AttackAudio.Sounds.Add(MakeMON10AudioSound(GetTransientPackage(), TEXT("MON10CurrentAttackSound")));
	TestTrue(TEXT("Current AttackAudio remains valid"), Attack.IsValidDefinition());
	TestTrue(TEXT("AttackAudio remains configured"), Attack.AttackAudio.HasConfiguredSound());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10AudioDeterministicVariantSelectionTest, "Grimrock.Monsters.MON10.Audio.AudioDeterministicVariantSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10AudioDeterministicVariantSelectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGuid PersistenceId(1, 2, 3, 4);
	const int32 Seed = FGridMonsterAudioSelector::BuildPresentationSeed(PersistenceId, TEXT("MON10_AudioRat"), EGridMonsterAudioEvent::Idle, 7);
	TestEqual(TEXT("The same presentation inputs produce the same seed"),
		FGridMonsterAudioSelector::BuildPresentationSeed(PersistenceId, TEXT("MON10_AudioRat"), EGridMonsterAudioEvent::Idle, 7), Seed);

	const int32 Index = FGridMonsterAudioSelector::SelectVariationIndex(Seed, 4);
	TestEqual(TEXT("The same presentation seed produces the same index"), FGridMonsterAudioSelector::SelectVariationIndex(Seed, 4), Index);
	TestTrue(TEXT("The selected index is in range"), Index >= 0 && Index < 4);

	const float Pitch = FGridMonsterAudioSelector::SelectPitch(Seed, 0.85f, 1.15f);
	TestEqual(TEXT("The same presentation seed produces the same pitch"), FGridMonsterAudioSelector::SelectPitch(Seed, 0.85f, 1.15f), Pitch);
	TestTrue(TEXT("The selected pitch is in range"), Pitch >= 0.85f && Pitch <= 1.15f);

	FRandomStream CombatRandomStream(1337);
	const int32 CombatSeedBefore = CombatRandomStream.GetCurrentSeed();
	for (int32 Occurrence = 1; Occurrence <= 32; ++Occurrence)
	{
		const int32 PresentationSeed =
			FGridMonsterAudioSelector::BuildPresentationSeed(PersistenceId, TEXT("MON10_AudioRat"), EGridMonsterAudioEvent::Attack, Occurrence);
		const int32 SelectedIndex = FGridMonsterAudioSelector::SelectVariationIndex(PresentationSeed, 5);
		const float SelectedPitch = FGridMonsterAudioSelector::SelectPitch(PresentationSeed, 0.75f, 1.25f);
		TestTrue(TEXT("Every variation index stays in range"), SelectedIndex >= 0 && SelectedIndex < 5);
		TestTrue(TEXT("Every pitch stays in range"), SelectedPitch >= 0.75f && SelectedPitch <= 1.25f);
	}
	TestEqual(TEXT("Presentation selection does not consume CombatRandomStream"), CombatRandomStream.GetCurrentSeed(), CombatSeedBefore);

	UGridTurnManagerComponent* TurnManager = NewObject<UGridTurnManagerComponent>();
	const int32 EncounterSeedBefore = TurnManager->EncounterRandomSeed;
	FGridMonsterAudioSelector::SelectVariationIndex(Seed, 3);
	TestEqual(TEXT("Presentation selection does not modify EncounterRandomSeed"), TurnManager->EncounterRandomSeed, EncounterSeedBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10AudioCombatStartAlertExactlyOnceTest, "Grimrock.Monsters.MON10.Audio.AudioCombatStartAlertExactlyOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10AudioCombatStartAlertExactlyOnceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10AudioTestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World ? TestWorld.World->SpawnActor<AGridLevelRuntimeActor>() : nullptr;
	ConfigureMON10AudioFloor(Runtime);
	AGrimrockPartyPawn* Party = MakeMON10AudioParty(TestWorld.World, Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON10AudioDefinition(Runtime);
	Definition->AlertAudio = MakeMON10AudioEvent(Definition, TEXT("MON10Alert"));

	AGridMonsterActor* First =
		SpawnMON10AudioMonster(TestWorld.World, Runtime, Definition, FGuid(10, 1, 1, 1), FIntPoint(1, 1), TEXT("MON10AudioAlertFirst"), true);
	AGridMonsterActor* Second =
		SpawnMON10AudioMonster(TestWorld.World, Runtime, Definition, FGuid(10, 1, 1, 2), FIntPoint(2, 1), TEXT("MON10AudioAlertSecond"), true);
	AGridMonsterActor* Dead = SpawnMON10AudioMonster(TestWorld.World, Runtime, Definition, FGuid(10, 1, 1, 3), FIntPoint(3, 1), TEXT("MON10AudioAlertDead"));
	AGridMonsterActor* Disabled =
		SpawnMON10AudioMonster(TestWorld.World, Runtime, Definition, FGuid(10, 1, 1, 4), FIntPoint(4, 1), TEXT("MON10AudioAlertDisabled"));
	if (!Runtime || !Party || !First || !Second || !Dead || !Disabled)
	{
		return false;
	}
	Dead->CurrentHealth = 0;
	Dead->MonsterState = EGridMonsterState::Dead;
	Disabled->bMonsterEnabled = false;

	UGridTurnManagerComponent* TurnManager = NewObject<UGridTurnManagerComponent>(Runtime, TEXT("MON10AudioAlertTurnManager"));
	TurnManager->bAutoInitialize = false;
	Runtime->AddInstanceComponent(TurnManager);
	TurnManager->RegisterComponent();
	if (!TurnManager->InitializeTurnManager(Runtime, Party))
	{
		return false;
	}

	TestTrue(TEXT("A valid encounter starts"), TurnManager->StartCombatWithAllMonsters());
	TestEqual(TEXT("The first participant alerts once"), First->AudioComponent->GetPlaybackRequestCountForEvent(EGridMonsterAudioEvent::Alert), 1);
	TestEqual(TEXT("The second participant alerts once"), Second->AudioComponent->GetPlaybackRequestCountForEvent(EGridMonsterAudioEvent::Alert), 1);
	TestEqual(TEXT("A dead monster does not alert"), Dead->AudioComponent->GetPlaybackRequestCountForEvent(EGridMonsterAudioEvent::Alert), 0);
	TestEqual(TEXT("A disabled monster does not alert"), Disabled->AudioComponent->GetPlaybackRequestCountForEvent(EGridMonsterAudioEvent::Alert), 0);

	TestFalse(TEXT("A second start is refused"), TurnManager->StartCombatWithAllMonsters());
	TestEqual(TEXT("A refused start adds no alert"), First->AudioComponent->GetPlaybackRequestCountForEvent(EGridMonsterAudioEvent::Alert), 1);

	TurnManager->AbortCombat();
	TestTrue(TEXT("A distinct encounter can start"), TurnManager->StartCombatWithAllMonsters());
	TestEqual(TEXT("A distinct encounter can alert again"), First->AudioComponent->GetPlaybackRequestCountForEvent(EGridMonsterAudioEvent::Alert), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10AudioAttackAndImpactExactlyOnceTest, "Grimrock.Monsters.MON10.Audio.AudioAttackAndImpactExactlyOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10AudioAttackAndImpactExactlyOnceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10AudioTestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	ConfigureMON10AudioFloor(Runtime);
	AGrimrockPartyPawn* Party = MakeMON10AudioParty(TestWorld.World, Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON10AudioDefinition(Runtime);
	Definition->Attacks[0].AttackAudio = MakeMON10AudioEvent(Definition, TEXT("MON10Attack"));
	Definition->Attacks[0].ImpactHitAudio = MakeMON10AudioEvent(Definition, TEXT("MON10ImpactHit"));
	Definition->Attacks[0].ImpactMissAudio = MakeMON10AudioEvent(Definition, TEXT("MON10ImpactMiss"));
	AGridMonsterActor* Monster = SpawnMON10AudioMonster(TestWorld.World, Runtime, Definition, FGuid(20, 1, 1, 1), FIntPoint(1, 1), TEXT("MON10AudioAttackRat"));
	UGridTurnManagerComponent* TurnManager = NewObject<UGridTurnManagerComponent>(Runtime);
	if (!Runtime || !Party || !Monster || !TurnManager || !Monster->CombatComponent->InitializeCombat(Party) ||
		!TurnManager->InitializeTurnManager(Runtime, Party))
	{
		return false;
	}

	FGridCombatAction Action;
	Action.Type = EGridCombatActionType::MeleeAttack;
	Action.TargetCharacterIndex = 0;
	Action.AttackId = Definition->Attacks[0].AttackId;
	TestTrue(TEXT("Attack presentation starts without a montage"), Monster->CombatComponent->StartAttackPresentation(Action, Definition->Attacks[0]));
	TestFalse(TEXT("The same active presentation cannot start twice"), Monster->CombatComponent->StartAttackPresentation(Action, Definition->Attacks[0]));
	TestEqual(TEXT("Attack audio is requested exactly once"), Monster->AudioComponent->GetPlaybackRequestCountForEvent(EGridMonsterAudioEvent::Attack), 1);

	TurnManager->CurrentMonster = Monster;
	TurnManager->CurrentCombatComponent = Monster->CombatComponent;
	TurnManager->ActiveAttackDefinition = Definition->Attacks[0];
	TurnManager->ActiveAction = Action;
	TurnManager->bHasActiveAction = true;
	TurnManager->bActiveAttackImpactCommitted = false;
	TurnManager->CombatRandomStream.Initialize(1337);
	const int32 HealthBefore = Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth;
	TurnManager->CommitActiveAttackImpact();
	const int32 HealthAfterFirst = Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth;
	TurnManager->CommitActiveAttackImpact();

	const int32 ImpactRequests = Monster->AudioComponent->GetPlaybackRequestCountForEvent(EGridMonsterAudioEvent::ImpactHit) +
		Monster->AudioComponent->GetPlaybackRequestCountForEvent(EGridMonsterAudioEvent::ImpactMiss);
	TestEqual(TEXT("Impact audio is requested exactly once"), ImpactRequests, 1);
	TestTrue(TEXT("The attack resolution ran"), HealthAfterFirst <= HealthBefore);
	TestEqual(TEXT("Damage is not applied by a second impact"),
		Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth, HealthAfterFirst);
	TestEqual(TEXT("OnAttackResolved remains unique"), TurnManager->AttackResolvedBroadcastCount, 1);
	TestEqual(TEXT("GridCombat attack feedback remains unique"), CountMON10AttackEntries(TurnManager), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10AudioHurtDeathExclusivityTest, "Grimrock.Monsters.MON10.Audio.AudioHurtDeathExclusivity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10AudioHurtDeathExclusivityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10AudioTestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	ConfigureMON10AudioFloor(Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON10AudioDefinition(Runtime);
	Definition->HurtAudio = MakeMON10AudioEvent(Definition, TEXT("MON10Hurt"));
	Definition->DeathAudio = MakeMON10AudioEvent(Definition, TEXT("MON10Death"));
	AGridMonsterActor* Monster =
		SpawnMON10AudioMonster(TestWorld.World, Runtime, Definition, FGuid(30, 1, 1, 1), FIntPoint(2, 2), TEXT("MON10AudioHurtDeathRat"), true);
	UGridMonsterMovementComponent* Movement = Monster ? Monster->FindComponentByClass<UGridMonsterMovementComponent>() : nullptr;
	if (!Monster || !Movement || !Movement->InitializeMovement(Runtime))
	{
		return false;
	}
	Monster->DeathComponent->InitializeDeathComponent(Runtime);

	FGridAttackResult NonFatal;
	NonFatal.bHit = true;
	NonFatal.HealthDamage = 2;
	Monster->ApplyAttackResult(NonFatal);
	TestEqual(TEXT("Non-fatal effective damage requests Hurt"), Monster->AudioComponent->GetPlaybackRequestCountForEvent(EGridMonsterAudioEvent::Hurt), 1);
	TestEqual(TEXT("Non-fatal damage requests no Death"), Monster->AudioComponent->GetPlaybackRequestCountForEvent(EGridMonsterAudioEvent::Death), 0);

	FGridAttackResult Fatal;
	Fatal.bHit = true;
	Fatal.HealthDamage = 100;
	Monster->ApplyAttackResult(Fatal);
	TestEqual(TEXT("Fatal damage requests Death once"), Monster->AudioComponent->GetPlaybackRequestCountForEvent(EGridMonsterAudioEvent::Death), 1);
	TestEqual(TEXT("Fatal damage adds no Hurt"), Monster->AudioComponent->GetPlaybackRequestCountForEvent(EGridMonsterAudioEvent::Hurt), 1);
	const int32 LogicalDeaths = Monster->DeathComponent->LogicalDeathEventCount;
	const int32 GeneratedLoot = Monster->DeathComponent->GeneratedLoot.Num();
	Monster->MarkDead();
	TestEqual(TEXT("A second MarkDead adds no Death audio"), Monster->AudioComponent->GetPlaybackRequestCountForEvent(EGridMonsterAudioEvent::Death), 1);
	TestEqual(TEXT("MonsterDied remains unique"), Monster->DeathComponent->LogicalDeathEventCount, LogicalDeaths);
	TestEqual(TEXT("Loot remains unique"), Monster->DeathComponent->GeneratedLoot.Num(), GeneratedLoot);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10AudioRestoreDeadSilentTest, "Grimrock.Monsters.MON10.Audio.AudioRestoreDeadSilent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10AudioRestoreDeadSilentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10AudioTestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	ConfigureMON10AudioFloor(Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON10AudioDefinition(Runtime);
	Definition->HurtAudio = MakeMON10AudioEvent(Definition, TEXT("MON10RestoreHurt"));
	Definition->DeathAudio = MakeMON10AudioEvent(Definition, TEXT("MON10RestoreDeath"));
	Definition->IdleAudio = MakeMON10AudioEvent(Definition, TEXT("MON10RestoreIdle"));
	Definition->bEnableIdleAudio = true;
	const FGuid PersistenceId(40, 1, 1, 1);
	AGridMonsterActor* Monster =
		SpawnMON10AudioMonster(TestWorld.World, Runtime, Definition, PersistenceId, FIntPoint(2, 2), TEXT("MON10AudioRestoreDeadRat"), true);
	if (!Monster)
	{
		return false;
	}

	Monster->AudioComponent->RefreshIdleAmbienceScheduling();
	TestTrue(TEXT("Idle is initially scheduled"), Monster->AudioComponent->IsIdleAmbienceScheduled());
	Monster->AudioComponent->PlayHurt();
	TestEqual(TEXT("The fixture has transient audio history"), Monster->AudioComponent->PlaybackRequestCount, 1);

	FGridRuntimeMonsterState State;
	State.PersistenceId = PersistenceId;
	State.SpawnObjectId = PersistenceId;
	State.MonsterDefinitionId = Definition->MonsterId;
	State.DungeonLevelId = TEXT("MON10AudioLevel");
	State.CellX = 2;
	State.CellY = 2;
	State.Facing = EGridEdge::North;
	State.MonsterState = EGridMonsterState::Dead;
	State.CurrentHealth = 0;
	State.CurrentPhysicalArmor = 0;
	State.CurrentMagicalArmor = 0;
	State.bMonsterEnabled = true;
	State.bIsDead = true;

	TestTrue(TEXT("The MON9 dead state restores"), Monster->RestoreRuntimeMonsterState(State, Runtime));
	TestTrue(TEXT("The restored monster remains dead"), Monster->IsDead());
	TestEqual(TEXT("Restore emits no Death request"), Monster->AudioComponent->GetPlaybackRequestCountForEvent(EGridMonsterAudioEvent::Death), 0);
	TestEqual(TEXT("Restore emits no Hurt request"), Monster->AudioComponent->GetPlaybackRequestCountForEvent(EGridMonsterAudioEvent::Hurt), 0);
	TestEqual(TEXT("Transient request history is reset"), Monster->AudioComponent->PlaybackRequestCount, 0);
	TestFalse(TEXT("A restored corpse has no idle timer"), Monster->AudioComponent->IsIdleAmbienceScheduled());
	TestTrue(TEXT("Loot stays committed without regeneration"), Monster->DeathComponent->bLootGenerated);
	TestEqual(TEXT("No new loot is generated"), Monster->DeathComponent->GeneratedLoot.Num(), 0);

	for (TFieldIterator<FProperty> Property(FGridMonsterAudioPlaybackRequest::StaticStruct()); Property; ++Property)
	{
		TestFalse(*FString::Printf(TEXT("%s is not SaveGame data"), *Property->GetName()), Property->HasAnyPropertyFlags(CPF_SaveGame));
	}
	TestNull(TEXT("MON9 monster state has no audio request"), FindFProperty<FProperty>(FGridRuntimeMonsterState::StaticStruct(), TEXT("LastPlaybackRequest")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10AudioIdleTimerLifecycleTest, "Grimrock.Monsters.MON10.Audio.AudioIdleTimerLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10AudioIdleTimerLifecycleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10AudioTestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	ConfigureMON10AudioFloor(Runtime);
	AGrimrockPartyPawn* Party = MakeMON10AudioParty(TestWorld.World, Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON10AudioDefinition(Runtime);
	Definition->AlertAudio = MakeMON10AudioEvent(Definition, TEXT("MON10IdleAlert"));
	Definition->DeathAudio = MakeMON10AudioEvent(Definition, TEXT("MON10IdleDeath"));
	Definition->IdleAudio = MakeMON10AudioEvent(Definition, TEXT("MON10Idle"));
	Definition->bEnableIdleAudio = true;
	AGridMonsterActor* Monster =
		SpawnMON10AudioMonster(TestWorld.World, Runtime, Definition, FGuid(50, 1, 1, 1), FIntPoint(2, 2), TEXT("MON10AudioIdleRat"), true);
	if (!Party || !Monster)
	{
		return false;
	}

	Monster->AudioComponent->RefreshIdleAmbienceScheduling();
	TestTrue(TEXT("Living Idle schedules ambience"), Monster->AudioComponent->IsIdleAmbienceScheduled());
	Monster->AudioComponent->PlayAlert();
	TestFalse(TEXT("Alert stops ambience"), Monster->AudioComponent->IsIdleAmbienceScheduled());

	Monster->SetMonsterState(EGridMonsterState::Idle);
	TestTrue(TEXT("Returning to Idle can schedule again"), Monster->AudioComponent->IsIdleAmbienceScheduled());
	Runtime->SetMonsterRuntimeLevelActive(Monster, false);
	TestFalse(TEXT("Runtime deactivation stops ambience"), Monster->AudioComponent->IsIdleAmbienceScheduled());

	Runtime->SetMonsterRuntimeLevelActive(Monster, true);
	Monster->SetMonsterState(EGridMonsterState::Idle);
	Monster->AudioComponent->RefreshIdleAmbienceScheduling();
	TestTrue(TEXT("A living reactivation can resume ambience"), Monster->AudioComponent->IsIdleAmbienceScheduled());
	Monster->MarkDead();
	TestFalse(TEXT("Death stops ambience"), Monster->AudioComponent->IsIdleAmbienceScheduled());
	TestFalse(TEXT("The audio component never ticks"), Monster->AudioComponent->PrimaryComponentTick.bCanEverTick);
	return true;
}

#endif
