#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridDirectionUtils.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "NiagaraSystem.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterVFXComponent.h"
#include "Sound/SoundWave.h"
#include "UObject/UnrealType.h"

#include <limits>

namespace
{
	struct FGridMON10VFXTestWorld
	{
		UWorld* World = nullptr;

		FGridMON10VFXTestWorld()
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
				FName(*FString::Printf(TEXT("MON10VFXTestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridMON10VFXTestWorld()
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

	UGridLevelAsset* ConfigureMON10VFXFloor(AGridLevelRuntimeActor* Runtime, int32 Width = 6, int32 Height = 6)
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
		Runtime->CurrentDungeonLevelId = TEXT("MON10VFXLevel");
		return LevelAsset;
	}

	FGridMonsterVFXEventDefinition MakeMON10VFXEvent(UObject* Outer, FName SystemName, bool bAttach = false)
	{
		FGridMonsterVFXEventDefinition Definition;
		Definition.Systems.Add(NewObject<UNiagaraSystem>(Outer, SystemName));
		Definition.bAttachToSource = bAttach;
		return Definition;
	}

	FGridMonsterAudioEventDefinition MakeMON10VFXAudioEvent(UObject* Outer, FName SoundName)
	{
		FGridMonsterAudioEventDefinition Definition;
		Definition.Sounds.Add(NewObject<USoundWave>(Outer, SoundName));
		return Definition;
	}

	UGridMonsterDefinitionAsset* MakeMON10VFXDefinition(UObject* Outer, FName MonsterId = TEXT("MON10_VFXRat"))
	{
		UGridMonsterDefinitionAsset* Definition = NewObject<UGridMonsterDefinitionAsset>(Outer);
		Definition->MonsterId = MonsterId;
		Definition->DisplayName = FText::FromString(TEXT("Rat VFX MON10"));
		Definition->CategoryId = TEXT("Vermin");
		Definition->MaxHealth = 10;
		Definition->Accuracy = 100;
		Definition->ActionPointsPerTurn = 2;
		Definition->DeathExpectedDuration = 1.0f;

		FGridMonsterAttackDefinition Attack;
		Attack.AttackId = TEXT("Attack_Bite");
		Attack.MinDamage = 2;
		Attack.MaxDamage = 2;
		Attack.DamageType = EGridDamageType::Physical;
		Attack.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
		Definition->Attacks.Add(Attack);
		return Definition;
	}

	AGridMonsterActor* SpawnMON10VFXMonster(UWorld* World, AGridLevelRuntimeActor* Runtime, UGridMonsterDefinitionAsset* Definition, const FGuid& PersistenceId,
		FIntPoint Cell, FName ActorName, bool bAddRuntimeComponents = false)
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
			UGridMonsterMovementComponent* Movement = NewObject<UGridMonsterMovementComponent>(Monster, TEXT("MON10VFXTestMovement"));
			Movement->bAutoInitialize = false;
			Movement->bInferCellFromActorLocation = false;
			Monster->AddInstanceComponent(Movement);
			Movement->RegisterComponent();

			UGridMonsterBehaviorComponent* Behavior = NewObject<UGridMonsterBehaviorComponent>(Monster, TEXT("MON10VFXTestBehavior"));
			Behavior->bAutoInitialize = false;
			Monster->AddInstanceComponent(Behavior);
			Behavior->RegisterComponent();
		}

		Monster->VFXComponent->bNativeSpawnEnabled = false;
		Monster->VFXComponent->InitializeMonsterVFX();
		Monster->AudioComponent->bNativePlaybackEnabled = false;
		Monster->AudioComponent->InitializeMonsterAudio();
		return Monster;
	}

	AGrimrockPartyPawn* MakeMON10VFXParty(UWorld* World, AGridLevelRuntimeActor* Runtime, int32 Health = 20)
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

	int32 CountMON10VFXAttackEntries(const UGridTurnManagerComponent* TurnManager)
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10VFXDefinitionValidationTest, "Grimrock.Monsters.MON10.VFX.VFXDefinitionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10VFXDefinitionValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMonsterVFXEventDefinition VFX;
	TestTrue(TEXT("An empty VFX definition is valid"), VFX.IsValidDefinition());
	TestFalse(TEXT("An empty definition has no system"), VFX.HasConfiguredSystem());
	VFX.Systems.Add(nullptr);
	TestFalse(TEXT("An explicit empty system is invalid"), VFX.IsValidDefinition());

