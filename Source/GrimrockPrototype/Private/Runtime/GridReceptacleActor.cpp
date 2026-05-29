#include "Runtime/GridReceptacleActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Runtime/GridInteractionUtils.h"
#include "Runtime/GridInteractableInterface.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "EngineUtils.h"


AGridReceptacleActor::AGridReceptacleActor ()
{
    PrimaryActorTick.bCanEverTick = false;

    if (MeshComponent)
    {
        MeshComponent->SetCollisionEnabled (ECollisionEnabled::QueryOnly);
        MeshComponent->SetCollisionResponseToAllChannels (ECR_Ignore);
        MeshComponent->SetCollisionResponseToChannel (ECC_Visibility, ECR_Block);
        MeshComponent->SetGenerateOverlapEvents (false);
    }

    ItemSocketRoot = CreateDefaultSubobject<USceneComponent> (TEXT ("ItemSocketRoot"));
    ItemSocketRoot->SetupAttachment (RootComponent);

    ItemAttachPoint = CreateDefaultSubobject<USceneComponent> (TEXT ("ItemAttachPoint"));
    ItemAttachPoint->SetupAttachment (ItemSocketRoot);

    ContainedItemMesh = CreateDefaultSubobject<UStaticMeshComponent> (TEXT ("ContainedItemMesh"));
    ContainedItemMesh->SetupAttachment (ItemSocketRoot);
    ContainedItemMesh->SetCollisionEnabled (ECollisionEnabled::NoCollision);
    ContainedItemMesh->SetCollisionResponseToAllChannels (ECR_Ignore);
    ContainedItemMesh->SetCollisionResponseToChannel (ECC_Visibility, ECR_Block);
    ContainedItemMesh->SetGenerateOverlapEvents (false);
    ContainedItemMesh->SetVisibility (false, true);
}

void AGridReceptacleActor::BeginPlay ()
{
    Super::BeginPlay ();

    if (ContainedItemActor)
    {
        ContainedItemActors.AddUnique (ContainedItemActor);
        AttachContainedItemActor ();
        SetContainedItem (ContainedItemActor->ArchetypeId);
    }
}

bool AGridReceptacleActor::CanInteract_Implementation (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const
{
    if (!InstigatorPawn || !HitComponent)
    {
        return false;
    }

    if (IsContainedItemHitComponent (HitComponent))
    {
        return HasItem () && bCanRemoveItem;
    }

    if (HitComponent != MeshComponent)
    {
        return false;
    }

    if (!bCanInsertItem || IsFull ())
    {
        return false;
    }

    const AGrimrockPartyPawn* PartyPawn = GridInteractionUtils::ResolvePartyPawn (InstigatorPawn);
    if (!PartyPawn)
    {
        return false;
    }

    const FName HeldItemId = PartyPawn->GetHeldItemArchetypeId ();
    if (HeldItemId.IsNone ())
    {
        return false;
    }

    TArray<FName> HeldItemTags;
    const AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor (InstigatorPawn, this);
    if (RuntimeActor)
    {
        if (const UGridObjectArchetypeAsset* ItemArchetype = RuntimeActor->FindObjectArchetype (HeldItemId))
        {
            HeldItemTags = ItemArchetype->ItemTags;
        }
    }

    return CanAcceptItemArchetype (HeldItemId, HeldItemTags);
}

void AGridReceptacleActor::Interact_Implementation (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent)
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

    AGridItemActor* PreviousPendingRemovalItemActor = PendingRemovalItemActor;
    if (!PreviousPendingRemovalItemActor && IsContainedItemHitComponent (HitComponent))
    {
        PendingRemovalItemActor = FindContainedItemActorForComponent (HitComponent);
    }

    AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor (InstigatorPawn, this);
    if (RuntimeActor)
    {
        RuntimeActor->TryInteractAtEdge (CellX, CellY, Edge, PartyPawn);
    }
    PendingRemovalItemActor = PreviousPendingRemovalItemActor;
}

void AGridReceptacleActor::InteractWithHit_Implementation (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent, const FHitResult& HitResult)
{
    if (bUsePhysicalPlacement && HitComponent == MeshComponent)
    {
        PendingPlacementHitResult = HitResult;
    }

    Interact_Implementation (InstigatorPawn, HitComponent);
    PendingPlacementHitResult.Reset ();
}

