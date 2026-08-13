#include "Runtime/GridMonsterEncounterComponent.h"

#include "Core/GridLevelAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/Monsters/GridMonsterActor.h"

UGridMonsterEncounterComponent::UGridMonsterEncounterComponent ()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UGridMonsterEncounterComponent::Initialize (
    AGridLevelRuntimeActor* InRuntimeActor)
{
    RuntimeActor = InRuntimeActor;
}

const FGridLevelObjectData* UGridMonsterEncounterComponent::FindSpawn (
    FGuid SpawnId) const
{
    return RuntimeActor && RuntimeActor->LevelAsset
        ? RuntimeActor->LevelAsset->FindMonsterSpawnById (SpawnId)
        : nullptr;
}

int32 UGridMonsterEncounterComponent::FindNextWaveIndex (
    const FGridRuntimeMonsterEncounterState& State,
    int32 AfterWaveIndex) const
{
    if (!RuntimeActor || !RuntimeActor->LevelAsset ||
        State.EncounterGroupId.IsNone ())
    {
        return INDEX_NONE;
    }

    int32 NextWaveIndex = INDEX_NONE;
    for (const FGridLevelObjectData& ObjectData :
        RuntimeActor->LevelAsset->Objects)
    {
        if (ObjectData.Type != EGridLevelObjectType::MonsterSpawn ||
            ObjectData.EncounterGroupId != State.EncounterGroupId ||
            ObjectData.EncounterWaveIndex <= AfterWaveIndex ||
            State.DefeatedSpawnIds.Contains (ObjectData.ObjectId))
        {
            continue;
        }

        if (NextWaveIndex == INDEX_NONE ||
            ObjectData.EncounterWaveIndex < NextWaveIndex)
        {
            NextWaveIndex = ObjectData.EncounterWaveIndex;
        }
    }
    return NextWaveIndex;
}

bool UGridMonsterEncounterComponent::IsWaveDefeated (
    const FGridRuntimeMonsterEncounterState& State,
    int32 WaveIndex) const
{
    if (!RuntimeActor || !RuntimeActor->LevelAsset ||
        State.EncounterGroupId.IsNone () || WaveIndex < 0)
    {
        return false;
    }

    bool bFoundWaveMember = false;
    for (const FGridLevelObjectData& ObjectData :
        RuntimeActor->LevelAsset->Objects)
    {
        if (ObjectData.Type != EGridLevelObjectType::MonsterSpawn ||
            ObjectData.EncounterGroupId != State.EncounterGroupId ||
            ObjectData.EncounterWaveIndex != WaveIndex)
        {
            continue;
        }

        bFoundWaveMember = true;
        if (!State.DefeatedSpawnIds.Contains (ObjectData.ObjectId))
        {
            return false;
        }
    }
    return bFoundWaveMember;
}

