#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridCombatProjectileActor.generated.h"

class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Presentation-only projectile used by monster ranged attacks.
 *
 * This actor never resolves hit/miss, applies damage, changes inventory or
 * creates a recoverable pickup. Combat remains authoritative in the TurnManager.
 */
UCLASS (BlueprintType)
class GRIMROCKPROTOTYPE_API AGridCombatProjectileActor : public AActor
{
    GENERATED_BODY ()

public:
    AGridCombatProjectileActor ();
    virtual void Tick (float DeltaSeconds) override;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Projectile")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Projectile")
    TObjectPtr<UStaticMeshComponent> ProjectileMeshComponent;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Combat|Projectile")
    FVector SourceWorldLocation = FVector::ZeroVector;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Combat|Projectile")
    FVector TargetWorldLocation = FVector::ZeroVector;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Combat|Projectile")
    float TravelDuration = 0.20f;

    UPROPERTY (VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Combat|Projectile")
    float ElapsedTravelTime = 0.0f;

    UFUNCTION (BlueprintCallable, Category = "Combat|Projectile")
    bool InitializeProjectilePresentation (
        UStaticMesh* InMesh,
        const FVector& InSourceWorldLocation,
        const FVector& InTargetWorldLocation,
        float InTravelDuration,
        const FVector& InVisualScale,
        const FRotator& InVisualRotationOffset);

    /** Delay from attack start that makes the projectile arrive at authored impact time. */
    static float CalculateLaunchDelay (
        float ImpactTimeSeconds,
        float ProjectileTravelDuration);

    /** Deterministic linear path used only for presentation. Alpha is clamped to [0,1]. */
    static FVector EvaluateTrajectoryLocation (
        const FVector& Source,
        const FVector& Target,
        float Alpha);

private:
    bool bPresentationActive = false;
};
