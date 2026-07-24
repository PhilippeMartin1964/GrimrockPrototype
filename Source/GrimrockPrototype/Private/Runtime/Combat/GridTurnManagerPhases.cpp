#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "Core/GridDirectionUtils.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterCombatComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"

namespace
{
    int32 GetMonsterInitiative (const AGridMonsterActor* Monster)
    {
        return Monster && Monster->MonsterDefinition
            ? Monster->MonsterDefinition->Initiative
            : MIN_int32;
    }
}

void UGridTurnManagerComponent::BeginRound ()
{
    if (!bCombatActive || !PhaseState.BeginRound ())
    {
        return;
    }

    RoundNumber = PhaseState.GetRoundNumber ();
    SetPhase (PhaseState.GetPhase ());
    SetPartyInputLocked (false);
    OnRoundStarted.Broadcast (RoundNumber);
}

void UGridTurnManagerComponent::BeginEnemyPhase ()
{
    if (!HasLivingPartyCharacter ())
    {
        FinishCombat (EGridCombatPhase::Defeat);
        return;
    }

    if (!HasLivingCombatMonster ())
    {
        FinishCombat (EGridCombatPhase::Victory);
        return;
    }

    BuildEnemyTurnOrder ();
    CurrentEnemyIndex = INDEX_NONE;
    BeginNextMonsterTurn ();
}

void UGridTurnManagerComponent::BuildEnemyTurnOrder ()
{
    EnemyTurnOrder.Reset ();
    for (AGridMonsterActor* Monster : CombatMonsters)
    {
        if (IsValid (Monster) && !Monster->IsDead ())
        {
            EnemyTurnOrder.Add (Monster);
        }
    }

    EnemyTurnOrder.Sort ([] (
        const TObjectPtr<AGridMonsterActor>& Left,
        const TObjectPtr<AGridMonsterActor>& Right)
    {
        const int32 LeftInitiative = GetMonsterInitiative (Left.Get ());
        const int32 RightInitiative = GetMonsterInitiative (Right.Get ());
        if (LeftInitiative != RightInitiative)
        {
            return LeftInitiative > RightInitiative;
        }

        const FString LeftId = Left && Left->SpawnObjectId.IsValid ()
            ? Left->SpawnObjectId.ToString (EGuidFormats::Digits)
            : GetNameSafe (Left.Get ());
        const FString RightId = Right && Right->SpawnObjectId.IsValid ()
            ? Right->SpawnObjectId.ToString (EGuidFormats::Digits)
            : GetNameSafe (Right.Get ());
        return LeftId < RightId;
    });
}

void UGridTurnManagerComponent::BeginNextMonsterTurn ()
{
    UnbindCurrentMovement ();
    UnbindCurrentCombat ();
    CurrentMonster = nullptr;
    PendingActions.Reset ();
    bHasActiveAction = false;
    ActiveAction = FGridCombatAction ();
    ActiveActionTimeoutRemaining = 0.0f;
    ResetActiveAttackState ();

    while (++CurrentEnemyIndex < EnemyTurnOrder.Num ())
    {
        AGridMonsterActor* Candidate = EnemyTurnOrder[CurrentEnemyIndex];
        if (!IsValid (Candidate) || Candidate->IsDead () || !PrepareMonsterForCombat (Candidate))
        {
            continue;
        }

        CurrentMonster = Candidate;
        CurrentMonsterMaximumActionPoints = FMath::Max (
            0,
            Candidate->MonsterDefinition->ActionPointsPerTurn);
        ActionPointBudget.Reset (CurrentMonsterMaximumActionPoints);
        CurrentMonsterRemainingActionPoints = ActionPointBudget.GetRemainingPoints ();

        BindCurrentMovement (
            Candidate->FindComponentByClass<UGridMonsterMovementComponent> ());
        BindCurrentCombat (
            Candidate->FindComponentByClass<UGridMonsterCombatComponent> ());
        OnMonsterTurnStarted.Broadcast (Candidate);
        PrepareCurrentMonsterActions ();
        ExecuteNextAction ();
        return;
    }

    FinishEnemyPhase ();
}

void UGridTurnManagerComponent::PrepareCurrentMonsterActions ()
{
    PendingActions.Reset ();
    if (!IsValid (CurrentMonster) || CurrentMonster->IsDead ())
    {
        return;
    }

    UGridMonsterBehaviorComponent* Behavior =
        CurrentMonster->FindComponentByClass<UGridMonsterBehaviorComponent> ();
    if (!Behavior)
    {
        FGridMonsterTurnPlanner::BuildMovementTurn (
            CurrentMonster->SpawnObjectId,
            CurrentMonster->CurrentCell,
            CurrentMonster->Facing,
            TArray<FIntPoint> (),
            CurrentMonsterRemainingActionPoints,
            PendingActions);
        return;
    }

    Behavior->RefreshPerception ();
    const bool bHasPartyPerception = Behavior->HasPartyPerception ();
    bool bFoundPath = false;
    if (bHasPartyPerception)
    {
        bFoundPath = Behavior->FindPursuitPath ();
    }
    else if (Behavior->bHasLastKnownPartyCell)
    {
        bFoundPath = Behavior->FindPathToLastKnownPartyCell ();
    }

    const TArray<FIntPoint> EmptyPath;
    const TArray<FIntPoint>& PlannedPath = bFoundPath ? Behavior->LastPath : EmptyPath;

    FGridMonsterAttackDefinition MeleeAttack;
    const bool bCanPlanMelee =
        bHasPartyPerception &&
        IsValid (CurrentCombatComponent) &&
        CurrentCombatComponent->GetPreferredMeleeAttack (MeleeAttack) &&
        IsValid (PartyPawn);

    if (bCanPlanMelee)
    {
        FGridMonsterTurnPlanner::BuildDirectMeleeTurn (
            CurrentMonster->SpawnObjectId,
            CurrentMonster->CurrentCell,
            CurrentMonster->Facing,
            FIntPoint (PartyPawn->CurrentCellX, PartyPawn->CurrentCellY),
            PlannedPath,
            CurrentMonsterRemainingActionPoints,
            MeleeAttack.AttackId,
            MeleeAttack.ActionPointCost,
            PendingActions);
        return;
    }

    FGridMonsterTurnPlanner::BuildMovementTurn (
        CurrentMonster->SpawnObjectId,
        CurrentMonster->CurrentCell,
        CurrentMonster->Facing,
        PlannedPath,
        CurrentMonsterRemainingActionPoints,
        PendingActions);
}

