#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "GridMechanismActor.generated.h"

UCLASS()
class GRIMROCKPROTOTYPE_API AGridMechanismActor : public AGridRuntimeObjectActor
{
	GENERATED_BODY()

public:
	AGridMechanismActor();
	virtual void InitializeMechanismVisuals(
		const FGridLevelObjectData& ObjectData, const UGridObjectArchetypeAsset* Archetype, const FTransform& WorldTransform);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mechanism")
	TObjectPtr<UStaticMeshComponent> FixedMeshComponent = nullptr;

	/** MovingPart[0]. Kept under the historical component name so existing one-part mechanism state machines stay compact. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mechanism")
	TObjectPtr<UStaticMeshComponent> MovingMeshComponent = nullptr;

	/** MovingPart[1]. Null mesh means the second moving slot is unused. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mechanism")
	TObjectPtr<UStaticMeshComponent> SecondaryMovingMeshComponent = nullptr;

protected:
	void SetFixedMesh(UStaticMesh* Mesh);
	void SetMovingMesh(UStaticMesh* Mesh);
	void SetSecondaryMovingMesh(UStaticMesh* Mesh);

	/** Local offset used by one-part mechanism state machines relative to MovingPart[0].LocalTransform. */
	void SetMovingRelativeLocation(const FVector& RelativeLocation);
	FVector GetMovingRelativeLocation() const;

	/** Local rotation used by the lever state machine relative to MovingPart[0].LocalTransform. */
	void SetMovingRelativeRotation(const FRotator& RelativeRotation);
	FRotator GetMovingRelativeRotation() const;

	/** Applies the authored generic Motion for Part0 or Part1 from its LocalTransform. */
	void ApplyMovingPartMotionAlpha(int32 PartIndex, float Alpha);
	void ApplyAllMovingPartMotionsAlpha(float Alpha);

	/** Longest authored moving-part duration. Paired moving parts share the same normalized state alpha. */
	float GetTargetMotionDuration() const;

	/** WORLDOBJ-MIG03.4: mechanisms exclusively use StaticPart/MovingParts. */
	bool UsesTargetVisualComposition() const
	{
		return true;
	}

	const FGridWorldObjectMotion& GetMovingPartMotion(int32 PartIndex) const
	{
		return PartIndex == 1 ? MovingPart1Motion : MovingPart0Motion;
	}

private:
	FTransform MovingPart0BaseTransform = FTransform::Identity;
	FTransform MovingPart1BaseTransform = FTransform::Identity;
	FGridWorldObjectMotion MovingPart0Motion;
	FGridWorldObjectMotion MovingPart1Motion;
};
