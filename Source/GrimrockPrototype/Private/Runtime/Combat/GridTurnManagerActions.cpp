#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "Core/GridDirectionUtils.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterPathfinder.h"

namespace
{
    FString GetActionTypeText (EGridCombatActionType Type)
    {
        if (const UEnum* Enum = StaticEnum<EGridCombatActionType> ())
        {
            return Enum->GetNameStringByValue (static_cast<int64> (Type));
        }
        return TEXT ("Unknown");
    }
}

void UGridTurnManagerComponent::ExecuteNextAction ()
{
    if (!bCombatActive || CurrentPhase != EGridCombatPhase::EnemyPhase || !IsValid (CurrentMonster))
    {
        return;
    }

    if (CurrentMonster->IsDead () || ActionPointBudget.IsExhausted () || PendingActions.IsEmpty ())
    {
        FinishCurrentMonsterTurn ();
        return;
    }

    const FGridCombatAction NextAction = PendingActions[0];
    PendingActions.RemoveAt (0);

    if (!NextAction.IsValid () || !ActionPointBudget.CanSpend (NextAction.ActionPointCost))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("[GridTurnManager] Invalid or unaffordable action skipped. Monster=%s Type=%s Cost=%d AP=%d"),
            *GetNameSafe (CurrentMonster),
            *GetActionTypeText (NextAction.Type),
            NextAction.ActionPointCost,
            ActionPointBudget.GetRemainingPoints ());
        ExecuteNextAction ();
        return;
    }

    ActiveAction = NextAction;
    bHasActiveAction = true;
    OnActionStarted.Broadcast (ActiveAction);

    if (!StartActiveAction (ActiveAction))
    {
        CompleteActiveAction (false);
    }
}

bool UGridTurnManagerComponent::StartActiveAction (const FGridCombatAction& Action)
{
    if (!IsValid (CurrentMonster))
    {
        return false;
    }

    if (Action.Type == EGridCombatActionType::Wait)
    {
        CompleteActiveAction (true);
        return true;
    }

    if (!CurrentMovementComponent)
    {
        return false;
    }

    const EGridEdge TargetDirection =
        FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells (
            CurrentMonster->CurrentCell,
            Action.TargetCell);
    if (TargetDirection == EGridEdge::None)
    {
        return false;
    }

    bool bStarted = false;
    if (Action.Type == EGridCombatActionType::Move)
    {
        bStarted = CurrentMovementComponent->TryMove (TargetDirection);
    }
    else if (Action.Type == EGridCombatActionType::Turn)
    {
        if (CurrentMonster->Facing == TargetDirection)
        {
            CompleteActiveAction (true);
            return true;
        }

        if (GridDirectionUtils::RotateLeft (CurrentMonster->Facing) == TargetDirection)
        {
            bStarted = CurrentMovementComponent->TryTurnLeft ();
        }
        else if (GridDirectionUtils::RotateRight (CurrentMonster->Facing) == TargetDirection)
        {
            bStarted = CurrentMovementComponent->TryTurnRight ();
        }
    }
    else
    {
        // MON6 will execute attack actions. Unsupported MON5 actions end safely.
        return false;
    }

    if (bStarted)
    {
        ActiveActionTimeoutRemaining =
            GetExpectedActionDuration (Action) + FMath::Max (0.0f, ActionTimeoutPadding);
        RefreshTickEnabled ();
    }
    return bStarted;
}

void UGridTurnManagerComponent::CompleteActiveAction (bool bSucceeded)
{
    if (!bHasActiveAction)
    {
        return;
    }

    const FGridCombatAction CompletedAction = ActiveAction;
    bHasActiveAction = false;
    ActiveAction = FGridCombatAction ();
    ActiveActionTimeoutRemaining = 0.0f;

    if (bSucceeded)
    {
        ActionPointBudget.Spend (CompletedAction.ActionPointCost);
    }
    CurrentMonsterRemainingActionPoints = ActionPointBudget.GetRemainingPoints ();
    OnActionCompleted.Broadcast (CompletedAction, bSucceeded);

    if (CompletedAction.Type == EGridCombatActionType::Wait)
    {
        PendingActions.Reset ();
        FinishCurrentMonsterTurn ();
        return;
    }

    if (ActionPointBudget.IsExhausted ())
    {
        FinishCurrentMonsterTurn ();
        return;
    }

    ExecuteNextAction ();
}

float UGridTurnManagerComponent::GetExpectedActionDuration (const FGridCombatAction& Action) const
{
    if (!IsValid (CurrentMonster) || !IsValid (CurrentMonster->MonsterDefinition))
    {
        return 0.01f;
    }

    if (Action.Type == EGridCombatActionType::Move)
    {
        return FMath::Max (0.01f, CurrentMonster->MonsterDefinition->MoveDuration);
    }
    if (Action.Type == EGridCombatActionType::Turn)
    {
        return FMath::Max (0.01f, CurrentMonster->MonsterDefinition->TurnDuration);
    }
    return 0.01f;
}

void UGridTurnManagerComponent::RefreshTickEnabled ()
{
    SetComponentTickEnabled (bWaitingForCombatStart || bHasActiveAction);
}

void UGridTurnManagerComponent::BindCurrentMovement (
    UGridMonsterMovementComponent* MovementComponent)
{
    UnbindCurrentMovement ();
    CurrentMovementComponent = MovementComponent;
    if (!CurrentMovementComponent)
    {
        return;
    }

    CurrentMovementComponent->OnMoveCompleted.AddDynamic (
        this,
        &UGridTurnManagerComponent::HandleMonsterMoveCompleted);
    CurrentMovementComponent->OnTurnCompleted.AddDynamic (
        this,
        &UGridTurnManagerComponent::HandleMonsterTurnCompleted);
}

void UGridTurnManagerComponent::UnbindCurrentMovement ()
{
    if (CurrentMovementComponent)
    {
        CurrentMovementComponent->OnMoveCompleted.RemoveDynamic (
            this,
            &UGridTurnManagerComponent::HandleMonsterMoveCompleted);
        CurrentMovementComponent->OnTurnCompleted.RemoveDynamic (
            this,
            &UGridTurnManagerComponent::HandleMonsterTurnCompleted);
    }
    CurrentMovementComponent = nullptr;
}

void UGridTurnManagerComponent::HandleMonsterMoveCompleted (
    FIntPoint FromCell,
    FIntPoint ToCell)
{
    (void)FromCell;
    (void)ToCell;

    if (bHasActiveAction && ActiveAction.Type == EGridCombatActionType::Move)
    {
        CompleteActiveAction (true);
    }
}

void UGridTurnManagerComponent::HandleMonsterTurnCompleted (
    EGridEdge FromFacing,
    EGridEdge ToFacing)
{
    (void)FromFacing;
    (void)ToFacing;

    if (bHasActiveAction && ActiveAction.Type == EGridCombatActionType::Turn)
    {
        CompleteActiveAction (true);
    }
}
