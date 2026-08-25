#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/Monsters/GridMonsterFastHarasserPlanner.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON7RetreatChanceTest, "Grimrock.Monsters.MON7.RetreatChance", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON7RetreatChanceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FRandomStream NeverStream(17);
	TestFalse(TEXT("A zero retreat chance never retreats"), FGridFastHarasserPlanner::ShouldRetreat(0.0f, NeverStream));

	FRandomStream AlwaysStream(17);
	TestTrue(TEXT("A full retreat chance retreats when a candidate exists"), FGridFastHarasserPlanner::ShouldRetreat(1.0f, AlwaysStream));

	FRandomStream FirstStream(1337);
	FRandomStream SecondStream(1337);
	float FirstRoll = 0.0f;
	float SecondRoll = 0.0f;
	const bool bFirstDecision = FGridFastHarasserPlanner::ShouldRetreat(0.40f, FirstStream, &FirstRoll);
	const bool bSecondDecision = FGridFastHarasserPlanner::ShouldRetreat(0.40f, SecondStream, &SecondRoll);
	TestEqual(TEXT("An identical seed produces the same roll"), FirstRoll, SecondRoll);
	TestEqual(TEXT("An identical seed produces the same retreat decision"), bFirstDecision, bSecondDecision);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON7RetreatScoringTest, "Grimrock.Monsters.MON7.RetreatScoring", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON7RetreatScoringTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const auto MakeCandidate = [](FIntPoint Cell, EGridEdge Direction, int32 Distance, int32 ExitCount, bool bCanContinue, bool bCulDeSac, int32 StableOrder)
	{
		FGridRetreatCandidate Candidate;
		Candidate.Cell = Cell;
		Candidate.Direction = Direction;
		Candidate.DistanceToParty = Distance;
		Candidate.ExitCount = ExitCount;
		Candidate.bCanContinuePursuit = bCanContinue;
		Candidate.bIsCulDeSac = bCulDeSac;
		Candidate.StableDirectionOrder = StableOrder;
		return Candidate;
	};

	FGridRetreatDecision Decision;
	TArray<FGridRetreatCandidate> Candidates = { MakeCandidate(FIntPoint(2, 1), EGridEdge::East, 1, 3, true, false, 1),
		MakeCandidate(FIntPoint(1, 2), EGridEdge::North, 2, 2, true, false, 0) };
	TestTrue(TEXT("A retreat candidate can be selected"), FGridFastHarasserPlanner::SelectBestRetreatCell(Candidates, 1, Decision));
	TestTrue(TEXT("Increasing distance dominates the score"), Decision.RetreatCell == FIntPoint(1, 2));

	Candidates = { MakeCandidate(FIntPoint(1, 2), EGridEdge::North, 2, 1, true, true, 0),
		MakeCandidate(FIntPoint(2, 1), EGridEdge::East, 2, 2, true, false, 1) };
	FGridFastHarasserPlanner::SelectBestRetreatCell(Candidates, 1, Decision);
	TestTrue(TEXT("An open cell beats a comparable cul-de-sac"), Decision.RetreatCell == FIntPoint(2, 1));

	Candidates = { MakeCandidate(FIntPoint(1, 2), EGridEdge::North, 2, 2, true, false, 0),
		MakeCandidate(FIntPoint(2, 1), EGridEdge::East, 2, 3, true, false, 1) };
	FGridFastHarasserPlanner::SelectBestRetreatCell(Candidates, 1, Decision);
	TestTrue(TEXT("More exits are favored at identical distance"), Decision.RetreatCell == FIntPoint(2, 1));

	Candidates = { MakeCandidate(FIntPoint(2, 1), EGridEdge::East, 2, 2, true, false, 1),
		MakeCandidate(FIntPoint(1, 2), EGridEdge::North, 2, 2, true, false, 0) };
	FGridFastHarasserPlanner::SelectBestRetreatCell(Candidates, 1, Decision);
	TestEqual(TEXT("A score tie follows North, East, South, West"), Decision.RetreatDirection, EGridEdge::North);

	Candidates.Reset();
	TestFalse(TEXT("No candidates produce an empty decision"), FGridFastHarasserPlanner::SelectBestRetreatCell(Candidates, 1, Decision));
	TestFalse(TEXT("The empty decision has no retreat cell"), Decision.bHasRetreatCell);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON7ActionPlanningTest, "Grimrock.Monsters.MON7.ActionPlanning", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON7ActionPlanningTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FGuid RatId(1, 2, 3, 4);
	const FName BiteId(TEXT("Attack_Bite"));
	FGridRetreatDecision Retreat;
	Retreat.bHasRetreatCell = true;
	Retreat.RetreatCell = FIntPoint(1, 2);
	Retreat.RetreatDirection = EGridEdge::North;
	Retreat.Score = 100;

	TArray<FGridCombatAction> Actions;
	FGridMonsterTurnPlanner::BuildFastHarasserTurn(
		RatId, FIntPoint(1, 1), EGridEdge::East, FIntPoint(2, 1), TArray<FIntPoint>(), 2, BiteId, 1, Retreat, true, Actions);
	TestEqual(TEXT("An adjacent FastHarasser bites, turns and retreats"), Actions.Num(), 3);
	if (Actions.Num() >= 3)
	{
		TestEqual(TEXT("The first FastHarasser action is the bite"), Actions[0].Type, EGridCombatActionType::MeleeAttack);
		TestEqual(TEXT("The retreat can turn for free after the bite"), Actions[1].Type, EGridCombatActionType::Turn);
		TestEqual(TEXT("The final FastHarasser action is a move"), Actions[2].Type, EGridCombatActionType::Move);
		TestTrue(TEXT("The retreat turn is marked as repositioning"), Actions[1].bIsRepositioningAction);
		TestTrue(TEXT("The retreat move is marked as repositioning"), Actions[2].bIsRepositioningAction);
	}

	const TArray<FIntPoint> OneStepPath = { FIntPoint(2, 1) };
	FGridMonsterTurnPlanner::BuildFastHarasserTurn(RatId, FIntPoint(1, 1), EGridEdge::East, FIntPoint(3, 1), OneStepPath, 2, BiteId, 1, Retreat, true, Actions);
	TestEqual(TEXT("At distance two the rat moves then bites without retreat"), Actions.Num(), 2);
	if (Actions.Num() >= 2)
	{
		TestEqual(TEXT("Distance two starts with Move"), Actions[0].Type, EGridCombatActionType::Move);
		TestEqual(TEXT("Distance two ends with MeleeAttack"), Actions[1].Type, EGridCombatActionType::MeleeAttack);
		TestFalse(TEXT("The pursuit move is not a repositioning move"), Actions[0].bIsRepositioningAction);
	}

	FGridMonsterTurnPlanner::BuildDirectMeleeTurn(RatId, FIntPoint(1, 1), EGridEdge::East, FIntPoint(2, 1), TArray<FIntPoint>(), 2, BiteId, 1, Actions);
	TestEqual(TEXT("DirectMelee adjacent behavior remains one bite"), Actions.Num(), 1);
	if (Actions.Num() >= 1)
	{
		TestEqual(TEXT("DirectMelee still plans MeleeAttack"), Actions[0].Type, EGridCombatActionType::MeleeAttack);
		TestFalse(TEXT("DirectMelee never marks a retreat"), Actions[0].bIsRepositioningAction);
	}

	FGridMonsterTurnPlanner::BuildFastHarasserTurn(
		RatId, FIntPoint(1, 1), EGridEdge::East, FIntPoint(2, 1), TArray<FIntPoint>(), 1, BiteId, 1, Retreat, true, Actions);
	TestEqual(TEXT("A FastHarasser with one action point only bites"), Actions.Num(), 1);
	if (Actions.Num() >= 1)
	{
		TestEqual(TEXT("The one-point action is MeleeAttack"), Actions[0].Type, EGridCombatActionType::MeleeAttack);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGridMonsterMON7GroupAggroTest, "Grimrock.Monsters.MON7.GroupAggro", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON7GroupAggroTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FGuid SourceId(1, 0, 0, 0);
	const FGuid NearId(2, 0, 0, 0);
	const FGuid SecondNearId(3, 0, 0, 0);
	const FGuid OtherGroupId(4, 0, 0, 0);
	const FGuid DeadId(5, 0, 0, 0);
	const FGuid FarId(6, 0, 0, 0);
	const FName RatId(TEXT("MON_RatGiant"));
	const FName GroupA(TEXT("RatRoom_A"));

	const auto MakeCandidate = [](FGuid Id, FName MonsterId, FName GroupId, FIntPoint Cell, bool bAlive, bool bEnabled)
	{
		FGridMonsterAggroCandidate Candidate;
		Candidate.SpawnObjectId = Id;
		Candidate.MonsterId = MonsterId;
		Candidate.EncounterGroupId = GroupId;
		Candidate.Cell = Cell;
		Candidate.bIsAlive = bAlive;
		Candidate.bIsEnabled = bEnabled;
		return Candidate;
	};

	TArray<FGridMonsterAggroCandidate> Candidates = { MakeCandidate(SecondNearId, RatId, GroupA, FIntPoint(0, 2), true, true),
		MakeCandidate(FarId, RatId, GroupA, FIntPoint(0, 6), true, true),
		MakeCandidate(OtherGroupId, RatId, FName(TEXT("RatRoom_B")), FIntPoint(0, 1), true, true),
		MakeCandidate(DeadId, RatId, GroupA, FIntPoint(1, 0), false, true), MakeCandidate(NearId, RatId, GroupA, FIntPoint(0, 1), true, true),
		MakeCandidate(NearId, RatId, GroupA, FIntPoint(0, 1), true, true) };

	TArray<FGuid> TargetIds;
	FGridFastHarasserPlanner::SelectAggroTargets(SourceId, RatId, GroupA, FIntPoint::ZeroValue, 5, Candidates, TargetIds);
	TestEqual(TEXT("Only two living in-range rats of the same group are selected"), TargetIds.Num(), 2);
	if (TargetIds.Num() >= 2)
	{
		TestEqual(TEXT("Aggro targets use deterministic SpawnObjectId order"), TargetIds[0], NearId);
		TestEqual(TEXT("The second deterministic target follows"), TargetIds[1], SecondNearId);
	}

	FGridFastHarasserPlanner::SelectAggroTargets(SourceId, RatId, NAME_None, FIntPoint::ZeroValue, 5, Candidates, TargetIds);
	TestTrue(TEXT("A None encounter group never propagates"), TargetIds.IsEmpty());

	return true;
}

#endif
