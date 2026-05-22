#include "Runtime/GridItemActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Runtime/GridLightEmitterComponent.h"

AGridItemActor::AGridItemActor ()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent> (TEXT ("Root"));
    SetRootComponent (SceneRoot);

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent> (TEXT ("Mesh"));
    MeshComponent->SetupAttachment (SceneRoot);
    MeshComponent->SetCollisionEnabled (ECollisionEnabled::NoCollision);
}

void AGridItemActor::InitializeItem (FName InArchetypeId, const TArray<FName>& InItemTags, UStaticMesh* Mesh, UMaterialInterface* Material)
{
    ArchetypeId = InArchetypeId;
    ItemTags = InItemTags;

    if (MeshComponent)
    {
        MeshComponent->SetStaticMesh (Mesh);
        if (Material)
        {
            MeshComponent->SetMaterial (0, Material);
        }
    }
}

void AGridItemActor::OnPlacedInWorld ()
{
    TArray<UGridLightEmitterComponent*> LightEmitters;
    GetComponents<UGridLightEmitterComponent> (LightEmitters);

    for (UGridLightEmitterComponent* LightEmitter : LightEmitters)
    {
        if (LightEmitter)
        {
            LightEmitter->SetLightEnabled (true);
        }
    }
}

void AGridItemActor::OnRemovedFromWorld ()
{
    TArray<UGridLightEmitterComponent*> LightEmitters;
    GetComponents<UGridLightEmitterComponent> (LightEmitters);

    for (UGridLightEmitterComponent* LightEmitter : LightEmitters)
    {
        if (LightEmitter)
        {
            LightEmitter->SetLightEnabled (false);
        }
    }
}

void AGridItemActor::ConfigureAsWorldPickup ()
{
    if (!MeshComponent)
    {
        return;
    }

    MeshComponent->SetCollisionEnabled (ECollisionEnabled::QueryAndPhysics);
    MeshComponent->SetCollisionProfileName (TEXT ("PhysicsActor"));
    MeshComponent->SetEnableGravity (true);
    MeshComponent->SetSimulatePhysics (true);
    MeshComponent->WakeRigidBody ();
}

void AGridItemActor::ConfigureAsAttachedItem ()
{
    if (!MeshComponent)
    {
        return;
    }

    MeshComponent->SetSimulatePhysics (false);
    MeshComponent->SetEnableGravity (false);
    MeshComponent->SetCollisionEnabled (ECollisionEnabled::NoCollision);
}

FName AGridItemActor::GetItemArchetypeId () const
{
    return ArchetypeId;
}

bool AGridItemActor::HasItemTag (FName Tag) const
{
    return !Tag.IsNone () && ItemTags.Contains (Tag);
}