bool UGridMonsterEncounterComponent::SpawnWaveAtomically (
    FGridRuntimeMonsterEncounterState& State,
    int32 WaveIndex,
    TArray<FGuid>& OutNewlySpawnedIds)
{
    OutNewlySpawnedIds.Reset ();
    if (!RuntimeActor || !RuntimeActor->LevelAsset || WaveIndex < 0)
    {
        return false;
    }

    TArray<const FGridLevelObjectData*> WaveSpawns;
    for (const FGridLevelObjectData& ObjectData :
        RuntimeActor->LevelAsset->Objects)
    {
        if (ObjectData.Type == EGridLevelObjectType::MonsterSpawn &&
            ObjectData.EncounterGroupId == State.EncounterGroupId &&
            ObjectData.EncounterWaveIndex == WaveIndex)
        {
            WaveSpawns.Add (&ObjectData);
        }
    }
    WaveSpawns.Sort (
        [] (const FGridLevelObjectData& Left,
            const FGridLevelObjectData& Right)
        {
            return Left.ObjectId.ToString (EGuidFormats::Digits) <
                Right.ObjectId.ToString (EGuidFormats::Digits);
        });
    if (WaveSpawns.IsEmpty ())
    {
        return false;
    }

    FGridLevelRuntimeState* LevelState =
        RuntimeActor->GetOrCreateRuntimeStateForCurrentLevel ();
    if (!LevelState)
    {
        return false;
    }

    const TMap<FGuid, FGridRuntimeMonsterPlacementState>
        PreviousPlacements = LevelState->MonsterPlacements;
    const TMap<FGuid, FGridRuntimeMonsterState>
        PreviousMonsters = LevelState->Monsters;
    const bool bPreviouslyVisited = LevelState->bHasBeenVisited;
    const TSet<FGuid> PreviousDefeatedSpawnIds =
        State.DefeatedSpawnIds;

    for (const FGridLevelObjectData* ObjectData : WaveSpawns)
    {
        if (!ObjectData ||
            State.DefeatedSpawnIds.Contains (ObjectData->ObjectId))
        {
            continue;
        }

        if (AGridMonsterActor* ExistingMonster =
            RuntimeActor->FindSpawnedMonsterActor (
                ObjectData->ObjectId))
        {
            if (ExistingMonster->IsDead ())
            {
                State.DefeatedSpawnIds.Add (ObjectData->ObjectId);
            }
            continue;
        }

        const FGridRuntimeMonsterPlacementState* PreviousPlacement =
            LevelState->MonsterPlacements.Find (ObjectData->ObjectId);
        const FGridRuntimeMonsterState* RestoreState =
            PreviousPlacement && PreviousPlacement->bHasMonsterState
                ? &PreviousPlacement->MonsterState
                : nullptr;
        AGridMonsterActor* SpawnedMonster =
            RuntimeActor->AddMonsterSpawnActor (
                *ObjectData,
                RestoreState);
        if (!SpawnedMonster ||
            !RuntimeActor->StoreMonsterPlacementState (
                *ObjectData,
                SpawnedMonster,
                true))
        {
            auto RollbackSpawnedActor = [this] (FGuid SpawnId)
            {
                if (AGridMonsterActor* Monster =
                    RuntimeActor->FindSpawnedMonsterActor (SpawnId))
                {
                    RuntimeActor->SetMonsterRuntimeLevelActive (
                        Monster,
                        false);
                    RuntimeActor->SpawnedMonsterActors.Remove (SpawnId);
                    Monster->Destroy ();
                }
            };
            for (const FGuid& SpawnId : OutNewlySpawnedIds)
            {
                RollbackSpawnedActor (SpawnId);
            }
            if (SpawnedMonster)
            {
                RollbackSpawnedActor (ObjectData->ObjectId);
            }
            LevelState->MonsterPlacements = PreviousPlacements;
            LevelState->Monsters = PreviousMonsters;
            LevelState->bHasBeenVisited = bPreviouslyVisited;
            State.DefeatedSpawnIds = PreviousDefeatedSpawnIds;
            OutNewlySpawnedIds.Reset ();
            UE_LOG (LogGridMonsterState, Warning,
                TEXT ("[GridMonsterEncounter] WaveRejected Encounter=%s Wave=%d Reason=AtomicSpawnFailed"),
                *State.EncounterGroupId.ToString (),
                WaveIndex);
            return false;
        }

        OutNewlySpawnedIds.Add (ObjectData->ObjectId);
        if (SpawnedMonster->IsDead ())
        {
            State.DefeatedSpawnIds.Add (ObjectData->ObjectId);
        }
    }

    LevelState->bHasBeenVisited = true;
    RuntimeActor->AbortActiveCombatAndMonsterActions ();
    return true;
}

bool UGridMonsterEncounterComponent::ActivateWave (
    FGridRuntimeMonsterEncounterState& State,
    int32 WaveIndex)
{
    TArray<FGuid> NewlySpawnedIds;
    if (!SpawnWaveAtomically (
            State,
            WaveIndex,
            NewlySpawnedIds))
    {
        return false;
    }

    State.bStarted = true;
    State.ActiveWaveIndex = WaveIndex;
    UE_LOG (LogGridMonsterState, Log,
        TEXT ("[GridMonsterEncounter] WaveStarted Encounter=%s Wave=%d Spawned=%d Anchor=%s"),
        *State.EncounterGroupId.ToString (),
        WaveIndex,
        NewlySpawnedIds.Num (),
        *State.AnchorSpawnId.ToString (
            EGuidFormats::DigitsWithHyphens));

    for (const FGuid& SpawnId : NewlySpawnedIds)
    {
        RuntimeActor->ExecuteLinksFromRuntimeObject (
            SpawnId,
            EGridObjectEvent::MonsterSpawned);
    }
    RuntimeActor->ExecuteLinksFromRuntimeObject (
        State.AnchorSpawnId,
        EGridObjectEvent::EncounterWaveStarted);

    return IsWaveDefeated (State, WaveIndex)
        ? AdvanceCompletedWave (State)
        : true;
}

bool UGridMonsterEncounterComponent::CompleteEncounter (
    FGridRuntimeMonsterEncounterState& State)
{
    if (State.bCompleted)
    {
        return true;
    }

    State.bCompleted = true;
    State.ActiveWaveIndex = INDEX_NONE;
    UE_LOG (LogGridMonsterState, Log,
        TEXT ("[GridMonsterEncounter] Completed Encounter=%s Defeated=%d Anchor=%s"),
        *State.EncounterGroupId.ToString (),
        State.DefeatedSpawnIds.Num (),
        *State.AnchorSpawnId.ToString (
            EGuidFormats::DigitsWithHyphens));
    RuntimeActor->ExecuteLinksFromRuntimeObject (
        State.AnchorSpawnId,
        EGridObjectEvent::EncounterCompleted);
    return true;
}

