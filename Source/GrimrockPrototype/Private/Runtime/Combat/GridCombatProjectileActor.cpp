#include "Runtime/Combat/GridCombatProjectileActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

AGridCombatProjectileActor::AGridCombatProjectileActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ProjectileMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMeshComponent->SetupAttachment(SceneRoot);
	ProjectileMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMeshComponent->SetGenerateOverlapEvents(false);
	ProjectileMeshComponent->SetSimulatePhysics(false);
	ProjectileMeshComponent->SetEnableGravity(false);
}

void AGridCombatProjectileActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bPresentationActive)
	{
		SetActorTickEnabled(false);
		return;
	}

	ElapsedTravelTime += FMath::Max(0.0f, DeltaSeconds);
	const float Alpha = TravelDuration > KINDA_SMALL_NUMBER ? FMath::Clamp(ElapsedTravelTime / TravelDuration, 0.0f, 1.0f) : 1.0f;

	SetActorLocation(EvaluateTrajectoryLocation(SourceWorldLocation, TargetWorldLocation, Alpha));

	if (Alpha >= 1.0f - KINDA_SMALL_NUMBER)
	{
		bPresentationActive = false;
		SetActorTickEnabled(false);
		Destroy();
	}
}

bool AGridCombatProjectileActor::InitializeProjectilePresentation(UStaticMesh* InMesh, const FVector& InSourceWorldLocation,
	const FVector& InTargetWorldLocation, float InTravelDuration, const FVector& InVisualScale, const FRotator& InVisualRotationOffset)
{
	if (!IsValid(InMesh) || InSourceWorldLocation.ContainsNaN() || InTargetWorldLocation.ContainsNaN() || !FMath::IsFinite(InTravelDuration) ||
		InTravelDuration <= 0.0f || InVisualScale.ContainsNaN() || InVisualScale.X <= 0.0f || InVisualScale.Y <= 0.0f || InVisualScale.Z <= 0.0f)
	{
		return false;
	}

	SourceWorldLocation = InSourceWorldLocation;
	TargetWorldLocation = InTargetWorldLocation;
	TravelDuration = InTravelDuration;
	ElapsedTravelTime = 0.0f;

	ProjectileMeshComponent->SetStaticMesh(InMesh);
	ProjectileMeshComponent->SetRelativeScale3D(InVisualScale);
	ProjectileMeshComponent->SetRelativeRotation(InVisualRotationOffset);

	SetActorLocation(SourceWorldLocation);
	const FVector FlightDirection = TargetWorldLocation - SourceWorldLocation;
	if (!FlightDirection.IsNearlyZero())
	{
		SetActorRotation(FlightDirection.Rotation());
	}

	bPresentationActive = true;
	SetActorTickEnabled(true);
	return true;
}

float AGridCombatProjectileActor::CalculateLaunchDelay(float ImpactTimeSeconds, float ProjectileTravelDuration)
{
	const float SafeImpactTime = FMath::Max(0.0f, ImpactTimeSeconds);
	const float SafeTravelDuration = FMath::Max(0.0f, ProjectileTravelDuration);
	return FMath::Max(0.0f, SafeImpactTime - SafeTravelDuration);
}

FVector AGridCombatProjectileActor::EvaluateTrajectoryLocation(const FVector& Source, const FVector& Target, float Alpha)
{
	return FMath::Lerp(Source, Target, FMath::Clamp(Alpha, 0.0f, 1.0f));
}
