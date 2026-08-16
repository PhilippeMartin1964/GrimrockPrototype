#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RPGLevelUpNotificationSubsystem.generated.h"

class UGridPartyInventoryComponent;
class UGridTurnManagerComponent;
class URPGLevelUpWidget;

/**
 * MON15.5 runtime coordinator that turns the source-aware level-up event into
 * a modal notification. Multiple simultaneous level-ups are queued.
 *
 * Combat safety: level-up notifications earned during combat remain queued
 * until the authoritative turn manager broadcasts OnCombatEnded. This keeps
 * combat resolution deterministic and prevents an enemy turn from executing
 * behind the modal.
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

private:
    struct FPendingNotification
    {
        TWeakObjectPtr<UGridPartyInventoryComponent> InventoryComponent;
        int32 CharacterIndex = INDEX_NONE;
        int32 PreviousLevel = 1;
        int32 NewLevel = 1;
        int32 LevelsGained = 0;
    };

    TArray<FPendingNotification> PendingNotifications;

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
};