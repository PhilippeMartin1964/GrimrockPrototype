#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/Monsters/GridMonsterTypes.h"
#include "GridMonsterPatrolSubsystem.generated.h"

class AGridLevelRuntimeActor;
class AGridMonsterActor;
class UGridMonsterBehaviorComponent;
class UGridMonsterMovementComponent;
class UGridTurnManagerComponent;

DECLARE_LOG_CATEGORY_EXTERN (LogGridMonsterPatrol, Log, All);

UENUM (BlueprintType)
enum class EGridMonsterExplorationActivity : uint8
{
    Inactive      UMETA (DisplayName = "Inactive"),
    Patrolling    UMETA (DisplayName = "Patrolling"),
    Waiting       UMETA (DisplayName = "Waiting"),
    Investigating UMETA (DisplayName = "Investigating"),
    Searching     UMETA (DisplayName = "Searching"),
    Engaging      UMETA (DisplayName = "Engaging"),
    Suspended     UMETA (DisplayName = "Suspended")
};

class GRIMROCKPROTOTYPE_API FGridMonsterPatrolCursorRules
{
public:
    static bool Initialize (
        const FIntPoint& CurrentCell,
        EGridMonsterPatrolMode PatrolMode,
        const TArray<FGridMonsterPatrolWaypoint>& Waypoints,
        int32& OutTargetWaypointIndex,
        int32& OutPingPongDirection);

    static bool Advance (
        EGridMonsterPatrolMode PatrolMode,
        int32 WaypointCount,
        int32 ArrivedWaypointIndex,
        int32& InOutPingPongDirection,
        int32& OutNextWaypointIndex);
};

/**
 * MON14.3 event-driven exploration AI orchestrator.
 * Movement/occupancy remain MON3 responsibilities, perception/pathfinding MON4
 * responsibilities. This subsystem adds no permanent AI Tick.
 */
UCLASS ()
class GRIMROCKPROTOTYPE_API UGridMonsterPatrolSubsystem : public UWorldSubsystem
{
    GENERATED_BODY ()

public:
    virtual void Deinitialize () override;

    void RegisterRuntime (AGridLevelRuntimeActor* RuntimeActor);

    void HandlePerceptionEvaluation (
        AGridLevelRuntimeActor* RuntimeActor,
        bool bCombatStarted,
        FName Reason);

    bool ProcessMonsterNow (
        AGridMonsterActor* Monster,
        FName Reason = NAME_None);

    UFUNCTION (BlueprintCallable, Category = "Monster|Patrol")
    void SuspendAllForCombat ();

    UFUNCTION (BlueprintPure, Category = "Monster|Patrol")
    int32 GetTrackedMonsterCount () const { return RuntimeEntries.Num (); }

    UFUNCTION (BlueprintPure, Category = "Monster|Patrol")
    EGridMonsterExplorationActivity GetMonsterActivity (FGuid MonsterId) const;

    UFUNCTION (BlueprintPure, Category = "Monster|Patrol")
    int32 GetMonsterTargetWaypointIndex (FGuid MonsterId) const;

private:
    struct FRuntimeEntry
    {
        TWeakObjectPtr<AGridMonsterActor> Monster;
        TWeakObjectPtr<AGridLevelRuntimeActor> RuntimeActor;
        EGridMonsterExplorationActivity Activity =
            EGridMonsterExplorationActivity::Inactive;
        int32 TargetWaypointIndex = INDEX_NONE;
        int32 PingPongDirection = 1;
        int32 SearchTurnsRemaining = 0;
        FTimerHandle TimerHandle;
    };

    TMap<FGuid, FRuntimeEntry> RuntimeEntries;
    TWeakObjectPtr<AGridLevelRuntimeActor> BoundRuntimeActor;
    TWeakObjectPtr<UGridTurnManagerComponent> BoundTurnManager;
    bool bHandlingCompletedAutomaticEvaluation = false;

    UFUNCTION ()
    void HandleCombatPhaseChanged (EGridCombatPhase NewPhase);

    UFUNCTION ()
    void HandleCombatEnded (EGridCombatPhase ResultPhase);

    void HandleScheduledStep (FGuid MonsterId);
    bool ProcessMonsterInternal (AGridMonsterActor* Monster, bool bRefreshPerception, FName Reason);
    bool ProcessPatrol (FRuntimeEntry& Entry, AGridMonsterActor* Monster);
    bool ProcessInvestigation (FRuntimeEntry& Entry, AGridMonsterActor* Monster);
    bool ProcessSearch (FRuntimeEntry& Entry, AGridMonsterActor* Monster);
    bool StartMoveTowardDirection (
        FRuntimeEntry& Entry,
        AGridMonsterActor* Monster,
        UGridMonsterMovementComponent* Movement,
        EGridEdge Direction,
        EGridMonsterExplorationActivity Activity);
    bool StartTurnToward (
        FRuntimeEntry& Entry,
        AGridMonsterActor* Monster,
        UGridMonsterMovementComponent* Movement,
        EGridEdge TargetFacing,
        EGridMonsterExplorationActivity Activity);
    void BeginSearch (FRuntimeEntry& Entry, AGridMonsterActor* Monster);
    void FinishInvestigationAndResumePatrol (
        FRuntimeEntry& Entry,
        AGridMonsterActor* Monster,
        UGridMonsterBehaviorComponent* Behavior);
    FRuntimeEntry* FindOrAddEntry (AGridMonsterActor* Monster);
    void ScheduleStep (FRuntimeEntry& Entry, float DelaySeconds);
    void CancelScheduledStep (FRuntimeEntry& Entry);
    void CancelExplorationMotion (FRuntimeEntry& Entry);
    bool IsRuntimeSafeForExploration (AGridLevelRuntimeActor* RuntimeActor) const;
    bool IsMonsterOwnedByRuntime (
        const AGridMonsterActor* Monster,
        const AGridLevelRuntimeActor* RuntimeActor) const;
    static bool IsCardinalFacing (EGridEdge Facing);
    static float GetMoveContinuationDelay (const AGridMonsterActor* Monster);
    static float GetTurnContinuationDelay (const AGridMonsterActor* Monster);
};
