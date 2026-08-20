#include "Runtime/Monsters/GridMonsterRangedKeeperPlanner.h"

#include "Core/GridDirectionUtils.h"
#include "Runtime/Monsters/GridMonsterPathfinder.h"
#include "Runtime/Monsters/GridMonsterRangedAttackPlanner.h"

namespace
{
    FGridCombatAction MON174MakeTurnAction (
        const FGuid& SourceActorId,
        const FIntPoint& CurrentCell,
        EGridEdge TargetFacing)
    {
        FGridCombatAction Action;
        Action.ActionId = FGuid::NewGuid ();
        Action.Type = EGridCombatActionType::Turn;
        Action.SourceActorId = SourceActorId;
        Action.TargetCell = FGridMonsterPathfinder::GetNeighborCell (
            CurrentCell,
            TargetFacing);
        Action.ActionPointCost = 0;
        Action.bIsRepositioningAction = true;
        return Action;
    }

    FGridCombatAction MON174MakeMoveAction (
        const FGuid& SourceActorId,
        const FIntPoint& TargetCell)
    {
        FGridCombatAction Action;
        Action.ActionId = FGuid::NewGuid ();
        Action.Type = EGridCombatActionType::Move;
        Action.SourceActorId = SourceActorId;
        Action.TargetCell = TargetCell;
        Action.ActionPointCost = 1;
        Action.bIsRepositioningAction = true;
        return Action;
    }

    bool MON174AppendTurnsToward (
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
            OutActions.Add (MON174MakeTurnAction (
                SourceActorId,
                CurrentCell,
                InOutFacing));
            return true;
        }

        if (GridDirectionUtils::RotateRight (InOutFacing) == DesiredFacing)
        {
            InOutFacing = GridDirectionUtils::RotateRight (InOutFacing);
            OutActions.Add (MON174MakeTurnAction (
                SourceActorId,
                CurrentCell,
                InOutFacing));
            return true;
        }

        InOutFacing = GridDirectionUtils::RotateLeft (InOutFacing);
        OutActions.Add (MON174MakeTurnAction (
            SourceActorId,
            CurrentCell,
            InOutFacing));
        InOutFacing = GridDirectionUtils::RotateLeft (InOutFacing);
        OutActions.Add (MON174MakeTurnAction (
            SourceActorId,
            CurrentCell,
            InOutFacing));
        return InOutFacing == DesiredFacing;
    }
}

void FGridMonsterRangedKeeperPlanner::BuildAxialFiringCandidates (
    const FIntPoint& PartyCell,
    int32 MinDistanceCells,
    int32 MaxDistanceCells,
    TArray<FIntPoint>& OutCandidates)
{
    OutCandidates.Reset ();

    const int32 SafeMinDistance = FMath::Max (1, MinDistanceCells);
    const int32 SafeMaxDistance = FMath::Max (
        SafeMinDistance,
        MaxDistanceCells);

    for (int32 Distance = SafeMinDistance;
        Distance <= SafeMaxDistance;
        ++Distance)
    {
        for (const EGridEdge Direction :
            FGridMonsterPathfinder::GetOrderedDirections ())
        {
            FIntPoint Candidate = PartyCell;
            for (int32 StepIndex = 0; StepIndex < Distance; ++StepIndex)
            {
                Candidate = FGridMonsterPathfinder::GetNeighborCell (
                    Candidate,
                    Direction);
            }
            OutCandidates.Add (Candidate);
        }
    }
}

bool FGridMonsterRangedKeeperPlanner::IsPreferredDistance (
    int32 DistanceCells,
    int32 PreferredMinDistance,
    int32 PreferredMaxDistance)
{
    const int32 SafeMin = FMath::Max (0, PreferredMinDistance);
    const int32 SafeMax = FMath::Max (SafeMin, PreferredMaxDistance);
    return DistanceCells >= SafeMin && DistanceCells <= SafeMax;
}

bool FGridMonsterRangedKeeperPlanner::BuildRepositionTurn (
    const FGuid& SourceActorId,
    const FIntPoint& StartCell,
    EGridEdge StartFacing,
    const FIntPoint& PartyCell,
    const TArray<FIntPoint>& PathToFiringCell,
    int32 AvailableActionPoints,
    const FGridMonsterAttackDefinition& Attack,
    bool bGoalLineOfSightSatisfied,
    bool bMayAttackThisTurn,
    TArray<FGridCombatAction>& OutActions)
{
    OutActions.Reset ();

    if (!SourceActorId.IsValid () ||
        !Attack.IsValidDefinition () ||
        !Attack.IsRangedAttack ())
    {
        return false;
    }

    FIntPoint SimulatedCell = StartCell;
    EGridEdge SimulatedFacing = StartFacing == EGridEdge::None
        ? EGridEdge::North
        : StartFacing;
    int32 RemainingActionPoints = FMath::Max (0, AvailableActionPoints);

    for (const FIntPoint& Step : PathToFiringCell)
    {
        if (RemainingActionPoints < 1)
        {
            break;
        }

        const EGridEdge DesiredFacing =
            FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells (
                SimulatedCell,
                Step);
        if (!MON174AppendTurnsToward (
            SourceActorId,
            SimulatedCell,
            DesiredFacing,
            SimulatedFacing,
            OutActions))
        {
            break;
        }

        OutActions.Add (MON174MakeMoveAction (SourceActorId, Step));
        SimulatedCell = Step;
        --RemainingActionPoints;
    }

    const bool bReachedFiringCell =
        PathToFiringCell.IsEmpty () ||
        SimulatedCell == PathToFiringCell.Last ();
    if (bReachedFiringCell && bMayAttackThisTurn)
    {
        TArray<FGridCombatAction> AttackActions;
        if (FGridMonsterRangedAttackPlanner::BuildStationaryRangedTurn (
            SourceActorId,
            SimulatedCell,
            SimulatedFacing,
            PartyCell,
            RemainingActionPoints,
            Attack,
            bGoalLineOfSightSatisfied,
            AttackActions))
        {
            OutActions.Append (AttackActions);
        }
    }

    return !OutActions.IsEmpty ();
}
