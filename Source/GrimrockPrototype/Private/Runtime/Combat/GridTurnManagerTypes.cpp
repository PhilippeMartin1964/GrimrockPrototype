#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "Core/GridDirectionUtils.h"
#include "Runtime/Monsters/GridMonsterFastHarasserPlanner.h"
#include "Runtime/Monsters/GridMonsterPathfinder.h"

namespace
{
    FGridCombatAction MakeTurnAction (
        const FGuid& SourceActorId,
        const FIntPoint& CurrentCell,
        EGridEdge TargetFacing)
    {
        FGridCombatAction Action;
        Action.ActionId = FGuid::NewGuid ();
        Action.Type = EGridCombatActionType::Turn;
        Action.SourceActorId = SourceActorId;
        Action.TargetCell = FGridMonsterPathfinder::GetNeighborCell (CurrentCell, TargetFacing);
        Action.ActionPointCost = 0;
        return Action;
    }

    FGridCombatAction MakeMoveAction (
        const FGuid& SourceActorId,
        const FIntPoint& TargetCell)
    {
        FGridCombatAction Action;
        Action.ActionId = FGuid::NewGuid ();
        Action.Type = EGridCombatActionType::Move;
        Action.SourceActorId = SourceActorId;
        Action.TargetCell = TargetCell;
        Action.ActionPointCost = 1;
        return Action;
    }

    FGridCombatAction MakeMeleeAttackAction (
        const FGuid& SourceActorId,
        const FIntPoint& PartyCell,
        FName AttackId,
        int32 ActionPointCost)
    {
        FGridCombatAction Action;
        Action.ActionId = FGuid::NewGuid ();
        Action.Type = EGridCombatActionType::MeleeAttack;
        Action.SourceActorId = SourceActorId;
        Action.TargetCell = PartyCell;
        Action.AttackId = AttackId;
        Action.ActionPointCost = FMath::Max (0, ActionPointCost);
        return Action;
    }

    FGridCombatAction MakeWaitAction (
        const FGuid& SourceActorId,
        const FIntPoint& CurrentCell)
    {
        FGridCombatAction Action;
        Action.ActionId = FGuid::NewGuid ();
        Action.Type = EGridCombatActionType::Wait;
        Action.SourceActorId = SourceActorId;
        Action.TargetCell = CurrentCell;
        Action.ActionPointCost = 0;
        return Action;
    }

    bool AppendTurnsToward (
        const FGuid& SourceActorId,
        const FIntPoint& CurrentCell,
        EGridEdge DesiredFacing,
        EGridEdge& InOutFacing,
        TArray<FGridCombatAction>& OutActions)
    {
        if (DesiredFacing == EGridEdge::None)
        {
            return false;
        }

        if (InOutFacing == EGridEdge::None)
        {
            InOutFacing = EGridEdge::North;
        }

        if (InOutFacing == DesiredFacing)
        {
            return true;
        }

        if (GridDirectionUtils::RotateLeft (InOutFacing) == DesiredFacing)
        {
            InOutFacing = GridDirectionUtils::RotateLeft (InOutFacing);
            OutActions.Add (MakeTurnAction (SourceActorId, CurrentCell, InOutFacing));
            return true;
        }

        if (GridDirectionUtils::RotateRight (InOutFacing) == DesiredFacing)
        {
            InOutFacing = GridDirectionUtils::RotateRight (InOutFacing);
            OutActions.Add (MakeTurnAction (SourceActorId, CurrentCell, InOutFacing));
            return true;
        }

        InOutFacing = GridDirectionUtils::RotateLeft (InOutFacing);
        OutActions.Add (MakeTurnAction (SourceActorId, CurrentCell, InOutFacing));
        InOutFacing = GridDirectionUtils::RotateLeft (InOutFacing);
        OutActions.Add (MakeTurnAction (SourceActorId, CurrentCell, InOutFacing));
        return InOutFacing == DesiredFacing;
    }
}

