#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "Core/GridDirectionUtils.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterCombatComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"
#include "Runtime/Monsters/GridMonsterOccupancySubsystem.h"
#include "Runtime/Monsters/GridMonsterPathfinder.h"

namespace
{
    FString GetTurnActionTypeText (EGridCombatActionType Type)
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
            *GetTurnActionTypeText (NextAction.Type),
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

    if (Action.bIsRepositioningAction && !CurrentMonster->IsDead ())
    {
        CurrentMonster->SetMonsterState (EGridMonsterState::Repositioning);
    }

    if (Action.Type == EGridCombatActionType::Wait)
    {
        CompleteActiveAction (true);
        return true;
    }

    if (Action.Type == EGridCombatActionType::MeleeAttack)
    {
        return StartActiveMeleeAttack ();
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

    if (bStarted)
    {
        ActiveActionTimeoutRemaining =
            GetExpectedActionDuration (Action) + FMath::Max (0.0f, ActionTimeoutPadding);
        RefreshTickEnabled ();
    }
    return bStarted;
}

bool UGridTurnManagerComponent::StartActiveMeleeAttack ()
{
    if (!IsValid (CurrentMonster) ||
        !IsValid (CurrentMonster->MonsterDefinition) ||
        !IsValid (CurrentCombatComponent) ||
        !IsValid (RuntimeActor))
    {
        return false;
    }

    const FGridMonsterAttackDefinition* Attack =
        CurrentMonster->MonsterDefinition->FindAttackDefinition (ActiveAction.AttackId);
    if (!Attack || !Attack->IsValidDefinition ())
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("[GridTurnManager] Attack definition unavailable. Monster=%s Attack=%s"),
            *GetNameSafe (CurrentMonster),
            *ActiveAction.AttackId.ToString ());
        return false;
    }

    const EGridEdge AttackDirection =
        FGridMonsterPathfinder::GetDirectionBetweenAdjacentCells (
            CurrentMonster->CurrentCell,
            ActiveAction.TargetCell);
    if (AttackDirection == EGridEdge::None ||
        !RuntimeActor->CanMove (
            CurrentMonster->CurrentCell.X,
            CurrentMonster->CurrentCell.Y,
            AttackDirection))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("[GridTurnManager] Melee attack blocked by grid edge. Monster=%s From=(%d,%d) Target=(%d,%d)"),
            *GetNameSafe (CurrentMonster),
            CurrentMonster->CurrentCell.X,
            CurrentMonster->CurrentCell.Y,
            ActiveAction.TargetCell.X,
            ActiveAction.TargetCell.Y);
        return false;
    }

    const int32 TargetCharacterIndex =
        CurrentCombatComponent->SelectPartyTarget (CombatRandomStream);
    if (TargetCharacterIndex == INDEX_NONE)
    {
        return false;
    }

    ActiveAction.TargetCharacterIndex = TargetCharacterIndex;
    ActiveAttackDefinition = *Attack;
    LastTargetCharacterIndex = TargetCharacterIndex;
    bActiveAttackImpactCommitted = false;
    ActiveAttackImpactTimeRemaining = FMath::Clamp (
        Attack->ImpactTimeSeconds,
        0.0f,
        Attack->ExpectedDuration);
    ActiveAttackCompleteTimeRemaining = FMath::Max (
        0.01f,
        Attack->ExpectedDuration);
    ActiveActionTimeoutRemaining =
        ActiveAttackCompleteTimeRemaining + FMath::Max (0.0f, ActionTimeoutPadding);

    if (!CurrentCombatComponent->StartAttackPresentation (ActiveAction, *Attack))
    {
        ResetActiveAttackState ();
        return false;
    }

    RefreshTickEnabled ();
    return true;
}

void UGridTurnManagerComponent::NotifyActiveAttackImpact ()
{
    CommitActiveAttackImpact ();
}

void UGridTurnManagerComponent::NotifyActiveAttackComplete ()
{
    if (!bHasActiveAction || ActiveAction.Type != EGridCombatActionType::MeleeAttack)
    {
        return;
    }

    CommitActiveAttackImpact ();
    if (CurrentCombatComponent && CurrentCombatComponent->bAttackPresentationActive)
    {
        CurrentCombatComponent->CancelAttackPresentation ();
    }
    CompleteActiveAction (ActiveAction.bOutcomeCommitted);
}