	VFX = FGridMonsterVFXEventDefinition();
	VFX.LocationOffset.X = std::numeric_limits<double>::infinity();
	TestFalse(TEXT("A non-finite offset is invalid"), VFX.IsValidDefinition());
	VFX = FGridMonsterVFXEventDefinition();
	VFX.RotationOffset.Yaw = std::numeric_limits<double>::quiet_NaN();
	TestFalse(TEXT("A non-finite rotation is invalid"), VFX.IsValidDefinition());
	VFX = FGridMonsterVFXEventDefinition();
	VFX.Scale.X = 0.0;
	TestFalse(TEXT("A zero scale is invalid"), VFX.IsValidDefinition());
	VFX = FGridMonsterVFXEventDefinition();
	VFX.Scale.Y = -1.0;
	TestFalse(TEXT("A negative scale is invalid"), VFX.IsValidDefinition());
	VFX = FGridMonsterVFXEventDefinition();
	VFX.CooldownSeconds = -0.1f;
	TestFalse(TEXT("A negative cooldown is invalid"), VFX.IsValidDefinition());

	UGridMonsterDefinitionAsset* Definition = MakeMON10VFXDefinition(GetTransientPackage());
	TestTrue(TEXT("A monster with no VFX remains valid"), Definition->IsValidDefinition());

	FGridMonsterAttackDefinition Attack;
	Attack.AttackId = TEXT("Attack_CurrentVFX");
	Attack.ImpactHitVFXDefinition.Systems.Add(NewObject<UNiagaraSystem>(GetTransientPackage(), TEXT("MON10CurrentImpactVFX")));
	TestTrue(TEXT("Current ImpactHitVFXDefinition remains valid"), Attack.IsValidDefinition());
	TestTrue(TEXT("ImpactHitVFXDefinition remains configured"), Attack.ImpactHitVFXDefinition.HasConfiguredSystem());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10VFXDeterministicVariationSelectionTest, "Grimrock.Monsters.MON10.VFX.VFXDeterministicVariationSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10VFXDeterministicVariationSelectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FGuid PersistenceId(1, 2, 3, 4);
	const int32 Seed = FGridMonsterVFXSelector::BuildPresentationSeed(PersistenceId, TEXT("MON10_VFXRat"), EGridMonsterVFXEvent::Attack, 7);
	TestEqual(TEXT("The same inputs produce the same seed"),
		FGridMonsterVFXSelector::BuildPresentationSeed(PersistenceId, TEXT("MON10_VFXRat"), EGridMonsterVFXEvent::Attack, 7), Seed);
	TestEqual(TEXT("The same seed produces the same index"), FGridMonsterVFXSelector::SelectVariationIndex(Seed, 4),
		FGridMonsterVFXSelector::SelectVariationIndex(Seed, 4));

	FRandomStream CombatRandomStream(1337);
	const int32 CombatSeedBefore = CombatRandomStream.GetCurrentSeed();
	TSet<int32> SelectedIndices;
	for (int32 Occurrence = 1; Occurrence <= 64; ++Occurrence)
	{
		const int32 Index = FGridMonsterVFXSelector::SelectVariationIndex(
			FGridMonsterVFXSelector::BuildPresentationSeed(PersistenceId, TEXT("MON10_VFXRat"), EGridMonsterVFXEvent::Attack, Occurrence), 5);
		TestTrue(TEXT("Every index stays in range"), Index >= 0 && Index < 5);
		SelectedIndices.Add(Index);
	}
	TestTrue(TEXT("Occurrences can select different variants"), SelectedIndices.Num() > 1);
	TestEqual(TEXT("CombatRandomStream is not consumed"), CombatRandomStream.GetCurrentSeed(), CombatSeedBefore);