void FGridActionPointBudget::Reset (int32 InMaximumPoints)
{
    MaximumPoints = FMath::Max (0, InMaximumPoints);
    RemainingPoints = MaximumPoints;
}

bool FGridActionPointBudget::CanSpend (int32 Cost) const
{
    return Cost >= 0 && Cost <= RemainingPoints;
}

bool FGridActionPointBudget::Spend (int32 Cost)
{
    if (!CanSpend (Cost))
    {
        return false;
    }

    RemainingPoints -= Cost;
    return true;
}

void FGridInitiativeOrderBuilder::RollAndSort (
    TArray<FGridCombatantInitiativeEntry>& Entries,
    FRandomStream& RandomStream)
{
    for (FGridCombatantInitiativeEntry& Entry : Entries)
    {
        Entry.InitiativeRoll = RandomStream.RandRange (1, 20);
        Entry.InitiativeTotal =
            Entry.InitiativeBase + Entry.InitiativeRoll;
    }

    Sort (Entries);
}

void FGridInitiativeOrderBuilder::Sort (
    TArray<FGridCombatantInitiativeEntry>& Entries)
{
    Entries.Sort ([] (
        const FGridCombatantInitiativeEntry& Left,
        const FGridCombatantInitiativeEntry& Right)
    {
        if (Left.InitiativeTotal != Right.InitiativeTotal)
        {
            return Left.InitiativeTotal > Right.InitiativeTotal;
        }
        if (Left.InitiativeBase != Right.InitiativeBase)
        {
            return Left.InitiativeBase > Right.InitiativeBase;
        }
        if (Left.Dexterity != Right.Dexterity)
        {
            return Left.Dexterity > Right.Dexterity;
        }

        const FString LeftId =
            Left.CombatantId.ToString (EGuidFormats::Digits);
        const FString RightId =
            Right.CombatantId.ToString (EGuidFormats::Digits);
        if (LeftId != RightId)
        {
            return LeftId < RightId;
        }
        return static_cast<uint8> (Left.Side) <
            static_cast<uint8> (Right.Side);
    });
}

bool FGridTurnPhaseStateMachine::StartCombat ()
{
    if (Phase != EGridCombatPhase::Exploration &&
        Phase != EGridCombatPhase::Victory &&
        Phase != EGridCombatPhase::Defeat)
    {
        return false;
    }

    Phase = EGridCombatPhase::StartingCombat;
    RoundNumber = 0;
    return true;
}

bool FGridTurnPhaseStateMachine::BeginRound ()
{
    if (Phase != EGridCombatPhase::StartingCombat)
    {
        return false;
    }

    ++RoundNumber;
    Phase = EGridCombatPhase::PlayerPhase;
    return true;
}

bool FGridTurnPhaseStateMachine::EndPlayerPhase ()
{
    if (Phase != EGridCombatPhase::PlayerPhase)
    {
        return false;
    }

    Phase = EGridCombatPhase::EnemyPhase;
    return true;
}

bool FGridTurnPhaseStateMachine::CompleteEnemyPhase ()
{
    if (Phase != EGridCombatPhase::EnemyPhase)
    {
        return false;
    }

    Phase = EGridCombatPhase::EndingRound;
    return true;
}

bool FGridTurnPhaseStateMachine::BeginNextRound ()
{
    if (Phase != EGridCombatPhase::EndingRound)
    {
        return false;
    }

    ++RoundNumber;
    Phase = EGridCombatPhase::PlayerPhase;
    return true;
}

bool FGridTurnPhaseStateMachine::BeginCombatantTurn (
    EGridCombatantSide Side)
{
    if (Phase != EGridCombatPhase::PlayerPhase &&
        Phase != EGridCombatPhase::EnemyPhase)
    {
        return false;
    }

    Phase = Side == EGridCombatantSide::Party
        ? EGridCombatPhase::PlayerPhase
        : EGridCombatPhase::EnemyPhase;
    return true;
}

