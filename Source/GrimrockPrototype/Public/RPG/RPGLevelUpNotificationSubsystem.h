#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Save/GrimrockPartySaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RPGLevelUpNotificationSubsystem.generated.h"

class UGridPartyInventoryComponent;
class UGridTurnManagerComponent;
class URPGLevelUpWidget;

/**
 * Runtime coordinator that turns source-aware level-up events into modal
 * notifications. MON15.6 additionally persists notifications that have not
 * yet been acknowledged.
 */
UCLASS ()
class GRIMROCKPROTOTYPE_API URPGLevelUpNotificationSubsystem
    : public UGameInstanceSubsystem
{
    GENERATED_BODY ()

public:
    virtual void Initialize (FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize () override;

    UFUNCTION (BlueprintPure, Category = "RPG|Level Up")
    int32 GetPendingLevelUpNotificationCount () const;

    UFUNCTION (BlueprintPure, Category = "RPG|Level Up")
    bool IsLevelUpModalOpen () const;

    /** Captures pending + currently displayed notifications for this party. */
    static bool CapturePersistentState (
        const FGridPartyInventoryState& PartyState,
        TArray<FRPGPendingLevelUpSaveState>& OutStates,
        FText& OutError);

    /**
     * Replaces the persistent mirror after SaveGame load. Live subsystems adopt
     * it on a following world tick, after PartyInventoryState is restored.
     */
    static void RestorePersistentState (
        const TArray<FRPGPendingLevelUpSaveState>& SavedStates);

private:
    struct FPendingNotification
    {
        TWeakObjectPtr<UGridPartyInventoryComponent> InventoryComponent;
        int32 CharacterIndex = INDEX_NONE;
        FGuid CharacterId;
        int32 PreviousLevel = 1;
        int32 NewLevel = 1;
        int32 LevelsGained = 0;
    };

    TArray<FPendingNotification> PendingNotifications;
    TOptional<FPendingNotification> ActiveNotification;
    TArray<FRPGPendingLevelUpSaveState> PendingPersistentRestoreStates;
    int32 PersistentRestoreRetryCount = 0;

    UPROPERTY (Transient)
    TObjectPtr<URPGLevelUpWidget> ActiveWidget;

    UPROPERTY (Transient)
    TObjectPtr<UGridTurnManagerComponent> DeferredCombatTurnManager;

    FDelegateHandle LevelUpDelegateHandle;
    FDelegateHandle WidgetClosedDelegateHandle;

    void HandleCharacterLevelUpApplied (
        UGridPartyInventoryComponent* PartyInventoryComponent,
        int32 CharacterIndex,
        int32 PreviousLevel,
        int32 NewLevel,
        int32 LevelsGained);
    void HandleWidgetClosed (URPGLevelUpWidget* ClosedWidget);

    UFUNCTION ()
    void HandleDeferredCombatEnded (EGridCombatPhase ResultPhase);

    void SetDeferredCombatTurnManager (
        UGridTurnManagerComponent* TurnManager);
    void ClearDeferredCombatTurnManager ();
    void TryPresentNextNotification ();
    void HandlePersistentStateRestored (
        const TArray<FRPGPendingLevelUpSaveState>& SavedStates);
    bool TryAdoptPersistentRestoreState ();
    void SchedulePersistentRestoreRetry ();
    void HandlePersistentRestoreRetryTick ();
    void SyncPersistentMirrorForCharacter (const FGuid& CharacterId);
};
