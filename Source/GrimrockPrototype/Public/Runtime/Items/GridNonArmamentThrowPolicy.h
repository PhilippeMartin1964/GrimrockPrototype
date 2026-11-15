#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridProjectileGeometry.h"
#include "GridNonArmamentThrowPolicy.generated.h"

/** Generic non-weapon visual motion, independent of weapon attack presentation. */
UENUM(BlueprintType)
enum class EGridNonArmamentThrowVisualMode : uint8
{
    Stable,
    Tumble,
    Spin
};

/**
 * ARM-CLEAN06.4N: one authored policy for the physical and visual launch
 * of a generic non-armament item (e.g. puzzle component, torch or stone).
 * This MUST NOT live in DA_WeaponCatalogue.ProjectilePolicies. The combat
 * flag is conserved for explicit action mapping, not permission to attack.
 */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridNonArmamentThrowPolicy
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Non-Armament|Throw")
    bool bCombatRecoverable = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Non-Armament|Throw", meta = (ClampMin = "0.0"))
    float InitialSpeed = 1800.0f;

    /** Preserve signed arcs. The real legacy launcher clamps in its executor. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Non-Armament|Throw")
    float Arc = 0.08f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Non-Armament|Throw", meta = (ClampMin = "0.0"))
    float LifeSeconds = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Non-Armament|Throw", meta = (ClampMin = "0.0"))
    float ImpactDropOffset = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Non-Armament|Throw|Visual")
    FRotator VisualRelativeRotation = FRotator(-90.0f, 0.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Non-Armament|Throw|Visual")
    FVector VisualRelativeScale = FVector(1.5f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Non-Armament|Throw|Visual")
    EGridProjectileForwardAxis MeshForwardAxis = EGridProjectileForwardAxis::PositiveX;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Non-Armament|Throw|Visual")
    EGridNonArmamentThrowVisualMode VisualMode = EGridNonArmamentThrowVisualMode::Tumble;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Non-Armament|Throw|Visual")
    FVector VisualTumbleAxis = FVector(0.65f, 0.35f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Non-Armament|Throw|Visual", meta = (ClampMin = "0.0"))
    float VisualTumbleDegreesPerSecond = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Non-Armament|Throw|Visual", meta = (ClampMin = "0.0"))
    float VisualSpinDegreesPerSecond = 1080.0f;
};