EGridInteractionCursor AGridReceptacleActor::GetInteractionCursor_Implementation (UPrimitiveComponent* HitComponent) const
{
    if (IsContainedItemHitComponent (HitComponent) && HasItem () && bCanRemoveItem)
    {
        return EGridInteractionCursor::Take;
    }

    if (HitComponent == MeshComponent && !IsFull () && bCanInsertItem)
    {
        return EGridInteractionCursor::Use;
    }

    return EGridInteractionCursor::Default;
}

FText AGridReceptacleActor::GetInteractionText_Implementation (UPrimitiveComponent* HitComponent) const
{
    if (IsContainedItemHitComponent (HitComponent) && HasItem () && bCanRemoveItem)
    {
        return FText::FromString (TEXT ("Take"));
    }

    if (HitComponent == MeshComponent && !IsFull () && bCanInsertItem)
    {
        return FText::FromString (TEXT ("Place item"));
    }

    return FText::GetEmpty ();
}

void AGridReceptacleActor::InitializeGridObject (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material,
    const FTransform& WorldTransform)
{
    AGridRuntimeObjectActor::InitializeGridObject (ObjectData, Mesh, Material, WorldTransform);

    bAcceptAnyItem = ObjectData.Behavior.Receptacle.bAcceptAnyItem;
    AcceptedItemTags = ObjectData.Behavior.Receptacle.AcceptedItemTags;
    AcceptedArchetypeIds = ObjectData.Behavior.Receptacle.AcceptedArchetypeIds;
    RejectedItemArchetypeIds = ObjectData.Behavior.Receptacle.RejectedItemArchetypeIds;
    InitialContainedItemArchetypeId = ObjectData.Behavior.Receptacle.InitialContainedItemArchetypeId;
    MaxContainedItems = FMath::Max (1, ObjectData.Behavior.Receptacle.MaxContainedItems);
    bUsePhysicalPlacement = ObjectData.Behavior.Receptacle.bUsePhysicalPlacement;
    bExtinguishItemOnPhysicalPlacement = ObjectData.Behavior.Receptacle.bExtinguishItemOnPhysicalPlacement;
    PhysicalPlacementSurfaceOffset = ObjectData.Behavior.Receptacle.PhysicalPlacementSurfaceOffset;
    PhysicalPlacementInitialRotationOffset = ObjectData.Behavior.Receptacle.PhysicalPlacementInitialRotationOffset;
    bStartsFilled = ObjectData.bInitiallyActive;
    bHadInitialItemAtSpawn = false;
    bInitialItemRemovedFromSpawn = false;
    InitialItemRuntimeObjectId.Invalidate ();
    InitialItemArchetypeIdAtSpawn = NAME_None;

    if (MeshComponent && bUsePhysicalPlacement)
    {
        MeshComponent->SetCollisionEnabled (ECollisionEnabled::QueryAndPhysics);
        MeshComponent->SetCollisionResponseToChannel (ECC_PhysicsBody, ECR_Block);
    }

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
    if (bUsePhysicalPlacement)
    {
        for (const TObjectPtr<AGridItemActor>& ItemActor : ContainedItemActors)
        {
            if (IsValid (ItemActor))
            {
                return true;
            }
        }
    }
    return !ContainedItemArchetypeId.IsNone () || IsValid (ContainedItemActor);
}

int32 AGridReceptacleActor::GetContainedItemCount () const
{
    if (bUsePhysicalPlacement)
    {
        int32 Count = 0;
        for (const TObjectPtr<AGridItemActor>& ItemActor : ContainedItemActors)
        {
            if (IsValid (ItemActor))
            {
                ++Count;
            }
        }
        if (Count == 0 && !ContainedItemArchetypeId.IsNone ())
        {
            return 1;
        }
        return Count;
    }
    return HasItem () ? 1 : 0;
}

bool AGridReceptacleActor::IsFull () const
{
    return bUsePhysicalPlacement
        ? GetContainedItemCount () >= FMath::Max (1, MaxContainedItems)
        : HasItem ();
}

