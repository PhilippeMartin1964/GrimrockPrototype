#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"

struct GRIMROCKPROTOTYPE_API FGridRetreatCandidate
{
    FIntPoint Cell = FIntPoint::ZeroValue;
    EGridEdge Direction = EGridEdge::None;
    int32 DistanceToParty = 0;
    int32 ExitCount = 0;
    bool bCanContinuePursuit = false;
    bool bIsCulDeSac = false;
    int32 StableDirectionOrder = MAX_int32;
};

struct GRIMROCKPROTOTYPE_API FGridRetreatDecision
{
    bool bHasRetreatCell = false;
    FIntPoint RetreatCell = FIntPoint::ZeroValue;
    EGridEdge RetreatDirection = EGridEdge::None;
    int32 Score = MIN_int32;
};

struct GRIMROCKPROTOTYPE_API FGridMonsterAggroCandidate
{
    FGuid SpawnObjectId;
    FName MonsterId = NAME_None;
    FName EncounterGroupId = NAME_None;
    FIntPoint Cell = FIntPoint::ZeroValue;
    bool bIsAlive = true;
    bool bIsEnabled = true;
};

/** Pure deterministic rules shared by MON7 runtime code and Automation Tests. */
class GRIMROCKPROTOTYPE_API FGridFastHarasserPlanner
{
public:
    /**
     * Distance dominates the score, then available exits and continued pursuit.
     * A cul-de-sac receives a large explicit penalty.
     */
    static int32 ScoreCandidate (
        const FGridRetreatCandidate& Candidate,
        int32 CurrentDistanceToParty);

    static bool SelectBestRetreatCell (
        const TArray<FGridRetreatCandidate>& Candidates,
        int32 CurrentDistanceToParty,
        FGridRetreatDecision& OutDecision);

    static bool ShouldRetreat (
        float RetreatChance,
        FRandomStream& RandomStream,
        float* OutRoll = nullptr);

    static void SelectAggroTargets (
        const FGuid& SourceObjectId,
        FName SourceMonsterId,
        FName EncounterGroupId,
        const FIntPoint& SourceCell,
        int32 AggroPropagationRange,
        const TArray<FGridMonsterAggroCandidate>& Candidates,
        TArray<FGuid>& OutTargetIds);
};
