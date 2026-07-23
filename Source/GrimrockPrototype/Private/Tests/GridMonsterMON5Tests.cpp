#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON5PhaseStateMachineTest,
    "Grimrock.Monsters.MON5.PhaseStateMachine",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON5PhaseStateMachineTest::RunTest (const FString& Parameters)
{
    (void)Parameters;

    FGridTurnPhaseStateMachine State;
    TestEqual (TEXT ("Combat starts in exploration"),
        State.GetPhase (), EGridCombatPhase::Exploration);
    TestEqual (TEXT ("No round exists before combat"), State.GetRoundNumber (), 0);
    TestFalse (TEXT ("The player phase cannot end during exploration"), State.EndPlayerPhase ());

    TestTrue (TEXT ("Combat enters StartingCombat"), State.StartCombat ());
    TestEqual (TEXT ("StartingCombat is active"),
        State.GetPhase (), EGridCombatPhase::StartingCombat);
    TestFalse (TEXT ("Combat cannot start twice"), State.StartCombat ());

    TestTrue (TEXT ("The first round starts"), State.BeginRound ());
    TestEqual (TEXT ("Round one begins in PlayerPhase"),
        State.GetPhase (), EGridCombatPhase::PlayerPhase);
    TestEqual (TEXT ("The first round is numbered one"), State.GetRoundNumber (), 1);

    TestTrue (TEXT ("The player phase ends"), State.EndPlayerPhase ());
    TestEqual (TEXT ("EnemyPhase follows PlayerPhase"),
        State.GetPhase (), EGridCombatPhase::EnemyPhase);

    TestTrue (TEXT ("The enemy phase completes"), State.CompleteEnemyPhase ());
    TestEqual (TEXT ("EndingRound follows EnemyPhase"),
        State.GetPhase (), EGridCombatPhase::EndingRound);

    TestTrue (TEXT ("The next round starts"), State.BeginNextRound ());
    TestEqual (TEXT ("The second round returns to PlayerPhase"),
        State.GetPhase (), EGridCombatPhase::PlayerPhase);
    TestEqual (TEXT ("The round counter increments"), State.GetRoundNumber (), 2);

    TestTrue (TEXT ("An active combat can end in victory"),
        State.FinishCombat (EGridCombatPhase::Victory));
    TestEqual (TEXT ("Victory is stored as the terminal phase"),
        State.GetPhase (), EGridCombatPhase::Victory);
    TestFalse (TEXT ("Victory is not an active combat phase"), State.IsCombatActive ());

    State.AbortCombat ();
    TestEqual (TEXT ("Abort returns to exploration"),
        State.GetPhase (), EGridCombatPhase::Exploration);
    TestEqual (TEXT ("Abort resets the round counter"), State.GetRoundNumber (), 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON5ActionPointsAndPlanningTest,
    "Grimrock.Monsters.MON5.ActionPointsAndPlanning",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON5ActionPointsAndPlanningTest::RunTest (const FString& Parameters)
{
    (void)Parameters;

    FGridActionPointBudget Budget;
    Budget.Reset (2);
    TestEqual (TEXT ("The budget stores its maximum"), Budget.GetMaximumPoints (), 2);
    TestEqual (TEXT ("The budget starts full"), Budget.GetRemainingPoints (), 2);
    TestTrue (TEXT ("One action point can be spent"), Budget.Spend (1));
    TestEqual (TEXT ("One action point remains"), Budget.GetRemainingPoints (), 1);
    TestFalse (TEXT ("A cost above the remaining budget is refused"), Budget.Spend (2));
    TestFalse (TEXT ("A negative action cost is refused"), Budget.Spend (-1));
    TestTrue (TEXT ("The final action point can be spent"), Budget.Spend (1));
    TestTrue (TEXT ("The budget is exhausted"), Budget.IsExhausted ());

    const FGuid RatId = FGuid::NewGuid ();
    const FIntPoint Start (1, 1);
    const TArray<FIntPoint> EastPath = { FIntPoint (2, 1), FIntPoint (3, 1) };

    TArray<FGridCombatAction> Actions;
    FGridMonsterTurnPlanner::BuildMovementTurn (
        RatId,
        Start,
        EGridEdge::North,
        EastPath,
        2,
        Actions);

    TestEqual (TEXT ("Turning east then moving twice produces three actions"), Actions.Num (), 3);
    if (Actions.Num () >= 3)
    {
        TestEqual (TEXT ("The first action turns toward the path"),
            Actions[0].Type, EGridCombatActionType::Turn);
        TestEqual (TEXT ("The second action moves to the first path cell"),
            Actions[1].Type, EGridCombatActionType::Move);
        TestTrue (TEXT ("The first move target is correct"),
            Actions[1].TargetCell == FIntPoint (2, 1));
        TestTrue (TEXT ("The second move target is correct"),
            Actions[2].TargetCell == FIntPoint (3, 1));
        TestEqual (TEXT ("Rotation is free"), Actions[0].ActionPointCost, 0);
        TestEqual (TEXT ("A move costs one action point"), Actions[1].ActionPointCost, 1);
    }

    int32 TotalMoveCost = 0;
    int32 MoveCount = 0;
    for (const FGridCombatAction& Action : Actions)
    {
        if (Action.Type == EGridCombatActionType::Move)
        {
            ++MoveCount;
            TotalMoveCost += Action.ActionPointCost;
        }
        TestTrue (TEXT ("Every planned action keeps the source monster id"),
            Action.SourceActorId == RatId);
    }
    TestEqual (TEXT ("Two moves are planned for two action points"), MoveCount, 2);
    TestEqual (TEXT ("The movement cost matches the available budget"), TotalMoveCost, 2);

    FGridMonsterTurnPlanner::BuildMovementTurn (
        RatId,
        Start,
        EGridEdge::North,
        EastPath,
        1,
        Actions);
    MoveCount = 0;
    for (const FGridCombatAction& Action : Actions)
    {
        MoveCount += Action.Type == EGridCombatActionType::Move ? 1 : 0;
    }
    TestEqual (TEXT ("Only one move is planned with one action point"), MoveCount, 1);

    const TArray<FIntPoint> SouthPath = { FIntPoint (1, 0) };
    FGridMonsterTurnPlanner::BuildMovementTurn (
        RatId,
        Start,
        EGridEdge::North,
        SouthPath,
        1,
        Actions);
    TestEqual (TEXT ("An opposite direction requires two turns and one move"), Actions.Num (), 3);
    if (Actions.Num () >= 3)
    {
        TestEqual (TEXT ("The first opposite-direction action is a turn"),
            Actions[0].Type, EGridCombatActionType::Turn);
        TestEqual (TEXT ("The second opposite-direction action is a turn"),
            Actions[1].Type, EGridCombatActionType::Turn);
        TestEqual (TEXT ("The final opposite-direction action is a move"),
            Actions[2].Type, EGridCombatActionType::Move);
    }

    FGridMonsterTurnPlanner::BuildMovementTurn (
        RatId,
        Start,
        EGridEdge::North,
        TArray<FIntPoint> (),
        2,
        Actions);
    TestEqual (TEXT ("An empty path produces one action"), Actions.Num (), 1);
    if (Actions.Num () >= 1)
    {
        TestEqual (TEXT ("An empty path produces Wait"),
            Actions[0].Type, EGridCombatActionType::Wait);
        TestEqual (TEXT ("Wait costs no action points"), Actions[0].ActionPointCost, 0);
    }

    return true;
}

#endif
