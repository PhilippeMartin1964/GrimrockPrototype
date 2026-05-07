#include "Runtime/GridReceptacleActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"


AGridReceptacleActor::AGridReceptacleActor ()
{
    PrimaryActorTick.bCanEverTick = false;

    ItemSocketRoot = CreateDefaultSubobject<USceneComponent> (TEXT ("ItemSocketRoot"));
    ItemSocketRoot->SetupAttachment (RootComponent);

    ItemAttachPoint = CreateDefaultSubobject<USceneComponent> (TEXT ("ItemAttachPoint"));
    ItemAttachPoint->SetupAttachment (ItemSocketRoot);

    ContainedItemMesh = CreateDefaultSubobject<UStaticMeshComponent> (TEXT ("ContainedItemMesh"));
    ContainedItemMesh->SetupAttachment (ItemSocketRoot);
    ContainedItemMesh->SetCollisionEnabled (ECollisionEnabled::NoCollision);
    ContainedItemMesh->SetVisibility (false, true);
}

void AGridReceptacleActor::BeginPlay ()
{
    Super::BeginPlay ();

    if (ContainedItemActor)
    {
        AttachContainedItemActor ();
        SetContainedItem (ContainedItemActor->ArchetypeId);
    }
}

void AGridReceptacleActor::InitializeGridObject (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material,
    const FTransform& WorldTransform)
{
    AGridRuntimeObjectActor::InitializeGridObject (ObjectData, Mesh, Material, WorldTransform);

    bAcceptAnyItem = ObjectData.Behavior.bAcceptAnyItem;
    AcceptedItemTags = ObjectData.Behavior.AcceptedItemTags;
    AcceptedArchetypeIds = ObjectData.Behavior.AcceptedArchetypeIds;
    InitialContainedItemArchetypeId = ObjectData.Behavior.InitialContainedItemArchetypeId;
    bStartsFilled = ObjectData.bInitiallyActive;

    if (!ObjectData.Tag.IsNone () && AcceptedItemTags.Num () == 0 && AcceptedArchetypeIds.Num () == 0)
    {
        AcceptedItemTags.Add (ObjectData.Tag);
        bAcceptAnyItem = false;
    }

    if (!bStartsFilled)
    {
        InitialContainedItemArchetypeId = NAME_None;
    }

    SetContainedItem (NAME_None);
}

bool AGridReceptacleActor::HasItem () const
{
    return !ContainedItemArchetypeId.IsNone () || IsValid (ContainedItemActor);
}

bool AGridReceptacleActor::CanAcceptItem (FName ItemId) const
{
    return CanAcceptItemArchetype (ItemId, TArray<FName> ());
}

bool AGridReceptacleActor::CanAcceptItemArchetype (FName ItemArchetypeId, const TArray<FName>& ItemTags) const
{
    if (!bCanInsertItem || HasItem () || ItemArchetypeId.IsNone ())
    {
        return false;
    }

    if (bAcceptAnyItem)
    {
        return true;
    }

    if (AcceptedArchetypeIds.Contains (ItemArchetypeId))
    {
        return true;
    }

    for (const FName& AcceptedTag : AcceptedItemTags)
    {
        if (!AcceptedTag.IsNone () && ItemTags.Contains (AcceptedTag))
        {
            return true;
        }
    }

    return false;
}

bool AGridReceptacleActor::TryInsertItem (FName ItemId)
{
    TArray<FName> ItemTags;
    AGridLevelRuntimeActor* RuntimeActor = Cast<AGridLevelRuntimeActor> (GetOwner ());
    if (RuntimeActor)
    {
        if (const UGridObjectArchetypeAsset* ItemArchetype = RuntimeActor->FindObjectArchetype (ItemId))
        {
            ItemTags = ItemArchetype->ItemTags;
        }
    }

    if (!CanAcceptItemArchetype (ItemId, ItemTags))
    {
        return false;
    }

    if (RuntimeActor)
    {
        if (AGridItemActor* ItemActor = RuntimeActor->SpawnItemActorForArchetype (ItemId, this, ItemAttachPoint))
        {
            return TryInsertItemActor (ItemActor);
        }

        UE_LOG (LogTemp, Warning, TEXT ("Receptacle %s: item actor spawn failed for %s; storing inventory id only."),
            *ObjectId.ToString (), *ItemId.ToString ());
    }

    SetContainedItem (ItemId);
    return true;
}

