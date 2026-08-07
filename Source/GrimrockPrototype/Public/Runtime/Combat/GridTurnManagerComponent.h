#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Runtime/Combat/GridCombatDiagnostics.h"
#include "Runtime/Combat/GridCombatLog.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/Monsters/GridMonsterTypes.h"
#include "GridTurnManagerComponent.generated.h"

class AGridLevelRuntimeActor;
class AGridMonsterActor;
class AGrimrockPartyPawn;
class UGridPartyInventoryComponent;
class UGridMonsterBehaviorComponent;
class UGridMonsterCombatComponent;
class UGridMonsterMovementComponent;
struct FGridRetreatDecision;

DECLARE_LOG_CATEGORY_EXTERN (LogGridTurnManager, Log, All);

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

/** Pure deterministic initiative rolling and ordering rules. */
class GRIMROCKPROTOTYPE_API FGridInitiativeOrderBuilder
{
public:
    static void RollAndSort (
        TArray<FGridCombatantInitiativeEntry>& Entries,
        FRandomStream& RandomStream);

    static void Sort (TArray<FGridCombatantInitiativeEntry>& Entries);

    static bool Precedes (
        const FGridCombatantInitiativeEntry& Left,
        const FGridCombatantInitiativeEntry& Right);
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
    bool BeginCombatantTurn (EGridCombatantSide Side);
    bool CompleteInitiativeRound ();
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (
    FGridCombatLogEntryAddedSignature,
    FGridCombatLogEntry, Entry);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (
    FGridPlayerAttackRequestedSignature,
    FGridPlayerAttackRequest, Request);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams (
    FGridPlayerAttackResolvedSignature,
    FGridPlayerAttackRequest, Request,
    AGridMonsterActor*, TargetMonster,
    FGridAttackResult, Result);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams (
    FGridPlayerAttackRejectedSignature,
    int32, AttackerCharacterIndex,
    EGridPlayerAttackRejectReason, RejectReason);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (
    FGridPlayerCharacterTurnStateChangedSignature,
    FGridPlayerCharacterTurnState, TurnState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (
    FGridPartyMobilityStateChangedSignature,
    FGridPartyMobilityState, MobilityState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams (
    FGridPartyMovementRejectedSignature,
    int32, CharacterIndex,
    EGridEdge, Direction,
    EGridPartyMovementRejectReason, RejectReason);

DECLARE_DYNAMIC_MULTICAST_DELEGATE (
    FGridTurnOrderChangedSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (
    FGridActiveCombatantChangedSignature,
    FGridCombatantInitiativeEntry, ActiveCombatant);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (
    FGridCombatantStateChangedSignature,
    FGridCombatantInitiativeEntry, Combatant);

enum class EGridPendingPartyMotionType : uint8
{
    None,
    Translation,
    Rotation
};

/**
 * Central global-initiative and individual-turn sequencer.
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

    /** Stable base seed used to derive each encounter seed. */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|Turn Manager")
    int32 EncounterRandomSeed = 1337;

    UPROPERTY (
        VisibleInstanceOnly,
        BlueprintReadOnly,
        Transient,
        Category = "Combat|Turn Manager|Random")
    int32 ActiveEncounterRandomSeed = 0;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|Turn Manager|Debug")
    bool bLogPhaseChanges = false;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Combat|Turn Manager|Diagnostics")
    bool bCollectRuntimeMetrics = false;

    UPROPERTY (
        VisibleInstanceOnly,
        BlueprintReadOnly,
        Transient,
        Category = "Combat|Turn Manager|Diagnostics")
    FGridCombatRuntimeMetrics RuntimeMetrics;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Combat|Feedback",
        meta = (ClampMin = "1", ClampMax = "512"))
    int32 MaxCombatLogEntries = 128;

    UPROPERTY (
        Transient,
        VisibleInstanceOnly,
        BlueprintReadOnly,
        Category = "Combat|Feedback")
    TArray<FGridCombatLogEntry> CombatLogEntries;

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

    /** Order rolled once at encounter start and reused by every round. */
    UPROPERTY (
        VisibleInstanceOnly,
        BlueprintReadOnly,
        Transient,
        Category = "Combat|Initiative")
    TArray<FGridCombatantInitiativeEntry> InitiativeOrder;

    UPROPERTY (
        VisibleInstanceOnly,
        BlueprintReadOnly,
        Transient,
        Category = "Combat|Initiative")
    int32 CurrentInitiativeIndex = INDEX_NONE;

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

    UPROPERTY (
        VisibleInstanceOnly,
        BlueprintReadOnly,
        Transient,
        Category = "Combat|Player Attack")
    FGridPlayerAttackRequest LastPlayerAttackRequest;

    UPROPERTY (
        VisibleInstanceOnly,
        BlueprintReadOnly,
        Transient,
        Category = "Combat|Player Attack")
    EGridPlayerAttackRejectReason LastPlayerAttackRejectReason =
        EGridPlayerAttackRejectReason::None;

    UPROPERTY (
        VisibleInstanceOnly,
        BlueprintReadOnly,
        Transient,
        Category = "Combat|Player Attack")
    FGridAttackResult LastPlayerAttackResult;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Combat|Player Turn",
        meta = (ClampMin = "2", ClampMax = "6"))
    int32 BasePlayerActionPointsPerTurn = 4;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Combat|Player Attack",
        meta = (ClampMin = "1", ClampMax = "6"))
    int32 PlayerAttackActionPointCost = 2;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Combat|Party Mobility",
        meta = (ClampMin = "0", ClampMax = "4"))
    int32 BasePartyMobilityActionPointsPerRound = 2;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Combat|Party Mobility",
        meta = (ClampMin = "1", ClampMax = "6"))
    int32 PartyTranslationActionPointCost = 1;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "Combat|Party Mobility",
        meta = (ClampMin = "1", ClampMax = "4"))
    int32 PartyTranslationMobilityActionPointCost = 1;

    UPROPERTY (
        VisibleInstanceOnly,
        BlueprintReadOnly,
        Transient,
        Category = "Combat|Player Turn")
    TArray<FGridPlayerCharacterTurnState> PlayerCharacterTurnStates;

    UPROPERTY (
        VisibleInstanceOnly,
        BlueprintReadOnly,
        Transient,
        Category = "Combat|Party Mobility")
    FGridPartyMobilityState PartyMobilityState;

    UPROPERTY (
        VisibleInstanceOnly,
        BlueprintReadOnly,
        Transient,
        Category = "Combat|Party Mobility")
    EGridPartyMovementRejectReason LastPartyMovementRejectReason =
        EGridPartyMovementRejectReason::None;

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

    UPROPERTY (BlueprintAssignable, Category = "Combat|Feedback")
    FGridCombatLogEntryAddedSignature OnCombatLogEntryAdded;

    UPROPERTY (BlueprintAssignable, Category = "Combat|Player Attack")
    FGridPlayerAttackRequestedSignature OnPlayerAttackRequested;

    UPROPERTY (BlueprintAssignable, Category = "Combat|Player Attack")
    FGridPlayerAttackResolvedSignature OnPlayerAttackResolved;

    UPROPERTY (BlueprintAssignable, Category = "Combat|Player Attack")
    FGridPlayerAttackRejectedSignature OnPlayerAttackRejected;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Combat|Player Attack|Visuals")
    TSoftObjectPtr<UTexture2D> UnarmedAttackIcon;

    UPROPERTY (BlueprintAssignable, Category = "Combat|Player Turn")
    FGridPlayerCharacterTurnStateChangedSignature
        OnPlayerCharacterTurnStateChanged;

    UPROPERTY (BlueprintAssignable, Category = "Combat|Party Mobility")
    FGridPartyMobilityStateChangedSignature
        OnPartyMobilityStateChanged;

    UPROPERTY (BlueprintAssignable, Category = "Combat|Party Mobility")
    FGridPartyMovementRejectedSignature
        OnPartyMovementRejected;

    /** The authoritative order or its visible states changed. */
    UPROPERTY (BlueprintAssignable, Category = "Combat|Initiative")
    FGridTurnOrderChangedSignature OnTurnOrderChanged;

    /** A new participant became active, or an invalid entry cleared it. */
    UPROPERTY (BlueprintAssignable, Category = "Combat|Initiative")
    FGridActiveCombatantChangedSignature OnActiveCombatantChanged;

    /** One participant changed Waiting/Active/Completed/vital state. */
    UPROPERTY (BlueprintAssignable, Category = "Combat|Initiative")
    FGridCombatantStateChangedSignature OnCombatantStateChanged;

    UPROPERTY (
        VisibleInstanceOnly,
        BlueprintReadOnly,
        Transient,
        Category = "Combat|Player Attack|Diagnostics")
    int32 PlayerAttackRejectedBroadcastCount = 0;

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

    /** Ends only the currently active party member turn. */
    UFUNCTION (BlueprintCallable, Category = "Combat|Initiative")
    bool EndActivePlayerTurn ();

    /** Authoritative availability used by every end-turn command surface. */
    UFUNCTION (BlueprintPure, Category = "Combat|Initiative")
    bool CanEndActivePlayerTurn () const;

    /**
     * Authorizes one party translation during combat and spends its personal
     * PA plus the shared PAM atomically. Exploration never calls this path.
     */
    UFUNCTION (BlueprintCallable, Category = "Combat|Party Mobility")
    bool RequestPartyTranslation (
        EGridEdge MoveDirection,
        FIntPoint& OutTargetCell,
        EGridPartyMovementRejectReason& OutRejectReason);

    /** Authorizes a free 90-degree rotation during the active party turn. */
    UFUNCTION (BlueprintCallable, Category = "Combat|Party Mobility")
    bool RequestPartyRotation (
        EGridEdge TargetFacing,
        EGridPartyMovementRejectReason& OutRejectReason);

    /** Called by the party pawn after its accepted grid interpolation ends. */
    UFUNCTION (BlueprintCallable, Category = "Combat|Party Mobility")
    bool NotifyPartyTranslationCompleted ();

    /** Called by the party pawn after its accepted 90-degree turn ends. */
    UFUNCTION (BlueprintCallable, Category = "Combat|Party Mobility")
    bool NotifyPartyRotationCompleted ();

    UFUNCTION (BlueprintPure, Category = "Combat|Party Mobility")
    FGridPartyMobilityState GetPartyMobilityState () const
    {
        return PartyMobilityState;
    }

    UFUNCTION (BlueprintPure, Category = "Combat|Party Mobility")
    bool IsPartyMotionInProgress () const
    {
        return PendingPartyMotionType !=
            EGridPendingPartyMotionType::None;
    }

    UFUNCTION (BlueprintCallable, Category = "Combat|Player Attack")
    bool RequestSelectedCharacterAttack (
        FGridPlayerAttackRequest& OutRequest,
        FGridAttackResult& OutResult,
        EGridPlayerAttackRejectReason& OutRejectReason);

    UFUNCTION (BlueprintCallable, Category = "Combat|Player Attack")
    bool RequestCharacterAttack (
        int32 AttackerCharacterIndex,
        FGridPlayerAttackRequest& OutRequest,
        FGridAttackResult& OutResult,
        EGridPlayerAttackRejectReason& OutRejectReason);

    /**
     * MON12.2 entry point for a combat-panel hand click. Unlike the legacy
     * automatic request, this function resolves only the requested hand. An
     * empty hand uses the unarmed profile; a non-offensive equipped item is
     * rejected without consuming the character action.
     */
    UFUNCTION (BlueprintCallable, Category = "Combat|Player Attack")
    bool RequestCharacterAttackFromSlot (
        int32 AttackerCharacterIndex,
        EGridEquipmentSlot RequestedEquipmentSlot,
        FGridPlayerAttackRequest& OutRequest,
        FGridAttackResult& OutResult,
        EGridPlayerAttackRejectReason& OutRejectReason);

    /** Builds the current action catalogue without consuming any resource. */
    UFUNCTION (BlueprintCallable, Category = "Combat|Action Catalog")
    void GetAvailableCombatActions (
        int32 CharacterIndex,
        TArray<FGridAvailableCombatAction>& OutActions) const;

    /**
     * Generic action entry point. MON12.6 executes Attack resolution profiles;
     * later milestones extend the same request to spells, effects and defense.
     */
    UFUNCTION (BlueprintCallable, Category = "Combat|Action Catalog")
    bool RequestCharacterCombatAction (
        int32 CharacterIndex,
        FName ActionId,
        EGridCombatActionSourcePolicy SourcePolicy,
        FName SourceDefinitionId,
        EGridEquipmentSlot SourceEquipmentSlot,
        FGridCombatActionRequestResult& OutResult);

    UFUNCTION (BlueprintCallable, Category = "Combat|Action Catalog|Debug")
    void LogAvailableCombatActions (int32 CharacterIndex = -1) const;

    UFUNCTION (
        BlueprintCallable,
        CallInEditor,
        Category = "Combat|Action Catalog|Debug")
    void LogSelectedCharacterAvailableCombatActions () const;

    /** Compatibility query: true after this character has spent any PA. */
    UFUNCTION (
        BlueprintPure,
        Category = "Combat|Player Turn",
        meta = (DeprecatedFunction, DeprecationMessage = "Use GetPlayerCharacterTurnState instead."))
    bool HasCharacterCommittedAttackThisPhase (int32 CharacterIndex) const;

    /**
     * Authoritative query for whether this party member may commit any
     * personal combat action now. UI code must not duplicate these rules.
     */
    UFUNCTION (BlueprintPure, Category = "Combat|Player Turn")
    bool CanCharacterAct (int32 CharacterIndex) const;

    /** Checks the common turn rules and a specific PA cost. */
    UFUNCTION (BlueprintPure, Category = "Combat|Player Turn")
    bool CanCharacterSpendActionPoints (
        int32 CharacterIndex,
        int32 ActionPointCost) const;

    /** Returns a copy of the authoritative (or safely derived) turn state. */
    UFUNCTION (BlueprintPure, Category = "Combat|Player Turn")
    bool GetPlayerCharacterTurnState (
        int32 CharacterIndex,
        FGridPlayerCharacterTurnState& OutTurnState) const;

    UFUNCTION (BlueprintPure, Category = "Combat|Initiative")
    const TArray<FGridCombatantInitiativeEntry>& GetInitiativeOrder () const
    {
        return InitiativeOrder;
    }

    /** Active participant followed by the remaining actionable turns. */
    UFUNCTION (BlueprintCallable, Category = "Combat|Initiative")
    void GetUpcomingInitiativeOrder (
        TArray<FGridCombatantInitiativeEntry>& OutEntries) const;

    /**
     * Builds a fixed-length prediction that continues across round boundaries.
     * The active activation is first; defeated or incapacitated entries never
     * appear. The HUD must display this order without sorting it.
     */
    UFUNCTION (BlueprintCallable, Category = "Combat|Initiative")
    void GetInitiativePreview (
        TArray<FGridInitiativePreviewEntry>& OutEntries,
        int32 MaximumEntries = 8) const;

    /**
     * Applies an absolute runtime haste/slow modifier. Only future activations
     * are reordered; the active combatant and completed turns never move.
     */
    UFUNCTION (BlueprintCallable, Category = "Combat|Initiative")
    bool SetCombatantInitiativeModifier (
        EGridCombatantSide Side,
        FGuid CombatantId,
        int32 InitiativeModifier);

    UFUNCTION (BlueprintPure, Category = "Combat|Initiative")
    bool GetActiveCombatant (
        FGridCombatantInitiativeEntry& OutCombatant) const;

    UFUNCTION (BlueprintPure, Category = "Combat|Initiative")
    bool IsActivePlayerCharacter (int32 CharacterIndex) const;

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

    UFUNCTION (BlueprintPure, Category = "Combat|Turn Manager|Random")
    int32 GetActiveEncounterRandomSeed () const
    {
        return ActiveEncounterRandomSeed;
    }

    UFUNCTION (BlueprintCallable, Category = "Combat|Turn Manager|Diagnostics")
    void ResetRuntimeMetrics ();

    UFUNCTION (BlueprintPure, Category = "Combat|Turn Manager|Diagnostics")
    FGridCombatRuntimeMetrics GetRuntimeMetrics () const
    {
        return RuntimeMetrics;
    }

    UFUNCTION (
        BlueprintCallable,
        CallInEditor,
        Category = "Combat|Turn Manager|Diagnostics")
    void LogRuntimeMetrics () const;

    UFUNCTION (BlueprintPure, Category = "Combat|Feedback")
    const TArray<FGridCombatLogEntry>& GetCombatLogEntries () const
    {
        return CombatLogEntries;
    }

    UFUNCTION (BlueprintPure, Category = "Combat|Feedback")
    bool GetLatestCombatLogEntry (FGridCombatLogEntry& OutEntry) const;

    UFUNCTION (BlueprintCallable, Category = "Combat|Feedback")
    void ClearCombatLog ();

    UFUNCTION (
        BlueprintCallable,
        CallInEditor,
        Category = "Combat|Feedback|Debug")
    void LogCombatHistory () const;

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
    FRandomStream InitiativeRandomStream;
    int32 NextCombatLogSequenceNumber = 1;
    int32 CombatLogBroadcastCount = 0;
    int32 AttackResolvedBroadcastCount = 0;
    int32 PlayerAttackRequestedBroadcastCount = 0;
    int32 PlayerAttackResolvedBroadcastCount = 0;
    bool bPlayerAttackResolutionInProgress = false;
    bool bPendingVictoryAfterPlayerAttack = false;
    TSet<FGuid> LoggedDefeatedMonsterIds;
    EGridPendingPartyMotionType PendingPartyMotionType =
        EGridPendingPartyMotionType::None;
    int32 PendingPartyMotionCharacterIndex = INDEX_NONE;
    FIntPoint PendingPartyTranslationFromCell = FIntPoint::ZeroValue;
    FIntPoint PendingPartyTranslationTargetCell = FIntPoint::ZeroValue;

    UPROPERTY (Transient)
    TObjectPtr<UGridMonsterMovementComponent> CurrentMovementComponent = nullptr;

    UPROPERTY (Transient)
    TObjectPtr<UGridMonsterCombatComponent> CurrentCombatComponent = nullptr;

    bool StartCombatInternal (const TArray<AGridMonsterActor*>& Monsters);
    bool RejectPlayerAttack (
        int32 AttackerCharacterIndex,
        EGridPlayerAttackRejectReason RejectReason,
        EGridPlayerAttackRejectReason& OutRejectReason);
    bool RequestCharacterAttackInternal (
        int32 AttackerCharacterIndex,
        EGridEquipmentSlot RequestedEquipmentSlot,
        bool bRequireRequestedEquipmentSlot,
        const FGridAvailableCombatAction* CombatActionOverride,
        FGridPlayerAttackRequest& OutRequest,
        FGridAttackResult& OutResult,
        EGridPlayerAttackRejectReason& OutRejectReason);
    void BuildPlayerCombatActionContributions (
        int32 CharacterIndex,
        TArray<FGridCombatActionContribution>& OutContributions) const;
    void ResolveSuggestedCombatActionTarget (
        FGridAvailableCombatAction& Action) const;
    bool BuildPlayerAttackResolutionInputs (
        const FGridInventoryCharacterSummary& CharacterSummary,
        const AGridMonsterActor* TargetMonster,
        const FGridOffensiveEquipmentProfile& OffensiveProfile,
        FGridAttackSourceStats& OutSource,
        FGridAttackTargetStats& OutTarget,
        FGridAttackDefinition& OutAttackDefinition) const;
    bool ResolvePlayerOffensiveProfile (
        const UGridPartyInventoryComponent* PartyInventory,
        int32 AttackerCharacterIndex,
        EGridEquipmentSlot RequestedEquipmentSlot,
        bool bRequireRequestedEquipmentSlot,
        FGridOffensiveEquipmentProfile& OutProfile,
        FName& OutItemDefinitionId,
        EGridEquipmentSlot& OutEquipmentSlot,
        EGridPlayerAttackRejectReason& OutRejectReason) const;
    bool IsCombatMonster (const AGridMonsterActor* Monster) const;
    void BuildGlobalInitiativeOrder ();
    void ResetInitiativeRound ();
    void BeginNextCombatantTurn ();
    bool BeginPlayerCombatantTurn (
        FGridCombatantInitiativeEntry& Entry);
    bool BeginMonsterCombatantTurn (
        FGridCombatantInitiativeEntry& Entry);
    void FinishActivePlayerTurn ();
    void FinishInitiativeRound ();
    void ClearInitiativeState (bool bBroadcast);
    void SetInitiativeEntryState (
        FGridCombatantInitiativeEntry& Entry,
        EGridCombatantTurnState NewState);
    void RefreshInitiativeEntryVitals (
        FGridCombatantInitiativeEntry& Entry) const;
    void BroadcastInitiativeOrderChanged ();
    void ReorderFutureInitiativeEntries ();
    FGuid ResolvePlayerCombatantId (int32 CharacterIndex) const;
    AGridMonsterActor* FindCombatMonsterById (
        const FGuid& CombatantId) const;
    FGridCombatantInitiativeEntry* FindInitiativeEntry (
        EGridCombatantSide Side,
        const FGuid& CombatantId);
    const FGridCombatantInitiativeEntry* FindInitiativeEntry (
        EGridCombatantSide Side,
        const FGuid& CombatantId) const;
    void BeginPlayerCharacterPhase ();
    void CompletePlayerCharacterPhase ();
    void ClearPlayerCharacterTurnStates ();
    bool SpendPlayerCharacterActionPoints (
        int32 CharacterIndex,
        int32 ActionPointCost);
    void RefreshPlayerCharacterVitalState (int32 CharacterIndex);
    FGridPlayerCharacterTurnState* EnsurePlayerCharacterTurnState (
        int32 CharacterIndex);
    const FGridPlayerCharacterTurnState* FindPlayerCharacterTurnState (
        const FGuid& CharacterId) const;
    FGridPlayerCharacterTurnState* FindPlayerCharacterTurnState (
        const FGuid& CharacterId);
    void BroadcastPlayerCharacterTurnState (
        const FGridPlayerCharacterTurnState& TurnState);
    int32 ResolveActivePartyCharacterIndex () const;
    bool RejectPartyMovement (
        int32 CharacterIndex,
        EGridEdge Direction,
        EGridPartyMovementRejectReason RejectReason,
        EGridPartyMovementRejectReason& OutRejectReason);
    void ResetPartyMobilityForRound ();
    void ClearPartyMobilityState (bool bBroadcast);
    void ClearPendingPartyMotion ();
    bool CompletePendingPartyMotion (
        EGridPendingPartyMotionType ExpectedMotionType);
    void CollectAllLivingMonsters (TArray<AGridMonsterActor*>& OutMonsters);
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
    void AppendCombatLogEntry (FGridCombatLogEntry Entry);
    FName ResolveMonsterLogId (const AGridMonsterActor* Monster) const;
    FText ResolveMonsterDisplayName (const AGridMonsterActor* Monster) const;
    FText ResolveCharacterDisplayName (int32 CharacterIndex) const;
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

    friend class FGridMonsterMON10CombatLogRingBufferTest;
    friend class FGridMonsterMON10CombatLogAttackExactlyOnceTest;
    friend class FGridMonsterMON10CombatLogCharacterDefeatedTest;
    friend class FGridMonsterMON10CombatLogMonsterDefeatedAndVictoryTest;
    friend class FGridMonsterMON10CombatLogFailedStartDoesNotClearHistoryTest;
    friend class FGridMonsterMON10AudioAttackAndImpactExactlyOnceTest;
    friend class FGridMonsterMON10VFXAttackAndImpactExactlyOnceTest;
    friend class FGridMonsterMON10OptimizationEncounterSeedLifecycleTest;
    friend class FGridMonsterMON10OptimizationMetricsLifecycleTest;
    friend class FGridMonsterMON10OptimizationMetricsNoGameplayInfluenceTest;
    friend class FGridMonsterMON11TargetingTest;
    friend class FGridMonsterMON11RequestValidationTest;
    friend class FGridMonsterMON11RequestAcceptedAndResolvedTest;
    friend class FGridMonsterMON11PerCharacterActionGateTest;
    friend class FGridMonsterMON11PlayerResolutionMappingTest;
    friend class FGridMonsterMON11PlayerResolutionDeterminismTest;
    friend class FGridMonsterMON11PlayerResolutionArmorAndCriticalTest;
    friend class FGridMonsterMON11PlayerResolutionDeathVictoryTest;
    friend class FGridMonsterMON11OffensiveProfileValidationTest;
    friend class FGridMonsterMON11EquippedWeaponMappingTest;
    friend class FGridMonsterMON11HandPriorityAndFallbackTest;
    friend class FGridMonsterMON11RangedWeaponTargetingTest;
    friend class FGridMonsterMON11ElementalOffensiveEquipmentTest;
    friend class FGridMonsterMON12CharacterActionPointLifecycleTest;
    friend class FGridMonsterMON12GlobalInitiativeLifecycleTest;
    friend class FGridMonsterMON12PartyMobilityLifecycleTest;
    friend class FGridMonsterMON12CombatHudLifecycleTest;
    friend class FGridMonsterMON12FreshPlacedCombatInitializationTest;
    friend class FGridMonsterMON12RestoredInjuredCombatInitializationTest;
    friend class FGridMonsterMON12RestoredDeadCombatInitializationTest;
    friend class FGridMonsterMON12InvalidDefinitionAdmissionTest;
    friend class FGridMonsterMON12UninitializedInitiativeStateTest;
    friend class FGridMonsterMON12InitializedPersistenceCaptureTest;
    friend class FGridMonsterMON1271InitiativePreviewTest;
    friend class FGridMonsterMON1271DynamicInitiativeTest;
};
