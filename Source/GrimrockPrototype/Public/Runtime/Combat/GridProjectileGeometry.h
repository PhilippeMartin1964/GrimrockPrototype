#pragma once

#include "CoreMinimal.h"
#include "GridProjectileGeometry.generated.h"

/** Tip axis in the imported Unreal StaticMesh's local coordinate system. */
UENUM(BlueprintType)
enum class EGridProjectileForwardAxis : uint8
{
	PositiveX UMETA(DisplayName = "+X (Unreal forward)"),
	NegativeX UMETA(DisplayName = "-X"),
	PositiveY UMETA(DisplayName = "+Y (World North)"),
	NegativeY UMETA(DisplayName = "-Y"),
	PositiveZ UMETA(DisplayName = "+Z"),
	NegativeZ UMETA(DisplayName = "-Z")
};

/** One geometric flight-frame rule for arrows, thrown items, monster shots and spells. */
struct GRIMROCKPROTOTYPE_API FGridProjectileGeometry
{
	static FVector MeshTipAxis(EGridProjectileForwardAxis Axis)
	{
		switch (Axis)
		{
			case EGridProjectileForwardAxis::NegativeX: return -FVector::ForwardVector;
			case EGridProjectileForwardAxis::PositiveY: return FVector::RightVector;
			case EGridProjectileForwardAxis::NegativeY: return -FVector::RightVector;
			case EGridProjectileForwardAxis::PositiveZ: return FVector::UpVector;
			case EGridProjectileForwardAxis::NegativeZ: return -FVector::UpVector;
			default: return FVector::ForwardVector;
		}
	}

	static FQuat MeshToFlightRotation(EGridProjectileForwardAxis Axis)
	{
		return FQuat::FindBetweenNormals(MeshTipAxis(Axis), FVector::ForwardVector);
	}
};
