#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Monsters/GridMonsterPathfinder.h"
#include "Runtime/Monsters/GridMonsterRangedKeeperPlanner.h"

namespace
{
	FGridMonsterAttackDefinition MakeMON1741ThrowAttack()
	{
		FGridMonsterAttackDefinition Attack;
		Attack.AttackId = TEXT("Attack_ThrowKnife");
		Attack.DisplayName = FText::FromString(TEXT("Throw Knife"));
		Attack.DamageType = EGridDamageType::Physical;
		Attack.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
		Attack.MinDamage = 2;
		Attack.MaxDamage = 5;
		Attack.MinRangeCells = 2;
		Attack.RangeCells = 6;
		Attack.Delivery = EGridMonsterAttackDelivery::Projectile;
		Attack.bRequiresLineOfSight = true;
		Attack.ActionPointCost = 2;
		Attack.Priority = 100;
		return Attack;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1741PreferredFiringCandidatesTest, "Grimrock.Monsters.MON17.4.1.PreferredFiringCandidates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1741PreferredFiringCandidatesTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FIntPoint PartyCell(10, 10);
	TArray<FIntPoint> Candidates;
	FGridMonsterRangedKeeperPlanner::BuildAxialFiringCandidates(PartyCell, 3, 5, Candidates);

	TestEqual(TEXT("Three preferred distances produce twelve cardinal candidates"), Candidates.Num(), 12);
	TestTrue(TEXT("North distance three exists"), Candidates.Contains(FIntPoint(10, 13)));
	TestTrue(TEXT("East distance four exists"), Candidates.Contains(FIntPoint(14, 10)));
	TestTrue(TEXT("South distance five exists"), Candidates.Contains(FIntPoint(10, 5)));
	TestTrue(TEXT("West distance three exists"), Candidates.Contains(FIntPoint(7, 10)));

	for (const FIntPoint& Candidate : Candidates)
	{
		const int32 Distance = FGridMonsterPathfinder::ManhattanDistance(Candidate, PartyCell);
		TestTrue(TEXT("Every generated candidate lies inside the preferred band"), FGridMonsterRangedKeeperPlanner::IsPreferredDistance(Distance, 3, 5));
		TestTrue(TEXT("Every generated candidate is axial to the party"), Candidate.X == PartyCell.X || Candidate.Y == PartyCell.Y);
	}

	TestFalse(TEXT("Distance two is below the preferred band"), FGridMonsterRangedKeeperPlanner::IsPreferredDistance(2, 3, 5));
	TestTrue(TEXT("Distance five is inside the preferred band"), FGridMonsterRangedKeeperPlanner::IsPreferredDistance(5, 3, 5));
	TestFalse(TEXT("Distance six is above the preferred band"), FGridMonsterRangedKeeperPlanner::IsPreferredDistance(6, 3, 5));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1741RepositionThenAttackTest, "Grimrock.Monsters.MON17.4.1.RepositionThenAttack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1741RepositionThenAttackTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FGridMonsterAttackDefinition Attack = MakeMON1741ThrowAttack();
	const FGuid GoblinId = FGuid::NewGuid();
	const TArray<FIntPoint> Path = { FIntPoint(2, 1) };
	TArray<FGridCombatAction> Actions;

	TestTrue(TEXT("One reposition cell plus two attack AP builds a complete turn"),
		FGridMonsterRangedKeeperPlanner::BuildRepositionTurn(
			GoblinId, FIntPoint(1, 1), EGridEdge::East, FIntPoint(5, 1), Path, 3, Attack, true, true, Actions));

	TestEqual(TEXT("Move then ranged attack"), Actions.Num(), 2);
	if (Actions.Num() == 2)
	{
		TestEqual(TEXT("First action is Move"), Actions[0].Type, EGridCombatActionType::Move);
		TestTrue(TEXT("Move is marked as repositioning"), Actions[0].bIsRepositioningAction);
		TestEqual(TEXT("Second action is RangedAttack"), Actions[1].Type, EGridCombatActionType::RangedAttack);
		TestEqual(TEXT("Attack id is preserved"), Actions[1].AttackId, Attack.AttackId);
		TestEqual(TEXT("Move and attack consume all three AP"), Actions[0].ActionPointCost + Actions[1].ActionPointCost, 3);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1741MultiTurnApproachTest, "Grimrock.Monsters.MON17.4.1.MultiTurnApproach",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1741MultiTurnApproachTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FGridMonsterAttackDefinition Attack = MakeMON1741ThrowAttack();
	const FGuid GoblinId = FGuid::NewGuid();
	TArray<FGridCombatAction> Actions;

	const TArray<FIntPoint> TwoStepPath = { FIntPoint(2, 1), FIntPoint(3, 1) };
	TestTrue(TEXT("A two-cell approach still produces movement actions"),
		FGridMonsterRangedKeeperPlanner::BuildRepositionTurn(
			GoblinId, FIntPoint(1, 1), EGridEdge::East, FIntPoint(6, 1), TwoStepPath, 3, Attack, true, true, Actions));
	TestEqual(TEXT("Two moves consume two AP and leave too little AP to throw"), Actions.Num(), 2);
	for (const FGridCombatAction& Action : Actions)
	{
		TestEqual(TEXT("No premature attack is appended"), Action.Type, EGridCombatActionType::Move);
	}

	const TArray<FIntPoint> OneStepPath = { FIntPoint(2, 1) };
	TestTrue(TEXT("A cooling-down monster may still reposition"),
		FGridMonsterRangedKeeperPlanner::BuildRepositionTurn(
			GoblinId, FIntPoint(1, 1), EGridEdge::East, FIntPoint(5, 1), OneStepPath, 3, Attack, true, false, Actions));
	TestEqual(TEXT("Cooldown reposition does not append an attack"), Actions.Num(), 1);
	if (Actions.Num() == 1)
	{
		TestEqual(TEXT("Cooldown action is Move"), Actions[0].Type, EGridCombatActionType::Move);
	}

	return true;
}

#endif
