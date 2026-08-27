#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RPGLevelUpNotificationSubsystem.generated.h"

class UGridPartyInventoryComponent;
class UGridTurnManagerComponent;
class URPGLevelUpWidget;

/**
 * Runtime coordinator that derives transient Level-Up presentation from the
 * durable FGridCharacterInventoryState::LastAcknowledgedLevel contract.
 *
 * TD07.3.3.9 removes all persistent notification queues and SaveGame mirrors.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API URPGLevelUpNotificationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "RPG|Level Up")
	int32 GetPendingLevelUpNotificationCount() const;

	UFUNCTION(BlueprintPure, Category = "RPG|Level Up")
	bool IsLevelUpModalOpen() const;

	/**
	 * Binds the authoritative party inventory and reconstructs transient
	 * notifications from LastAcknowledgedLevel < Level.
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

	UPROPERTY(Transient)
	TObjectPtr<URPGLevelUpWidget> ActiveWidget;

	UPROPERTY(Transient)
	TObjectPtr<UGridTurnManagerComponent> DeferredCombatTurnManager;

	FDelegateHandle LevelUpDelegateHandle;
	FDelegateHandle WidgetClosedDelegateHandle;

	void HandleCharacterLevelUpApplied(
		UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex, int32 PreviousLevel, int32 NewLevel, int32 LevelsGained);
	void HandleWidgetClosed(URPGLevelUpWidget* ClosedWidget);

	UFUNCTION()
	void HandlePartyInventoryChanged(int32 CharacterIndex);

	UFUNCTION()
	void HandleDeferredCombatEnded(EGridCombatPhase ResultPhase);

	void BindPartyInventory(UGridPartyInventoryComponent* PartyInventoryComponent);
	void RebuildPendingNotificationsFromPartyState(UGridPartyInventoryComponent* PartyInventoryComponent);
	void AcknowledgeNotification(const FPendingNotification& Notification);
	void SetDeferredCombatTurnManager(UGridTurnManagerComponent* TurnManager);
	void ClearDeferredCombatTurnManager();
	void TryPresentNextNotification();
};