	UGridTurnManagerComponent* TurnManager = NewObject<UGridTurnManagerComponent>();
	const int32 EncounterSeedBefore = TurnManager->EncounterRandomSeed;
	FGridMonsterVFXSelector::SelectVariationIndex(Seed, 3);
	TestEqual(TEXT("EncounterRandomSeed stays unchanged"), TurnManager->EncounterRandomSeed, EncounterSeedBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10VFXCombatStartAlertExactlyOnceTest, "Grimrock.Monsters.MON10.VFX.VFXCombatStartAlertExactlyOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10VFXCombatStartAlertExactlyOnceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10VFXTestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	ConfigureMON10VFXFloor(Runtime);
	AGrimrockPartyPawn* Party = MakeMON10VFXParty(TestWorld.World, Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON10VFXDefinition(Runtime);
	Definition->AlertVFX = MakeMON10VFXEvent(Definition, TEXT("MON10VFXAlert"));

	AGridMonsterActor* First =
		SpawnMON10VFXMonster(TestWorld.World, Runtime, Definition, FGuid(10, 1, 1, 1), FIntPoint(1, 1), TEXT("MON10VFXAlertFirst"), true);
	AGridMonsterActor* Second =
		SpawnMON10VFXMonster(TestWorld.World, Runtime, Definition, FGuid(10, 1, 1, 2), FIntPoint(2, 1), TEXT("MON10VFXAlertSecond"), true);
	AGridMonsterActor* Dead = SpawnMON10VFXMonster(TestWorld.World, Runtime, Definition, FGuid(10, 1, 1, 3), FIntPoint(3, 1), TEXT("MON10VFXAlertDead"));
	AGridMonsterActor* Disabled =
		SpawnMON10VFXMonster(TestWorld.World, Runtime, Definition, FGuid(10, 1, 1, 4), FIntPoint(4, 1), TEXT("MON10VFXAlertDisabled"));
	if (!Party || !First || !Second || !Dead || !Disabled)
	{
		return false;
	}
	Dead->CurrentHealth = 0;
	Dead->MonsterState = EGridMonsterState::Dead;
	Disabled->bMonsterEnabled = false;

	UGridTurnManagerComponent* TurnManager = NewObject<UGridTurnManagerComponent>(Runtime);
	TurnManager->bAutoInitialize = false;
	Runtime->AddInstanceComponent(TurnManager);
	TurnManager->RegisterComponent();
	if (!TurnManager->InitializeTurnManager(Runtime, Party))
	{
		return false;
	}

	TestTrue(TEXT("A valid encounter starts"), TurnManager->StartCombatWithAllMonsters());
	TestEqual(TEXT("First participant alerts once"), First->VFXComponent->GetSpawnRequestCountForEvent(EGridMonsterVFXEvent::Alert), 1);
	TestEqual(TEXT("Second participant alerts once"), Second->VFXComponent->GetSpawnRequestCountForEvent(EGridMonsterVFXEvent::Alert), 1);
	TestEqual(TEXT("Dead monster does not alert"), Dead->VFXComponent->GetSpawnRequestCountForEvent(EGridMonsterVFXEvent::Alert), 0);
	TestEqual(TEXT("Disabled monster does not alert"), Disabled->VFXComponent->GetSpawnRequestCountForEvent(EGridMonsterVFXEvent::Alert), 0);
	TestFalse(TEXT("A second start is refused"), TurnManager->StartCombatWithAllMonsters());
	TestEqual(TEXT("A refused start adds no alert"), First->VFXComponent->GetSpawnRequestCountForEvent(EGridMonsterVFXEvent::Alert), 1);
	TurnManager->AbortCombat();
	TestTrue(TEXT("A distinct encounter starts"), TurnManager->StartCombatWithAllMonsters());
	TestEqual(TEXT("A distinct encounter alerts again"), First->VFXComponent->GetSpawnRequestCountForEvent(EGridMonsterVFXEvent::Alert), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10VFXAttackAndImpactExactlyOnceTest, "Grimrock.Monsters.MON10.VFX.VFXAttackAndImpactExactlyOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10VFXAttackAndImpactExactlyOnceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10VFXTestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	ConfigureMON10VFXFloor(Runtime);
	AGrimrockPartyPawn* Party = MakeMON10VFXParty(TestWorld.World, Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON10VFXDefinition(Runtime);
	FGridMonsterAttackDefinition& Attack = Definition->Attacks[0];
	Attack.AttackVFXDefinition = MakeMON10VFXEvent(Definition, TEXT("MON10VFXAttack"), true);
	Attack.ImpactHitVFXDefinition = MakeMON10VFXEvent(Definition, TEXT("MON10VFXImpactHit"));
	Attack.ImpactMissVFXDefinition = MakeMON10VFXEvent(Definition, TEXT("MON10VFXImpactMiss"));
	Attack.ImpactHitAudio = MakeMON10VFXAudioEvent(Definition, TEXT("MON10VFXImpactAudio"));
	Attack.ImpactMissAudio = MakeMON10VFXAudioEvent(Definition, TEXT("MON10VFXMissAudio"));
	AGridMonsterActor* Monster = SpawnMON10VFXMonster(TestWorld.World, Runtime, Definition, FGuid(20, 1, 1, 1), FIntPoint(1, 1), TEXT("MON10VFXAttackRat"));
	UGridTurnManagerComponent* TurnManager = NewObject<UGridTurnManagerComponent>(Runtime);
	if (!Party || !Monster || !TurnManager || !Monster->CombatComponent->InitializeCombat(Party) || !TurnManager->InitializeTurnManager(Runtime, Party))
	{
		return false;
	}

	FGridCombatAction Action;
	Action.Type = EGridCombatActionType::MeleeAttack;
	Action.TargetCharacterIndex = 0;
	Action.AttackId = Attack.AttackId;
	TestTrue(TEXT("Attack presentation starts without montage"), Monster->CombatComponent->StartAttackPresentation(Action, Attack));
	TestFalse(TEXT("Active presentation cannot start twice"), Monster->CombatComponent->StartAttackPresentation(Action, Attack));
	TestEqual(TEXT("Attack VFX is requested once"), Monster->VFXComponent->GetSpawnRequestCountForEvent(EGridMonsterVFXEvent::Attack), 1);

	TurnManager->CurrentMonster = Monster;
	TurnManager->CurrentCombatComponent = Monster->CombatComponent;
	TurnManager->ActiveAttackDefinition = Attack;
	TurnManager->ActiveAction = Action;
	TurnManager->bHasActiveAction = true;
	TurnManager->bActiveAttackImpactCommitted = false;
	TurnManager->CombatRandomStream.Initialize(1337);
	const int32 HealthBefore = Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth;
	TurnManager->CommitActiveAttackImpact();
	const int32 HealthAfterFirst = Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth;
	TurnManager->CommitActiveAttackImpact();

	const int32 ImpactVFXCount = Monster->VFXComponent->GetSpawnRequestCountForEvent(EGridMonsterVFXEvent::ImpactHit) +
		Monster->VFXComponent->GetSpawnRequestCountForEvent(EGridMonsterVFXEvent::ImpactMiss);
	const int32 ImpactAudioCount = Monster->AudioComponent->GetPlaybackRequestCountForEvent(EGridMonsterAudioEvent::ImpactHit) +
		Monster->AudioComponent->GetPlaybackRequestCountForEvent(EGridMonsterAudioEvent::ImpactMiss);
	TestEqual(TEXT("Impact VFX remains unique"), ImpactVFXCount, 1);
	TestEqual(TEXT("Impact audio remains unique"), ImpactAudioCount, 1);
	TestTrue(TEXT("The attack resolution ran"), HealthAfterFirst <= HealthBefore);
	TestEqual(TEXT("Damage is not applied twice"), Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth,
		HealthAfterFirst);
	TestEqual(TEXT("OnAttackResolved remains unique"), TurnManager->AttackResolvedBroadcastCount, 1);
	TestEqual(TEXT("GridCombat attack entry remains unique"), CountMON10VFXAttackEntries(TurnManager), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10VFXHurtDeathExclusivityTest, "Grimrock.Monsters.MON10.VFX.VFXHurtDeathExclusivity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10VFXHurtDeathExclusivityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10VFXTestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	ConfigureMON10VFXFloor(Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON10VFXDefinition(Runtime);
	Definition->HurtVFX = MakeMON10VFXEvent(Definition, TEXT("MON10VFXHurt"), true);
	Definition->DeathVFX = MakeMON10VFXEvent(Definition, TEXT("MON10VFXDeath"), true);
	AGridMonsterActor* Monster =
		SpawnMON10VFXMonster(TestWorld.World, Runtime, Definition, FGuid(30, 1, 1, 1), FIntPoint(2, 2), TEXT("MON10VFXHurtDeathRat"), true);
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
	TestEqual(TEXT("Non-fatal damage requests Hurt"), Monster->VFXComponent->GetSpawnRequestCountForEvent(EGridMonsterVFXEvent::Hurt), 1);

	FGridAttackResult Fatal;
	Fatal.bHit = true;
	Fatal.HealthDamage = 100;
	Monster->ApplyAttackResult(Fatal);
	TestEqual(TEXT("Fatal damage requests Death once"), Monster->VFXComponent->GetSpawnRequestCountForEvent(EGridMonsterVFXEvent::Death), 1);
	TestEqual(TEXT("Fatal damage adds no Hurt"), Monster->VFXComponent->GetSpawnRequestCountForEvent(EGridMonsterVFXEvent::Hurt), 1);
	const int32 LogicalDeaths = Monster->DeathComponent->LogicalDeathEventCount;
	const int32 GeneratedLoot = Monster->DeathComponent->GeneratedLoot.Num();
	Monster->MarkDead();
	TestEqual(TEXT("Second MarkDead adds no Death VFX"), Monster->VFXComponent->GetSpawnRequestCountForEvent(EGridMonsterVFXEvent::Death), 1);
	TestEqual(TEXT("MonsterDied remains unique"), Monster->DeathComponent->LogicalDeathEventCount, LogicalDeaths);
	TestEqual(TEXT("Loot remains unique"), Monster->DeathComponent->GeneratedLoot.Num(), GeneratedLoot);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10VFXRestoreDeadSilentTest, "Grimrock.Monsters.MON10.VFX.VFXRestoreDeadSilent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10VFXRestoreDeadSilentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10VFXTestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	ConfigureMON10VFXFloor(Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON10VFXDefinition(Runtime);
	Definition->AlertVFX = MakeMON10VFXEvent(Definition, TEXT("MON10VFXRestoreAlert"));
	Definition->HurtVFX = MakeMON10VFXEvent(Definition, TEXT("MON10VFXRestoreHurt"));
	Definition->DeathVFX = MakeMON10VFXEvent(Definition, TEXT("MON10VFXRestoreDeath"));
	const FGuid PersistenceId(40, 1, 1, 1);
	AGridMonsterActor* Monster = SpawnMON10VFXMonster(TestWorld.World, Runtime, Definition, PersistenceId, FIntPoint(2, 2), TEXT("MON10VFXRestoreRat"), true);
	if (!Monster)
	{
		return false;
	}
	Monster->VFXComponent->PlayAlertVFX();

	FGridRuntimeMonsterState State;
	State.PersistenceId = PersistenceId;
	State.SpawnObjectId = PersistenceId;
	State.MonsterDefinitionId = Definition->MonsterId;
	State.DungeonLevelId = TEXT("MON10VFXLevel");
	State.CellX = 2;
	State.CellY = 2;
	State.Facing = EGridEdge::North;
	State.MonsterState = EGridMonsterState::Dead;
	State.CurrentHealth = 0;
	State.bMonsterEnabled = true;
	State.bIsDead = true;

	TestTrue(TEXT("The MON9 dead state restores"), Monster->RestoreRuntimeMonsterState(State, Runtime));
	TestTrue(TEXT("The restored monster remains dead"), Monster->IsDead());
	TestEqual(TEXT("Restore emits no VFX requests"), Monster->VFXComponent->SpawnRequestCount, 0);
	TestEqual(TEXT("Restore keeps no native Niagara"), Monster->VFXComponent->GetActiveNiagaraComponentCount(), 0);
	TestTrue(TEXT("Loot remains committed"), Monster->DeathComponent->bLootGenerated);
	TestEqual(TEXT("No loot is regenerated"), Monster->DeathComponent->GeneratedLoot.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10VFXRequestDataAndNativeSpawnDisabledTest, "Grimrock.Monsters.MON10.VFX.VFXRequestDataAndNativeSpawnDisabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10VFXRequestDataAndNativeSpawnDisabledTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10VFXTestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	ConfigureMON10VFXFloor(Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON10VFXDefinition(Runtime);
	FGridMonsterAttackDefinition& Attack = Definition->Attacks[0];
	Attack.DamageType = EGridDamageType::Physical;
	Attack.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
	Attack.ImpactHitVFXDefinition = MakeMON10VFXEvent(Definition, TEXT("MON10VFXRequest"));
	AGridMonsterActor* Monster = SpawnMON10VFXMonster(TestWorld.World, Runtime, Definition, FGuid(50, 1, 1, 1), FIntPoint(2, 2), TEXT("MON10VFXRequestRat"));
	if (!Monster)
	{
		return false;
	}

	FGridAttackResult Result;
	Result.bHit = true;
	Result.bCriticalHit = true;
	Result.HealthDamage = 5;
	Result.DamageType = Attack.DamageType;
	Result.PhysicalSubtype = Attack.PhysicalSubtype;
	TestTrue(TEXT("Native-disabled impact still requests VFX"), Monster->VFXComponent->PlayAttackImpactVFX(Attack, Result, FVector(100.0, 200.0, 300.0), 0));

	const FGridMonsterVFXSpawnRequest& Request = Monster->VFXComponent->LastSpawnRequest;
	TestEqual(TEXT("One request is counted"), Monster->VFXComponent->SpawnRequestCount, 1);
	TestEqual(TEXT("Delegate broadcast is counted once"), Monster->VFXComponent->VFXSpawnBroadcastCount, 1);
	TestEqual(TEXT("MonsterId is exposed"), Request.MonsterId, Definition->MonsterId);
	TestEqual(TEXT("AttackId is exposed"), Request.AttackId, Attack.AttackId);
	TestEqual(TEXT("Target index is exposed"), Request.TargetCharacterIndex, 0);
	TestEqual(TEXT("Damage type is exposed"), Request.DamageType, Attack.DamageType);
	TestEqual(TEXT("Physical subtype is exposed"), Request.PhysicalSubtype, Attack.PhysicalSubtype);
	TestTrue(TEXT("Attack result is present"), Request.bHasAttackResult);
	TestTrue(TEXT("Critical flag is preserved"), Request.AttackResult.bCriticalHit);
	TestEqual(TEXT("Applied damage is preserved"), Request.AttackResult.GetTotalAppliedDamage(), 5);
	TestFalse(TEXT("Impact is always world-space"), Request.bAttachToSource);
	TestEqual(TEXT("No native Niagara component is created"), Monster->VFXComponent->GetActiveNiagaraComponentCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10VFXNoPersistenceNoTickAndCleanupTest, "Grimrock.Monsters.MON10.VFX.VFXNoPersistenceNoTickAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10VFXNoPersistenceNoTickAndCleanupTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10VFXTestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	ConfigureMON10VFXFloor(Runtime);
	UGridMonsterDefinitionAsset* Definition = MakeMON10VFXDefinition(Runtime);
	Definition->AlertVFX = MakeMON10VFXEvent(Definition, TEXT("MON10VFXCleanup"));
	AGridMonsterActor* Monster =
		SpawnMON10VFXMonster(TestWorld.World, Runtime, Definition, FGuid(60, 1, 1, 1), FIntPoint(2, 2), TEXT("MON10VFXCleanupRat"), true);
	if (!Monster)
	{
		return false;
	}

	TestFalse(TEXT("The VFX component never ticks"), Monster->VFXComponent->PrimaryComponentTick.bCanEverTick);
	for (TFieldIterator<FProperty> Property(FGridMonsterVFXSpawnRequest::StaticStruct()); Property; ++Property)
	{
		TestFalse(*FString::Printf(TEXT("%s is not SaveGame data"), *Property->GetName()), Property->HasAnyPropertyFlags(CPF_SaveGame));
	}
	TestNull(TEXT("MON9 state has no VFX request"), FindFProperty<FProperty>(FGridRuntimeMonsterState::StaticStruct(), TEXT("LastSpawnRequest")));

	Monster->VFXComponent->PlayAlertVFX();
	Monster->VFXComponent->StopAllMonsterVFX();
	TestEqual(TEXT("Stop clears active Niagara references"), Monster->VFXComponent->GetActiveNiagaraComponentCount(), 0);
	Runtime->SetMonsterRuntimeLevelActive(Monster, false);
	TestEqual(TEXT("Runtime deactivation leaves no active Niagara"), Monster->VFXComponent->GetActiveNiagaraComponentCount(), 0);
	Monster->VFXComponent->BeginPlay();
	Monster->VFXComponent->EndPlay(EEndPlayReason::Destroyed);
	TestEqual(TEXT("EndPlay leaves no active Niagara"), Monster->VFXComponent->GetActiveNiagaraComponentCount(), 0);
	return true;
}

#endif
