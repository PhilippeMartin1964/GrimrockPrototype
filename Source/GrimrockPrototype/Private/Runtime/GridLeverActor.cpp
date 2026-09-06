#include "Runtime/GridLeverActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Runtime/GridInteractionUtils.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"

AGridLeverActor::AGridLeverActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(false);

	if (FixedMeshComponent)
	{
		FixedMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		FixedMeshComponent->SetGenerateOverlapEvents(false);
	}

	if (MovingMeshComponent)
	{
		MovingMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		MovingMeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		MovingMeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		MovingMeshComponent->SetGenerateOverlapEvents(false);
	}
}

void AGridLeverActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsAnimating)
	{
		UpdateAnimation(DeltaSeconds);
	}
}

void AGridLeverActor::SetLeverState(bool bNewOn)
{
	if (bIsOn == bNewOn && !bIsAnimating)
	{
		return;
	}

	bIsOn = bNewOn;
	AnimElapsed = 0.f;
	AnimStartMotionAlpha = CurrentMotionAlpha;
	AnimTargetMotionAlpha = bIsOn ? 1.0f : 0.0f;
	const float Travel = FMath::Abs(AnimTargetMotionAlpha - AnimStartMotionAlpha);
	if (Travel <= KINDA_SMALL_NUMBER)
	{
		CurrentMotionAlpha = AnimTargetMotionAlpha;
		ApplyMovingPartMotionAlpha(0, CurrentMotionAlpha);
		bIsAnimating = false;
		CurrentToggleDuration = 0.0f;
		SetActorTickEnabled(false);
		return;
	}

	CurrentToggleDuration = FMath::Max(0.01f, ToggleDuration * Travel);
	bIsAnimating = true;
	SetActorTickEnabled(true);
}

void AGridLeverActor::ToggleLever()
{
	SetLeverState(!bIsOn);
}

void AGridLeverActor::UpdateAnimation(float DeltaSeconds)
{
	const float SafeDuration = FMath::Max(0.01f, CurrentToggleDuration);
	AnimElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(AnimElapsed / SafeDuration, 0.f, 1.f);
	CurrentMotionAlpha = FMath::Lerp(AnimStartMotionAlpha, AnimTargetMotionAlpha, Alpha);
	ApplyMovingPartMotionAlpha(0, CurrentMotionAlpha);

	if (Alpha >= 1.f)
	{
		CurrentMotionAlpha = AnimTargetMotionAlpha;
		ApplyMovingPartMotionAlpha(0, CurrentMotionAlpha);
		bIsAnimating = false;
		AnimElapsed = 0.f;
		CurrentToggleDuration = 0.0f;
		SetActorTickEnabled(false);
	}
}

void AGridLeverActor::InitializeGridObject(
	const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform)
{
	(void)Mesh;
	AGridRuntimeObjectActor::InitializeGridObject(ObjectData, nullptr, WorldTransform);

	// WORLDOBJ-MIG04: the lever owns only logical alpha; Motion owns geometry and timing.
	ToggleDuration = GetTargetMotionDuration();
	bIsOn = ObjectData.bInitiallyActive;
	bIsAnimating = false;
	AnimElapsed = 0.f;
	CurrentToggleDuration = 0.0f;
	CurrentMotionAlpha = bIsOn ? 1.0f : 0.0f;
	AnimStartMotionAlpha = CurrentMotionAlpha;
	AnimTargetMotionAlpha = CurrentMotionAlpha;
	ApplyMovingPartMotionAlpha(0, CurrentMotionAlpha);
	SetActorTickEnabled(false);
}

bool AGridLeverActor::CanInteract_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const
{
	if (!InstigatorPawn || !HitComponent)
	{
		return false;
	}

	if (HitComponent != MovingMeshComponent)
	{
		return false;
	}

	const AGrimrockPartyPawn* PartyPawn = GridInteractionUtils::ResolvePartyPawn(InstigatorPawn);
	AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor(InstigatorPawn, this);
	return !bIsAnimating && PartyPawn && RuntimeActor && RuntimeActor->CanPartyInteractWithEdgeObject(CellX, CellY, Edge, PartyPawn);
}

void AGridLeverActor::Interact_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent)
{
	if (!CanInteract_Implementation(InstigatorPawn, HitComponent))
	{
		return;
	}

	AGrimrockPartyPawn* PartyPawn = GridInteractionUtils::ResolvePartyPawn(InstigatorPawn);
	if (!PartyPawn)
	{
		return;
	}

	AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor(InstigatorPawn, this);
	if (RuntimeActor)
	{
		RuntimeActor->TryInteractAtEdge(CellX, CellY, Edge, PartyPawn);
	}
}

EGridInteractionCursor AGridLeverActor::GetInteractionCursor_Implementation(UPrimitiveComponent* HitComponent) const
{
	if (HitComponent == MovingMeshComponent && !bIsAnimating)
	{
		return EGridInteractionCursor::Pull;
	}

	return EGridInteractionCursor::Default;
}

FText AGridLeverActor::GetInteractionText_Implementation(UPrimitiveComponent* HitComponent) const
{
	if (HitComponent == MovingMeshComponent)
	{
		return FText::FromString(bIsOn ? TEXT("Push lever") : TEXT("Pull lever"));
	}

	return FText::GetEmpty();
}