void UGridTurnManagerComponent::CommitActiveAttackImpact ()
{
    if (!bHasActiveAction ||
        ActiveAction.Type != EGridCombatActionType::MeleeAttack ||
        bActiveAttackImpactCommitted)
    {
        return;
    }

    bActiveAttackImpactCommitted = true;
    ActiveAttackImpactTimeRemaining = 0.0f;

    if (!IsValid (CurrentCombatComponent) || !ActiveAttackDefinition.IsValidDefinition ())
    {
        return;
    }

    int32 TargetCharacterIndex = ActiveAction.TargetCharacterIndex;
    if (!IsValid (PartyPawn) ||
        !IsValid (PartyPawn->PartyInventoryComponent) ||
        !PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters.IsValidIndex (
            TargetCharacterIndex) ||
        PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters[
            TargetCharacterIndex].DerivedStats.CurrentHealth <= 0)
    {
        TargetCharacterIndex = CurrentCombatComponent->SelectPartyTarget (CombatRandomStream);
        ActiveAction.TargetCharacterIndex = TargetCharacterIndex;
    }

    if (TargetCharacterIndex == INDEX_NONE)
    {
        return;
    }

    FGridAttackResult Result;
    if (!CurrentCombatComponent->ResolveAndApplyPartyAttack (
        TargetCharacterIndex,
        ActiveAttackDefinition,
        CombatRandomStream,
        Result))
    {
        return;
    }

    LastTargetCharacterIndex = TargetCharacterIndex;
    LastAttackResult = Result;
    ActiveAction.bHit = Result.bHit;
    ActiveAction.bCriticalHit = Result.bCriticalHit;
    ActiveAction.RolledDamage = Result.RawDamage;
    ActiveAction.bOutcomeCommitted = true;
    OnAttackResolved.Broadcast (CurrentMonster, TargetCharacterIndex, Result);
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

    const bool bFailedReposition =
        CompletedAction.bIsRepositioningAction && !bSucceeded;
    if (CompletedAction.bIsRepositioningAction &&
        IsValid (CurrentMonster) &&
        !CurrentMonster->IsDead ())
    {
        if (bFailedReposition ||
            CompletedAction.Type == EGridCombatActionType::Move)
        {
            CurrentMonster->SetMonsterState (EGridMonsterState::Pursuing);
        }
    }

    OnActionCompleted.Broadcast (CompletedAction, bSucceeded);

    if (bFailedReposition)
    {
        if (CurrentMovementComponent)
        {
            CurrentMovementComponent->CancelCurrentAction ();
            if (UGridMonsterOccupancySubsystem* OccupancySubsystem =
                CurrentMovementComponent->GetOccupancySubsystem ())
            {
                OccupancySubsystem->CancelReservation (CurrentMonster);
            }
        }
        PendingActions.Reset ();
        FinishCurrentMonsterTurn ();
        return;
    }

    if (CompletedAction.Type == EGridCombatActionType::MeleeAttack)
    {
        ResetActiveAttackState ();
        if (!HasLivingPartyCharacter ())
        {
            FinishCombat (EGridCombatPhase::Defeat);
            return;
        }
    }

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
    if (Action.Type == EGridCombatActionType::MeleeAttack)
    {
        if (ActiveAttackDefinition.IsValidDefinition ())
        {
            return FMath::Max (0.01f, ActiveAttackDefinition.ExpectedDuration);
        }
        if (const FGridMonsterAttackDefinition* Attack =
            CurrentMonster->MonsterDefinition->FindAttackDefinition (Action.AttackId))
        {
            return FMath::Max (0.01f, Attack->ExpectedDuration);
        }
    }
    return 0.01f;
}

void UGridTurnManagerComponent::ResetActiveAttackState ()
{
    ActiveAttackDefinition = FGridMonsterAttackDefinition ();
    ActiveAttackImpactTimeRemaining = 0.0f;
    ActiveAttackCompleteTimeRemaining = 0.0f;
    bActiveAttackImpactCommitted = false;
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

void UGridTurnManagerComponent::BindCurrentCombat (
    UGridMonsterCombatComponent* CombatComponent)
{
    UnbindCurrentCombat ();
    CurrentCombatComponent = CombatComponent;
    if (!CurrentCombatComponent)
    {
        return;
    }

    CurrentCombatComponent->OnAttackImpactNotify.AddDynamic (
        this,
        &UGridTurnManagerComponent::HandleMonsterAttackImpactNotify);
    CurrentCombatComponent->OnActionCompleteNotify.AddDynamic (
        this,
        &UGridTurnManagerComponent::HandleMonsterActionCompleteNotify);
}

void UGridTurnManagerComponent::UnbindCurrentCombat ()
{
    if (CurrentCombatComponent)
    {
        CurrentCombatComponent->OnAttackImpactNotify.RemoveDynamic (
            this,
            &UGridTurnManagerComponent::HandleMonsterAttackImpactNotify);
        CurrentCombatComponent->OnActionCompleteNotify.RemoveDynamic (
            this,
            &UGridTurnManagerComponent::HandleMonsterActionCompleteNotify);
    }
    CurrentCombatComponent = nullptr;
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

void UGridTurnManagerComponent::HandleMonsterAttackImpactNotify ()
{
    NotifyActiveAttackImpact ();
}

void UGridTurnManagerComponent::HandleMonsterActionCompleteNotify ()
{
    NotifyActiveAttackComplete ();
}