bool UGridMonsterEncounterComponent::AdvanceCompletedWave (
    FGridRuntimeMonsterEncounterState& State)
{
    if (!State.bStarted || State.bCompleted ||
        !IsWaveDefeated (State, State.ActiveWaveIndex))
    {
        return State.bStarted;
    }

    const int32 NextWaveIndex = FindNextWaveIndex (
        State,
        State.ActiveWaveIndex);
    return NextWaveIndex == INDEX_NONE
        ? CompleteEncounter (State)
        : ActivateWave (State, NextWaveIndex);
}

bool UGridMonsterEncounterComponent::StartEncounter (
    FGuid AnchorSpawnId)
{
    const FGridLevelObjectData* AnchorSpawn = FindSpawn (AnchorSpawnId);
    if (!AnchorSpawn || AnchorSpawn->EncounterGroupId.IsNone ())
    {
        UE_LOG (LogGridMonsterState, Warning,
            TEXT ("[GridMonsterEncounter] StartRejected Anchor=%s Reason=MissingEncounterGroup"),
            *AnchorSpawnId.ToString (
                EGuidFormats::DigitsWithHyphens));
        return false;
    }

    FGridLevelRuntimeState* LevelState =
        RuntimeActor->GetOrCreateRuntimeStateForCurrentLevel ();
    if (!LevelState)
    {
        return false;
    }

    FGridRuntimeMonsterEncounterState& State =
        LevelState->MonsterEncounters.FindOrAdd (
            AnchorSpawn->EncounterGroupId);
    State.EncounterGroupId = AnchorSpawn->EncounterGroupId;
    if (!State.AnchorSpawnId.IsValid ())
    {
        State.AnchorSpawnId = AnchorSpawnId;
    }

    if (State.bCompleted)
    {
        return true;
    }
    if (State.bStarted)
    {
        return IsWaveDefeated (State, State.ActiveWaveIndex)
            ? AdvanceCompletedWave (State)
            : true;
    }

    const int32 FirstWaveIndex = FindNextWaveIndex (State, INDEX_NONE);
    if (FirstWaveIndex == INDEX_NONE)
    {
        UE_LOG (LogGridMonsterState, Warning,
            TEXT ("[GridMonsterEncounter] StartRejected Encounter=%s Reason=NoUndefeatedWave"),
            *State.EncounterGroupId.ToString ());
        return false;
    }
    return ActivateWave (State, FirstWaveIndex);
}

bool UGridMonsterEncounterComponent::NotifyMonsterDied (
    FGuid SpawnId)
{
    if (!RuntimeActor || !SpawnId.IsValid ())
    {
        return false;
    }

    const FGridLevelObjectData* Spawn =
        FindSpawn (SpawnId);
    FGridLevelRuntimeState* LevelState =
        RuntimeActor->GetOrCreateRuntimeStateForCurrentLevel ();
    FGridRuntimeMonsterEncounterState* State =
        Spawn && LevelState && !Spawn->EncounterGroupId.IsNone ()
            ? LevelState->MonsterEncounters.Find (
                Spawn->EncounterGroupId)
            : nullptr;
    if (!State || !State->bStarted || State->bCompleted ||
        Spawn->EncounterWaveIndex != State->ActiveWaveIndex)
    {
        return false;
    }

    const bool bNewDefeat =
        !State->DefeatedSpawnIds.Contains (Spawn->ObjectId);
    State->DefeatedSpawnIds.Add (Spawn->ObjectId);
    if (bNewDefeat)
    {
        UE_LOG (LogGridMonsterState, Log,
            TEXT ("[GridMonsterEncounter] MemberDefeated Encounter=%s Wave=%d SpawnId=%s"),
            *State->EncounterGroupId.ToString (),
            State->ActiveWaveIndex,
            *Spawn->ObjectId.ToString (
                EGuidFormats::DigitsWithHyphens));
    }
    return IsWaveDefeated (*State, State->ActiveWaveIndex)
        ? AdvanceCompletedWave (*State)
        : true;
}

bool UGridMonsterEncounterComponent::IsEncounterCompleted (
    FName EncounterGroupId) const
{
    const FGridLevelRuntimeState* LevelState = RuntimeActor
        ? RuntimeActor->FindRuntimeStateForCurrentLevel ()
        : nullptr;
    const FGridRuntimeMonsterEncounterState* State =
        LevelState
            ? LevelState->MonsterEncounters.Find (EncounterGroupId)
            : nullptr;
    return State && State->bCompleted;
}

int32 UGridMonsterEncounterComponent::GetActiveWaveIndex (
    FName EncounterGroupId) const
{
    const FGridLevelRuntimeState* LevelState = RuntimeActor
        ? RuntimeActor->FindRuntimeStateForCurrentLevel ()
        : nullptr;
    const FGridRuntimeMonsterEncounterState* State =
        LevelState
            ? LevelState->MonsterEncounters.Find (EncounterGroupId)
            : nullptr;
    return State && State->bStarted && !State->bCompleted
        ? State->ActiveWaveIndex
        : INDEX_NONE;
}
