#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON6CombatResolverTest, "Grimrock.Monsters.MON6.CombatResolver", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON6CombatResolverTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridAttackSourceStats Source;
	Source.Accuracy = 100;
	Source.DamageBonus = 1;

	FGridAttackTargetStats Target;
	Target.Evasion = 0;
	Target.CurrentHealth = 10;
	Target.PhysicalArmor = 3;

	FGridAttackDefinition Attack;
	Attack.DamageType = EGridDamageType::Physical;
	Attack.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
	Attack.MinDamage = 4;
	Attack.MaxDamage = 4;

	FGridAttackResult Result = FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Attack, 1, 4);
	TestFalse(TEXT("A natural one always misses"), Result.bHit);
	TestEqual(TEXT("A miss applies no damage"), Result.GetTotalAppliedDamage(), 0);

	Source.Accuracy = -100;
	Result = FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Attack, 20, 4);
	TestTrue(TEXT("A natural twenty always hits"), Result.bHit);
	TestTrue(TEXT("A natural twenty is critical"), Result.bCriticalHit);
	TestEqual(TEXT("Critical damage doubles roll plus bonus"), Result.RawDamage, 10);
	TestEqual(TEXT("Physical armor absorbs first"), Result.PhysicalArmorDamage, 3);
	TestEqual(TEXT("Remaining physical damage reaches health"), Result.HealthDamage, 7);
	TestEqual(TEXT("Health is clamped after damage"), Result.TargetHealthAfter, 3);

	Source.Accuracy = 10;
	Source.DamageBonus = 0;
	Target.CurrentHealth = 10;
	Target.PhysicalArmor = 3;
	Target.MagicalArmor = 0;
	Target.ResistancePercent = 0;
	Attack.MinDamage = 5;
	Attack.MaxDamage = 5;
	Result = FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Attack, 10, 5);
	TestEqual(TEXT("Three physical armor points are removed"), Result.PhysicalArmorDamage, 3);
	TestEqual(TEXT("Two physical damage points spill to health"), Result.HealthDamage, 2);

	Attack.DamageType = EGridDamageType::Fire;
	Attack.PhysicalSubtype = EGridPhysicalDamageSubtype::None;
	Attack.MinDamage = 4;
	Attack.MaxDamage = 4;
	Target.PhysicalArmor = 9;
	Target.MagicalArmor = 2;
	Result = FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Attack, 10, 4);
	TestEqual(TEXT("Elemental damage ignores physical armor"), Result.PhysicalArmorDamage, 0);
	TestEqual(TEXT("Elemental damage consumes magical armor"), Result.MagicalArmorDamage, 2);
	TestEqual(TEXT("Elemental overflow reaches health"), Result.HealthDamage, 2);

	Attack.DamageType = EGridDamageType::Physical;
	Attack.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
	Attack.MinDamage = 8;
	Attack.MaxDamage = 8;
	Target.PhysicalArmor = 0;
	Target.MagicalArmor = 0;
	Target.ResistancePercent = 50;
	Result = FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Attack, 10, 8);
	TestEqual(TEXT("Fifty percent resistance halves damage"), Result.DamageAfterModifiers, 4);

	Target.ResistancePercent = -50;
	Result = FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Attack, 10, 8);
	TestEqual(TEXT("Negative resistance represents vulnerability"), Result.DamageAfterModifiers, 12);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON6PartyTargetSelectorTest, "Grimrock.Monsters.MON6.PartyTargetSelector",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON6PartyTargetSelectorTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridPartyInventoryState Party;
	Party.ActiveCharacters.SetNum(5);
	for (FGridCharacterInventoryState& Character : Party.ActiveCharacters)
	{
		Character.DerivedStats.CurrentHealth = 0;
	}

	Party.ActiveCharacters[1].DerivedStats.CurrentHealth = 5;
	Party.ActiveCharacters[3].DerivedStats.CurrentHealth = 5;

	FRandomStream RandomStream(42);
	TestEqual(TEXT("A living front-line character is preferred"), FGridPartyTargetSelector::SelectTarget(Party, RandomStream, 3), 1);

	Party.ActiveCharacters[1].DerivedStats.CurrentHealth = 0;
	RandomStream.Initialize(42);
	TestEqual(TEXT("The second line is used when the front line is defeated"), FGridPartyTargetSelector::SelectTarget(Party, RandomStream, 3), 3);

	Party.ActiveCharacters[3].DerivedStats.CurrentHealth = 0;
	RandomStream.Initialize(42);
	TestEqual(TEXT("No living character produces no target"), FGridPartyTargetSelector::SelectTarget(Party, RandomStream, 3), INDEX_NONE);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON6DirectMeleePlannerTest, "Grimrock.Monsters.MON6.DirectMeleePlanner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON6DirectMeleePlannerTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FGuid RatId = FGuid::NewGuid();
	const FName BiteId(TEXT("Attack_Bite"));
	TArray<FGridCombatAction> Actions;

	FGridMonsterTurnPlanner::BuildDirectMeleeTurn(RatId, FIntPoint(1, 1), EGridEdge::North, FIntPoint(2, 1), TArray<FIntPoint>(), 2, BiteId, 1, Actions);
	TestEqual(TEXT("An adjacent rat turns then bites"), Actions.Num(), 2);
	if (Actions.Num() == 2)
	{
		TestEqual(TEXT("The first adjacent action is Turn"), Actions[0].Type, EGridCombatActionType::Turn);
		TestEqual(TEXT("The second adjacent action is MeleeAttack"), Actions[1].Type, EGridCombatActionType::MeleeAttack);
		TestEqual(TEXT("The bite uses Attack_Bite"), Actions[1].AttackId, BiteId);
	}

	const TArray<FIntPoint> OneStepPath = { FIntPoint(2, 1) };
	FGridMonsterTurnPlanner::BuildDirectMeleeTurn(RatId, FIntPoint(1, 1), EGridEdge::East, FIntPoint(3, 1), OneStepPath, 2, BiteId, 1, Actions);
	TestEqual(TEXT("A rat two cells away moves then bites"), Actions.Num(), 2);
	if (Actions.Num() == 2)
	{
		TestEqual(TEXT("Distance two starts with Move"), Actions[0].Type, EGridCombatActionType::Move);
		TestEqual(TEXT("Distance two ends with MeleeAttack"), Actions[1].Type, EGridCombatActionType::MeleeAttack);
	}

	const TArray<FIntPoint> TwoStepPath = { FIntPoint(2, 1), FIntPoint(3, 1) };
	FGridMonsterTurnPlanner::BuildDirectMeleeTurn(RatId, FIntPoint(1, 1), EGridEdge::East, FIntPoint(4, 1), TwoStepPath, 2, BiteId, 1, Actions);
	TestEqual(TEXT("A distant rat spends both points moving"), Actions.Num(), 2);
	if (Actions.Num() == 2)
	{
		TestEqual(TEXT("First distant action is Move"), Actions[0].Type, EGridCombatActionType::Move);
		TestEqual(TEXT("Second distant action is Move"), Actions[1].Type, EGridCombatActionType::Move);
	}

	return true;
}

#endif
