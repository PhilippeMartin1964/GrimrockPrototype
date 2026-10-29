#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridInventoryTypes.h"
#include "Runtime/GridItemActor.h"
#include "GridThrownItemActor.generated.h"

class AGridLevelRuntimeActor;
class AGridMonsterActor;
class USkeletalMeshComponent;
class UGridItemDefinitionAsset;
enum class EGridProjectileForwardAxis : uint8;
class UProjectileMovementComponent;
class USphereComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogGridThrownItem, Log, All);

UCLASS(Blueprintable)
class GRIMROCKPROTOTYPE_API AGridThrownItemActor : public AGridItemActor
{
	GENERATED_BODY()

public:
	AGridThrownItemActor();
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	UPROPERTY(Transient)
	TObjectPtr<AGridLevelRuntimeActor> RuntimeActor;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile")
	FGridItemInstance ThrownItemInstance;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile")
	int32 SourceCellX = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile")
	int32 SourceCellY = INDEX_NONE;

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void InitializeThrownItem(AGridLevelRuntimeActor* InRuntimeActor, const FGridItemInstance& InItemInstance, UGridItemDefinitionAsset* InDefinition,
		const FVector& LaunchVelocity, int32 InSourceCellX, int32 InSourceCellY);

	/**
     * Presentation-only target interception. It decides where the recoverable
     * item stops, never whether the combat attack hits and never applies damage.
     */
	void ConfigureCombatPresentationTarget(bool bStopAtTarget, const FVector& TargetWorldLocation, float AcceptanceRadius,
		FIntPoint PreferredTargetCell = FIntPoint(INDEX_NONE, INDEX_NONE));

	/** Stable flight and recoverable landing for a consumed ranged ammo unit. */
	void ConfigureAsRangedAmmunition();

	/** Only authoritative ranged hits may visually embed the actual ammo. */
	void ConfigureRangedHitEmbedding(AGridMonsterActor* HitMonster);

	/**
	 * A ranged Miss follows one side of the target (T-shaped aiming plane),
	 * then gravity resumes after passing the monster. No combat side effects.
	 */
	bool ConfigureRangedMissDeflection(const FVector& TargetCenter, const FVector& TargetBoxExtent);

	bool IsEmbeddedInMonster() const { return bEmbeddedInMonster; }

	bool IsRangedMissDeflected() const { return bRangedMissDeflected; }
	bool HasRangedMissGravityStarted() const { return bRangedMissGravityStarted; }
	int32 GetRangedMissSideSign() const { return RangedMissSideSign; }
	AGridMonsterActor* GetEmbeddedMonster() const { return EmbeddedMonster.Get(); }

	/** Shared source-collision policy for all launched items. */
	void ConfigureLaunchSourceActor(AActor* SourceActor);

	/** Unreal-local axis of the tip, before the visual flight correction. */
	static FVector ResolveMeshForwardAxis(EGridProjectileForwardAxis Axis);

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Projectile|Ammunition")
	bool bRangedAmmunitionProjectile = false;

	bool HasCompletedImpactConversion() const
	{
		return bConversionAttempted;
	}

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Projectile|Combat Presentation")
	bool bStopsAtCombatPresentationTarget = false;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Projectile|Combat Presentation")
	FVector CombatPresentationTargetLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Projectile|Combat Presentation")
	float CombatPresentationTargetAcceptanceRadius = 24.0f;

	/** Local Pitch/Yaw/Roll angular velocity applied to the presentation mesh. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Projectile|Visual")
	FVector AppliedThrowVisualAngularVelocityDegreesPerSecond = FVector::ZeroVector;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleProjectileImpact(
		UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void HandleProjectileStopped(const FHitResult& Hit);

private:
	void HandleProjectileExpired();
	void ConvertToWorldPickupAtImpact(const FHitResult& Hit);
	bool TryEmbedInMonster();
	void ReleaseEmbeddedArrowToPickup();
	/** Fallback only when no usable Physics Asset is present. */
	static FName FindNearestVisualBone(USkeletalMeshComponent* Skeletal, const FVector& WorldPoint);

	bool bConversionAttempted = false;
	bool bEmbeddedInMonster = false;
	bool bRangedMissDeflected = false;
	bool bRangedMissGravityStarted = false;
	int32 RangedMissSideSign = 0;
	FVector RangedMissTargetLocation = FVector::ZeroVector;
	FVector RangedMissForward = FVector::ZeroVector;
	TWeakObjectPtr<AGridMonsterActor> PendingHitMonster;
	TWeakObjectPtr<AGridMonsterActor> EmbeddedMonster;
	float ImpactDropOffset = 12.0f;
	FVector InitialFlightWorldLocation = FVector::ZeroVector;
	FVector PreviousPresentationLocation = FVector::ZeroVector;
	FIntPoint LastWalkableFlightCell = FIntPoint(INDEX_NONE, INDEX_NONE);
	FVector LastWalkableFlightOffset = FVector::ZeroVector;
	TWeakObjectPtr<AActor> LaunchSourceActor;
	void AlignMeshForwardToFlight();
	FIntPoint PreferredImpactCell = FIntPoint(INDEX_NONE, INDEX_NONE);
	FTimerHandle ExpirationTimerHandle;
};