bool AGridReceptacleActor::CanAcceptItem (FName ItemId) const
{
    return CanAcceptItemArchetype (ItemId, TArray<FName> ());
}

bool AGridReceptacleActor::CanAcceptItemArchetype (FName ItemArchetypeId, const TArray<FName>& ItemTags) const
{
    return GetItemAcceptanceFailureReason (ItemArchetypeId, ItemTags).IsEmpty ();
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
        UE_LOG (LogTemp, Warning, TEXT ("Receptacle %s rejected item %s: %s"),
            *ObjectId.ToString (),
            *ItemId.ToString (),
            *GetItemAcceptanceFailureReason (ItemId, ItemTags));
        return false;
    }

    if (ItemId.IsNone ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("Receptacle %s cannot insert an inventory item without an ArchetypeId."),
            *ObjectId.ToString ());
        return false;
    }

    if (RuntimeActor)
    {
        USceneComponent* AttachParent = bUsePhysicalPlacement ? nullptr : ItemAttachPoint.Get ();
        if (AGridItemActor* ItemActor = RuntimeActor->SpawnItemActorForArchetype (ItemId, this, AttachParent))
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
    if (!ItemActor)
    {
        UE_LOG (LogTemp, Warning, TEXT ("Receptacle %s rejected item actor: item actor is null."),
            *ObjectId.ToString ());
        return false;
    }

    if (!CanAcceptItemArchetype (ItemActor->ArchetypeId, ItemActor->ItemTags))
    {
        UE_LOG (LogTemp, Warning, TEXT ("Receptacle %s rejected item actor %s: %s"),
            *ObjectId.ToString (),
            *ItemActor->ArchetypeId.ToString (),
            *GetItemAcceptanceFailureReason (ItemActor->ArchetypeId, ItemActor->ItemTags));
        return false;
    }

    ContainedItemActor = ItemActor;
    ContainedItemActors.AddUnique (ItemActor);
    AttachContainedItemActor (ItemActor, PendingPlacementHitResult.IsSet () ? &PendingPlacementHitResult.GetValue () : nullptr);
    RebuildContainedItemState ();
    return true;
}