bool AGridReceptacleActor::TryInsertItemActor (AGridItemActor* ItemActor)
{
    if (!ItemActor || !CanAcceptItemArchetype (ItemActor->ArchetypeId, ItemActor->ItemTags))
    {
        return false;
    }

    ContainedItemActor = ItemActor;
    AttachContainedItemActor ();
    SetContainedItem (ItemActor->ArchetypeId);
    return true;
}

bool AGridReceptacleActor::TryRemoveItem (FName& OutRemovedItemId)
{
    OutRemovedItemId = NAME_None;
    if (!bCanRemoveItem || !HasItem ())
    {
        return false;
    }

    OutRemovedItemId = ContainedItemArchetypeId;
    ClearContainedItemActor ();
    SetContainedItem (NAME_None);
    return true;
}

bool AGridReceptacleActor::TryTakeContainedItem (AGrimrockPartyPawn* PartyPawn, FName& OutRemovedItemId)
{
    OutRemovedItemId = NAME_None;
    if (!PartyPawn)
    {
        return false;
    }

    if (!TryRemoveItem (OutRemovedItemId))
    {
        return false;
    }

    if (!OutRemovedItemId.IsNone ())
    {
        PartyPawn->AddInventoryItem (OutRemovedItemId);
        return true;
    }

    UE_LOG (LogTemp, Warning, TEXT ("Receptacle %s: removed item has no archetype id; inventory handoff TODO."),
        *ObjectId.ToString ());
    return false;
}

void AGridReceptacleActor::ConfigureContainedItemVisual (UStaticMesh* InMesh, UMaterialInterface* InMaterial)
{
    RuntimeContainedItemMesh = InMesh;
    RuntimeContainedItemMaterial = InMaterial;
    if (!ContainedItemMesh)
    {
        return;
    }
    ContainedItemMesh->SetStaticMesh (RuntimeContainedItemMesh);

    if (RuntimeContainedItemMaterial)
    {
        ContainedItemMesh->SetMaterial (0, RuntimeContainedItemMaterial);
    }
    ContainedItemMesh->SetCollisionEnabled (ECollisionEnabled::NoCollision);
    ContainedItemMesh->SetHiddenInGame (!HasItem (), true);
    ContainedItemMesh->SetVisibility (HasItem (), true);
}

void AGridReceptacleActor::SetContainedItem (FName NewItemId)
{
    ContainedItemArchetypeId = NewItemId;
    ContainedItemId = NewItemId;
    const bool bVisible = HasItem ();
    if (!ContainedItemMesh)
    {
        return;
    }
    if (bVisible && RuntimeContainedItemMesh)
    {
        ContainedItemMesh->SetStaticMesh (RuntimeContainedItemMesh);

        if (RuntimeContainedItemMaterial)
        {
            ContainedItemMesh->SetMaterial (0, RuntimeContainedItemMaterial);
        }
    }
    ContainedItemMesh->SetHiddenInGame (!bVisible, true);
    ContainedItemMesh->SetVisibility (bVisible, true);
    ContainedItemMesh->MarkRenderStateDirty ();
}

void AGridReceptacleActor::SetInitialContainedItemActor (AGridItemActor* ItemActor)
{
    ContainedItemActor = ItemActor;
    if (ContainedItemActor)
    {
        ContainedItemTags = ContainedItemActor->ItemTags;
        SetContainedItem (ContainedItemActor->ArchetypeId);
        AttachContainedItemActor ();
    }
}

void AGridReceptacleActor::AttachContainedItemActor ()
{
    if (!ContainedItemActor)
    {
        return;
    }

    USceneComponent* AttachTarget = ItemAttachPoint ? ItemAttachPoint.Get () : ItemSocketRoot.Get ();
    ContainedItemActor->AttachToComponent (AttachTarget, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    ContainedItemActor->SetActorRelativeTransform (FTransform::Identity);
    ContainedItemTags = ContainedItemActor->ItemTags;
    ContainedItemActor->OnPlacedInWorld ();

    if (ContainedItemMesh)
    {
        ContainedItemMesh->SetHiddenInGame (true, true);
        ContainedItemMesh->SetVisibility (false, true);
    }
}

void AGridReceptacleActor::ClearContainedItemActor ()
{
    if (!ContainedItemActor)
    {
        ContainedItemTags.Reset ();
        return;
    }

    ContainedItemActor->OnRemovedFromWorld ();
    ContainedItemActor->Destroy ();
    ContainedItemActor = nullptr;
    ContainedItemTags.Reset ();
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
