#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridMonsterDeathComponent.generated.h"

class AGridLevelRuntimeActor;
class AGridMonsterActor;
class UBoxComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

UCLASS(ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridMonsterDeathComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGridMonsterDeathComponent();

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death")
	TObjectPtr<AGridMonsterActor> OwnerMonster = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death")
	TObjectPtr<AGridLevelRuntimeActor> RuntimeActor = nullptr;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death")
	FIntPoint DeathCell = FIntPoint::ZeroValue;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death")
	bool bDeathCommitted = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death")
	bool bLootGenerated = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death")
	bool bDeathPresentationActive = false;

	/** True when the pre-death fall probe found blocking physical geometry. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Monster|Death|Collision")
	bool bDeathObstacleDetected = false;

	/** Presentation-only skeletal ragdoll. Never restores grid occupancy or gameplay collision. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Monster|Death|Collision")
	bool bDeathRagdollActive = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Monster|Death|Collision")
	FVector LastDeathObstacleImpactPoint = FVector::ZeroVector;

	/** Number of temporary invisible physics guards currently protecting query-only death obstacles. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Monster|Death|Collision")
	int32 DeathCollisionGuardCount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Monster|Death|Dissolve")
	bool bDeathDissolveActive = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Monster|Death|Dissolve", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DeathDissolveAlpha = 0.0f;

	/** Successfully placed runtime item instances. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death")
	TArray<FGridItemInstance> GeneratedLoot;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death")
	int32 PlacedLootCount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death")
	int32 FailedLootCount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death|Debug")
	int32 LogicalDeathEventCount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Monster|Death|Debug")
	int32 LinkExecutionAttemptCount = 0;

	UFUNCTION(BlueprintCallable, Category = "Monster|Death")
	bool InitializeDeathComponent(AGridLevelRuntimeActor* InRuntimeActor = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Monster|Death")
	bool CommitDeath();

	/**
	 * Restores an already committed logical death without replaying presentation.
	 * A restored dead monster is always considered visually recycled and its
	 * skeletal mesh stays hidden. No persistent-corpse option exists.
	 */
	UFUNCTION(BlueprintCallable, Category = "Monster|Death|Persistence")
	void RestoreCommittedDeathState(FIntPoint InDeathCell);

	UFUNCTION(BlueprintCallable, Category = "Monster|Death|Persistence")
	void RestoreLivingState();

	UFUNCTION(BlueprintCallable, Category = "Monster|Death")
	void StartDeathPresentation();

	UFUNCTION(BlueprintCallable, Category = "Monster|Death|Animation Notify")
	void NotifyDeathPresentationComplete();

	/** Resolves the authored local fall direction in world space using the monster's current facing. */
	UFUNCTION(BlueprintPure, Category = "Monster|Death|Collision")
	FVector ResolveDeathFallWorldDirection() const;

	/** Sweeps the authored fall corridor and returns the nearest physically blocking obstacle. */
	UFUNCTION(BlueprintCallable, Category = "Monster|Death|Collision")
	bool ProbeDeathObstacle(FHitResult& OutHit) const;

	/** Stops presentation-only corpse physics and optionally restores the authored skeletal visual pose. */
	UFUNCTION(BlueprintCallable, Category = "Monster|Death|Collision")
	void ResetDeathRagdollPresentation(bool bRestoreVisualPose = true);

	/** Clears transient dissolve state and optionally restores source materials/visibility. */
	UFUNCTION(BlueprintCallable, Category = "Monster|Death|Dissolve")
	void ResetDeathDissolvePresentation(bool bRestoreOriginalMaterials = true, bool bRestoreVisibility = false);

	UFUNCTION(BlueprintPure, Category = "Monster|Death")
	bool IsDeathCommitted() const
	{
		return bDeathCommitted;
	}

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Monster|Death|Debug")
	void DebugKillMonster();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	FTimerHandle DeathPresentationTimerHandle;
	FTimerHandle DeathDissolveDelayTimerHandle;
	FTimerHandle DeathDissolveStepTimerHandle;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> DeathOriginalMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DeathDissolveMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBoxComponent>> DeathCollisionGuards;

	float DeathDissolveStartWorldTime = 0.0f;

	UFUNCTION()
	void HandleOwnerMonsterDied(AGridMonsterActor* Monster, FIntPoint InDeathCell);

	void ScheduleDeathDissolve();
	void StartDeathDissolve();
	void UpdateDeathDissolve();
	void FinishDeathDissolve();
	void ClearDeathDissolveTimers();

	bool TryStartObstacleAwareDeathRagdoll();
	bool BuildDeathCollisionGuard(const FHitResult& ObstacleHit);
	void ClearDeathCollisionGuards();
	void ScheduleDeathPresentationCompletion();
	void GenerateAndPlaceLoot();
	AGridLevelRuntimeActor* FindRuntimeActor() const;
};
