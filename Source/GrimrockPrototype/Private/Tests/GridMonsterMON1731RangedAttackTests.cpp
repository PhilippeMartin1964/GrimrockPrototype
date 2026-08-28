#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/Monsters/GridMonsterPathfinder.h"
#include "Runtime/Monsters/GridMonsterRangedAttackPlanner.h"

namespace
{
	FGridMonsterAttackDefinition MakeThrowKnifeAttack()
	{
		FGridMonsterAttackDefinition Attack;
		Attack.AttackId = TEXT("Attack_ThrowKnife");
		Attack.DisplayName = FText::FromString(TEXT("Couteau lance"));
		Attack.DamageType = EGridDamageType::Physical;
		Attack.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
		Attack.MinDamage = 2;
		Attack.MaxDamage = 5;
		Attack.MinRangeCells = 2;
		Attack.MaxRangeCells = 6;
		Attack.Delivery = EGridMonsterAttackDelivery::Projectile;
		Attack.bRequiresLineOfSight = true;
		Attack.ActionPointCost = 2;
		Attack.Priority = 100;
		return Attack;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1731StationaryRangedPlannerTest, "Grimrock.Monsters.MON17.3.1.StationaryRangedPlanner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1731StationaryRangedPlannerTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FGuid GoblinId = FGuid::NewGuid();
	const FGridMonsterAttackDefinition Attack = MakeThrowKnifeAttack();
	TArray<FGridCombatAction> Actions;

	TestTrue(TEXT("Distance two with LOS is a legal firing position"),
		FGridMonsterRangedAttackPlanner::BuildStationaryRangedTurn(GoblinId, FIntPoint(1, 1), EGridEdge::East, FIntPoint(3, 1), 3, Attack, true, Actions));
	TestEqual(TEXT("Already facing the target produces one action"), Actions.Num(), 1);
	if (Actions.Num() == 1)
	{
		TestEqual(TEXT("The action is RangedAttack"), Actions[0].Type, EGridCombatActionType::RangedAttack);
		TestEqual(TEXT("The ranged action uses Attack_ThrowKnife"), Actions[0].AttackId, Attack.AttackId);
		TestEqual(TEXT("The ranged action costs two AP"), Actions[0].ActionPointCost, 2);
	}

	TestTrue(TEXT("A stationary ranged monster may turn before firing"),
		FGridMonsterRangedAttackPlanner::BuildStationaryRangedTurn(GoblinId, FIntPoint(1, 1), EGridEdge::North, FIntPoint(4, 1), 3, Attack, true, Actions));
	TestEqual(TEXT("North to east produces Turn then RangedAttack"), Actions.Num(), 2);
	if (Actions.Num() == 2)
	{
		TestEqual(TEXT("The first action is Turn"), Actions[0].Type, EGridCombatActionType::Turn);
		TestEqual(TEXT("The second action is RangedAttack"), Actions[1].Type, EGridCombatActionType::RangedAttack);
	}

	TestFalse(TEXT("Distance one is below the authored minimum"),
		FGridMonsterRangedAttackPlanner::BuildStationaryRangedTurn(GoblinId, FIntPoint(1, 1), EGridEdge::East, FIntPoint(2, 1), 3, Attack, true, Actions));
	TestTrue(TEXT("Failed planning leaves no actions"), Actions.IsEmpty());

	TestFalse(TEXT("Distance seven is beyond the authored maximum"),
		FGridMonsterRangedAttackPlanner::BuildStationaryRangedTurn(GoblinId, FIntPoint(1, 1), EGridEdge::East, FIntPoint(8, 1), 3, Attack, true, Actions));

	TestFalse(TEXT("A diagonal target is not a legal axial shot"),
		FGridMonsterRangedAttackPlanner::BuildStationaryRangedTurn(GoblinId, FIntPoint(1, 1), EGridEdge::East, FIntPoint(3, 2), 3, Attack, true, Actions));

	TestFalse(TEXT("One AP cannot pay a two AP ranged attack"),
		FGridMonsterRangedAttackPlanner::BuildStationaryRangedTurn(GoblinId, FIntPoint(1, 1), EGridEdge::East, FIntPoint(3, 1), 1, Attack, true, Actions));

	TestFalse(TEXT("A LOS-required attack is rejected when LOS is false"),
		FGridMonsterRangedAttackPlanner::BuildStationaryRangedTurn(GoblinId, FIntPoint(1, 1), EGridEdge::East, FIntPoint(3, 1), 3, Attack, false, Actions));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON1731LineOfSightTest, "Grimrock.Monsters.MON17.3.1.LineOfSight", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1731LineOfSightTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FIntPoint Source(1, 1);
	const FIntPoint Target(4, 1);

	TestTrue(TEXT("An open axial corridor has ranged LOS"),
		FGridMonsterPerception::HasStraightLineOfSight(Source, Target, 6,
			[](const FIntPoint& From, const FIntPoint& To)
			{
				(void)From;
				(void)To;
				return true;
			}));

	TestFalse(TEXT("One blocked crossed edge breaks ranged LOS"),
		FGridMonsterPerception::HasStraightLineOfSight(Source, Target, 6,
			[](const FIntPoint& From, const FIntPoint& To)
			{
				return !(From == FIntPoint(2, 1) && To == FIntPoint(3, 1));
			}));

	TestFalse(TEXT("Diagonal cells do not have straight grid LOS"),
		FGridMonsterPerception::HasStraightLineOfSight(Source, FIntPoint(3, 2), 6,
			[](const FIntPoint& From, const FIntPoint& To)
			{
				(void)From;
				(void)To;
				return true;
			}));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMonsterMON1731MeleeRegressionPlannerTest, "Grimrock.Monsters.MON17.3.1.MeleeRegressionPlanner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1731MeleeRegressionPlannerTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TArray<FGridCombatAction> Actions;
	const FName BiteId(TEXT("Attack_Bite"));
	FGridMonsterTurnPlanner::BuildDirectMeleeTurn(
		FGuid::NewGuid(), FIntPoint(1, 1), EGridEdge::East, FIntPoint(2, 1), TArray<FIntPoint>(), 2, BiteId, 1, Actions);

	TestEqual(TEXT("Existing adjacent melee planning still emits one attack"), Actions.Num(), 1);
	if (Actions.Num() == 1)
	{
		TestEqual(TEXT("Rat melee remains MeleeAttack"), Actions[0].Type, EGridCombatActionType::MeleeAttack);
		TestEqual(TEXT("Rat melee keeps Attack_Bite"), Actions[0].AttackId, BiteId);
	}

	return true;
}

#endif
