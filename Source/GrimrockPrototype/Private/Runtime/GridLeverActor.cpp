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

void AGridLeverActor::InitializeLever(const FGridLevelObjectData& ObjectData, UStaticMesh* InLeverMesh,
	const FVector& InWorldLocation, const FRotator& InWorldRotation, bool bStartOn)
{
	AGridRuntimeObjectActor::InitializeGridObject(ObjectData, nullptr, FTransform(InWorldRotation, InWorldLocation));

	LeverOffPitch = ObjectData.Behavior.LeverAnimation.LeverOffPitch;
	LeverOnPitch = ObjectData.Behavior.LeverAnimation.LeverOnPitch;
	ToggleDuration = ObjectData.Behavior.LeverAnimation.ToggleDuration;

	OffRelativeRotation = FRotator(LeverOffPitch, 0.f, 0.f);
	OnRelativeRotation = FRotator(LeverOnPitch, 0.f, 0.f);

	bIsOn = bStartOn;
	bIsAnimating = false;
	AnimElapsed = 0.f;

	SetMovingRelativeRotation(bIsOn ? OnRelativeRotation : OffRelativeRotation);
}

void AGridLeverActor::SetLeverState(bool bNewOn)
{
	if (bIsOn == bNewOn && !bIsAnimating)
	{
		return;
	}

	bIsOn = bNewOn;
	AnimElapsed = 0.f;
	bIsAnimating = true;

	AnimStartRotation = GetMovingRelativeRotation();
	AnimTargetRotation = bIsOn ? OnRelativeRotation : OffRelativeRotation;
	SetActorTickEnabled(true);
}

void AGridLeverActor::ToggleLever()
{
	SetLeverState(!bIsOn);
}

void AGridLeverActor::UpdateAnimation(float DeltaSeconds)
{
	const float SafeDuration = FMath::Max(0.01f, ToggleDuration);

	AnimElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(AnimElapsed / SafeDuration, 0.f, 1.f);

	SetMovingRelativeRotation(FMath::Lerp(AnimStartRotation, AnimTargetRotation, Alpha));

	if (Alpha >= 1.f)
	{
		SetMovingRelativeRotation(AnimTargetRotation);
		bIsAnimating = false;
		AnimElapsed = 0.f;
		SetActorTickEnabled(false);
	}
}

void AGridLeverActor::InitializeGridObject(
	const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform)
{
	InitializeLever(ObjectData, Mesh, WorldTransform.GetLocation(), WorldTransform.Rotator(), ObjectData.bInitiallyActive);
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