bool FGridTurnPhaseStateMachine::CompleteInitiativeRound ()
{
    if (Phase != EGridCombatPhase::PlayerPhase &&
        Phase != EGridCombatPhase::EnemyPhase)
    {
        return false;
    }

    Phase = EGridCombatPhase::EndingRound;
    return true;
}

bool FGridTurnPhaseStateMachine::FinishCombat (EGridCombatPhase ResultPhase)
{
    if (!IsCombatActive () ||
        (ResultPhase != EGridCombatPhase::Victory && ResultPhase != EGridCombatPhase::Defeat))
    {
        return false;
    }

    Phase = ResultPhase;
    return true;
}

void FGridTurnPhaseStateMachine::AbortCombat ()
{
    Phase = EGridCombatPhase::Exploration;
    RoundNumber = 0;
}

bool FGridTurnPhaseStateMachine::IsCombatActive () const
{
    return Phase == EGridCombatPhase::StartingCombat ||
        Phase == EGridCombatPhase::PlayerPhase ||
        Phase == EGridCombatPhase::EnemyPhase ||
        Phase == EGridCombatPhase::EndingRound;
}

void FGridMonsterTurnPlanner::BuildMovementTurn (
    const FGuid& SourceActorId,
    const FIntPoint& StartCell,
    EGridEdge StartFacing,
    const TArray<FIntPoint>& Path,
    int32 AvailableActionPoints,
    TArray<FGridCombatAction>& OutActions)
{
    OutActions.Reset ();

    const int32 SafeActionPoints = FMath::Max (0, AvailableActionPoints);
    FIntPoint SimulatedCell = StartCell;
    EGridEdge SimulatedFacing = StartFacing == EGridEdge::None ? EGridEdge::North : StartFacing;
    int32 SpentActionPoints = 0;

    for (const FIntPoint& Step : Path)
    {
        if (SpentActionPoints >= SafeActionPoints)
        {
            break;
        }

        const EGridEdge DesiredFacing =
            FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells (SimulatedCell, Step);
        if (!AppendTurnsToward (
            SourceActorId,
            SimulatedCell,
            DesiredFacing,
            SimulatedFacing,
            OutActions))
        {
            break;
        }

        OutActions.Add (MakeMoveAction (SourceActorId, Step));
        SimulatedCell = Step;
        ++SpentActionPoints;
    }

    if (OutActions.IsEmpty ())
    {
        OutActions.Add (MakeWaitAction (SourceActorId, StartCell));
    }
}

void FGridMonsterTurnPlanner::BuildDirectMeleeTurn (
    const FGuid& SourceActorId,
    const FIntPoint& StartCell,
    EGridEdge StartFacing,
    const FIntPoint& PartyCell,
    const TArray<FIntPoint>& Path,
    int32 AvailableActionPoints,
    FName AttackId,
    int32 AttackActionPointCost,
    TArray<FGridCombatAction>& OutActions)
{
    OutActions.Reset ();

    FIntPoint SimulatedCell = StartCell;
    EGridEdge SimulatedFacing = StartFacing == EGridEdge::None
        ? EGridEdge::North
        : StartFacing;
    int32 RemainingActionPoints = FMath::Max (0, AvailableActionPoints);
    const int32 SafeAttackCost = FMath::Max (1, AttackActionPointCost);
    int32 PathIndex = 0;

    while (true)
    {
        const int32 DistanceToParty =
            FMath::Abs (SimulatedCell.X - PartyCell.X) +
            FMath::Abs (SimulatedCell.Y - PartyCell.Y);
        if (DistanceToParty == 1 &&
            !AttackId.IsNone () &&
            RemainingActionPoints >= SafeAttackCost)
        {
            const EGridEdge AttackFacing =
                FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells (
                    SimulatedCell,
                    PartyCell);
            if (AppendTurnsToward (
                SourceActorId,
                SimulatedCell,
                AttackFacing,
                SimulatedFacing,
                OutActions))
            {
                OutActions.Add (MakeMeleeAttackAction (
                    SourceActorId,
                    PartyCell,
                    AttackId,
                    SafeAttackCost));
            }
            break;
        }

        if (RemainingActionPoints <= 0 || !Path.IsValidIndex (PathIndex))
        {
            break;
        }

        const FIntPoint Step = Path[PathIndex++];
        const EGridEdge DesiredFacing =
            FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells (
                SimulatedCell,
                Step);
        if (!AppendTurnsToward (
            SourceActorId,
            SimulatedCell,
            DesiredFacing,
            SimulatedFacing,
            OutActions))
        {
            break;
        }

        OutActions.Add (MakeMoveAction (SourceActorId, Step));
        SimulatedCell = Step;
        --RemainingActionPoints;
    }

    if (OutActions.IsEmpty ())
    {
        OutActions.Add (MakeWaitAction (SourceActorId, StartCell));
    }
}