void UGridTurnManagerComponent::FinishCurrentMonsterTurn ()
{
    AGridMonsterActor* CompletedMonster = CurrentMonster;
    UnbindCurrentMovement ();
    UnbindCurrentCombat ();
    PendingActions.Reset ();
    bHasActiveAction = false;
    ActiveAction = FGridCombatAction ();
    ActiveActionTimeoutRemaining = 0.0f;
    ResetActiveAttackState ();
    CurrentMonsterMaximumActionPoints = 0;
    CurrentMonsterRemainingActionPoints = 0;
    ActionPointBudget.Reset (0);
    CurrentMonster = nullptr;

    if (IsValid (CompletedMonster))
    {
        OnMonsterTurnEnded.Broadcast (CompletedMonster);
    }

    BeginNextMonsterTurn ();
}

void UGridTurnManagerComponent::FinishEnemyPhase ()
{
    if (!bCombatActive || !PhaseState.CompleteEnemyPhase ())
    {
        return;
    }

    SetPhase (PhaseState.GetPhase ());

    if (!HasLivingPartyCharacter ())
    {
        FinishCombat (EGridCombatPhase::Defeat);
        return;
    }

    if (!HasLivingCombatMonster ())
    {
        FinishCombat (EGridCombatPhase::Victory);
        return;
    }

    if (!PhaseState.BeginNextRound ())
    {
        AbortCombat ();
        return;
    }

    RoundNumber = PhaseState.GetRoundNumber ();
    SetPhase (PhaseState.GetPhase ());
    SetPartyInputLocked (false);
    OnRoundStarted.Broadcast (RoundNumber);
}

void UGridTurnManagerComponent::FinishCombat (EGridCombatPhase ResultPhase)
{
    if (!bCombatActive || !PhaseState.FinishCombat (ResultPhase))
    {
        return;
    }

    if (CurrentMovementComponent && CurrentMovementComponent->IsBusy ())
    {
        CurrentMovementComponent->CancelCurrentAction ();
    }
    if (CurrentCombatComponent)
    {
        CurrentCombatComponent->CancelAttackPresentation ();
    }

    UnbindCurrentMovement ();
    UnbindCurrentCombat ();
    bWaitingForCombatStart = false;
    CombatStartDelayRemaining = 0.0f;
    ActiveActionTimeoutRemaining = 0.0f;
    bHasActiveAction = false;
    ActiveAction = FGridCombatAction ();
    PendingActions.Reset ();
    EnemyTurnOrder.Reset ();
    CurrentMonster = nullptr;
    CurrentEnemyIndex = INDEX_NONE;
    CurrentMonsterMaximumActionPoints = 0;
    CurrentMonsterRemainingActionPoints = 0;
    ActionPointBudget.Reset (0);
    ResetActiveAttackState ();
    bCombatActive = false;

    SetPhase (PhaseState.GetPhase ());
    SetPartyInputLocked (false);
    OnCombatEnded.Broadcast (ResultPhase);
    RefreshTickEnabled ();
}

bool UGridTurnManagerComponent::IsPartyAtRest () const
{
    if (!IsValid (PartyPawn) || !IsValid (RuntimeActor))
    {
        return false;
    }

    const FVector ExpectedLocation = RuntimeActor->GetCellCenterWorld (
        PartyPawn->CurrentCellX,
        PartyPawn->CurrentCellY,
        PartyPawn->EyeHeight);
    const float LocationErrorSquared =
        FVector::DistSquared (PartyPawn->GetActorLocation (), ExpectedLocation);
    const float ExpectedYaw = GridDirectionUtils::ToYaw (PartyPawn->Facing);
    const float YawError = FMath::Abs (FMath::FindDeltaAngleDegrees (
        PartyPawn->GetActorRotation ().Yaw,
        ExpectedYaw));

    return LocationErrorSquared <= FMath::Square (2.0f) && YawError <= 1.0f;
}

bool UGridTurnManagerComponent::HasLivingCombatMonster () const
{
    for (const AGridMonsterActor* Monster : CombatMonsters)
    {
        if (IsValid (Monster) && !Monster->IsDead ())
        {
            return true;
        }
    }
    return false;
}

bool UGridTurnManagerComponent::HasLivingPartyCharacter () const
{
    if (!IsValid (PartyPawn) || !IsValid (PartyPawn->PartyInventoryComponent))
    {
        return false;
    }

    for (const FGridCharacterInventoryState& Character :
        PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters)
    {
        if (Character.DerivedStats.CurrentHealth > 0)
        {
            return true;
        }
    }
    return false;
}
