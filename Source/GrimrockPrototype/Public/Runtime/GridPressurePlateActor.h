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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plate")
	float ReleasedHeightAboveFloor = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plate")
	float PressedHeightAboveFloor = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Plate")
	float MoveDuration = 0.08f;

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
		const FGridLevelObjectData& ObjectData, UStaticMesh* InPlateMesh, UMaterialInterface* InMaterial, const FVector& InWorldLocation, bool bStartPressed);

	UFUNCTION(BlueprintCallable, Category = "Plate")
	void SetPressed(bool bNewPressed);

	UFUNCTION(BlueprintCallable, Category = "Plate|Weight")
	void SetWeightState(float InCurrentItemWeight, float InRequiredItemWeight, bool bInUseItemWeight, bool bInActivateWhenPartyPresent);

	virtual void InitializeGridObject(
		const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& WorldTransform) override;

protected:
	void UpdateAnimation(float DeltaSeconds);

private:
	FVector ReleasedLocation = FVector::ZeroVector;
	FVector PressedLocation = FVector::ZeroVector;
	FVector AnimStartLocation = FVector::ZeroVector;
	FVector AnimTargetLocation = FVector::ZeroVector;

	bool bIsAnimating = false;
	float AnimElapsed = 0.f;
};
