#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "GridTurnManagerComponent.generated.h"

class AGridLevelRuntimeActor;
class AGridMonsterActor;
class AGrimrockPartyPawn;
class UGridMonsterBehaviorComponent;
class UGridMonsterMovementComponent;

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

/** Pure MON5 phase rules, independent from actors, animation and frame rate. */
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

/**
 * Converts a MON4 path into deterministic Turn/Move actions for one monster turn.
 * Rotation is free in MON5; every Move costs one action point. Attacks start in MON6.
 */
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (
    FGridCombatEndedSignature,
    EGridCombatPhase, ResultPhase);

/**
 * MON5 central combat phase and monster-turn sequencer.
 *
 * The component owns no combat formulas. It only decides when actors may act,
 * spends action points, executes one visual action at a time and protects the
 * sequence with timeouts. MON6 will add attack resolution.
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

    /** Additional delay after party motion/input-buffer expiry when combat starts. */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|Turn Manager", meta = (ClampMin = "0.0"))
    float CombatStartSafetyPadding = 0.05f;

    /** Added to the expected visual duration before an action is force-completed. */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|Turn Manager", meta = (ClampMin = "0.0"))
    float ActionTimeoutPadding = 0.50f;

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
    FGridCombatEndedSignature OnCombatEnded;

    UFUNCTION (BlueprintCallable, Category = "Combat|Turn Manager")
    bool InitializeTurnManager (
        AGridLevelRuntimeActor* InRuntimeActor = nullptr,
        AGrimrockPartyPawn* InPartyPawn = nullptr);

    /** Starts combat with living monsters that currently see or hear the party. */
    UFUNCTION (BlueprintCallable, Category = "Combat|Turn Manager")
    bool StartCombatFromPerception ();

    /** Debug/prototype entry point that includes every living GridMonsterActor. */
    UFUNCTION (BlueprintCallable, Category = "Combat|Turn Manager|Debug")
    bool StartCombatWithAllMonsters ();

    /** Ends the interactive player phase. Rejected while the party is between grid poses. */
    UFUNCTION (BlueprintCallable, Category = "Combat|Turn Manager")
    bool EndPlayerPhase ();

    UFUNCTION (BlueprintCallable, Category = "Combat|Turn Manager")
    void AbortCombat ();

    UFUNCTION (BlueprintCallable, Category = "Combat|Turn Manager|Debug")
    void ForceVictory ();

    UFUNCTION (BlueprintCallable, Category = "Combat|Turn Manager|Debug")
    void ForceDefeat ();

    UFUNCTION (BlueprintCallable, CallInEditor, Category = "Combat|Turn Manager|Debug")
    void LogCurrentTurnState () const;

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

    FGridTurnPhaseStateMachine PhaseState;
    FGridActionPointBudget ActionPointBudget;

    UPROPERTY (Transient)
    TObjectPtr<UGridMonsterMovementComponent> CurrentMovementComponent = nullptr;

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
    void CompleteActiveAction (bool bSucceeded);
    void FinishCurrentMonsterTurn ();
    void FinishEnemyPhase ();
    void FinishCombat (EGridCombatPhase ResultPhase);

    void SetPhase (EGridCombatPhase NewPhase);
    void SetPartyInputLocked (bool bLocked);
    bool IsPartyAtRest () const;
    bool HasLivingCombatMonster () const;
    float CalculateCombatStartDelay () const;
    float GetExpectedActionDuration (const FGridCombatAction& Action) const;
    void RefreshTickEnabled ();
    void BindCurrentMovement (UGridMonsterMovementComponent* MovementComponent);
    void UnbindCurrentMovement ();

    AGridLevelRuntimeActor* FindRuntimeActor () const;
    AGrimrockPartyPawn* FindPartyPawn () const;

    UFUNCTION ()
    void HandleMonsterMoveCompleted (FIntPoint FromCell, FIntPoint ToCell);

    UFUNCTION ()
    void HandleMonsterTurnCompleted (EGridEdge FromFacing, EGridEdge ToFacing);
};
