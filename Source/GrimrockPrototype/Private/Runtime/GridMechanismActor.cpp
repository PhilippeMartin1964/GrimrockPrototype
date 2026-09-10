#include "Runtime/GridMechanismActor.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridWorldObjectInstanceVisual.h"

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


void AGridMechanismActor::InitializeRuntimeMechanismVisuals(
	const FGridRuntimeWorldObjectData& ObjectData, const UGridWorldObjectDefinitionAsset* Definition, const FTransform& WorldTransform)
{
	ObjectId = ObjectData.ObjectId;
	ObjectType = ObjectData.Type;
	CellX = ObjectData.CellX;
	CellY = ObjectData.CellY;
	Edge = ObjectData.Edge;

	SetActorTransform(WorldTransform);

	MovingPart0BaseTransform = FTransform::Identity;
	MovingPart1BaseTransform = FTransform::Identity;
	MovingPart0Motion = FGridWorldObjectMotion();
	MovingPart1Motion = FGridWorldObjectMotion();

	if (!Definition)
	{
		SetFixedMesh(nullptr);
		SetMovingMesh(nullptr);
		SetSecondaryMovingMesh(nullptr);
		return;
	}

	// WORLDOBJ-MIG03.4: StaticPart/MovingParts are the only mechanism presentation contract.
	SetFixedMesh(Definition->StaticPart.Mesh.Get());
	if (FixedMeshComponent)
	{
		FixedMeshComponent->SetRelativeTransform(Definition->StaticPart.LocalTransform);
	}

	const FGridWorldObjectMovingPart ResolvedPart0 = GridWorldObjectInstanceVisual::ResolveMovingPart(
		Definition->MovingParts.Part0, ObjectData.MovingPartOverrides, 0);
	const FGridWorldObjectMovingPart ResolvedPart1 = GridWorldObjectInstanceVisual::ResolveMovingPart(
		Definition->MovingParts.Part1, ObjectData.MovingPartOverrides, 1);

	MovingPart0BaseTransform = ResolvedPart0.LocalTransform;
	MovingPart0Motion = ResolvedPart0.Motion;
	SetMovingMesh(ResolvedPart0.Mesh.Get());
	if (MovingMeshComponent)
	{
		MovingMeshComponent->SetRelativeTransform(MovingPart0BaseTransform);
	}

	MovingPart1BaseTransform = ResolvedPart1.LocalTransform;
	MovingPart1Motion = ResolvedPart1.Motion;
	SetSecondaryMovingMesh(ResolvedPart1.Mesh.Get());
	if (SecondaryMovingMeshComponent)
	{
		SecondaryMovingMeshComponent->SetRelativeTransform(MovingPart1BaseTransform);
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

float AGridMechanismActor::GetTargetMotionDuration(bool bReverse) const
{
	float Duration = 0.0f;
	if (MovingMeshComponent && MovingMeshComponent->GetStaticMesh())
	{
		Duration = FMath::Max(Duration, FMath::Max(0.0f, MovingPart0Motion.GetDuration(bReverse)));
	}
	if (SecondaryMovingMeshComponent && SecondaryMovingMeshComponent->GetStaticMesh())
	{
		Duration = FMath::Max(Duration, FMath::Max(0.0f, MovingPart1Motion.GetDuration(bReverse)));
	}
	return Duration;
}
