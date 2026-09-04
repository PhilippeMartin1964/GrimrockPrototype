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
}

void AGridMechanismActor::InitializeMechanismVisuals(
	const FGridLevelObjectData& ObjectData, const UGridObjectArchetypeAsset* Archetype, const FTransform& WorldTransform)
{
	ObjectId = ObjectData.ObjectId;
	CellX = ObjectData.CellX;
	CellY = ObjectData.CellY;
	Edge = ObjectData.Edge;

	SetActorTransform(WorldTransform);

	if (!Archetype)
	{
		return;
	}

	SetFixedMesh(Archetype->FixedMesh.Get());
	SetMovingMesh(Archetype->MovingMesh ? Archetype->MovingMesh.Get() : Archetype->PreviewMesh.Get());
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
	if (MovingMeshComponent)
	{
		MovingMeshComponent->SetRelativeLocation(RelativeLocation);
	}
}

FVector AGridMechanismActor::GetMovingRelativeLocation() const
{
	return MovingMeshComponent ? MovingMeshComponent->GetRelativeLocation() : FVector::ZeroVector;
}
