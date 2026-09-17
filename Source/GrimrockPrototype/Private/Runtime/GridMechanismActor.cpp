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

	for (int32 Index = MovingPartMeshComponents.Num() - 1; Index >= 1; --Index)
	{
		if (UStaticMeshComponent* Component = MovingPartMeshComponents[Index])
		{
			Component->DestroyComponent();
		}
	}
	MovingPartMeshComponents.Reset();
	MovingPartBaseTransforms.Reset();
	MovingPartMotions.Reset();

	if (!Definition)
	{
		SetFixedMesh(nullptr);
		SetMovingMesh(nullptr);
		return;
	}

	// WORLDOBJ-MIG03.4: StaticPart/MovingParts are the only mechanism presentation contract.
	SetFixedMesh(Definition->StaticPart.Mesh.Get());
	if (FixedMeshComponent)
	{
		FixedMeshComponent->SetRelativeTransform(Definition->StaticPart.LocalTransform);
	}

	MovingPartMeshComponents.Reserve(Definition->MovingParts.Num());
	MovingPartBaseTransforms.Reserve(Definition->MovingParts.Num());
	MovingPartMotions.Reserve(Definition->MovingParts.Num());
	for (int32 PartIndex = 0; PartIndex < Definition->MovingParts.Num(); ++PartIndex)
	{
		const FGridWorldObjectMovingPart ResolvedPart = GridWorldObjectInstanceVisual::ResolveMovingPart(
			Definition->MovingParts[PartIndex], ObjectData.MovingPartOverrides, PartIndex);
		UStaticMeshComponent* Component = MovingMeshComponent;
		if (PartIndex > 0)
		{
			Component = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("MovingPart%d"), PartIndex));
			Component->SetupAttachment(RootComponent);
			Component->SetMobility(EComponentMobility::Movable);
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Component->SetGenerateOverlapEvents(false);
			AddInstanceComponent(Component);
			Component->RegisterComponent();
		}

		MovingPartMeshComponents.Add(Component);
		MovingPartBaseTransforms.Add(ResolvedPart.LocalTransform);
		MovingPartMotions.Add(ResolvedPart.Motion);
		Component->SetStaticMesh(ResolvedPart.Mesh.Get());
		Component->SetRelativeTransform(ResolvedPart.LocalTransform);
		Component->SetVisibility(ResolvedPart.Mesh != nullptr);
	}
	if (Definition->MovingParts.IsEmpty())
	{
		SetMovingMesh(nullptr);
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

void AGridMechanismActor::SetMovingRelativeLocation(const FVector& RelativeLocation)
{
	if (MovingMeshComponent && MovingPartBaseTransforms.IsValidIndex(0))
	{
		MovingMeshComponent->SetRelativeLocation(MovingPartBaseTransforms[0].GetLocation() + RelativeLocation);
	}
}

FVector AGridMechanismActor::GetMovingRelativeLocation() const
{
	return MovingMeshComponent && MovingPartBaseTransforms.IsValidIndex(0)
		? MovingMeshComponent->GetRelativeLocation() - MovingPartBaseTransforms[0].GetLocation()
		: FVector::ZeroVector;
}

void AGridMechanismActor::SetMovingRelativeRotation(const FRotator& RelativeRotation)
{
	if (!MovingMeshComponent || !MovingPartBaseTransforms.IsValidIndex(0))
	{
		return;
	}

	const FQuat Result = MovingPartBaseTransforms[0].GetRotation() * RelativeRotation.Quaternion();
	MovingMeshComponent->SetRelativeRotation(Result.GetNormalized());
}

FRotator AGridMechanismActor::GetMovingRelativeRotation() const
{
	if (!MovingMeshComponent || !MovingPartBaseTransforms.IsValidIndex(0))
	{
		return FRotator::ZeroRotator;
	}

	const FQuat Relative = MovingPartBaseTransforms[0].GetRotation().Inverse() * MovingMeshComponent->GetRelativeRotation().Quaternion();
	return Relative.GetNormalized().Rotator();
}

void AGridMechanismActor::ApplyMovingPartMotionAlpha(int32 PartIndex, float Alpha)
{
	if (MovingPartMeshComponents.IsValidIndex(PartIndex) && MovingPartBaseTransforms.IsValidIndex(PartIndex) &&
		MovingPartMotions.IsValidIndex(PartIndex) && MovingPartMeshComponents[PartIndex])
	{
		MovingPartMeshComponents[PartIndex]->SetRelativeTransform(
			MovingPartMotions[PartIndex].Evaluate(MovingPartBaseTransforms[PartIndex], Alpha));
	}
}

void AGridMechanismActor::ApplyAllMovingPartMotionsAlpha(float Alpha)
{
	for (int32 PartIndex = 0; PartIndex < MovingPartMeshComponents.Num(); ++PartIndex)
	{
		ApplyMovingPartMotionAlpha(PartIndex, Alpha);
	}
}

float AGridMechanismActor::GetTargetMotionDuration(bool bReverse) const
{
	float Duration = 0.0f;
	for (int32 PartIndex = 0; PartIndex < MovingPartMeshComponents.Num(); ++PartIndex)
	{
		if (MovingPartMeshComponents[PartIndex] && MovingPartMeshComponents[PartIndex]->GetStaticMesh() && MovingPartMotions.IsValidIndex(PartIndex))
		{
			Duration = FMath::Max(Duration, FMath::Max(0.0f, MovingPartMotions[PartIndex].GetDuration(bReverse)));
		}
	}
	return Duration;
}

const FGridWorldObjectMotion& AGridMechanismActor::GetMovingPartMotion(int32 PartIndex) const
{
	static const FGridWorldObjectMotion EmptyMotion;
	return MovingPartMotions.IsValidIndex(PartIndex) ? MovingPartMotions[PartIndex] : EmptyMotion;
}

UStaticMeshComponent* AGridMechanismActor::GetMovingPartComponent(int32 PartIndex) const
{
	return MovingPartMeshComponents.IsValidIndex(PartIndex) ? MovingPartMeshComponents[PartIndex].Get() : nullptr;
}
