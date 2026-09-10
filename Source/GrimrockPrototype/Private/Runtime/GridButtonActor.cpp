#include "Runtime/GridButtonActor.h"

#include "Components/StaticMeshComponent.h"
#include "Runtime/GridInteractionUtils.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"

AGridButtonActor::AGridButtonActor()
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

void AGridButtonActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateAnimation(DeltaSeconds);
}

void AGridButtonActor::TriggerPress()
{
	AnimState = EButtonAnimState::Pressing;
	StateElapsed = 0.f;
	SetActorTickEnabled(true);
}

void AGridButtonActor::UpdateAnimation(float DeltaSeconds)
{
	switch (AnimState)
	{
		case EButtonAnimState::Idle:
			return;

		case EButtonAnimState::Pressing:
		{
			StateElapsed += DeltaSeconds;
			const float Alpha = FMath::Clamp(StateElapsed / FMath::Max(0.01f, PressDuration), 0.f, 1.f);
			ApplyMovingPartMotionAlpha(0, Alpha);

			if (Alpha >= 1.f)
			{
				AnimState = EButtonAnimState::Holding;
				StateElapsed = 0.f;
				ApplyMovingPartMotionAlpha(0, 1.0f);
			}
			break;
		}

		case EButtonAnimState::Holding:
		{
			StateElapsed += DeltaSeconds;
			if (StateElapsed >= HoldTime)
			{
				AnimState = EButtonAnimState::Releasing;
				StateElapsed = 0.f;
			}
			break;
		}

		case EButtonAnimState::Releasing:
		{
			StateElapsed += DeltaSeconds;
			const float Alpha = FMath::Clamp(StateElapsed / FMath::Max(0.01f, ReleaseDuration), 0.f, 1.f);
			ApplyMovingPartMotionAlpha(0, 1.0f - Alpha);

			if (Alpha >= 1.f)
			{
				AnimState = EButtonAnimState::Idle;
				StateElapsed = 0.f;
				ApplyMovingPartMotionAlpha(0, 0.0f);
				SetActorTickEnabled(false);
			}
			break;
		}

		default:
			break;
	}
}

void AGridButtonActor::InitializeRuntimeWorldObject(
	const FGridRuntimeWorldObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform)
{
	(void)Mesh;
	AGridRuntimeObjectActor::InitializeRuntimeWorldObject(ObjectData, nullptr, WorldTransform);

	// RECOVERY01-C2: generic Motion owns both forward and optional reverse timing.
	PressDuration = GetTargetMotionDuration(false);
	ReleaseDuration = GetTargetMotionDuration(true);

	// WORLDOBJ-MIG06/MIG09-E2: shared gameplay comes from Definition + instance-owned runtime overrides.
	const FGridObjectBehaviorParams EffectiveBehavior = ResolveEffectiveBehavior(ObjectData);
	HoldTime = FMath::Max(0.0f, EffectiveBehavior.ButtonAnimation.ButtonHoldTime);

	ApplyMovingPartMotionAlpha(0, 0.0f);
	AnimState = EButtonAnimState::Idle;
	StateElapsed = 0.f;
	SetActorTickEnabled(false);
}

bool AGridButtonActor::CanInteract_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const
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
	return AnimState == EButtonAnimState::Idle && PartyPawn && RuntimeActor && RuntimeActor->CanPartyInteractWithEdgeObject(CellX, CellY, Edge, PartyPawn);
}

void AGridButtonActor::Interact_Implementation(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent)
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

EGridInteractionCursor AGridButtonActor::GetInteractionCursor_Implementation(UPrimitiveComponent* HitComponent) const
{
	if (HitComponent == MovingMeshComponent && AnimState == EButtonAnimState::Idle)
	{
		return EGridInteractionCursor::Push;
	}

	return EGridInteractionCursor::Default;
}

FText AGridButtonActor::GetInteractionText_Implementation(UPrimitiveComponent* HitComponent) const
{
	if (HitComponent == MovingMeshComponent)
	{
		return FText::FromString(TEXT("Push button"));
	}

	return FText::GetEmpty();
}
