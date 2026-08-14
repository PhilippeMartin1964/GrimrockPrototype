#include "Runtime/Monsters/GridAutomaticPerceptionEngagementSubsystem.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterPatrolSubsystem.h"

DEFINE_LOG_CATEGORY (LogGridAutomaticEngagement);

namespace
{
    int32 GVisualSourceRequirementDepth = 0;

    bool IsPartyBetweenGridCells (
        const AGridLevelRuntimeActor* RuntimeActor,
        const AGrimrockPartyPawn* PartyPawn)
    {
        if (!IsValid (RuntimeActor) || !IsValid (PartyPawn))
        {
            return false;
        }

        const FVector CurrentLocation = PartyPawn->GetActorLocation ();
        const FVector CellCenter = RuntimeActor->GetCellCenterWorld (
            PartyPawn->CurrentCellX,
            PartyPawn->CurrentCellY,
            CurrentLocation.Z);
        const FVector2D DeltaXY (
            CurrentLocation.X - CellCenter.X,
            CurrentLocation.Y - CellCenter.Y);
        return !DeltaXY.IsNearlyZero (0.5f);
    }
}

namespace GridAutomaticPerceptionEngagement
{
    FScopedVisualSourceRequirement::FScopedVisualSourceRequirement ()
    {
        ++GVisualSourceRequirementDepth;
    }

    FScopedVisualSourceRequirement::~FScopedVisualSourceRequirement ()
    {
        GVisualSourceRequirementDepth = FMath::Max (0, GVisualSourceRequirementDepth - 1);
    }

    bool IsVisualSourceRequired ()
    {
        return GVisualSourceRequirementDepth > 0;
    }

    void Request (
        AGridLevelRuntimeActor* RuntimeActor,
        FName Reason)
    {
        if (!IsValid (RuntimeActor))
        {
            return;
        }

        UWorld* World = RuntimeActor->GetWorld ();
        if (!World || !World->IsGameWorld ())
        {
            return;
        }

        if (UGridAutomaticPerceptionEngagementSubsystem* Subsystem =
            World->GetSubsystem<UGridAutomaticPerceptionEngagementSubsystem> ())
        {
            Subsystem->RequestEvaluation (RuntimeActor, Reason);
        }
    }
}

void UGridAutomaticPerceptionEngagementSubsystem::RequestEvaluation (
    AGridLevelRuntimeActor* RuntimeActor,
    FName Reason)
{
    UWorld* World = GetWorld ();
    if (!IsValid (RuntimeActor) || !World || !World->IsGameWorld () ||
        RuntimeActor->GetWorld () != World)
    {
        return;
    }

    if (UGridMonsterPatrolSubsystem* PatrolSubsystem =
        World->GetSubsystem<UGridMonsterPatrolSubsystem> ())
    {
        PatrolSubsystem->RegisterRuntime (RuntimeActor);
    }

    ++QueuedRequestCount;
    PendingRuntimeActor = RuntimeActor;
    if (!Reason.IsNone ())
    {
        PendingReason = Reason;
    }

    if (bEvaluationQueued)
    {
        return;
    }

    bEvaluationQueued = true;
    World->GetTimerManager ().SetTimerForNextTick (
        FTimerDelegate::CreateUObject (
            this,
            &UGridAutomaticPerceptionEngagementSubsystem::HandleDeferredEvaluation));
}

void UGridAutomaticPerceptionEngagementSubsystem::HandleDeferredEvaluation ()
{
    ProcessPendingEvaluationNow ();
}

void UGridAutomaticPerceptionEngagementSubsystem::RequeueAfterUnsafeRuntime (
    AGridLevelRuntimeActor* RuntimeActor)
{
    if (!IsValid (RuntimeActor) || RuntimeActor->GetWorld () != GetWorld ())
    {
        return;
    }

    UWorld* World = GetWorld ();
    if (!World || !World->IsGameWorld ())
    {
        return;
    }

    PendingRuntimeActor = RuntimeActor;
    bEvaluationQueued = true;
    World->GetTimerManager ().SetTimerForNextTick (
        FTimerDelegate::CreateUObject (
            this,
            &UGridAutomaticPerceptionEngagementSubsystem::HandleDeferredEvaluation));
}

