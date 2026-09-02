#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridMechanismActor.h"
#include "GridPitTrapdoorActor.generated.h"

class UAudioComponent;
class USceneComponent;
class UStaticMeshComponent;

/** PitObjectId, previous settled gameplay state, new settled gameplay state. */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnGridPitTrapdoorAnimationFinished, FGuid, bool, bool);

/**
 * PIT03.2 dual-leaf controlled pit trapdoor.
 * Gameplay authority remains in AGridLevelRuntimeActor.
 * FixedMesh = permanent pit surround/open-pit geometry.
 * PitLeftLeafMesh/PitRightLeafMesh = two independent leaves rotating around local-Y hinge pivots.
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

	UFUNCTION(BlueprintCallable, Category = "Pit")
	void SetPitOpenVisualState(bool bOpen, bool bPlayAudio = true);

	UFUNCTION(BlueprintCallable, Category = "Pit")
	void SnapPitOpenState(bool bOpen);

	UFUNCTION(BlueprintPure, Category = "Pit")
	bool IsPitOpenVisualState() const { return bIsOpen; }

	UFUNCTION(BlueprintPure, Category = "Pit")
	bool IsTargetOpen() const { return bTargetOpen; }

	UFUNCTION(BlueprintPure, Category = "Pit")
	bool IsAnimating() const { return bIsAnimating; }

	UFUNCTION(BlueprintPure, Category = "Pit")
	float GetCurrentOpenAlpha() const { return CurrentOpenAlpha; }

	UFUNCTION(BlueprintPure, Category = "Pit|Trapdoor")
	bool HasCompleteTrapdoorCover() const;

	UFUNCTION(BlueprintPure, Category = "Pit|Trapdoor")
	float GetLeftLeafPitch() const;

	UFUNCTION(BlueprintPure, Category = "Pit|Trapdoor")
	float GetRightLeafPitch() const;

	UFUNCTION(BlueprintPure, Category = "Pit|Trapdoor")
	FVector GetLeftHingeLocation() const;

	UFUNCTION(BlueprintPure, Category = "Pit|Trapdoor")
	FVector GetRightHingeLocation() const;

	FOnGridPitTrapdoorAnimationFinished OnPitAnimationFinished;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pit|Animation")
	FVector LeftHingeLocation = FVector(-85.0f, 0.0f, -5.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pit|Animation")
	FVector RightHingeLocation = FVector(85.0f, 0.0f, -5.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pit|Animation", meta = (ClampMin = "0.0", ClampMax = "120.0"))
	float OpenAngleDegrees = 80.0f;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pit|Trapdoor")
	TObjectPtr<USceneComponent> LeftHingeComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pit|Trapdoor")
	TObjectPtr<USceneComponent> RightHingeComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pit|Trapdoor")
	TObjectPtr<UStaticMeshComponent> LeftLeafMeshComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pit|Trapdoor")
	TObjectPtr<UStaticMeshComponent> RightLeafMeshComponent = nullptr;

protected:
	void UpdateAnimation(float DeltaSeconds);
	void ApplyOpenAlpha(float Alpha);
	void ConfigureLeafGeometry();
	void RefreshTrapdoorCollision();
	void RefreshTickEnabled();
	void StartPitMotionSound(bool bOpening, float StartTimeSeconds, bool bEnableNativePlayback);
	void StopPitMotionSound();

private:
	float MoveStartAlpha = 1.0f;
	float MoveTargetAlpha = 1.0f;
	float MoveElapsed = 0.0f;
	float CurrentMoveDuration = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActivePitAudioComponent = nullptr;
};
