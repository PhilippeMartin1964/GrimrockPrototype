#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/Monsters/GridMonsterTypes.h"
#include "GridTurnManagerComponent.generated.h"

class AGridLevelRuntimeActor;
class AGridMonsterActor;
class AGrimrockPartyPawn;
class UGridMonsterBehaviorComponent;
class UGridMonsterCombatComponent;
class UGridMonsterMovementComponent;
struct FGridRetreatDecision;

/** Small deterministic action-point budget used by the turn manager and tests. */
class GRIMROCKPROTOTYPE_API FGridActionPointBudget
{
public:
    void Reset (int32 InMaximumPoints);
    bool CanSpend (int32 Cost) const;
    bool Spend (int32 Cost);

    int32 GetMaximumPoints () const { return MaximumPoints; }
    int32 GetRemainingPoints () const { return RemainingPoints; }
    bool IsExhausted () const { return RemainingPoints <= 0; }

private:
    int32 MaximumPoints = 0;
    int32 RemainingPoints = 0;
};

/** Pure phase rules, independent from actors, animation and frame rate. */
class GRIMROCKPROTOTYPE_API FGridTurnPhaseStateMachine
{
public:
    bool StartCombat ();
    bool BeginRound ();
    bool EndPlayerPhase ();
    bool CompleteEnemyPhase ();
    bool BeginNextRound ();
    bool FinishCombat (EGridCombatPhase ResultPhase);
    void AbortCombat ();

    EGridCombatPhase GetPhase () const { return Phase; }
    int32 GetRoundNumber () const { return RoundNumber; }
    bool IsCombatActive () const;

private:
    EGridCombatPhase Phase = EGridCombatPhase::Exploration;
    int32 RoundNumber = 0;
};

/** Converts MON4 paths into deterministic monster actions. */
class GRIMROCKPROTOTYPE_API FGridMonsterTurnPlanner
{
public:
    static void BuildMovementTurn (
        const FGuid& SourceActorId,
        const FIntPoint& StartCell,
        EGridEdge StartFacing,
        const TArray<FIntPoint>& Path,
        int32 AvailableActionPoints,
        TArray<FGridCombatAction>& OutActions);

    /**
     * DirectMelee policy used by MON6. A monster attacks when adjacent and can
     * move then attack when its remaining action points permit it.
     */
    static void BuildDirectMeleeTurn (
        const FGuid& SourceActorId,
        const FIntPoint& StartCell,
        EGridEdge StartFacing,
        const FIntPoint& PartyCell,
        const TArray<FIntPoint>& Path,
        int32 AvailableActionPoints,
        FName AttackId,
        int32 AttackActionPointCost,
        TArray<FGridCombatAction>& OutActions);