bool AGridReceptacleActor::TryRemoveItem (FName& OutRemovedItemId)
{
    OutRemovedItemId = NAME_None;
    if (!bCanRemoveItem || !HasItem ())
    {
        return false;
    }

    AGridItemActor* ItemActorToRemove = IsValid (PendingRemovalItemActor) ? PendingRemovalItemActor.Get () : GetDefaultContainedItemActor ();
    OutRemovedItemId = ItemActorToRemove ? ItemActorToRemove->ArchetypeId : ContainedItemArchetypeId;
    const FGuid RemovedRuntimeObjectId = ItemActorToRemove ? ItemActorToRemove->GetRuntimeObjectId () : FGuid ();
    const bool bRemovingInitialItem =
        bHadInitialItemAtSpawn &&
        ((RemovedRuntimeObjectId.IsValid () && RemovedRuntimeObjectId == InitialItemRuntimeObjectId) ||
            (!RemovedRuntimeObjectId.IsValid () && OutRemovedItemId == InitialItemArchetypeIdAtSpawn));
    ClearContainedItemActor (ItemActorToRemove);
    if (!ItemActorToRemove)
    {
        SetContainedItem (NAME_None);
    }
    RebuildContainedItemState ();
    if (bRemovingInitialItem)
    {
        bInitialItemRemovedFromSpawn = true;
        UE_LOG (LogTemp, Log,
            TEXT ("GridReceptacle Pickup InitialItem Removed ReceptacleId=%s ItemId=%s"),
            *ObjectId.ToString (),
            *InitialItemRuntimeObjectId.ToString ());
    }
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

bool AGridReceptacleActor::TryTakeContainedItemActor (AGridItemActor* ItemActor, AGrimrockPartyPawn* PartyPawn, FName& OutRemovedItemId)
{
    PendingRemovalItemActor = ItemActor;
    const bool bTaken = TryTakeContainedItem (PartyPawn, OutRemovedItemId);
    PendingRemovalItemActor = nullptr;
    return bTaken;
}

bool AGridReceptacleActor::TryInteractWithParty (AGrimrockPartyPawn* PartyPawn)
{
    if (!PartyPawn)
    {
        return false;
    }

    if (IsValid (PendingRemovalItemActor) || (!bUsePhysicalPlacement && HasItem ()))
    {
        FName RemovedItemId = NAME_None;
        if (!TryTakeContainedItem (PartyPawn, RemovedItemId))
        {
            return false;
        }

        UE_LOG (LogTemp, Log, TEXT ("Receptacle returned item %s"), *RemovedItemId.ToString ());
        return true;
    }

    const FName HeldItemId = PartyPawn->GetHeldItemArchetypeId ();
    if (HeldItemId.IsNone ())
    {
        return false;
    }

    TArray<FName> HeldItemTags;
    if (AGridLevelRuntimeActor* RuntimeActor = Cast<AGridLevelRuntimeActor> (GetOwner ()))
    {
        if (const UGridObjectArchetypeAsset* ItemArchetype = RuntimeActor->FindObjectArchetype (HeldItemId))
        {
            HeldItemTags = ItemArchetype->ItemTags;
        }
    }

    if (!CanAcceptItemArchetype (HeldItemId, HeldItemTags))
    {
        UE_LOG (LogTemp, Warning, TEXT ("Receptacle %s rejected held item %s: %s"),
            *ObjectId.ToString (),
            *HeldItemId.ToString (),
            *GetItemAcceptanceFailureReason (HeldItemId, HeldItemTags));
        return false;
    }

    if (!PartyPawn->RemoveInventoryItem (HeldItemId, 1))
    {
        UE_LOG (LogTemp, Warning, TEXT ("Receptacle failed to remove held item %s from inventory"), *HeldItemId.ToString ());
        return false;
    }

    PartyPawn->ClearHeldItem ();

    if (!TryInsertItem (HeldItemId))
    {
        PartyPawn->AddInventoryItem (HeldItemId, 1);
        UE_LOG (LogTemp, Warning, TEXT ("Receptacle failed to insert item %s"), *HeldItemId.ToString ());
        return false;
    }

    UE_LOG (LogTemp, Log, TEXT ("Receptacle accepted item %s"), *HeldItemId.ToString ());
    return true;
}

void AGridReceptacleActor::ConfigureContainedItemVisual (UStaticMesh* InMesh, UMaterialInterface* InMaterial)
{
    RuntimeContainedItemMesh = InMesh;
    RuntimeContainedItemMaterial = InMaterial;
    if (!ContainedItemMesh)
    {
        UpdateContainedItemInteractionCollision ();
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
    UpdateContainedItemInteractionCollision ();
}

void AGridReceptacleActor::SetContainedItem (FName NewItemId)
{
    ContainedItemArchetypeId = NewItemId;
    ContainedItemId = NewItemId;
    const bool bVisible = HasItem () && !IsValid (ContainedItemActor);
    if (!ContainedItemMesh)
    {
        UpdateContainedItemInteractionCollision ();
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
    UpdateContainedItemInteractionCollision ();
}

void AGridReceptacleActor::SetInitialContainedItemActor (AGridItemActor* ItemActor)
{
    ContainedItemActor = ItemActor;
    if (ContainedItemActor)
    {
        bHadInitialItemAtSpawn = true;
        bInitialItemRemovedFromSpawn = false;
        InitialItemRuntimeObjectId = ContainedItemActor->GetRuntimeObjectId ();
        InitialItemArchetypeIdAtSpawn = ContainedItemActor->ArchetypeId;
        ContainedItemActors.AddUnique (ContainedItemActor);
        AttachContainedItemActor (ContainedItemActor);
        RebuildContainedItemState ();
    }
    UpdateContainedItemInteractionCollision ();
}

void AGridReceptacleActor::CaptureRuntimeReceptacleState (FGridRuntimeReceptacleState& OutState) const
{
    OutState.ObjectId = ObjectId;
    OutState.ContainedItemObjectIds.Reset ();
    OutState.ContainedItems.Reset ();

    auto AddItemState = [this, &OutState] (const AGridItemActor* ItemActor)
    {
        if (!IsValid (ItemActor) || ItemActor->ArchetypeId.IsNone ())
        {
            return;
        }

        FGridRuntimeItemState ItemState;
        ItemState.ObjectId = ItemActor->GetRuntimeObjectId ().IsValid () ? ItemActor->GetRuntimeObjectId () : FGuid::NewGuid ();
        ItemState.ArchetypeId = ItemActor->ArchetypeId;
        ItemState.Transform = ItemActor->GetActorTransform ();
        ItemState.bIsContainedInReceptacle = true;
        ItemState.ReceptacleObjectId = ObjectId;
        ItemState.bLightsEnabled = ItemActor->AreItemLightsEnabled ();
        ItemState.bIsSimulatingPhysics = ItemActor->MeshComponent && ItemActor->MeshComponent->IsSimulatingPhysics ();

        OutState.ContainedItemObjectIds.Add (ItemState.ObjectId);
        OutState.ContainedItems.Add (ItemState);
    };

    if (bUsePhysicalPlacement)
    {
        for (const TObjectPtr<AGridItemActor>& ItemActor : ContainedItemActors)
        {
            AddItemState (ItemActor.Get ());
        }
    }
    else
    {
        AddItemState (ContainedItemActor.Get ());
    }

    if (OutState.ContainedItems.Num () == 0 && !ContainedItemArchetypeId.IsNone () && !bInitialItemRemovedFromSpawn)
    {
        FGridRuntimeItemState ItemState;
        ItemState.ObjectId = ObjectId.IsValid () ? ObjectId : FGuid::NewGuid ();
        ItemState.ArchetypeId = ContainedItemArchetypeId;
        ItemState.Transform = ItemAttachPoint ? ItemAttachPoint->GetComponentTransform () : GetActorTransform ();
        ItemState.bIsContainedInReceptacle = true;
        ItemState.ReceptacleObjectId = ObjectId;
        ItemState.bLightsEnabled = !bUsePhysicalPlacement || !bExtinguishItemOnPhysicalPlacement;
        ItemState.bIsSimulatingPhysics = bUsePhysicalPlacement;

        OutState.ContainedItemObjectIds.Add (ItemState.ObjectId);
        OutState.ContainedItems.Add (ItemState);
    }

    UE_LOG (LogTemp, Log,
        TEXT ("GridRuntimeState Capture Receptacle ObjectId=%s Items=%d"),
        *ObjectId.ToString (),
        OutState.ContainedItems.Num ());
}

int32 AGridReceptacleActor::ClearRuntimeContainedItems ()
{
    int32 ClearedItemCount = 0;

    TArray<TObjectPtr<AGridItemActor>> ItemActors = ContainedItemActors;
    for (const TObjectPtr<AGridItemActor>& ItemActor : ItemActors)
    {
        if (IsValid (ItemActor))
        {
            if (bHadInitialItemAtSpawn && ItemActor->GetRuntimeObjectId () == InitialItemRuntimeObjectId)
            {
                bInitialItemRemovedFromSpawn = true;
            }
            ++ClearedItemCount;
            ClearContainedItemActor (ItemActor.Get ());
        }
    }

    if (IsValid (ContainedItemActor))
    {
        if (bHadInitialItemAtSpawn && ContainedItemActor->GetRuntimeObjectId () == InitialItemRuntimeObjectId)
        {
            bInitialItemRemovedFromSpawn = true;
        }
        ++ClearedItemCount;
        ClearContainedItemActor (ContainedItemActor.Get ());
    }

    if (UWorld* World = GetWorld ())
    {
        for (TActorIterator<AGridItemActor> It (World); It; ++It)
        {
            AGridItemActor* ItemActor = *It;
            if (!IsValid (ItemActor) || ItemActor->GetOwner () != this)
            {
                continue;
            }

            if (bHadInitialItemAtSpawn && ItemActor->GetRuntimeObjectId () == InitialItemRuntimeObjectId)
            {
                bInitialItemRemovedFromSpawn = true;
            }
            ++ClearedItemCount;
            ItemActor->OnRemovedFromWorld ();
            ItemActor->Destroy ();
        }
    }

    if (bHadInitialItemAtSpawn && !ContainedItemArchetypeId.IsNone () && ContainedItemArchetypeId == InitialItemArchetypeIdAtSpawn)
    {
        bInitialItemRemovedFromSpawn = true;
    }

    ContainedItemActors.Reset ();
    ContainedItemActor = nullptr;
    SetContainedItem (NAME_None);
    RebuildContainedItemState ();
    UpdateContainedItemInteractionCollision ();

    return ClearedItemCount;
}

bool AGridReceptacleActor::RestoreRuntimeContainedItem (const FGridRuntimeItemState& ItemState, AGridItemActor* ItemActor)
{
    if (!ItemActor || ItemState.ArchetypeId.IsNone ())
    {
        return false;
    }

    ItemActor->SetRuntimeObjectId (ItemState.ObjectId);
    ContainedItemActor = ItemActor;
    ContainedItemActors.AddUnique (ItemActor);

    if (bUsePhysicalPlacement)
    {
        ItemActor->DetachFromActor (FDetachmentTransformRules::KeepWorldTransform);
        ItemActor->SetActorTransform (ItemState.Transform, false, nullptr, ETeleportType::TeleportPhysics);
        ItemActor->ConfigureAsWorldPickup ();
        ItemActor->SetItemLightsEnabled (ItemState.bLightsEnabled);
    }
    else
    {
        USceneComponent* AttachTarget = ItemAttachPoint ? ItemAttachPoint.Get () : ItemSocketRoot.Get ();
        ItemActor->ConfigureAsAttachedItem ();
        ItemActor->AttachToComponent (AttachTarget, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        ItemActor->SetActorRelativeTransform (FTransform::Identity);
        ItemActor->SetItemLightsEnabled (ItemState.bLightsEnabled);
    }

    RebuildContainedItemState ();
    UpdateContainedItemInteractionCollision ();
    return true;
}

void AGridReceptacleActor::AttachContainedItemActor (const FHitResult* PlacementHitResult)
{
    AttachContainedItemActor (ContainedItemActor, PlacementHitResult);
}

void AGridReceptacleActor::AttachContainedItemActor (AGridItemActor* ItemActor, const FHitResult* PlacementHitResult)
{
    if (!ItemActor)
    {
        UpdateContainedItemInteractionCollision ();
        return;
    }

    if (bUsePhysicalPlacement)
    {
        ItemActor->DetachFromActor (FDetachmentTransformRules::KeepWorldTransform);
        const FVector PlacementLocation = PlacementHitResult && PlacementHitResult->bBlockingHit
            ? PlacementHitResult->ImpactPoint + PlacementHitResult->ImpactNormal * PhysicalPlacementSurfaceOffset
            : (ItemAttachPoint ? ItemAttachPoint->GetComponentLocation () : GetActorLocation ());
        const FQuat BaseRotation = ItemAttachPoint ? ItemAttachPoint->GetComponentQuat () : GetActorQuat ();
        const FQuat PlacementRotation = BaseRotation * PhysicalPlacementInitialRotationOffset.Quaternion ();
        ItemActor->SetActorLocationAndRotation (PlacementLocation, PlacementRotation, false, nullptr, ETeleportType::TeleportPhysics);
        ItemActor->ConfigureAsWorldPickup ();
        ItemActor->SetItemLightsEnabled (!bExtinguishItemOnPhysicalPlacement);
    }
    else
    {
        USceneComponent* AttachTarget = ItemAttachPoint ? ItemAttachPoint.Get () : ItemSocketRoot.Get ();
        ItemActor->ConfigureAsAttachedItem ();
        ItemActor->AttachToComponent (AttachTarget, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        ItemActor->SetActorRelativeTransform (FTransform::Identity);
        ItemActor->OnPlacedInWorld ();
    }

    if (ContainedItemMesh)
    {
        ContainedItemMesh->SetHiddenInGame (true, true);
        ContainedItemMesh->SetVisibility (false, true);
    }
    UpdateContainedItemInteractionCollision ();
}

void AGridReceptacleActor::ClearContainedItemActor (AGridItemActor* ItemActor)
{
    AGridItemActor* ItemActorToClear = ItemActor ? ItemActor : ContainedItemActor.Get ();
    if (!ItemActorToClear)
    {
        ContainedItemTags.Reset ();
        UpdateContainedItemInteractionCollision ();
        return;
    }

    ContainedItemActors.Remove (ItemActorToClear);
    if (ContainedItemActor == ItemActorToClear)
    {
        ContainedItemActor = nullptr;
    }
    ItemActorToClear->OnRemovedFromWorld ();
    ItemActorToClear->Destroy ();
    RebuildContainedItemState ();
    UpdateContainedItemInteractionCollision ();
}

void AGridReceptacleActor::UpdateContainedItemInteractionCollision ()
{
    const bool bCanTake = HasItem () && bCanRemoveItem;

    if (MeshComponent)
    {
        const bool bCanPlaceMorePhysicalItems = bUsePhysicalPlacement && bCanInsertItem && !IsFull ();
        const bool bExposePhysicalItemToCursor = bUsePhysicalPlacement && !bCanPlaceMorePhysicalItems;
        MeshComponent->SetCollisionResponseToChannel (ECC_Visibility, bExposePhysicalItemToCursor ? ECR_Ignore : ECR_Block);
    }

    if (ContainedItemMesh)
    {
        const bool bEnableContainedMeshTrace = bCanTake && ContainedItemMesh->IsVisible ();
        ContainedItemMesh->SetCollisionEnabled (bEnableContainedMeshTrace ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
        ContainedItemMesh->SetCollisionResponseToAllChannels (ECR_Ignore);
        ContainedItemMesh->SetCollisionResponseToChannel (ECC_Visibility, ECR_Block);
        ContainedItemMesh->SetGenerateOverlapEvents (false);
    }

    TArray<AGridItemActor*> ItemActorsToUpdate;
    if (bUsePhysicalPlacement)
    {
        for (const TObjectPtr<AGridItemActor>& ItemActor : ContainedItemActors)
        {
            if (IsValid (ItemActor))
            {
                ItemActorsToUpdate.Add (ItemActor.Get ());
            }
        }
    }
    else if (ContainedItemActor)
    {
        ItemActorsToUpdate.Add (ContainedItemActor.Get ());
    }

    for (AGridItemActor* ItemActor : ItemActorsToUpdate)
    {
        if (!ItemActor || !ItemActor->MeshComponent)
        {
            continue;
        }
        if (bUsePhysicalPlacement)
        {
            ItemActor->MeshComponent->SetCollisionEnabled (ECollisionEnabled::QueryAndPhysics);
            ItemActor->MeshComponent->SetCollisionProfileName (TEXT ("PhysicsActor"));
            ItemActor->MeshComponent->SetCollisionResponseToChannel (ECC_Visibility, ECR_Block);
        }
        else
        {
            ItemActor->MeshComponent->SetCollisionEnabled (bCanTake ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
            ItemActor->MeshComponent->SetCollisionResponseToAllChannels (ECR_Ignore);
            ItemActor->MeshComponent->SetCollisionResponseToChannel (ECC_Visibility, ECR_Block);
        }
        ItemActor->MeshComponent->SetGenerateOverlapEvents (false);
    }
}

bool AGridReceptacleActor::IsContainedItemHitComponent (UPrimitiveComponent* HitComponent) const
{
    if (!HitComponent)
    {
        return false;
    }

    if (HitComponent == ContainedItemMesh)
    {
        return true;
    }

    return FindContainedItemActorForComponent (HitComponent) != nullptr;
}

AGridItemActor* AGridReceptacleActor::FindContainedItemActorForComponent (UPrimitiveComponent* HitComponent) const
{
    if (!HitComponent)
    {
        return nullptr;
    }
    if (bUsePhysicalPlacement)
    {
        for (const TObjectPtr<AGridItemActor>& ItemActor : ContainedItemActors)
        {
            if (IsValid (ItemActor) && HitComponent == ItemActor->MeshComponent)
            {
                return ItemActor.Get ();
            }
        }
        return nullptr;
    }
    return ContainedItemActor && HitComponent == ContainedItemActor->MeshComponent
        ? ContainedItemActor.Get ()
        : nullptr;
}

AGridItemActor* AGridReceptacleActor::GetDefaultContainedItemActor () const
{
    if (IsValid (PendingRemovalItemActor))
    {
        return PendingRemovalItemActor.Get ();
    }
    if (bUsePhysicalPlacement)
    {
        for (int32 Index = ContainedItemActors.Num () - 1; Index >= 0; --Index)
        {
            if (IsValid (ContainedItemActors[Index]))
            {
                return ContainedItemActors[Index].Get ();
            }
        }
        return nullptr;
    }
    return ContainedItemActor.Get ();
}

void AGridReceptacleActor::RebuildContainedItemState ()
{
    ContainedItemActors.RemoveAll ([] (const TObjectPtr<AGridItemActor>& ItemActor)
    {
        return !IsValid (ItemActor);
    });

    if (bUsePhysicalPlacement)
    {
        ContainedItemActor = GetDefaultContainedItemActor ();
    }

    ContainedItemTags.Reset ();
    ContainedItemArchetypeId = NAME_None;
    ContainedItemId = NAME_None;

    if (bUsePhysicalPlacement)
    {
        for (const TObjectPtr<AGridItemActor>& ItemActor : ContainedItemActors)
        {
            if (!IsValid (ItemActor))
            {
                continue;
            }
            if (ContainedItemArchetypeId.IsNone ())
            {
                ContainedItemArchetypeId = ItemActor->ArchetypeId;
                ContainedItemId = ItemActor->ArchetypeId;
            }
            for (const FName& ItemTag : ItemActor->ItemTags)
            {
                ContainedItemTags.AddUnique (ItemTag);
            }
        }
    }
    else if (ContainedItemActor)
    {
        ContainedItemArchetypeId = ContainedItemActor->ArchetypeId;
        ContainedItemId = ContainedItemActor->ArchetypeId;
        ContainedItemTags = ContainedItemActor->ItemTags;
    }

    const bool bVisible = HasItem () && !IsValid (ContainedItemActor);
    if (ContainedItemMesh)
    {
        ContainedItemMesh->SetHiddenInGame (!bVisible, true);
        ContainedItemMesh->SetVisibility (bVisible, true);
        ContainedItemMesh->MarkRenderStateDirty ();
    }
}

FString AGridReceptacleActor::GetItemAcceptanceFailureReason (FName ItemArchetypeId, const TArray<FName>& ItemTags) const
{
    if (!bCanInsertItem)
    {
        return TEXT ("insertion is disabled");
    }

    if (IsFull ())
    {
        return FString::Printf (TEXT ("receptacle is full (%d/%d)"), GetContainedItemCount (), FMath::Max (1, MaxContainedItems));
    }

    if (ItemArchetypeId.IsNone ())
    {
        return bAcceptAnyItem
            ? FString ()
            : FString (TEXT ("item has no ArchetypeId and the receptacle uses strict acceptance rules"));
    }

    if (RejectedItemArchetypeIds.Contains (ItemArchetypeId))
    {
        return FString::Printf (TEXT ("item ArchetypeId '%s' is explicitly rejected"), *ItemArchetypeId.ToString ());
    }

    if (bAcceptAnyItem)
    {
        return FString ();
    }

    if (AcceptedArchetypeIds.Contains (ItemArchetypeId))
    {
        return FString ();
    }

    for (const FName& AcceptedTag : AcceptedItemTags)
    {
        if (!AcceptedTag.IsNone () && ItemTags.Contains (AcceptedTag))
        {
            return FString ();
        }
    }

    return FString::Printf (TEXT ("item ArchetypeId '%s' and tags are not accepted"), *ItemArchetypeId.ToString ());
}

void AGridReceptacleActor::ExecuteInsertionLinks ()
{
    if (AGridLevelRuntimeActor* RuntimeActor = Cast<AGridLevelRuntimeActor> (GetOwner ()))
    {
        RuntimeActor->ExecuteLinksFromRuntimeObject (ObjectId, EGridObjectEvent::ItemInserted);
    }
}

void AGridReceptacleActor::ExecuteRemovalLinks ()
{
    if (AGridLevelRuntimeActor* RuntimeActor = Cast<AGridLevelRuntimeActor> (GetOwner ()))
    {
        RuntimeActor->ExecuteLinksFromRuntimeObject (ObjectId, EGridObjectEvent::ItemRemoved);
    }
}
