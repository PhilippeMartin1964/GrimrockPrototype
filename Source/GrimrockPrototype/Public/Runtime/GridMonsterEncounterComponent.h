#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GridMonsterEncounterComponent.generated.h"

class AGridLevelRuntimeActor;
struct FGridLevelObjectData;
struct FGridRuntimeMonsterEncounterState;

/** Coordinates persistent MON13.4 encounter waves owned by MonsterSpawn data. */
UCLASS (ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridMonsterEncounterComponent : public UActorComponent
{
    GENERATED_BODY ()

public:
    UGridMonsterEncounterComponent ();

    void Initialize (AGridLevelRuntimeActor* InRuntimeActor);

    bool StartEncounter (FGuid AnchorSpawnId);
    bool NotifyMonsterDied (FGuid SpawnId);

    bool IsEncounterCompleted (FName EncounterGroupId) const;
    int32 GetActiveWaveIndex (FName EncounterGroupId) const;

private:
    const FGridLevelObjectData* FindSpawn (FGuid SpawnId) const;
    int32 FindNextWaveIndex (
        const FGridRuntimeMonsterEncounterState& State,
        int32 AfterWaveIndex) const;
    bool IsWaveDefeated (
        const FGridRuntimeMonsterEncounterState& State,
        int32 WaveIndex) const;
    bool ActivateWave (
        FGridRuntimeMonsterEncounterState& State,
        int32 WaveIndex);
    bool SpawnWaveAtomically (
        FGridRuntimeMonsterEncounterState& State,
        int32 WaveIndex,
        TArray<FGuid>& OutNewlySpawnedIds);
    bool AdvanceCompletedWave (
        FGridRuntimeMonsterEncounterState& State);
    bool CompleteEncounter (
        FGridRuntimeMonsterEncounterState& State);

private:
    UPROPERTY (Transient)
    TObjectPtr<AGridLevelRuntimeActor> RuntimeActor;
};
