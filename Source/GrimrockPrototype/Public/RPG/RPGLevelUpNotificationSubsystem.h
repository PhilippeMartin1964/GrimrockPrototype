#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RPGLevelUpNotificationSubsystem.generated.h"

class UGridPartyInventoryComponent;

/**
 * RPG-LEVELUX01 non-modal Level-Up feedback coordinator.
 *
 * New level-ups are acknowledged immediately by FRPGLevelUpService. This
 * subsystem owns only a transient toast queue. LastAcknowledgedLevel remains
 * durable for current SaveGame compatibility and for one-time catch-up of
 * saves that still contain an older unacknowledged gap.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API URPGLevelUpNotificationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Number of transient non-modal Level-Up toasts waiting or currently shown. */
	UFUNCTION(BlueprintPure, Category = "RPG|Level Up")
	int32 GetPendingLevelUpNotificationCount() const;

	/** Legacy compatibility query. RPG-LEVELUX01 never opens a Level-Up modal. */
	UFUNCTION(BlueprintPure, Category = "RPG|Level Up")
	bool IsLevelUpModalOpen() const { return false; }

	/**
	 * Binds the authoritative party inventory and consumes any legacy
	 * LastAcknowledgedLevel < Level gap as non-modal catch-up feedback.
	 */
	void RefreshFromPartyState(UGridPartyInventoryComponent* PartyInventoryComponent);

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
	TWeakObjectPtr<UGridPartyInventoryComponent> ObservedPartyInventory;
	FTimerHandle ActiveToastTimerHandle;
	FDelegateHandle LevelUpDelegateHandle;

	void HandleCharacterLevelUpApplied(
		UGridPartyInventoryComponent* PartyInventoryComponent,
		int32 CharacterIndex,
		int32 PreviousLevel,
		int32 NewLevel,
		int32 LevelsGained);

	void BindPartyInventory(UGridPartyInventoryComponent* PartyInventoryComponent);
	void EnqueueNotification(
		UGridPartyInventoryComponent* PartyInventoryComponent,
		int32 CharacterIndex,
		int32 PreviousLevel,
		int32 NewLevel,
		int32 LevelsGained);
	void AcknowledgeNotification(const FPendingNotification& Notification);
	bool PresentNotification(const FPendingNotification& Notification, float& OutDurationSeconds);
	void TryPresentNextNotification();
	void HandleActiveToastExpired();
};
