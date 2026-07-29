#pragma once

#include "CoreMinimal.h"
#include "GridCombatDiagnostics.generated.h"

DECLARE_LOG_CATEGORY_EXTERN (LogGridCombatPerformance, Log, All);

/** Builds the deterministic gameplay seed for one stable encounter. */
class GRIMROCKPROTOTYPE_API FGridEncounterSeedBuilder
{
public:
    static int32 BuildEncounterSeed (
        int32 BaseSeed,
        FName DungeonLevelId,
        const TArray<FGuid>& ParticipantIds);
};

/** Optional counters and timings. They are never consulted by gameplay. */
USTRUCT (BlueprintType)
struct FGridCombatRuntimeMetrics
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Transient)
    int32 SuccessfulCombatStarts = 0;

    UPROPERTY (BlueprintReadOnly, Transient)
    int32 RejectedCombatStarts = 0;

    UPROPERTY (BlueprintReadOnly, Transient)
    int32 RoundsStarted = 0;

    UPROPERTY (BlueprintReadOnly, Transient)
    int32 MonsterTurnsStarted = 0;

    UPROPERTY (BlueprintReadOnly, Transient)
    int32 ActionsStarted = 0;

    UPROPERTY (BlueprintReadOnly, Transient)
    int32 ActionsCompleted = 0;

    UPROPERTY (BlueprintReadOnly, Transient)
    int32 AttacksResolved = 0;

    UPROPERTY (BlueprintReadOnly, Transient)
    int32 PerceptionCandidatesScanned = 0;

    UPROPERTY (BlueprintReadOnly, Transient)
    int32 PeakCombatMonsterCount = 0;

    UPROPERTY (BlueprintReadOnly, Transient)
    int32 PeakPendingActionCount = 0;

    UPROPERTY (BlueprintReadOnly, Transient)
    int32 TurnManagerTickFrames = 0;

    UPROPERTY (BlueprintReadOnly, Transient)
    float ActiveTickSeconds = 0.0f;

    UPROPERTY (BlueprintReadOnly, Transient)
    float LastPerceptionEvaluationMilliseconds = 0.0f;

    UPROPERTY (BlueprintReadOnly, Transient)
    float MaximumPerceptionEvaluationMilliseconds = 0.0f;

    UPROPERTY (BlueprintReadOnly, Transient)
    float LastTurnPlanningMilliseconds = 0.0f;

    UPROPERTY (BlueprintReadOnly, Transient)
    float MaximumTurnPlanningMilliseconds = 0.0f;

    void Reset ();
};
