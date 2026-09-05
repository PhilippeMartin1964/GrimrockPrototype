#include "Runtime/GridMechanismActor.h"
#include "Components/StaticMeshComponent.h"

AGridMechanismActor::AGridMechanismActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(false);

	FixedMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FixedMesh"));
	FixedMeshComponent->SetupAttachment(RootComponent);

	MovingMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MovingMesh"));
	MovingMeshComponent->SetupAttachment(RootComponent);
	MovingMeshComponent->SetMobility(EComponentMobility::Movable);

	SecondaryMovingMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MovingMesh1"));
	SecondaryMovingMeshComponent->SetupAttachment(RootComponent);
	SecondaryMovingMeshComponent->SetMobility(EComponentMobility::Movable);
}

void AGridMechanismActor::InitializeMechanismVisuals(
	const FGridLevelObjectData& ObjectData, const UGridObjectArchetypeAsset* Archetype, const FTransform& WorldTransform)
{
	ObjectId = ObjectData.ObjectId;
	ObjectType = ObjectData.Type;
	CellX = ObjectData.CellX;
	CellY = ObjectData.CellY;
	Edge = ObjectData.Edge;

	SetActorTransform(WorldTransform);

	bUsesTargetVisualComposition = false;
	MovingPart0BaseTransform = FTransform::Identity;
	MovingPart1BaseTransform = FTransform::Identity;
	MovingPart0Motion = FGridWorldObjectMotion();
	MovingPart1Motion = FGridWorldObjectMotion();

	if (!Archetype)
	{
		SetFixedMesh(nullptr);
		SetMovingMesh(nullptr);
		SetSecondaryMovingMesh(nullptr);
		return;
	}

	bUsesTargetVisualComposition = Archetype->StaticPart.IsDefined() || !Archetype->MovingParts.IsEmpty();
	if (bUsesTargetVisualComposition)
	{
		SetFixedMesh(Archetype->StaticPart.Mesh.Get());
		if (FixedMeshComponent)
		{
			FixedMeshComponent->SetRelativeTransform(Archetype->StaticPart.LocalTransform);
		}

		MovingPart0BaseTransform = Archetype->MovingParts.Part0.LocalTransform;
		MovingPart0Motion = Archetype->MovingParts.Part0.Motion;
		SetMovingMesh(Archetype->MovingParts.Part0.Mesh.Get());
		if (MovingMeshComponent)
		{
			MovingMeshComponent->SetRelativeTransform(MovingPart0BaseTransform);
		}

		MovingPart1BaseTransform = Archetype->MovingParts.Part1.LocalTransform;
		MovingPart1Motion = Archetype->MovingParts.Part1.Motion;
		SetSecondaryMovingMesh(Archetype->MovingParts.Part1.Mesh.Get());
		if (SecondaryMovingMeshComponent)
		{
			SecondaryMovingMeshComponent->SetRelativeTransform(MovingPart1BaseTransform);
		}
		return;
	}

	// MIG03 legacy asset bridge. Existing .uasset files remain playable until they are resaved with StaticPart/MovingParts.
	SetFixedMesh(Archetype->FixedMesh.Get());
	if (FixedMeshComponent)
	{
		FixedMeshComponent->SetRelativeTransform(FTransform::Identity);
	}
	SetMovingMesh(Archetype->MovingMesh ? Archetype->MovingMesh.Get() : Archetype->PreviewMesh.Get());
	if (MovingMeshComponent)
	{
		MovingMeshComponent->SetRelativeTransform(FTransform::Identity);
	}
	SetSecondaryMovingMesh(nullptr);
	if (SecondaryMovingMeshComponent)
	{
		SecondaryMovingMeshComponent->SetRelativeTransform(FTransform::Identity);
	}
}

void AGridMechanismActor::SetFixedMesh(UStaticMesh* Mesh)
{
	if (!FixedMeshComponent)
	{
		return;
	}

	FixedMeshComponent->SetStaticMesh(Mesh);
	FixedMeshComponent->SetVisibility(Mesh != nullptr);
}

void AGridMechanismActor::SetMovingMesh(UStaticMesh* Mesh)
{
	if (!MovingMeshComponent)
	{
		return;
	}

	MovingMeshComponent->SetStaticMesh(Mesh);
	MovingMeshComponent->SetVisibility(Mesh != nullptr);
}

void AGridMechanismActor::SetSecondaryMovingMesh(UStaticMesh* Mesh)
{
	if (!SecondaryMovingMeshComponent)
	{
		return;
	}

	SecondaryMovingMeshComponent->SetStaticMesh(Mesh);
	SecondaryMovingMeshComponent->SetVisibility(Mesh != nullptr);
	SecondaryMovingMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SecondaryMovingMeshComponent->SetGenerateOverlapEvents(false);
}

void AGridMechanismActor::SetMovingRelativeLocation(const FVector& RelativeLocation)
{
	if (MovingMeshComponent)
	{
		MovingMeshComponent->SetRelativeLocation(MovingPart0BaseTransform.GetLocation() + RelativeLocation);
	}
}

FVector AGridMechanismActor::GetMovingRelativeLocation() const
{
	return MovingMeshComponent ? MovingMeshComponent->GetRelativeLocation() - MovingPart0BaseTransform.GetLocation() : FVector::ZeroVector;
}

void AGridMechanismActor::SetMovingRelativeRotation(const FRotator& RelativeRotation)
{
	if (!MovingMeshComponent)
	{
		return;
	}

	const FQuat Result = MovingPart0BaseTransform.GetRotation() * RelativeRotation.Quaternion();
	MovingMeshComponent->SetRelativeRotation(Result.GetNormalized());
}

FRotator AGridMechanismActor::GetMovingRelativeRotation() const
{
	if (!MovingMeshComponent)
	{
		return FRotator::ZeroRotator;
	}

	const FQuat Relative = MovingPart0BaseTransform.GetRotation().Inverse() * MovingMeshComponent->GetRelativeRotation().Quaternion();
	return Relative.GetNormalized().Rotator();
}

void AGridMechanismActor::ApplyMovingPartMotionAlpha(int32 PartIndex, float Alpha)
{
	if (PartIndex == 0)
	{
		if (MovingMeshComponent)
		{
			MovingMeshComponent->SetRelativeTransform(MovingPart0Motion.Evaluate(MovingPart0BaseTransform, Alpha));
		}
		return;
	}

	if (PartIndex == 1 && SecondaryMovingMeshComponent)
	{
		SecondaryMovingMeshComponent->SetRelativeTransform(MovingPart1Motion.Evaluate(MovingPart1BaseTransform, Alpha));
	}
}

void AGridMechanismActor::ApplyAllMovingPartMotionsAlpha(float Alpha)
{
	ApplyMovingPartMotionAlpha(0, Alpha);
	ApplyMovingPartMotionAlpha(1, Alpha);
}

float AGridMechanismActor::GetTargetMotionDuration() const
{
	float Duration = 0.0f;
	if (MovingMeshComponent && MovingMeshComponent->GetStaticMesh())
	{
		Duration = FMath::Max(Duration, FMath::Max(0.0f, MovingPart0Motion.Duration));
	}
	if (SecondaryMovingMeshComponent && SecondaryMovingMeshComponent->GetStaticMesh())
	{
		Duration = FMath::Max(Duration, FMath::Max(0.0f, MovingPart1Motion.Duration));
	}
	return Duration;
}
