#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "Runtime/Monsters/GridMonsterAudioTypes.h"
#include "Runtime/Monsters/GridMonsterTypes.h"
#include "GridMonsterAudioComponent.generated.h"

class AGridMonsterActor;

DECLARE_LOG_CATEGORY_EXTERN(LogGridMonsterAudio, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridMonsterAudioPlaybackRequestedSignature, FGridMonsterAudioPlaybackRequest, Request);

UCLASS(ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridMonsterAudioComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGridMonsterAudioComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monster|Audio")
	bool bAudioEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Transient, Category = "Monster|Audio|Debug")
	bool bNativePlaybackEnabled = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Monster|Audio")
	TObjectPtr<AGridMonsterActor> OwnerMonster = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Monster|Audio")
	FGridMonsterAudioPlaybackRequest LastPlaybackRequest;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Monster|Audio|Debug")
	int32 PlaybackRequestCount = 0;

	UPROPERTY(BlueprintAssignable, Category = "Monster|Audio")
	FGridMonsterAudioPlaybackRequestedSignature OnAudioPlaybackRequested;

	UFUNCTION(BlueprintCallable, Category = "Monster|Audio")
	bool InitializeMonsterAudio();

	UFUNCTION(BlueprintPure, Category = "Monster|Audio")
	bool IsInitialized() const
	{
		return bInitialized;
	}

	UFUNCTION(BlueprintCallable, Category = "Monster|Audio")
	bool PlayAlert();

	UFUNCTION(BlueprintCallable, Category = "Monster|Audio")
	bool PlayAttack(const FGridMonsterAttackDefinition& Attack);

	UFUNCTION(BlueprintCallable, Category = "Monster|Audio")
	bool PlayAttackImpact(const FGridMonsterAttackDefinition& Attack, const FGridAttackResult& Result, FVector ImpactWorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Monster|Audio")
	bool PlayHurt();

	UFUNCTION(BlueprintCallable, Category = "Monster|Audio")
	bool PlayDeath();

	UFUNCTION(BlueprintCallable, Category = "Monster|Audio")
	bool PlayIdleAmbienceNow();

	UFUNCTION(BlueprintCallable, Category = "Monster|Audio")
	void RefreshIdleAmbienceScheduling();

	UFUNCTION(BlueprintCallable, Category = "Monster|Audio")
	void StopIdleAmbience();

	UFUNCTION(BlueprintCallable, Category = "Monster|Audio")
	void StopAllMonsterAudio();

	/** Clears only transient presentation history; it never touches gameplay. */
	UFUNCTION(BlueprintCallable, Category = "Monster|Audio")
	void ResetTransientAudioState();

	UFUNCTION(BlueprintPure, Category = "Monster|Audio|Debug")
	int32 GetPlaybackRequestCountForEvent(EGridMonsterAudioEvent Event) const;

	UFUNCTION(BlueprintPure, Category = "Monster|Audio|Debug")
	bool IsIdleAmbienceScheduled() const;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Monster|Audio|Debug")
	void LogMonsterAudioState() const;

private:
	static constexpr int32 AudioEventCount = 7;

	bool bInitialized = false;
	int32 NextSequenceNumber = 1;
	int32 IdleScheduleOccurrence = 0;
	TArray<int32> EventOccurrenceCounts;
	TArray<int32> EventPlaybackRequestCounts;
	TArray<double> LastAcceptedEventTimes;
	FTimerHandle IdleAmbienceTimerHandle;

	bool PlayDefinition(EGridMonsterAudioEvent Event, const FGridMonsterAudioEventDefinition& Definition, FName AttackId, const FVector& WorldLocation);

	const FGridMonsterAudioEventDefinition* GetMonsterDefinition(EGridMonsterAudioEvent Event) const;

	bool CanRequestAudio(EGridMonsterAudioEvent Event) const;
	bool CanScheduleIdleAmbience() const;
	int32 GetEventIndex(EGridMonsterAudioEvent Event) const;

	UFUNCTION()
	void HandleIdleAmbienceTimer();
};