    /**
     * Preserves DirectMelee planning and optionally appends a marked retreat
     * after the single planned melee attack.
     */
    static void BuildFastHarasserTurn (
        const FGuid& SourceActorId,
        const FIntPoint& StartCell,
        EGridEdge StartFacing,
        const FIntPoint& PartyCell,
        const TArray<FIntPoint>& Path,
        int32 AvailableActionPoints,
        FName AttackId,
        int32 AttackActionPointCost,
        const FGridRetreatDecision& RetreatDecision,
        bool bShouldRetreat,
        TArray<FGridCombatAction>& OutActions);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (
    FGridCombatPhaseChangedSignature,
    EGridCombatPhase, NewPhase);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (
    FGridCombatRoundStartedSignature,
    int32, RoundNumber);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (
    FGridMonsterTurnSignature,
    AGridMonsterActor*, Monster);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (
    FGridCombatActionStartedSignature,
    FGridCombatAction, Action);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams (
    FGridCombatActionCompletedSignature,
    FGridCombatAction, Action,
    bool, bSucceeded);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams (
    FGridMonsterAttackResolvedSignature,
    AGridMonsterActor*, Monster,
    int32, TargetCharacterIndex,
    FGridAttackResult, Result);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (
    FGridCombatEndedSignature,
    EGridCombatPhase, ResultPhase);

/**
 * Central combat phase and monster-turn sequencer.
 *
 * MON6 adds deterministic melee resolution while keeping rule calculation in
 * FGridCombatResolver and presentation in UGridMonsterCombatComponent.
 */
UCLASS (ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridTurnManagerComponent : public UActorComponent
{
    GENERATED_BODY ()

public:
    UGridTurnManagerComponent ();

    virtual void BeginPlay () override;
    virtual void EndPlay (const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent (
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|Turn Manager")
    bool bAutoInitialize = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|Turn Manager", meta = (ClampMin = "0.0"))
    float CombatStartSafetyPadding = 0.05f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|Turn Manager", meta = (ClampMin = "0.0"))
    float ActionTimeoutPadding = 0.50f;

    /** Stable seed used for attack rolls and party target selection. */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|Turn Manager")
    int32 EncounterRandomSeed = 1337;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|Turn Manager|Debug")
    bool bLogPhaseChanges = true;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Turn Manager")
    TObjectPtr<AGridLevelRuntimeActor> RuntimeActor = nullptr;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Turn Manager")
    TObjectPtr<AGrimrockPartyPawn> PartyPawn = nullptr;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Turn Manager")
    EGridCombatPhase CurrentPhase = EGridCombatPhase::Exploration;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Turn Manager")
    bool bCombatActive = false;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Turn Manager")
    int32 RoundNumber = 0;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Turn Manager")
    TArray<TObjectPtr<AGridMonsterActor>> CombatMonsters;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Turn Manager")
    TArray<TObjectPtr<AGridMonsterActor>> EnemyTurnOrder;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Turn Manager")
    TObjectPtr<AGridMonsterActor> CurrentMonster = nullptr;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Turn Manager")
    int32 CurrentEnemyIndex = INDEX_NONE;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Turn Manager")
    int32 CurrentMonsterMaximumActionPoints = 0;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Turn Manager")
    int32 CurrentMonsterRemainingActionPoints = 0;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Turn Manager")
    TArray<FGridCombatAction> PendingActions;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Turn Manager")
    FGridCombatAction ActiveAction;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Turn Manager")
    bool bHasActiveAction = false;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Turn Manager|Attack")
    FGridMonsterAttackDefinition ActiveAttackDefinition;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Turn Manager|Attack")
    FGridAttackResult LastAttackResult;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Turn Manager|Attack")
    int32 LastTargetCharacterIndex = INDEX_NONE;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Category = "Combat|Turn Manager|Attack")
    bool bActiveAttackImpactCommitted = false;

    UPROPERTY (BlueprintAssignable, Category = "Combat|Turn Manager")
    FGridCombatPhaseChangedSignature OnPhaseChanged;

    UPROPERTY (BlueprintAssignable, Category = "Combat|Turn Manager")
    FGridCombatRoundStartedSignature OnRoundStarted;

    UPROPERTY (BlueprintAssignable, Category = "Combat|Turn Manager")
    FGridMonsterTurnSignature OnMonsterTurnStarted;

    UPROPERTY (BlueprintAssignable, Category = "Combat|Turn Manager")
    FGridMonsterTurnSignature OnMonsterTurnEnded;

    UPROPERTY (BlueprintAssignable, Category = "Combat|Turn Manager")
    FGridCombatActionStartedSignature OnActionStarted;

    UPROPERTY (BlueprintAssignable, Category = "Combat|Turn Manager")
    FGridCombatActionCompletedSignature OnActionCompleted;

    UPROPERTY (BlueprintAssignable, Category = "Combat|Turn Manager")
    FGridMonsterAttackResolvedSignature OnAttackResolved;

    UPROPERTY (BlueprintAssignable, Category = "Combat|Turn Manager")
    FGridCombatEndedSignature OnCombatEnded;

    UFUNCTION (BlueprintCallable, Category = "Combat|Turn Manager")
    bool InitializeTurnManager (
        AGridLevelRuntimeActor* InRuntimeActor = nullptr,
        AGrimrockPartyPawn* InPartyPawn = nullptr);

    UFUNCTION (BlueprintCallable, Category = "Combat|Turn Manager")
    bool StartCombatFromPerception ();

    UFUNCTION (BlueprintCallable, Category = "Combat|Turn Manager|Debug")
    bool StartCombatWithAllMonsters ();

    UFUNCTION (BlueprintCallable, Category = "Combat|Turn Manager")
    bool EndPlayerPhase ();

    UFUNCTION (BlueprintCallable, Category = "Combat|Turn Manager")
    void AbortCombat ();

    UFUNCTION (BlueprintCallable, Category = "Combat|Turn Manager|Debug")
    void ForceVictory ();

    UFUNCTION (BlueprintCallable, Category = "Combat|Turn Manager|Debug")
    void ForceDefeat ();

    /** Entry point for Monster.AttackImpact Anim Notifies. Safe to call twice. */
    UFUNCTION (BlueprintCallable, Category = "Combat|Turn Manager|Animation Notify")
    void NotifyActiveAttackImpact ();

    /** Entry point for Monster.ActionComplete Anim Notifies. */
    UFUNCTION (BlueprintCallable, Category = "Combat|Turn Manager|Animation Notify")
    void NotifyActiveAttackComplete ();

    UFUNCTION (BlueprintCallable, CallInEditor, Category = "Combat|Turn Manager|Debug")
    void LogCurrentTurnState () const;

    UFUNCTION (BlueprintCallable, CallInEditor, Category = "Combat|Turn Manager|Debug")
    void LogPartyCombatState () const;

    UFUNCTION (BlueprintPure, Category = "Combat|Turn Manager")
    bool IsInitialized () const { return bInitialized; }

    UFUNCTION (BlueprintPure, Category = "Combat|Turn Manager")
    bool IsPartyInputLocked () const { return bPartyInputLocked; }

    UFUNCTION (BlueprintPure, Category = "Combat|Turn Manager")
    bool IsExecutingAction () const { return bHasActiveAction; }

private:
    bool bInitialized = false;
    bool bPartyInputLocked = false;
    bool bWaitingForCombatStart = false;
    float CombatStartDelayRemaining = 0.0f;
    float ActiveActionTimeoutRemaining = 0.0f;
    float ActiveAttackImpactTimeRemaining = 0.0f;
    float ActiveAttackCompleteTimeRemaining = 0.0f;

    FGridTurnPhaseStateMachine PhaseState;
    FGridActionPointBudget ActionPointBudget;
    FRandomStream CombatRandomStream;

    UPROPERTY (Transient)
    TObjectPtr<UGridMonsterMovementComponent> CurrentMovementComponent = nullptr;

    UPROPERTY (Transient)
    TObjectPtr<UGridMonsterCombatComponent> CurrentCombatComponent = nullptr;

    bool StartCombatInternal (const TArray<AGridMonsterActor*>& Monsters);
    void CollectAllLivingMonsters (TArray<AGridMonsterActor*>& OutMonsters) const;
    void CollectPerceivingMonsters (TArray<AGridMonsterActor*>& OutMonsters);
    bool PrepareMonsterForCombat (AGridMonsterActor* Monster);

    void BeginRound ();
    void BeginEnemyPhase ();
    void BuildEnemyTurnOrder ();
    void BeginNextMonsterTurn ();
    void PrepareCurrentMonsterActions ();
    void ExecuteNextAction ();
    bool StartActiveAction (const FGridCombatAction& Action);
    bool StartActiveMeleeAttack ();
    void CommitActiveAttackImpact ();
    void CompleteActiveAction (bool bSucceeded);
    void FinishCurrentMonsterTurn ();
    void FinishEnemyPhase ();
    void FinishCombat (EGridCombatPhase ResultPhase);

    void ResetActiveAttackState ();
    void SetPhase (EGridCombatPhase NewPhase);
    void SetPartyInputLocked (bool bLocked);
    bool IsPartyAtRest () const;
    bool HasLivingCombatMonster () const;
    bool HasLivingPartyCharacter () const;
    float CalculateCombatStartDelay () const;
    float GetExpectedActionDuration (const FGridCombatAction& Action) const;
    void RefreshTickEnabled ();
    void BindCurrentMovement (UGridMonsterMovementComponent* MovementComponent);
    void UnbindCurrentMovement ();
    void BindCurrentCombat (UGridMonsterCombatComponent* CombatComponent);
    void UnbindCurrentCombat ();
    void BindCombatMonsterDeaths ();
    void UnbindCombatMonsterDeaths ();

    AGridLevelRuntimeActor* FindRuntimeActor () const;
    AGrimrockPartyPawn* FindPartyPawn () const;

    UFUNCTION ()
    void HandleMonsterMoveCompleted (FIntPoint FromCell, FIntPoint ToCell);

    UFUNCTION ()
    void HandleMonsterTurnCompleted (EGridEdge FromFacing, EGridEdge ToFacing);

    UFUNCTION ()
    void HandleMonsterAttackImpactNotify ();

    UFUNCTION ()
    void HandleMonsterActionCompleteNotify ();

    UFUNCTION ()
    void HandleCombatMonsterDied (
        AGridMonsterActor* Monster,
        FIntPoint DeathCell);
};