bool UGridAutomaticPerceptionEngagementSubsystem::ProcessPendingEvaluationNow ()
{
    AGridLevelRuntimeActor* RuntimeActor = PendingRuntimeActor.Get ();
    const FName Reason = PendingReason;

    bEvaluationQueued = false;
    PendingRuntimeActor.Reset ();
    PendingReason = NAME_None;

    UWorld* World = GetWorld ();
    if (!IsValid (RuntimeActor) || !World || !World->IsGameWorld () ||
        RuntimeActor->GetWorld () != World)
    {
        return false;
    }

    if (RuntimeActor->bIsExecutingDungeonTransition)
    {
        PendingReason = Reason;
        RequeueAfterUnsafeRuntime (RuntimeActor);
        return false;
    }

    UGridTurnManagerComponent* TurnManager =
        RuntimeActor->FindComponentByClass<UGridTurnManagerComponent> ();
    if (!TurnManager)
    {
        UE_LOG (LogGridAutomaticEngagement, Verbose,
            TEXT ("[MON14.1] Evaluation skipped Runtime=%s Reason=%s Cause=MissingTurnManager"),
            *GetNameSafe (RuntimeActor),
            *Reason.ToString ());
        return false;
    }

    if (!TurnManager->IsInitialized () &&
        !TurnManager->InitializeTurnManager (RuntimeActor, nullptr))
    {
        UE_LOG (LogGridAutomaticEngagement, Verbose,
            TEXT ("[MON14.1] Evaluation skipped Runtime=%s Reason=%s Cause=TurnManagerNotReady"),
            *GetNameSafe (RuntimeActor),
            *Reason.ToString ());
        return false;
    }

    if (IsPartyBetweenGridCells (RuntimeActor, TurnManager->PartyPawn))
    {
        PendingReason = Reason;
        RequeueAfterUnsafeRuntime (RuntimeActor);
        return false;
    }

    // Victory is a terminal combat presentation state but StartCombatInternal
    // already knows how to reset it before a later encounter. Allowing a safe
    // post-victory perception pass is required for guards that keep patrolling
    // elsewhere in the same dungeon level.
    const bool bExplorationCompatiblePhase =
        TurnManager->CurrentPhase == EGridCombatPhase::Exploration ||
        (!TurnManager->bCombatActive &&
            TurnManager->CurrentPhase == EGridCombatPhase::Victory);
    if (TurnManager->bCombatActive ||
        !bExplorationCompatiblePhase ||
        TurnManager->IsExecutingAction () ||
        TurnManager->IsPartyMotionInProgress ())
    {
        UE_LOG (LogGridAutomaticEngagement, Verbose,
            TEXT ("[MON14.1] Evaluation skipped Runtime=%s Reason=%s Cause=UnsafeCombatState Phase=%d Active=%s Action=%s Motion=%s"),
            *GetNameSafe (RuntimeActor),
            *Reason.ToString (),
            static_cast<int32> (TurnManager->CurrentPhase),
            TurnManager->bCombatActive ? TEXT ("true") : TEXT ("false"),
            TurnManager->IsExecutingAction () ? TEXT ("true") : TEXT ("false"),
            TurnManager->IsPartyMotionInProgress () ? TEXT ("true") : TEXT ("false"));
        return false;
    }

    ++EffectiveEvaluationCount;

    bool bStartedCombat = false;
    {
        GridAutomaticPerceptionEngagement::FScopedVisualSourceRequirement VisualOnlyScope;
        bStartedCombat = TurnManager->StartCombatFromPerception ();
    }

    if (bStartedCombat)
    {
        ++SuccessfulStartCount;
        UE_LOG (LogGridAutomaticEngagement, Log,
            TEXT ("[MON14.1] Automatic combat started Runtime=%s Reason=%s Evaluation=%d"),
            *GetNameSafe (RuntimeActor),
            *Reason.ToString (),
            EffectiveEvaluationCount);
    }
    else
    {
        UE_LOG (LogGridAutomaticEngagement, Verbose,
            TEXT ("[MON14.1] No automatic engagement Runtime=%s Reason=%s Evaluation=%d"),
            *GetNameSafe (RuntimeActor),
            *Reason.ToString (),
            EffectiveEvaluationCount);
    }

    if (UGridMonsterPatrolSubsystem* PatrolSubsystem =
        World->GetSubsystem<UGridMonsterPatrolSubsystem> ())
    {
        PatrolSubsystem->HandlePerceptionEvaluation (
            RuntimeActor,
            bStartedCombat,
            Reason);
    }

    return bStartedCombat;
}
