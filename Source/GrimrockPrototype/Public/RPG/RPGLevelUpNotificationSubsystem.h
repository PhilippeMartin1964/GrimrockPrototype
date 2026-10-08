#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RPGLevelUpNotificationSubsystem.generated.h"

class UGridPartyInventoryComponent;

/**
 * RPG-LEVELUX01 non-modal Level-Up feedback coordinator.
 *
 * The subsystem owns only transient toast sequencing. No durable acknowledgement
 * state exists: FRPGLevelUpService is the sole level authority and this observer
 * never blocks gameplay or changes SaveGame state.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API URPGLevelUpNotificationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	struct FPendingNotification
	{
		TWeakObjectPtr<UGridPartyInventoryComponent> InventoryComponent;
		int32 CharacterIndex = INDEX_NONE;
		FGuid CharacterId;
		int32 PreviousLevel = 1;
		int32 NewLevel = 1;
	};

	TArray<FPendingNotification> PendingNotifications;
	TOptional<FPendingNotification> ActiveNotification;
	FTimerHandle ActiveToastTimerHandle;
	FDelegateHandle LevelUpDelegateHandle;

	void HandleCharacterLevelUpApplied(
		UGridPartyInventoryComponent* PartyInventoryComponent,
		int32 CharacterIndex,
		int32 PreviousLevel,
		int32 NewLevel,
		int32 LevelsGained);

	void EnqueueNotification(
		UGridPartyInventoryComponent* PartyInventoryComponent,
		int32 CharacterIndex,
		int32 PreviousLevel,
		int32 NewLevel);
	bool PresentNotification(const FPendingNotification& Notification, float& OutDurationSeconds);
	void TryPresentNextNotification();
	void HandleActiveToastExpired();
};
