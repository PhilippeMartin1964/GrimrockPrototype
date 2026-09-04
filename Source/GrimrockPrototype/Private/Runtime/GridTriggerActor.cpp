#include "Runtime/GridTriggerActor.h"

AGridTriggerActor::AGridTriggerActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AGridTriggerActor::InitializeGridObject(
	const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, const FTransform& WorldTransform)
{
	InitializeGridObjectBase(ObjectData, Mesh, WorldTransform.GetLocation(), WorldTransform.GetRotation().Rotator());
	SetActorHiddenInGame(true);
	if (MeshComponent)
	{
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
