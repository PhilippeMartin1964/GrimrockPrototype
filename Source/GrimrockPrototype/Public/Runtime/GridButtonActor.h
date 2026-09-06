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
	/** Runtime cache. Geometric travel and duration are authored by MovingPart[0].Motion. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Button")
	float PressDuration = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Button")
	float ReleaseDuration = 0.0f;

	/** Gameplay rule: time spent held at logical alpha 1 before release starts. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Button")
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

private:
	enum class EButtonAnimState : uint8
	{
		Idle,
		Pressing,
		Holding,
		Releasing
	};

	EButtonAnimState AnimState = EButtonAnimState::Idle;
	float StateElapsed = 0.f;
};