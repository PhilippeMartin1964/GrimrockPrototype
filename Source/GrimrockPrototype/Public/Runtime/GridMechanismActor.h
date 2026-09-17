#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "GridMechanismActor.generated.h"

UCLASS()
class GRIMROCKPROTOTYPE_API AGridMechanismActor : public AGridRuntimeObjectActor
{
	GENERATED_BODY()

public:
	AGridMechanismActor();

	/** Runtime-native mechanism presentation boundary. */
	virtual void InitializeRuntimeMechanismVisuals(
		const FGridRuntimeWorldObjectData& ObjectData, const UGridWorldObjectDefinitionAsset* Definition, const FTransform& WorldTransform);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mechanism")
	TObjectPtr<UStaticMeshComponent> FixedMeshComponent = nullptr;

	/** MovingPart[0]. Kept under the historical component name so existing one-part mechanism state machines stay compact. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mechanism")
	TObjectPtr<UStaticMeshComponent> MovingMeshComponent = nullptr;

	/** Runtime components matching Definition->MovingParts by index. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Mechanism")
	TArray<TObjectPtr<UStaticMeshComponent>> MovingPartMeshComponents;

protected:
	void SetFixedMesh(UStaticMesh* Mesh);
	void SetMovingMesh(UStaticMesh* Mesh);

	/** Local offset used by one-part mechanism state machines relative to MovingPart[0].LocalTransform. */
	void SetMovingRelativeLocation(const FVector& RelativeLocation);
	FVector GetMovingRelativeLocation() const;

	/** Local rotation used by the lever state machine relative to MovingPart[0].LocalTransform. */
	void SetMovingRelativeRotation(const FRotator& RelativeRotation);
	FRotator GetMovingRelativeRotation() const;

	/** Applies the authored generic Motion for one moving part from its LocalTransform. */
	void ApplyMovingPartMotionAlpha(int32 PartIndex, float Alpha);
	void ApplyAllMovingPartMotionsAlpha(float Alpha);

	/**
	 * Longest authored moving-part duration for the requested direction.
	 * ReverseDuration <= 0 falls back to Duration per moving part.
	 */
	float GetTargetMotionDuration(bool bReverse = false) const;

	/** WORLDOBJ-MIG03.4: mechanisms exclusively use StaticPart/MovingParts. */
	bool UsesTargetVisualComposition() const
	{
		return true;
	}

	const FGridWorldObjectMotion& GetMovingPartMotion(int32 PartIndex) const;
	UStaticMeshComponent* GetMovingPartComponent(int32 PartIndex) const;

private:
	TArray<FTransform> MovingPartBaseTransforms;
	TArray<FGridWorldObjectMotion> MovingPartMotions;
};
