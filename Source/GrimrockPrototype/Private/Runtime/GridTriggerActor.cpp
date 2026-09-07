#include "Runtime/GridTriggerActor.h"

AGridTriggerActor::AGridTriggerActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AGridTriggerActor::InitializeRuntimeWorldObject(
	const FGridRuntimeWorldObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform)
{
	InitializeRuntimeWorldObjectBase(ObjectData, Mesh, WorldTransform.GetLocation(), WorldTransform.GetRotation().Rotator());
	SetActorHiddenInGame(true);
	if (MeshComponent)
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