void FGridMonsterTurnPlanner::BuildFastHarasserTurn (
    const FGuid& SourceActorId,
    const FIntPoint& StartCell,
    EGridEdge StartFacing,
    const FIntPoint& PartyCell,
    const TArray<FIntPoint>& Path,
    int32 AvailableActionPoints,
    FName AttackId,
    int32 AttackActionPointCost,
    const FGridRetreatDecision& RetreatDecision,
    bool bShouldRetreat,
    TArray<FGridCombatAction>& OutActions)
{
    BuildDirectMeleeTurn (
        SourceActorId,
        StartCell,
        StartFacing,
        PartyCell,
        Path,
        AvailableActionPoints,
        AttackId,
        AttackActionPointCost,
        OutActions);

    if (!bShouldRetreat || !RetreatDecision.bHasRetreatCell)
    {
        return;
    }

    FIntPoint SimulatedCell = StartCell;
    EGridEdge SimulatedFacing = StartFacing == EGridEdge::None
        ? EGridEdge::North
        : StartFacing;
    int32 RemainingActionPoints = FMath::Max (0, AvailableActionPoints);
    bool bMeleeAttackPlanned = false;

    for (const FGridCombatAction& Action : OutActions)
    {
        if (Action.Type == EGridCombatActionType::Turn)
        {
            const EGridEdge TurnFacing =
                FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells (
                    SimulatedCell,
                    Action.TargetCell);
            if (TurnFacing != EGridEdge::None)
            {
                SimulatedFacing = TurnFacing;
            }
        }
        else if (Action.Type == EGridCombatActionType::Move)
        {
            SimulatedFacing =
                FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells (
                    SimulatedCell,
                    Action.TargetCell);
            SimulatedCell = Action.TargetCell;
            RemainingActionPoints -= Action.ActionPointCost;
        }
        else if (Action.Type == EGridCombatActionType::MeleeAttack)
        {
            RemainingActionPoints -= Action.ActionPointCost;
            bMeleeAttackPlanned = true;
            break;
        }
    }

    if (!bMeleeAttackPlanned ||
        RemainingActionPoints < 1 ||
        FGridMonsterPathfinder::GetNeighborCell (
            SimulatedCell,
            RetreatDecision.RetreatDirection) != RetreatDecision.RetreatCell)
    {
        return;
    }

    const int32 FirstRetreatActionIndex = OutActions.Num ();
    if (!AppendTurnsToward (
        SourceActorId,
        SimulatedCell,
        RetreatDecision.RetreatDirection,
        SimulatedFacing,
        OutActions))
    {
        return;
    }

    for (int32 Index = FirstRetreatActionIndex; Index < OutActions.Num (); ++Index)
    {
        OutActions[Index].bIsRepositioningAction = true;
    }

    FGridCombatAction RetreatMove =
        MakeMoveAction (SourceActorId, RetreatDecision.RetreatCell);
    RetreatMove.bIsRepositioningAction = true;
    OutActions.Add (RetreatMove);
}
