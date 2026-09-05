#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "GridWorldObjectVisual.generated.h"

/**
 * WORLDOBJ-MIG03 visual composition foundation.
 * A moving part always represents a real animation; absence is represented by Mesh == nullptr.
 */
UENUM(BlueprintType)
enum class EGridWorldObjectMotionType : uint8
{
	Rotation UMETA(DisplayName = "Rotation"),
	Translation UMETA(DisplayName = "Translation")
};

UENUM(BlueprintType)
enum class EGridWorldObjectMotionAxis : uint8
{
	X UMETA(DisplayName = "X"),
	Y UMETA(DisplayName = "Y"),
	Z UMETA(DisplayName = "Z")
};

/** Optional fixed geometry of a world object. Mesh == nullptr means no static part. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridWorldObjectStaticPart
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	/** Transform relative to the placed world-object origin, not to the grid cell. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FTransform LocalTransform = FTransform::Identity;

	bool IsDefined() const
	{
		return Mesh != nullptr;
	}
};

/** Motion applied to one moving visual part. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridWorldObjectMotion
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Motion")
	EGridWorldObjectMotionType Type = EGridWorldObjectMotionType::Rotation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Motion")
	EGridWorldObjectMotionAxis Axis = EGridWorldObjectMotionAxis::Z;

	/** Local pivot used only for Rotation motion. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Motion",
		meta = (EditCondition = "Type == EGridWorldObjectMotionType::Rotation", EditConditionHides))
	FVector Pivot = FVector::ZeroVector;

	/** Degrees for Rotation, centimeters for Translation. Signed value carries the direction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Motion")
	float Amount = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Motion", meta = (ClampMin = "0.0"))
	float Duration = 0.0f;
};

/** One animated mesh. Mesh == nullptr means the slot is unused. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridWorldObjectMovingPart
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	/** Transform relative to the placed world-object origin, before Motion is applied. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FTransform LocalTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FGridWorldObjectMotion Motion;

	bool IsDefined() const
	{
		return Mesh != nullptr;
	}
};

/**
 * Strict 0..2 moving-parts container.
 * Two optional fixed slots are intentional: the authoring schema cannot create a third moving part,
 * so no MaxMovingParts parameter or array-size validation is required.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridWorldObjectMovingParts
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (DisplayName = "Moving Part 0"))
	FGridWorldObjectMovingPart Part0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (DisplayName = "Moving Part 1"))
	FGridWorldObjectMovingPart Part1;

	int32 NumDefined() const
	{
		return (Part0.IsDefined() ? 1 : 0) + (Part1.IsDefined() ? 1 : 0);
	}

	bool IsEmpty() const
	{
		return NumDefined() == 0;
	}
};
