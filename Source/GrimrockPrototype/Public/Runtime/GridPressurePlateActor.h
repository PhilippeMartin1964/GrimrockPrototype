#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridMechanismActor.h"
#include "GridPressurePlateActor.generated.h"

UCLASS()
class GRIMROCKPROTOTYPE_API AGridPressurePlateActor : public AGridMechanismActor
{
	GENERATED_BODY()

public:
	AGridPressurePlateActor();

	virtual void Tick(float DeltaSeconds) override;

public:
	/** Runtime cache. Geometric travel and duration are authored by MovingPart[0].Motion. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Plate")
	float MoveDuration = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Plate")
	bool bIsPressed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Plate|Weight")
	float CurrentItemWeight = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Plate|Weight")
	float RequiredItemWeight = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Plate|Weight")
	bool bUseItemWeight = false;

	UPROPERTY(BlueprintReadOnly, Category = "Plate|Weight")
	bool bActivateWhenPartyPresent = true;

	UFUNCTION(BlueprintCallable, Category = "Plate")
	void InitializePlate(
		const FGridLevelObjectData& ObjectData, UStaticMesh* InPlateMesh, const FVector& InWorldLocation, bool bStartPressed);

	UFUNCTION(BlueprintCallable, Category = "Plate")
	void SetPressed(bool bNewPressed);

	UFUNCTION(BlueprintCallable, Category = "Plate|Weight")
	void SetWeightState(float InCurrentItemWeight, float InRequiredItemWeight, bool bInUseItemWeight, bool bInActivateWhenPartyPresent);

	virtual void InitializeGridObject(
		const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform) override;

protected:
	void UpdateAnimation(float DeltaSeconds);

private:
	float CurrentMotionAlpha = 0.0f;
	float AnimStartMotionAlpha = 0.0f;
	float AnimTargetMotionAlpha = 0.0f;
	float CurrentMoveDuration = 0.0f;

	bool bIsAnimating = false;
	float AnimElapsed = 0.f;
};