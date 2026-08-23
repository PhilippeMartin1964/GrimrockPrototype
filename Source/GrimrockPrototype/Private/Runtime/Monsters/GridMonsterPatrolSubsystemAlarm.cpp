#include "Runtime/Monsters/GridMonsterPatrolSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterBehaviorComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterFastHarasserPlanner.h"

int32 UGridMonsterPatrolSubsystem::HandleExplorationAlert (
    AGridMonsterActor* SourceMonster,
    const FIntPoint& KnownPartyCell,
    FName Reason)
{
    if (!IsValid (SourceMonster) ||
        SourceMonster->IsDead () ||
        !SourceMonster->bMonsterEnabled ||
        !SourceMonster->IsRuntimeLevelActive () ||
        !IsValid (SourceMonster->MonsterDefinition) ||
        !SourceMonster->MonsterDefinition->bSharesAggroWithGroup ||
        SourceMonster->EncounterGroupId.IsNone () ||
        !GetWorld ())
    {
        return 0;
    }

    AGridLevelRuntimeActor* RuntimeActor =
        Cast<AGridLevelRuntimeActor> (SourceMonster->GetOwner ());
    if (!IsValid (RuntimeActor))
    {
        RuntimeActor = BoundRuntimeActor.Get ();
    }
    if (!IsValid (RuntimeActor) ||
        !RuntimeActor->IsValidCell (KnownPartyCell.X, KnownPartyCell.Y))
    {
        return 0;
    }

    RegisterRuntime (RuntimeActor);
    if (!IsRuntimeSafeForExploration (RuntimeActor))
    {
        return 0;
    }

    TArray<FGridMonsterAggroCandidate> Candidates;
    TMap<FGuid, AGridMonsterActor*> CandidateActors;

    for (TActorIterator<AGridMonsterActor> It (GetWorld ()); It; ++It)
    {
        AGridMonsterActor* CandidateMonster = *It;
        if (!IsMonsterOwnedByRuntime (CandidateMonster, RuntimeActor) ||
            !IsValid (CandidateMonster->MonsterDefinition))
        {
            continue;
        }

        const FGuid CandidateId = CandidateMonster->ResolvePersistenceId ();
        if (!CandidateId.IsValid ())
        {
            continue;
        }

        FGridMonsterAggroCandidate Candidate;
        Candidate.SpawnObjectId = CandidateId;
        Candidate.MonsterId = CandidateMonster->MonsterDefinition->MonsterId;
        Candidate.EncounterGroupId = CandidateMonster->EncounterGroupId;
        Candidate.Cell = CandidateMonster->CurrentCell;
        Candidate.bIsAlive = !CandidateMonster->IsDead ();
        Candidate.bIsEnabled = CandidateMonster->bMonsterEnabled;
        Candidates.Add (Candidate);
        CandidateActors.Add (CandidateId, CandidateMonster);
    }

    TArray<FGuid> TargetIds;
    FGridFastHarasserPlanner::SelectAggroTargets (
        SourceMonster->ResolvePersistenceId (),
        SourceMonster->MonsterDefinition->MonsterId,
        SourceMonster->EncounterGroupId,
        SourceMonster->CurrentCell,
        SourceMonster->MonsterDefinition->AggroPropagationRange,
        Candidates,
        TargetIds);

    int32 AlertedCount = 0;
    for (const FGuid& TargetId : TargetIds)
    {
        AGridMonsterActor* TargetMonster = CandidateActors.FindRef (TargetId);
        if (!IsValid (TargetMonster) || TargetMonster == SourceMonster)
        {
            continue;
        }

        FRuntimeEntry* TargetEntry = FindOrAddEntry (TargetMonster);
        UGridMonsterBehaviorComponent* TargetBehavior =
            TargetMonster->FindComponentByClass<UGridMonsterBehaviorComponent> ();
        if (!TargetEntry || !TargetBehavior)
        {
            continue;
        }

        if (!TargetBehavior->IsInitialized () &&
            !TargetBehavior->InitializeBehavior (
                RuntimeActor,
                BoundTurnManager.IsValid () ? BoundTurnManager->PartyPawn : nullptr))
        {
            continue;
        }

        if (TargetEntry->Activity == EGridMonsterExplorationActivity::Engaging)
        {
            continue;
        }

        const bool bAlreadyInvestigatingSameCell =
            TargetBehavior->bHasLastKnownPartyCell &&
            TargetBehavior->LastKnownPartyCell == KnownPartyCell &&
            (TargetEntry->Activity == EGridMonsterExplorationActivity::Investigating ||
             TargetEntry->Activity == EGridMonsterExplorationActivity::Searching) &&
            (TargetMonster->MonsterState == EGridMonsterState::Alert ||
             TargetMonster->MonsterState == EGridMonsterState::Pursuing);
        if (bAlreadyInvestigatingSameCell)
        {
            continue;
        }

        CancelScheduledStep (*TargetEntry);
        CancelExplorationMotion (*TargetEntry);

        TargetBehavior->bHasLastKnownPartyCell = true;
        TargetBehavior->LastKnownPartyCell = KnownPartyCell;
        TargetMonster->SetMonsterState (EGridMonsterState::Alert);
        TargetEntry->Activity = EGridMonsterExplorationActivity::Investigating;
        TargetEntry->SearchTurnsRemaining = 0;
        ScheduleStep (*TargetEntry, 0.01f);
        ++AlertedCount;
    }

    if (AlertedCount > 0)
    {
        UE_LOG (
            LogGridMonsterPatrol,
            Log,
            TEXT ("[MON14.4] ExplorationAlert Source=%s Group=%s Cell=(%d,%d) Range=%d Alerted=%d Reason=%s"),
            *GetNameSafe (SourceMonster),
            *SourceMonster->EncounterGroupId.ToString (),
            KnownPartyCell.X,
            KnownPartyCell.Y,
            SourceMonster->MonsterDefinition->AggroPropagationRange,
            AlertedCount,
            *Reason.ToString ());
    }

    return AlertedCount;
}
