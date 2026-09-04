#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridMechanismActor.h"
#include "Runtime/GridInteractableInterface.h"
#include "Core/GridTypes.h"
#include "GridButtonActor.generated.h"

UCLASS()
class GRIMROCKPROTOTYPE_API AGridButtonActor : public AGridMechanismActor, public IGridInteractableInterface
{
	GENERATED_BODY()

public:
	AGridButtonActor();

	virtual void Tick(float DeltaSeconds) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button")
	float PressDistance = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button")
	float PressDuration = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button")
	float ReleaseDuration = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Button")
	float HoldTime = 0.15f;

	UFUNCTION(BlueprintCallable, Category = "Button")
	void InitializeButton(const FGridLevelObjectData& ObjectData, UStaticMesh* InButtonMesh, const FVector& InWorldLocation,
		const FRotator& InWorldRotation);

	UFUNCTION(BlueprintCallable, Category = "Button")
	void TriggerPress();

	virtual void InitializeGridObject(
		const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform) override;

	virtual bool CanInteract_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const override;
	virtual void Interact_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) override;
	virtual EGridInteractionCursor GetInteractionCursor_Implementation(UPrimitiveComponent* HitComponent) const override;
	virtual FText GetInteractionText_Implementation(UPrimitiveComponent* HitComponent) const override;

protected:
	void UpdateAnimation(float DeltaSeconds);
	FVector GetPressAxis() const;

private:
	enum class EButtonAnimState : uint8
	{
		Idle,
		Pressing,
		Holding,
		Releasing
	};

	FVector ReleasedLocation = FVector::ZeroVector;
	FVector PressedLocation = FVector::ZeroVector;

	EButtonAnimState AnimState = EButtonAnimState::Idle;
	float StateElapsed = 0.f;
};
