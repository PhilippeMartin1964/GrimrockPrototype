#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridMechanismActor.h"
#include "GridPitTrapdoorActor.generated.h"

class UAudioComponent;

/** PitObjectId, previous settled gameplay state, new settled gameplay state. */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnGridPitTrapdoorAnimationFinished, FGuid, bool, bool);

/**
 * PIT03/PIT03.1 presentation actor for a controlled pit.
 * Gameplay authority remains in AGridLevelRuntimeActor.
 * FixedMesh = permanent open-pit geometry.
 * MovingMesh = optional animated trapdoor cover.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API AGridPitTrapdoorActor : public AGridMechanismActor
{
	GENERATED_BODY()

public:
	AGridPitTrapdoorActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void InitializeMechanismVisuals(
		const FGridLevelObjectData& ObjectData, const UGridObjectArchetypeAsset* Archetype, const FTransform& WorldTransform) override;

	virtual void InitializeGridObject(
		const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& WorldTransform) override;

	/**
	 * Requests an animated target state.
	 * If no MovingMesh exists, the state settles immediately.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pit")
	void SetPitOpenVisualState(bool bOpen, bool bPlayAudio = true);

	/** Restores a persisted state without animation or gameplay event. */
	UFUNCTION(BlueprintCallable, Category = "Pit")
	void SnapPitOpenState(bool bOpen);

	UFUNCTION(BlueprintPure, Category = "Pit")
	bool IsPitOpenVisualState() const
	{
		return bIsOpen;
	}

	UFUNCTION(BlueprintPure, Category = "Pit")
	bool IsTargetOpen() const
	{
		return bTargetOpen;
	}

	UFUNCTION(BlueprintPure, Category = "Pit")
	bool IsAnimating() const
	{
		return bIsAnimating;
	}

	UFUNCTION(BlueprintPure, Category = "Pit")
	float GetCurrentOpenAlpha() const
	{
		return CurrentOpenAlpha;
	}

	/** Native runtime notification used to settle gameplay only at an animation endpoint. */
	FOnGridPitTrapdoorAnimationFinished OnPitAnimationFinished;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pit|Animation")
	FRotator OpenRelativeRotation = FRotator(-90.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pit|Animation", meta = (ClampMin = "0.0"))
	float MoveDuration = 0.75f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Pit|Animation")
	bool bIsOpen = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Pit|Animation")
	bool bTargetOpen = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Pit|Animation")
	bool bIsAnimating = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Pit|Animation")
	float CurrentOpenAlpha = 1.0f;

protected:
	void UpdateAnimation(float DeltaSeconds);
	void ApplyOpenAlpha(float Alpha);
	void RefreshMovingMeshCollision();
	void RefreshTickEnabled();
	void StartPitMotionSound(bool bOpening, float StartTimeSeconds, bool bEnableNativePlayback);
	void StopPitMotionSound();

private:
	FRotator ClosedRelativeRotation = FRotator::ZeroRotator;
	float MoveStartAlpha = 1.0f;
	float MoveTargetAlpha = 1.0f;
	float MoveElapsed = 0.0f;
	float CurrentMoveDuration = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActivePitAudioComponent = nullptr;
};
