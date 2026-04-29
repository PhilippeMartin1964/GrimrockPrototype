#include "Runtime/GridTriggerActor.h"

AGridTriggerActor::AGridTriggerActor ()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AGridTriggerActor::InitializeTrigger (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material,
    const FVector& WorldLocation, const FRotator& WorldRotation)
{
    InitializeGridObjectBase (ObjectData, Mesh, Material, WorldLocation, WorldRotation);

    SetActorHiddenInGame (true);

    if (MeshComponent)
    {
        MeshComponent->SetCollisionEnabled (ECollisionEnabled::NoCollision);
    }
}

void AGridTriggerActor::InitializeGridObject (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material,
    const FTransform& WorldTransform)
{
	InitializeTrigger (ObjectData, Mesh, Material, WorldTransform.GetLocation (), WorldTransform.Rotator ());
}