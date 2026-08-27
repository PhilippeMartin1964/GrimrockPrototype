#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/Combat/GridCombatLog.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "UObject/UnrealType.h"

namespace
{
	struct FGridMON10TestWorld
	{
		UWorld* World = nullptr;

		FGridMON10TestWorld()
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
				FName(*FString::Printf(TEXT("MON10TestWorld_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))), nullptr, true, ERHIFeatureLevel::Num,
				&InitializationValues);
			if (World && GEngine)
			{
				FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
				Context.SetCurrentWorld(World);
			}
		}

		~FGridMON10TestWorld()
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

	UGridMonsterDefinitionAsset* MakeMON10Definition(UObject* Outer, int32 Damage = 3)
	{
		UGridMonsterDefinitionAsset* Definition = NewObject<UGridMonsterDefinitionAsset>(Outer);
		Definition->MonsterId = TEXT("MON10_Rat");
		Definition->DisplayName = FText::FromString(TEXT("Rat géant"));
		Definition->CategoryId = TEXT("Vermin");
		Definition->MaxHealth = 8;
		Definition->Accuracy = 100;
		Definition->ActionPointsPerTurn = 2;

		FGridMonsterAttackDefinition Attack;
		Attack.AttackId = TEXT("Attack_Bite");
		Attack.MinDamage = Damage;
		Attack.MaxDamage = Damage;
		Attack.DamageType = EGridDamageType::Physical;
		Attack.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
		Definition->Attacks.Add(Attack);
		return Definition;
	}

	AGridMonsterActor* MakeMON10Monster(UWorld* World, UGridMonsterDefinitionAsset* Definition)
	{
		AGridMonsterActor* Monster = World ? World->SpawnActor<AGridMonsterActor>() : nullptr;
		if (Monster)
		{
			Monster->InitializeMonster(Definition, FGuid::NewGuid(), FIntPoint(1, 1), EGridEdge::North);
		}
		return Monster;
	}

	AGrimrockPartyPawn* MakeMON10Party(UWorld* World, AGridLevelRuntimeActor* Runtime, int32 Health)
	{
		AGrimrockPartyPawn* Party = World ? World->SpawnActor<AGrimrockPartyPawn>() : nullptr;
		if (!Party || !Party->PartyInventoryComponent)
		{
			return Party;
		}

		Party->LevelRuntimeActor = Runtime;
		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid::NewGuid();
		Character.DisplayName = FText::FromString(TEXT("Elias"));
		Character.DerivedStats.MaxHealth = Health;
		Character.Resources.CurrentHealth = Health;
		Character.Resources.CurrentPhysicalArmor = 0;
		Character.Resources.CurrentMagicalArmor = 0;
		Character.DerivedStats.Evasion = 0;
		Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters = { Character };
		return Party;
	}

	int32 CountEntries(const UGridTurnManagerComponent* TurnManager, EGridCombatLogEntryType Type)
	{
		int32 Count = 0;
		if (TurnManager)
		{
			for (const FGridCombatLogEntry& Entry : TurnManager->CombatLogEntries)
			{
				Count += Entry.Type == Type ? 1 : 0;
			}
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10CombatLogFormatterTest, "Grimrock.Monsters.MON10.CombatLogFormatter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10CombatLogFormatterTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	TestTrue(TEXT("Round feedback contains its number"), FGridCombatLogFormatter::FormatRoundStarted(3).ToString().Contains(TEXT("3")));
	TestFalse(TEXT("Player phase feedback is not empty"), FGridCombatLogFormatter::FormatPhaseChanged(EGridCombatPhase::PlayerPhase).IsEmpty());
	TestFalse(TEXT("Enemy phase feedback is not empty"), FGridCombatLogFormatter::FormatPhaseChanged(EGridCombatPhase::EnemyPhase).IsEmpty());

	const FText Rat = FText::FromString(TEXT("Rat géant"));
	const FText Elias = FText::FromString(TEXT("Elias"));
	FGridAttackResult Miss;
	Miss.AttackRoll = 8;
	Miss.DefenseValue = 12;
	const FString MissText = FGridCombatLogFormatter::FormatMonsterAttack(Rat, Elias, TEXT("Attack_Bite"), Miss).ToString();
	TestTrue(TEXT("Miss feedback contains both participants"), MissText.Contains(TEXT("Rat géant")) && MissText.Contains(TEXT("Elias")));
	TestTrue(TEXT("Miss feedback contains roll and defense"), MissText.Contains(TEXT("8")) && MissText.Contains(TEXT("12")));

	FGridAttackResult Hit;
	Hit.bHit = true;
	Hit.HealthDamage = 4;
	Hit.TargetHealthBefore = 8;
	Hit.TargetHealthAfter = 4;
	const FString HitText = FGridCombatLogFormatter::FormatMonsterAttack(Rat, Elias, TEXT("Attack_Bite"), Hit).ToString();
	TestTrue(TEXT("Hit feedback contains damage and health"), HitText.Contains(TEXT("4")) && HitText.Contains(TEXT("8")));

	Hit.bCriticalHit = true;
	TestTrue(TEXT("Critical feedback is identified"),
		FGridCombatLogFormatter::FormatMonsterAttack(Rat, Elias, TEXT("Attack_Bite"), Hit).ToString().Contains(TEXT("critique")));

	FGridAttackResult ArmorOnly;
	ArmorOnly.bHit = true;
	ArmorOnly.PhysicalArmorDamage = 4;
	ArmorOnly.TargetHealthBefore = 8;
	ArmorOnly.TargetHealthAfter = 8;
	TestTrue(TEXT("Armor-only feedback identifies armor"),
		FGridCombatLogFormatter::FormatMonsterAttack(Rat, Elias, TEXT("Attack_Bite"), ArmorOnly).ToString().Contains(TEXT("armure")));

	FGridAttackResult Split;
	Split.bHit = true;
	Split.PhysicalArmorDamage = 2;
	Split.HealthDamage = 3;
	Split.TargetHealthBefore = 8;
	Split.TargetHealthAfter = 5;
	const FString SplitText = FGridCombatLogFormatter::FormatMonsterAttack(Rat, Elias, TEXT("Attack_Bite"), Split).ToString();
	TestTrue(TEXT("Split feedback contains armor and health damage"),
		SplitText.Contains(TEXT("2")) && SplitText.Contains(TEXT("3")) && SplitText.Contains(TEXT("5")));
	TestFalse(TEXT("Victory feedback is not empty"), FGridCombatLogFormatter::FormatCombatEnded(EGridCombatPhase::Victory).IsEmpty());
	TestFalse(TEXT("Defeat feedback is not empty"), FGridCombatLogFormatter::FormatCombatEnded(EGridCombatPhase::Defeat).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10CombatLogRingBufferTest, "Grimrock.Monsters.MON10.CombatLogRingBuffer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10CombatLogRingBufferTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UGridTurnManagerComponent* TurnManager = NewObject<UGridTurnManagerComponent>();
	TurnManager->MaxCombatLogEntries = 3;
	TurnManager->ClearCombatLog();
	for (int32 Index = 1; Index <= 5; ++Index)
	{
		FGridCombatLogEntry Entry;
		Entry.RoundNumber = Index;
		Entry.Type = EGridCombatLogEntryType::RoundStarted;
		Entry.Message = FGridCombatLogFormatter::FormatRoundStarted(Index);
		TurnManager->AppendCombatLogEntry(Entry);
	}

	TestEqual(TEXT("Only the configured capacity remains"), TurnManager->CombatLogEntries.Num(), 3);
	TestEqual(TEXT("The oldest retained sequence is three"), TurnManager->CombatLogEntries[0].SequenceNumber, 3);
	TestEqual(TEXT("The newest retained sequence is five"), TurnManager->CombatLogEntries[2].SequenceNumber, 5);
	TestEqual(TEXT("Every append broadcasts once"), TurnManager->CombatLogBroadcastCount, 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10CombatLogAttackExactlyOnceTest, "Grimrock.Monsters.MON10.CombatLogAttackExactlyOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10CombatLogAttackExactlyOnceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10TestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	AGrimrockPartyPawn* Party = MakeMON10Party(TestWorld.World, Runtime, 20);
	UGridMonsterDefinitionAsset* Definition = MakeMON10Definition(Runtime, 3);
	AGridMonsterActor* Monster = MakeMON10Monster(TestWorld.World, Definition);
	UGridTurnManagerComponent* TurnManager = NewObject<UGridTurnManagerComponent>(Runtime);
	TestNotNull(TEXT("The attack fixture exists"), Monster);
	if (!Runtime || !Party || !Monster || !TurnManager || !Monster->CombatComponent->InitializeCombat(Party) ||
		!TurnManager->InitializeTurnManager(Runtime, Party))
	{
		return false;
	}

	TurnManager->CurrentMonster = Monster;
	TurnManager->CurrentCombatComponent = Monster->CombatComponent;
	TurnManager->ActiveAttackDefinition = Definition->Attacks[0];
	TurnManager->ActiveAction.Type = EGridCombatActionType::MeleeAttack;
	TurnManager->ActiveAction.AttackId = Definition->Attacks[0].AttackId;
	TurnManager->ActiveAction.TargetCharacterIndex = 0;
	TurnManager->bHasActiveAction = true;
	TurnManager->bActiveAttackImpactCommitted = false;
	TurnManager->CombatRandomStream.Initialize(1337);

	TurnManager->CommitActiveAttackImpact();
	const int32 HealthAfterFirst = Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth;
	TurnManager->CommitActiveAttackImpact();

	TestTrue(TEXT("The guaranteed-accuracy attack hits"), TurnManager->LastAttackResult.bHit);
	TestEqual(TEXT("Damage is applied only once"), Party->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth,
		HealthAfterFirst);
	TestEqual(TEXT("Exactly one attack entry is appended"), CountEntries(TurnManager, EGridCombatLogEntryType::AttackHit), 1);
	TestEqual(TEXT("Attack resolution broadcasts exactly once"), TurnManager->AttackResolvedBroadcastCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10CombatLogCharacterDefeatedTest, "Grimrock.Monsters.MON10.CombatLogCharacterDefeated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10CombatLogCharacterDefeatedTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10TestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	AGrimrockPartyPawn* Party = MakeMON10Party(TestWorld.World, Runtime, 1);
	UGridMonsterDefinitionAsset* Definition = MakeMON10Definition(Runtime, 2);
	AGridMonsterActor* Monster = MakeMON10Monster(TestWorld.World, Definition);
	UGridTurnManagerComponent* TurnManager = NewObject<UGridTurnManagerComponent>(Runtime);
	if (!Runtime || !Party || !Monster || !TurnManager || !Monster->CombatComponent->InitializeCombat(Party) ||
		!TurnManager->InitializeTurnManager(Runtime, Party))
	{
		return false;
	}

	TurnManager->PhaseState.StartCombat();
	TurnManager->PhaseState.BeginRound();
	TurnManager->PhaseState.EndPlayerPhase();
	TurnManager->CurrentPhase = EGridCombatPhase::EnemyPhase;
	TurnManager->RoundNumber = 1;
	TurnManager->bCombatActive = true;
	TurnManager->CombatMonsters.Add(Monster);
	TurnManager->CurrentMonster = Monster;
	TurnManager->CurrentCombatComponent = Monster->CombatComponent;
	TurnManager->ActiveAttackDefinition = Definition->Attacks[0];
	TurnManager->ActiveAction.Type = EGridCombatActionType::MeleeAttack;
	TurnManager->ActiveAction.TargetCharacterIndex = 0;
	TurnManager->ActiveAction.ActionPointCost = 1;
	TurnManager->bHasActiveAction = true;
	TurnManager->CombatRandomStream.Initialize(1337);

	TurnManager->CommitActiveAttackImpact();
	TurnManager->CompleteActiveAction(true);
	TurnManager->CommitActiveAttackImpact();

	TestEqual(TEXT("The terminal sequence contains three entries"), TurnManager->CombatLogEntries.Num(), 3);
	if (TurnManager->CombatLogEntries.Num() == 3)
	{
		TestEqual(TEXT("Damage is reported first"), TurnManager->CombatLogEntries[0].Type, EGridCombatLogEntryType::AttackHit);
		TestEqual(TEXT("Character defeat follows damage"), TurnManager->CombatLogEntries[1].Type, EGridCombatLogEntryType::CharacterDefeated);
		TestEqual(TEXT("Combat defeat is last"), TurnManager->CombatLogEntries[2].Type, EGridCombatLogEntryType::Defeat);
	}
	TestEqual(TEXT("Character defeat is not duplicated"), CountEntries(TurnManager, EGridCombatLogEntryType::CharacterDefeated), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10CombatLogMonsterDefeatedAndVictoryTest, "Grimrock.Monsters.MON10.CombatLogMonsterDefeatedAndVictory",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10CombatLogMonsterDefeatedAndVictoryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10TestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	AGrimrockPartyPawn* Party = MakeMON10Party(TestWorld.World, Runtime, 10);
	UGridMonsterDefinitionAsset* Definition = MakeMON10Definition(Runtime);
	AGridMonsterActor* Monster = MakeMON10Monster(TestWorld.World, Definition);
	UGridTurnManagerComponent* TurnManager = NewObject<UGridTurnManagerComponent>(Runtime);
	if (!Runtime || !Party || !Monster || !TurnManager || !TurnManager->InitializeTurnManager(Runtime, Party))
	{
		return false;
	}

	TurnManager->PhaseState.StartCombat();
	TurnManager->PhaseState.BeginRound();
	TurnManager->CurrentPhase = EGridCombatPhase::PlayerPhase;
	TurnManager->RoundNumber = 1;
	TurnManager->bCombatActive = true;
	TurnManager->CombatMonsters.Add(Monster);
	Monster->CurrentHealth = 0;
	Monster->MonsterState = EGridMonsterState::Dead;

	TurnManager->HandleCombatMonsterDied(Monster, Monster->CurrentCell);
	TurnManager->HandleCombatMonsterDied(Monster, Monster->CurrentCell);

	TestEqual(TEXT("The last monster creates two terminal entries"), TurnManager->CombatLogEntries.Num(), 2);
	if (TurnManager->CombatLogEntries.Num() == 2)
	{
		TestEqual(TEXT("Monster defeat precedes victory"), TurnManager->CombatLogEntries[0].Type, EGridCombatLogEntryType::MonsterDefeated);
		TestEqual(TEXT("Victory follows the last monster death"), TurnManager->CombatLogEntries[1].Type, EGridCombatLogEntryType::Victory);
	}
	TestEqual(TEXT("Monster defeat is unique"), CountEntries(TurnManager, EGridCombatLogEntryType::MonsterDefeated), 1);
	TestEqual(TEXT("Victory is unique"), CountEntries(TurnManager, EGridCombatLogEntryType::Victory), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10CombatLogFailedStartDoesNotClearHistoryTest,
	"Grimrock.Monsters.MON10.CombatLogFailedStartDoesNotClearHistory", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10CombatLogFailedStartDoesNotClearHistoryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridMON10TestWorld TestWorld;
	AGridLevelRuntimeActor* Runtime = TestWorld.World->SpawnActor<AGridLevelRuntimeActor>();
	AGrimrockPartyPawn* Party = MakeMON10Party(TestWorld.World, Runtime, 10);
	UGridTurnManagerComponent* TurnManager = NewObject<UGridTurnManagerComponent>(Runtime);
	if (!Runtime || !Party || !TurnManager || !TurnManager->InitializeTurnManager(Runtime, Party))
	{
		return false;
	}

	FGridCombatLogEntry Existing;
	Existing.Type = EGridCombatLogEntryType::Victory;
	Existing.Message = FGridCombatLogFormatter::FormatCombatEnded(EGridCombatPhase::Victory);
	TurnManager->AppendCombatLogEntry(Existing);
	const bool bStarted = TurnManager->StartCombatInternal(TArray<AGridMonsterActor*>());

	TestFalse(TEXT("An encounter without monsters does not start"), bStarted);
	TestEqual(TEXT("Failed start preserves prior history"), TurnManager->CombatLogEntries.Num(), 1);
	TestEqual(TEXT("Failed start adds no CombatStarted entry"), CountEntries(TurnManager, EGridCombatLogEntryType::CombatStarted), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON10CombatLogNoPersistenceRegressionTest, "Grimrock.Monsters.MON10.CombatLogNoPersistenceRegression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON10CombatLogNoPersistenceRegressionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	for (TFieldIterator<FProperty> Property(FGridCombatLogEntry::StaticStruct()); Property; ++Property)
	{
		TestFalse(*FString::Printf(TEXT("%s is not a SaveGame field"), *Property->GetName()), Property->HasAnyPropertyFlags(CPF_SaveGame));
	}

	const FProperty* RuntimeLogProperty =
		FindFProperty<FProperty>(UGridTurnManagerComponent::StaticClass(), GET_MEMBER_NAME_CHECKED(UGridTurnManagerComponent, CombatLogEntries));
	TestNotNull(TEXT("The runtime log property is reflected"), RuntimeLogProperty);
	if (RuntimeLogProperty)
	{
		TestTrue(TEXT("The runtime log is transient"), RuntimeLogProperty->HasAnyPropertyFlags(CPF_Transient));
		TestFalse(TEXT("The runtime log is not SaveGame data"), RuntimeLogProperty->HasAnyPropertyFlags(CPF_SaveGame));
	}

	TestNull(TEXT("Monster save state has no combat log"), FindFProperty<FProperty>(FGridRuntimeMonsterState::StaticStruct(), TEXT("CombatLogEntries")));
	TestNull(TEXT("Level save state has no combat log"), FindFProperty<FProperty>(FGridLevelRuntimeState::StaticStruct(), TEXT("CombatLogEntries")));
	return true;
}

#endif
