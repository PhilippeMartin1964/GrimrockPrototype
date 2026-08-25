#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Runtime/Monsters/GridMonsterIdleVariationTypes.h"
#include "GridMonsterIdleVariationComponent.generated.h"

class AGridMonsterActor;
class UAnimMontage;

DECLARE_LOG_CATEGORY_EXTERN(LogGridMonsterIdleVariation, Log, All);

UCLASS(ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridMonsterIdleVariationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGridMonsterIdleVariationComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Idle Variation")
	bool bIdleVariationsEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "Monster|Idle Variation|Debug")
	bool bNativePlaybackEnabled = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Monster|Idle Variation")
	TObjectPtr<AGridMonsterActor> OwnerMonster = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Monster|Idle Variation")
	FGridMonsterIdleVariationPlaybackRequest LastPlaybackRequest;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Monster|Idle Variation|Debug")
	int32 PlaybackRequestCount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Monster|Idle Variation|Debug")
	int32 PlaybackRequestBroadcastCount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Monster|Idle Variation")
	int32 CurrentVariationIndex = INDEX_NONE;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Monster|Idle Variation")
	bool bIdleVariationActive = false;

	UPROPERTY(BlueprintAssignable, Category = "Monster|Idle Variation")
	FGridMonsterIdleVariationPlaybackRequestedSignature OnIdleVariationPlaybackRequested;

	UFUNCTION(BlueprintCallable, Category = "Monster|Idle Variation")
	bool InitializeIdleVariations();

	UFUNCTION(BlueprintPure, Category = "Monster|Idle Variation")
	bool IsInitialized() const
	{
		return bInitialized;
	}

	UFUNCTION(BlueprintCallable, Category = "Monster|Idle Variation")
	void RefreshIdleVariationScheduling();

	UFUNCTION(BlueprintCallable, Category = "Monster|Idle Variation")
	bool PlayIdleVariationNow();

	UFUNCTION(BlueprintCallable, Category = "Monster|Idle Variation")
	void StopIdleVariations();

	UFUNCTION(BlueprintCallable, Category = "Monster|Idle Variation")
	void ResetTransientIdleVariationState();

	UFUNCTION(BlueprintPure, Category = "Monster|Idle Variation")
	bool IsIdleVariationDelayScheduled() const;

	UFUNCTION(BlueprintPure, Category = "Monster|Idle Variation")
	bool IsIdleVariationActive() const
	{
		return bIdleVariationActive;
	}

	UFUNCTION(BlueprintPure, Category = "Monster|Idle Variation")
	int32 GetPlaybackRequestCount() const
	{
		return PlaybackRequestCount;
	}

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Monster|Idle Variation|Debug")
	void LogIdleVariationState() const;

private:
	bool bInitialized = false;
	int32 NextSequenceNumber = 1;
	int32 CompletedOccurrenceNumber = 0;
	int32 ScheduledOccurrenceNumber = 0;
	int32 PreviousVariationIndex = INDEX_NONE;
	float LastScheduledDelay = 0.0f;
	FTimerHandle IdleVariationDelayTimerHandle;
	FTimerHandle IdleVariationDurationTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveIdleDynamicMontage = nullptr;

	bool CanUseIdleVariations() const;
	void ClearDelayTimer();
	void ClearDurationTimer();

	UFUNCTION()
	void HandleIdleVariationDelayTimer();

	UFUNCTION()
	void HandleIdleVariationDurationTimer();

	friend class FGridMonsterMON10IdleVariationSchedulingLifecycleTest;
	friend class FGridMonsterMON10IdleVariationRequestExactlyOnceTest;
	friend class FGridMonsterMON10IdleVariationRestoreSilentTest;
	friend class FGridMonsterMON10IdleVariationNoTickNoPersistenceAndIsolationTest;
};
