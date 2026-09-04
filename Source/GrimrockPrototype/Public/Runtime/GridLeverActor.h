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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lever")
	float LeverOffPitch = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lever")
	float LeverOnPitch = 135.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lever")
	float ToggleDuration = 0.10f;

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
	FRotator OffRelativeRotation = FRotator::ZeroRotator;
	FRotator OnRelativeRotation = FRotator::ZeroRotator;
	FRotator AnimStartRotation = FRotator::ZeroRotator;
	FRotator AnimTargetRotation = FRotator::ZeroRotator;

	bool bIsAnimating = false;
	float AnimElapsed = 0.f;
};
