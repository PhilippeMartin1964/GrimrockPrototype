#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridMechanismActor.h"
#include "Runtime/GridInteractableInterface.h"
#include "Core/GridTypes.h"
#include "GridLeverActor.generated.h"

UCLASS()
class GRIMROCKPROTOTYPE_API AGridLeverActor : public AGridMechanismActor, public IGridInteractableInterface
{
	GENERATED_BODY()

public:
	AGridLeverActor();

	virtual void Tick(float DeltaSeconds) override;

public:
	/** Runtime cache. Rotation/translation geometry is authored exclusively by MovingPart[0].Motion. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Lever")
	float ToggleDuration = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Lever")
	bool bIsOn = false;

	UFUNCTION(BlueprintCallable, Category = "Lever")
	void InitializeLever(const FGridLevelObjectData& ObjectData, UStaticMesh* InLeverMesh, const FVector& InWorldLocation,
		const FRotator& InWorldRotation, bool bStartOn);

	UFUNCTION(BlueprintCallable, Category = "Lever")
	void SetLeverState(bool bNewOn);

	UFUNCTION(BlueprintCallable, Category = "Lever")
	void ToggleLever();

	virtual void InitializeGridObject(
		const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform) override;

	virtual bool CanInteract_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const override;
	virtual void Interact_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) override;
	virtual EGridInteractionCursor GetInteractionCursor_Implementation(UPrimitiveComponent* HitComponent) const override;
	virtual FText GetInteractionText_Implementation(UPrimitiveComponent* HitComponent) const override;

protected:
	void UpdateAnimation(float DeltaSeconds);

private:
	float CurrentMotionAlpha = 0.0f;
	float AnimStartMotionAlpha = 0.0f;
	float AnimTargetMotionAlpha = 0.0f;
	float CurrentToggleDuration = 0.0f;

	bool bIsAnimating = false;
	float AnimElapsed = 0.f;
};