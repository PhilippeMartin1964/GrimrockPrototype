#include "Runtime/GridItemActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Runtime/GridInteractionUtils.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLightEmitterComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridReceptacleActor.h"

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
    SetItemLightsEnabled (true);
}

void AGridItemActor::OnRemovedFromWorld ()
{
    SetItemLightsEnabled (false);
}

void AGridItemActor::SetItemLightsEnabled (bool bEnabled)
{
    TArray<UGridLightEmitterComponent*> LightEmitters;
    GetComponents<UGridLightEmitterComponent> (LightEmitters);

    for (UGridLightEmitterComponent* LightEmitter : LightEmitters)
    {
        if (LightEmitter)
        {
            LightEmitter->SetLightEnabled (bEnabled);
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
    MeshComponent->SetCollisionResponseToChannel (ECC_Visibility, ECR_Block);
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

void AGridItemActor::SetRuntimeCell (int32 InCellX, int32 InCellY)
{
    RuntimeCellX = InCellX;
    RuntimeCellY = InCellY;
}

bool AGridItemActor::CanInteract_Implementation (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const
{
    if (!InstigatorPawn || !HitComponent || HitComponent != MeshComponent || ArchetypeId.IsNone ())
    {
        return false;
    }
    if (const AGridReceptacleActor* OwnerReceptacle = Cast<AGridReceptacleActor> (GetOwner ()))
    {
        return OwnerReceptacle->CanInteract_Implementation (const_cast<APawn*>(InstigatorPawn), MeshComponent);
    }
    return true;
}

void AGridItemActor::Interact_Implementation (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent)
{
    if (!CanInteract_Implementation (InstigatorPawn, HitComponent))
    {
        return;
    }

    AGrimrockPartyPawn* PartyPawn = GridInteractionUtils::ResolvePartyPawn (InstigatorPawn);
    if (!PartyPawn)
    {
        return;
    }

    if (AGridReceptacleActor* OwnerReceptacle = Cast<AGridReceptacleActor> (GetOwner ()))
    {
        OwnerReceptacle->Interact_Implementation (PartyPawn, MeshComponent);
        return;
    }

    AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor (InstigatorPawn, this);
    if (RuntimeActor)
    {
        RuntimeActor->TryPickupItemActor (this, PartyPawn);
    }
}

EGridInteractionCursor AGridItemActor::GetInteractionCursor_Implementation (UPrimitiveComponent* HitComponent) const
{
    if (HitComponent != MeshComponent || ArchetypeId.IsNone ())
    {
        return EGridInteractionCursor::Default;
    }
    if (const AGridReceptacleActor* OwnerReceptacle = Cast<AGridReceptacleActor> (GetOwner ()))
    {
        return OwnerReceptacle->GetInteractionCursor_Implementation (MeshComponent);
    }
    return EGridInteractionCursor::Take;
}

FText AGridItemActor::GetInteractionText_Implementation (UPrimitiveComponent* HitComponent) const
{
    if (HitComponent != MeshComponent || ArchetypeId.IsNone ())
    {
        return FText::GetEmpty ();
    }
    if (const AGridReceptacleActor* OwnerReceptacle = Cast<AGridReceptacleActor> (GetOwner ()))
    {
        return OwnerReceptacle->GetInteractionText_Implementation (MeshComponent);
    }
    return FText::FromString (TEXT ("Take"));
}
