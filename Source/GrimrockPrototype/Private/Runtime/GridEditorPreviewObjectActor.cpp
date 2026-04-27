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

    bIsHovered = false;
    bIsSelected = false;
    RefreshStencilState ();
}

void AGridEditorPreviewObjectActor::SetHovered (bool bHovered)
{
    bIsHovered = bHovered;
    RefreshStencilState ();
}

void AGridEditorPreviewObjectActor::SetSelected (bool bSelected)
{
    bIsSelected = bSelected;
    RefreshStencilState ();
}

void AGridEditorPreviewObjectActor::RefreshStencilState ()
{
    if (!MeshComponent)
    {
        return;
    }

    int32 StencilValue = 0;

    if (bIsSelected)
    {
        StencilValue = 2;
    } else if (bIsHovered)
    {
        StencilValue = 1;
    }

    MeshComponent->SetRenderCustomDepth (StencilValue != 0);
    MeshComponent->SetCustomDepthStencilValue (StencilValue);
    MeshComponent->MarkRenderStateDirty ();
}