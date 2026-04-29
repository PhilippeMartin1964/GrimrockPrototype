#include "Runtime/GridRuntimeObjectActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AGridRuntimeObjectActor::AGridRuntimeObjectActor ()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent> (TEXT ("Root"));
    SetRootComponent (SceneRoot);

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent> (TEXT ("Mesh"));
    MeshComponent->SetupAttachment (SceneRoot);
    MeshComponent->SetMobility (EComponentMobility::Movable);
    MeshComponent->SetCollisionEnabled (ECollisionEnabled::NoCollision);
}

void AGridRuntimeObjectActor::InitializeGridObjectBase (
    const FGridLevelObjectData& ObjectData,
    UStaticMesh* Mesh,
    UMaterialInterface* Material,
    const FVector& WorldLocation,
    const FRotator& WorldRotation)
{
    ObjectId = ObjectData.ObjectId;
    ObjectType = ObjectData.Type;
    CellX = ObjectData.CellX;
    CellY = ObjectData.CellY;
    Edge = ObjectData.Edge;

    if (MeshComponent)
    {
        MeshComponent->SetStaticMesh (Mesh);

        if (Material)
        {
            MeshComponent->SetMaterial (0, Material);
        }
    }

    SetActorLocation (WorldLocation);
    SetActorRotation (WorldRotation);
}

bool AGridRuntimeObjectActor::MatchesObjectId (FGuid InObjectId) const
{
    return ObjectId.IsValid () && ObjectId == InObjectId;
}

bool AGridRuntimeObjectActor::MatchesCell (int32 InCellX, int32 InCellY) const
{
    return CellX == InCellX && CellY == InCellY;
}

bool AGridRuntimeObjectActor::MatchesEdge (
    int32 InCellX,
    int32 InCellY,
    EGridEdge InEdge) const
{
    return CellX == InCellX && CellY == InCellY && Edge == InEdge;
}

void AGridRuntimeObjectActor::InitializeGridObject (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh,
    UMaterialInterface* Material, const FTransform& WorldTransform)
{
    InitializeGridObjectBase (ObjectData, Mesh, Material, WorldTransform.GetLocation (), WorldTransform.GetRotation ().Rotator ());
}