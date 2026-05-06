#include "Runtime/GridReceptacleActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"


AGridReceptacleActor::AGridReceptacleActor ()
{
    PrimaryActorTick.bCanEverTick = false;

    ItemSocketRoot = CreateDefaultSubobject<USceneComponent> (TEXT ("ItemSocketRoot"));
    ItemSocketRoot->SetupAttachment (RootComponent);

    ContainedItemMesh = CreateDefaultSubobject<UStaticMeshComponent> (TEXT ("ContainedItemMesh"));
    ContainedItemMesh->SetupAttachment (ItemSocketRoot);
    ContainedItemMesh->SetCollisionEnabled (ECollisionEnabled::NoCollision);
    ContainedItemMesh->SetVisibility (false, true);
}

void AGridReceptacleActor::BeginPlay ()
{
    Super::BeginPlay ();

    if (bStartsFilled)
    {
        SetContainedItem (AcceptedItemId.IsNone () ? FName (TEXT ("Torch")) : AcceptedItemId);
    } else
    {
        SetContainedItem (NAME_None);
    }
}

void AGridReceptacleActor::InitializeGridObject (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material,
    const FTransform& WorldTransform)
{
    AGridRuntimeObjectActor::InitializeGridObject (ObjectData, Mesh, Material, WorldTransform);

    // First data-driven convention: Tag defines the accepted item.
    // Example: Tag = Torch for a torch holder.
    AcceptedItemId = ObjectData.Tag;
    bStartsFilled = ObjectData.bInitiallyActive;

    if (bStartsFilled)
    {
        SetContainedItem (AcceptedItemId.IsNone () ? FName (TEXT ("Torch")) : AcceptedItemId);
    } else
    {
        SetContainedItem (NAME_None);
    }
}

bool AGridReceptacleActor::HasItem () const
{
    return !ContainedItemId.IsNone ();
}

bool AGridReceptacleActor::CanAcceptItem (FName ItemId) const
{
    if (!bCanInsertItem || HasItem () || ItemId.IsNone ())
    {
        return false;
    }

    // Empty AcceptedItemId = accepts any item.
    return AcceptedItemId.IsNone () || AcceptedItemId == ItemId;
}

bool AGridReceptacleActor::TryInsertItem (FName ItemId)
{
    if (!CanAcceptItem (ItemId))
    {
        return false;
    }

    SetContainedItem (ItemId);
    ExecuteInsertionLinks ();
    return true;
}

bool AGridReceptacleActor::TryRemoveItem (FName& OutRemovedItemId)
{
    OutRemovedItemId = NAME_None;

    if (!bCanRemoveItem || !HasItem ())
    {
        return false;
    }

    OutRemovedItemId = ContainedItemId;
    SetContainedItem (NAME_None);
    ExecuteRemovalLinks ();
    return true;
}

bool AGridReceptacleActor::TryToggleTorchForParty (AGrimrockPartyPawn* PartyPawn)
{
    if (!PartyPawn)
    {
        return false;
    }

    static const FName TorchItemId (TEXT ("Torch"));

    if (HasItem ())
    {
        FName RemovedItemId = NAME_None;
        if (!TryRemoveItem (RemovedItemId))
        {
            return false;
        }

        return PartyPawn->AddInventoryItem (RemovedItemId);
    }

    if (!CanAcceptItem (TorchItemId) || !PartyPawn->HasInventoryItem (TorchItemId))
    {
        return false;
    }

    if (!PartyPawn->RemoveInventoryItem (TorchItemId))
    {
        return false;
    }

    if (!TryInsertItem (TorchItemId))
    {
        // Rollback to avoid losing the item if insertion fails unexpectedly.
        PartyPawn->AddInventoryItem (TorchItemId);
        return false;
    }

    return true;
}

void AGridReceptacleActor::SetContainedItem (FName NewItemId)
{
    ContainedItemId = NewItemId;

    const bool bVisible = HasItem ();
    if (ContainedItemMesh)
    {
        ContainedItemMesh->SetVisibility (bVisible, true);
    }
}

void AGridReceptacleActor::ExecuteInsertionLinks ()
{
    if (AGridLevelRuntimeActor* RuntimeActor = Cast<AGridLevelRuntimeActor> (GetOwner ()))
    {
        RuntimeActor->ExecuteLinksFromRuntimeObject (ObjectId, false);
    }
}

void AGridReceptacleActor::ExecuteRemovalLinks ()
{
    if (AGridLevelRuntimeActor* RuntimeActor = Cast<AGridLevelRuntimeActor> (GetOwner ()))
    {
        RuntimeActor->ExecuteLinksFromRuntimeObject (ObjectId, true);
    }
}
