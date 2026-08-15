#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RPGLevelUpNotificationSubsystem.generated.h"

class UGridPartyInventoryComponent;
class URPGLevelUpWidget;

/**
 * MON15.5 runtime coordinator that turns the source-aware level-up event into
 * a modal notification. Multiple simultaneous level-ups are queued.
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

    FDelegateHandle LevelUpDelegateHandle;
    FDelegateHandle WidgetClosedDelegateHandle;

    void HandleCharacterLevelUpApplied (
        UGridPartyInventoryComponent* PartyInventoryComponent,
        int32 CharacterIndex,
        int32 PreviousLevel,
        int32 NewLevel,
        int32 LevelsGained);
    void HandleWidgetClosed (URPGLevelUpWidget* ClosedWidget);
    void TryPresentNextNotification ();
};