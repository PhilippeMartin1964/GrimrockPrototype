#include "Runtime/GridEditorPreviewObjectActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AGridEditorPreviewObjectActor::AGridEditorPreviewObjectActor ()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent> (TEXT ("Root"));
    SetRootComponent (SceneRoot);

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent> (TEXT ("Mesh"));
    MeshComponent->SetupAttachment (SceneRoot);
    MeshComponent->SetCollisionEnabled (ECollisionEnabled::NoCollision);
    MeshComponent->SetMobility (EComponentMobility::Movable);
}

void AGridEditorPreviewObjectActor::InitializePreviewObject (
    const FGridLevelObjectData& ObjectData,
    UStaticMesh* Mesh,
    UMaterialInterface* NormalMaterial,
    UMaterialInterface* HighlightMaterial)
{
    ObjectId = ObjectData.ObjectId;
    ObjectType = ObjectData.Type;

    CachedNormalMaterial = NormalMaterial;
    CachedHighlightMaterial = HighlightMaterial;

    if (!MeshComponent)
    {
        return;
    }

    MeshComponent->SetStaticMesh (Mesh);

    if (CachedNormalMaterial)
    {
        MeshComponent->SetMaterial (0, CachedNormalMaterial);
    }

    MeshComponent->SetRenderCustomDepth (false);
    MeshComponent->SetCustomDepthStencilValue (0);
    MeshComponent->MarkRenderStateDirty ();
}

void AGridEditorPreviewObjectActor::SetHighlighted (bool bHighlighted)
{
    if (!MeshComponent)
    {
        return;
    }

    MeshComponent->SetRenderCustomDepth (bHighlighted);
    MeshComponent->SetCustomDepthStencilValue (bHighlighted ? 1 : 0);
    MeshComponent->MarkRenderStateDirty ();
}