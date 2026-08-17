#pragma once

#include "CoreMinimal.h"
#include "Templates/UnrealTemplate.h"
#include "Subsystems/WorldSubsystem.h"
#include "RPG/StatusEffects/GridStatusEffectTypes.h"
#include "Runtime/Combat/GridCombatLog.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "GridStatusEffectLifecycleSubsystem.generated.h"

class AActor;
class AGridMonsterActor;
class UGridStatusEffectDefinitionAsset;
class UGridTurnManagerComponent;
struct FGridCharacterInventoryState;

/** Latest status feedback event. There is no second history/ring buffer. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam (
    FGridStatusEffectFeedbackSignature,
    FGridCombatLogEntry, Entry);

UCLASS ()
class GRIMROCKPROTOTYPE_API UGridStatusEffectLifecycleSubsystem : public UWorldSubsystem
{
    GENERATED_BODY ()

public:
    virtual void Initialize (FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize () override;
    virtual void OnWorldBeginPlay (UWorld& InWorld) override;
    virtual bool DoesSupportWorldType (EWorldType::Type WorldType) const override;

    void BindToTurnManager (UGridTurnManagerComponent* TurnManager);
    void UnbindFromTurnManager ();

    UGridTurnManagerComponent* GetBoundTurnManager () const
    {
        return BoundTurnManager.Get ();
    }

    /**
     * Canonical combat-time mutation path for a party status effect. MON16.4
     * immediately projects any resulting initiative contribution through the
     * existing TurnManager InitiativeModifier field.
     */
    bool TryApplyStatusEffectToPartyCharacter (
        int32 CharacterIndex,
        UGridStatusEffectDefinitionAsset* Definition,
        const FGuid& SourceId,
        FGridStatusEffectApplyResult& OutResult,
        FString& OutError,
        int32 InitialStackCount = 1,
        int32 DurationOverride = INDEX_NONE,
        int32 PotencyOverride = INDEX_NONE);

    /** Same mutation path for a runtime monster. */
    bool TryApplyStatusEffectToMonster (
        AGridMonsterActor* Monster,
        UGridStatusEffectDefinitionAsset* Definition,
        const FGuid& SourceId,
        FGridStatusEffectApplyResult& OutResult,
        FString& OutError,
        int32 InitialStackCount = 1,
        int32 DurationOverride = INDEX_NONE,
        int32 PotencyOverride = INDEX_NONE);

    /** Reprojects every authoritative status collection into InitiativeModifier. */
    void RefreshAllInitiativeModifiers ();

    /** MON16.6 transient feedback; the TurnManager remains owner of CombatLog history. */
    UPROPERTY (BlueprintReadOnly, Transient, Category = "RPG|Status Effects|Feedback")
    FGridCombatLogEntry LastStatusEffectFeedback;

    UPROPERTY (BlueprintAssignable, Category = "RPG|Status Effects|Feedback")
    FGridStatusEffectFeedbackSignature OnStatusEffectFeedback;

private:
    TWeakObjectPtr<UGridTurnManagerComponent> BoundTurnManager;
    FDelegateHandle ActorSpawnedDelegateHandle;
    int32 LastObservedRoundNumber = 0;
    bool bRefreshingInitiativeModifiers = false;

    void TryBindFromActor (AActor* Actor);
    void AdvanceAllRoundEffects (int32 BoundaryCount);
    void ApplyPeriodicDamageToCharacter (
        FGridCharacterInventoryState& Character,
        int32 CharacterIndex,
        EGridStatusEffectDurationUnit DurationUnit);
    void ApplyPeriodicDamageToMonster (
        AGridMonsterActor* Monster,
        EGridStatusEffectDurationUnit DurationUnit);
    void RefreshInitiativeModifierForPartyCharacter (int32 CharacterIndex);
    void RefreshInitiativeModifierForMonster (AGridMonsterActor* Monster);
    bool HasLivingPartyCharacter () const;
    void NotifyPartyStatusPresentationChanged (int32 CharacterIndex);
    void EmitStatusFeedback (
        EGridCombatLogEntryType Type,
        const FGridStatusEffectRuntimeState& State,
        int32 CharacterIndex,
        AGridMonsterActor* Monster,
        const FGridAttackResult* DamageResult = nullptr);
    void EmitExpiredFeedback (
        const TArray<FGridStatusEffectRuntimeState>& PreviousStates,
        const FGridStatusEffectAdvanceResult& AdvanceResult,
        int32 CharacterIndex,
        AGridMonsterActor* Monster);

    UFUNCTION ()
    void HandleActorSpawned (AActor* Actor);

    UFUNCTION ()
    void HandleCombatantStateChanged (FGridCombatantInitiativeEntry Combatant);

    UFUNCTION ()
    void HandleRoundStarted (int32 RoundNumber);

    UFUNCTION ()
    void HandleCombatEnded (EGridCombatPhase ResultPhase);

    UFUNCTION ()
    void HandleTurnOrderChanged ();
};
