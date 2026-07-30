#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridInventoryTypes.h"
#include "Runtime/GridItemActor.h"
#include "GridThrownItemActor.generated.h"

class AGridLevelRuntimeActor;
class UGridItemDefinitionAsset;
class UProjectileMovementComponent;
class USphereComponent;

UCLASS (Blueprintable)
class GRIMROCKPROTOTYPE_API AGridThrownItemActor : public AGridItemActor
{
    GENERATED_BODY ()

public:
    AGridThrownItemActor ();
    virtual void Tick (float DeltaSeconds) override;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
    TObjectPtr<USphereComponent> CollisionComponent;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
    TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

    UPROPERTY (Transient)
    TObjectPtr<AGridLevelRuntimeActor> RuntimeActor;

    UPROPERTY (BlueprintReadOnly, Category = "Projectile")
    FGridItemInstance ThrownItemInstance;

    UPROPERTY (BlueprintReadOnly, Category = "Projectile")
    int32 SourceCellX = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Projectile")
    int32 SourceCellY = INDEX_NONE;

    UFUNCTION (BlueprintCallable, Category = "Projectile")
    void InitializeThrownItem (
        AGridLevelRuntimeActor* InRuntimeActor,
        const FGridItemInstance& InItemInstance,
        UGridItemDefinitionAsset* InDefinition,
        const FVector& LaunchVelocity,
        int32 InSourceCellX,
        int32 InSourceCellY);

    /**
     * Presentation-only target interception. It decides where the recoverable
     * item stops, never whether the combat attack hits and never applies damage.
     */
    void ConfigureCombatPresentationTarget (
        bool bStopAtTarget,
        const FVector& TargetWorldLocation,
        float AcceptanceRadius);

    bool HasCompletedImpactConversion () const
    {
        return bConversionAttempted;
    }

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Projectile|Combat Presentation")
    bool bStopsAtCombatPresentationTarget = false;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Projectile|Combat Presentation")
    FVector CombatPresentationTargetLocation = FVector::ZeroVector;

    UPROPERTY (BlueprintReadOnly, Transient, Category = "Projectile|Combat Presentation")
    float CombatPresentationTargetAcceptanceRadius = 24.0f;

protected:
    virtual void BeginPlay () override;

    UFUNCTION ()
    void HandleProjectileImpact (
        UPrimitiveComponent* HitComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        FVector NormalImpulse,
        const FHitResult& Hit);

    UFUNCTION ()
    void HandleProjectileStopped (const FHitResult& Hit);

private:
    void HandleProjectileExpired ();
    void ConvertToWorldPickupAtImpact (const FHitResult& Hit);

    bool bConversionAttempted = false;
    float ImpactDropOffset = 12.0f;
    FVector PreviousPresentationLocation = FVector::ZeroVector;
    FTimerHandle ExpirationTimerHandle;
};
