#include "Runtime/GridReceptacleActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Runtime/GridInteractionUtils.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    static constexpr float MultiItemVisualSpacing = 18.0f;

    FName ResolveDefinitionId (const UGridItemDefinitionAsset* Definition, FName FallbackId)
    {
        if (Definition && !Definition->ItemDefinitionId.IsNone ())
        {
            return Definition->ItemDefinitionId;
        }

        return FallbackId;
    }

    FName ResolveItemActorDefinitionOrArchetypeId (const AGridItemActor* ItemActor)
    {
        if (!ItemActor)
        {
            return NAME_None;
        }
        if (const UGridItemDefinitionAsset* Definition = ItemActor->GetItemDefinitionAsset ())
        {
            if (!Definition->ItemDefinitionId.IsNone ())
            {
                return Definition->ItemDefinitionId;
            }
        }
        if (!ItemActor->GetItemDefinitionId ().IsNone ())
        {
            return ItemActor->GetItemDefinitionId ();
        }
        return ItemActor->GetItemArchetypeId ();
    }

    UGridItemDefinitionAsset* ResolveItemDefinition (UGridPartyInventoryComponent* PartyInventoryComponent, UGridItemDefinitionAsset* DirectDefinition, FName ItemDefinitionId)
    {
        if (DirectDefinition)
        {
            return DirectDefinition;
        }

        if (PartyInventoryComponent && !ItemDefinitionId.IsNone ())
        {
            return PartyInventoryComponent->FindItemDefinition (ItemDefinitionId);
        }

        return nullptr;
    }

    AGridLevelRuntimeActor* FindRuntimeActor (UWorld* World)
    {
        if (!World)
        {
            return nullptr;
        }

        for (TActorIterator<AGridLevelRuntimeActor> It (World); It; ++It)
        {
            return *It;
        }

        return nullptr;
    }

    bool ResolveHeldEquipmentItem (
        const AGrimrockPartyPawn* PartyPawn,
        FGridItemInstance& OutItem,
        EGridEquipmentSlot& OutSlot)
    {
        OutItem = FGridItemInstance ();
        OutSlot = EGridEquipmentSlot::None;
        if (!PartyPawn || !PartyPawn->PartyInventoryComponent)
        {
            return false;
        }

        UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent;
        const int32 CharacterIndex = Inventory->GetSelectedCharacterIndex ();
        const FName HeldItemDefinitionId = PartyPawn->GetHeldItemDefinitionId ();

        FGridItemInstance EquippedItem;
        if (Inventory->GetEquippedItem (CharacterIndex, EGridEquipmentSlot::MainHand, EquippedItem) &&
            (HeldItemDefinitionId.IsNone () || EquippedItem.ItemDefinitionId == HeldItemDefinitionId))
        {
            OutItem = EquippedItem;
            OutSlot = EGridEquipmentSlot::MainHand;
            return true;
        }
        if (Inventory->GetEquippedItem (CharacterIndex, EGridEquipmentSlot::OffHand, EquippedItem) &&
            (HeldItemDefinitionId.IsNone () || EquippedItem.ItemDefinitionId == HeldItemDefinitionId))
        {
            OutItem = EquippedItem;
            OutSlot = EGridEquipmentSlot::OffHand;
            return true;
        }
        return false;
    }
}

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

    if (!bInitialItemsInitialized)
    {
        UE_LOG (LogTemp, Log,
            TEXT ("GridReceptacle BeginPlay InitialItems ObjectId=%s InitialItems=%d ContainedItems=%d InitialDefinitionId=%s InitialArchetypeId=%s"),
            *ObjectId.ToString (),
            InitialContainedItems.Num (),
            ContainedItems.Num (),
            InitialContainedItems.Num () > 0 ? *InitialContainedItems[0].ItemDefinitionId.ToString () : TEXT ("None"),
            *InitialContainedItemArchetypeId.ToString ());
        InitializeInitialContainedItems ();
    }

    UpdateContainedItemInteractionCollision ();
}

void AGridReceptacleActor::EndPlay (const EEndPlayReason::Type EndPlayReason)
{
    ForceClearRuntimeContents (false);
    Super::EndPlay (EndPlayReason);
}

void AGridReceptacleActor::InitializeGridObject (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh, UMaterialInterface* Material, const FTransform& WorldTransform)
{
    AGridRuntimeObjectActor::InitializeGridObject (ObjectData, Mesh, Material, WorldTransform);

    const FGridReceptacleBehaviorParams& Params = ObjectData.Behavior.Receptacle;

    bCanInsertItem = true;
    bCanRemoveItem = true;

    bAcceptAnyItem = Params.bAcceptAnyItem;
    MaxContainedItems = Params.MaxContainedItems;
    bUsePhysicalPlacement = Params.bUsePhysicalPlacement;
    bExtinguishItemOnPhysicalPlacement = Params.bExtinguishItemOnPhysicalPlacement;
    PhysicalPlacementSurfaceOffset = Params.PhysicalPlacementSurfaceOffset;
    PhysicalPlacementInitialRotationOffset = Params.PhysicalPlacementInitialRotationOffset;

    if (MeshComponent && bUsePhysicalPlacement)
    {
        MeshComponent->SetCollisionEnabled (ECollisionEnabled::QueryAndPhysics);
        MeshComponent->SetCollisionResponseToChannel (ECC_PhysicsBody, ECR_Block);
    }

    AcceptedItemDefinitionIds.Reset ();
    RejectedItemDefinitionIds.Reset ();
    InitialContainedItems.Reset ();
    ContainedItems.Reset ();
    RemovedInitialItemDefinitionIds.Reset ();
    bInitialItemsInitialized = false;
    InitialContainedItemArchetypeId = Params.InitialContainedItemArchetypeId;
    ContainedItemArchetypeId = NAME_None;

    // ItemDefinitionId remains the primary runtime identity.
    // Legacy AcceptedArchetypeIds / RejectedItemArchetypeIds remain valid item ids while GridObjectBehavior still exposes them.
    AcceptedItemDefinitionIds = Params.AcceptedArchetypeIds;
    RejectedItemDefinitionIds = Params.RejectedItemArchetypeIds;

    if (!ObjectData.Tag.IsNone () && AcceptedItemDefinitionIds.Num () == 0)
    {
        AcceptedItemDefinitionIds.Add (ObjectData.Tag);
        bAcceptAnyItem = false;
    }

    if (ObjectData.bInitiallyActive)
    {
        FGridInitialReceptacleItem InitialItem;
        InitialItem.ItemDefinition = Params.InitialContainedItemDefinition;
        InitialItem.ItemDefinitionId = ResolveDefinitionId (Params.InitialContainedItemDefinition, Params.InitialContainedItemDefinitionId);
        InitialItem.ItemArchetypeId = Params.InitialContainedItemArchetypeId;
        InitialItem.Quantity = 1;

        if (!InitialItem.ItemDefinitionId.IsNone () || !InitialItem.ItemArchetypeId.IsNone () || InitialItem.ItemDefinition)
        {
            InitialContainedItems.Add (InitialItem);
        }
    }

    if (ContainedItemMesh)
    {
        ContainedItemMesh->SetStaticMesh (nullptr);
        ContainedItemMesh->SetVisibility (false, true);
        ContainedItemMesh->SetCollisionEnabled (ECollisionEnabled::NoCollision);
    }

    UE_LOG (LogTemp, Log,
        TEXT ("GridReceptacle InitializeGridObject InitialItems ObjectId=%s InitialItems=%d ContainedItems=%d InitialDefinitionId=%s InitialArchetypeId=%s"),
        *ObjectId.ToString (),
        InitialContainedItems.Num (),
        ContainedItems.Num (),
        InitialContainedItems.Num () > 0 ? *InitialContainedItems[0].ItemDefinitionId.ToString () : TEXT ("None"),
        *InitialContainedItemArchetypeId.ToString ());

    if (!bInitialItemsInitialized)
    {
        InitializeInitialContainedItems ();
    }
}

bool AGridReceptacleActor::HasItem () const
{
    return ContainedItems.Num () > 0;
}

int32 AGridReceptacleActor::GetContainedItemCount () const
{
    return ContainedItems.Num ();
}

bool AGridReceptacleActor::IsFull () const
{
    return MaxContainedItems > 0 && ContainedItems.Num () >= MaxContainedItems;
}

bool AGridReceptacleActor::IsEmpty () const
{
    return ContainedItems.Num () <= 0;
}

bool AGridReceptacleActor::IsValidContainedItemIndex (int32 ItemIndex) const
{
    return ContainedItems.IsValidIndex (ItemIndex);
}

FName AGridReceptacleActor::GetContainedItemDefinitionId (int32 ItemIndex) const
{
    if (!ContainedItems.IsValidIndex (ItemIndex))
    {
        return NAME_None;
    }

    return ContainedItems[ItemIndex].ItemDefinitionId;
}

AGridItemActor* AGridReceptacleActor::GetContainedItemActor (int32 ItemIndex) const
{
    if (!ContainedItems.IsValidIndex (ItemIndex))
    {
        return nullptr;
    }

    return ContainedItems[ItemIndex].ItemActor.Get ();
}

bool AGridReceptacleActor::CanAcceptItem (FName ItemDefinitionId) const
{
    if (ItemDefinitionId.IsNone ())
    {
        return false;
    }

    if (RejectedItemDefinitionIds.Contains (ItemDefinitionId))
    {
        return false;
    }

    if (bAcceptAnyItem)
    {
        return true;
    }

    return AcceptedItemDefinitionIds.Contains (ItemDefinitionId);
}

bool AGridReceptacleActor::CanAcceptItemInstance (const FGridItemInstance& Item) const
{
    const bool bResult = Item.IsValid () &&
        bCanInsertItem &&
        !IsFull () &&
        CanAcceptItem (Item.ItemDefinitionId);

    const TCHAR* Reason = TEXT ("AcceptedDefinition");
    if (!Item.IsValid () || Item.ItemDefinitionId.IsNone ())
    {
        Reason = TEXT ("Invalid");
    }
    else if (!bCanInsertItem)
    {
        Reason = TEXT ("InsertDisabled");
    }
    else if (IsFull ())
    {
        Reason = TEXT ("Full");
    }
    else if (!CanAcceptItem (Item.ItemDefinitionId))
    {
        Reason = TEXT ("Incompatible");
    }
    else if (bAcceptAnyItem)
    {
        Reason = TEXT ("AcceptAnyItem");
    }

    UE_LOG (LogTemp, Verbose,
        TEXT ("GridReceptacle CanAccept CursorItem Item=%s ObjectId=%s Result=%s Reason=%s"),
        *Item.ItemDefinitionId.ToString (),
        *Item.RuntimeObjectId.ToString (),
        bResult ? TEXT ("true") : TEXT ("false"),
        Reason);
    return bResult;
}

bool AGridReceptacleActor::CanAcceptCursorItemFromParty (const AGrimrockPartyPawn* PartyPawn) const
{
    FGridItemInstance CursorItem;
    return PartyPawn && PartyPawn->GetCursorItem (CursorItem) && CanAcceptItemInstance (CursorItem);
}

bool AGridReceptacleActor::TryInsertItem (FName ItemDefinitionId, UGridItemDefinitionAsset* ItemDefinition, AGrimrockPartyPawn* PartyPawn)
{
    ItemDefinitionId = ResolveDefinitionId (ItemDefinition, ItemDefinitionId);

    if (!bCanInsertItem)
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("Receptacle insert refused: ObjectId=%s Item=%s Reason=insert disabled"),
            *ObjectId.ToString (),
            *ItemDefinitionId.ToString ());
        return false;
    }

    if (IsFull ())
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("Receptacle insert refused: ObjectId=%s Item=%s Reason=full Count=%d Max=%d"),
            *ObjectId.ToString (),
            *ItemDefinitionId.ToString (),
            ContainedItems.Num (),
            MaxContainedItems);
        return false;
    }

    if (!CanAcceptItem (ItemDefinitionId))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("Receptacle insert refused: ObjectId=%s Item=%s Reason=%s"),
            *ObjectId.ToString (),
            *ItemDefinitionId.ToString (),
            *GetItemAcceptanceFailureReason (ItemDefinitionId));
        return false;
    }

    UGridPartyInventoryComponent* PartyInventoryComponent = PartyPawn ? PartyPawn->PartyInventoryComponent : nullptr;
    ItemDefinition = ResolveItemDefinition (PartyInventoryComponent, ItemDefinition, ItemDefinitionId);

    const int32 NewIndex = AddContainedItem (
        ItemDefinitionId,
        ItemDefinition,
        nullptr,
        false,
        1
    );

    if (NewIndex == INDEX_NONE)
    {
        return false;
    }

    ExecuteInsertionLinks ();

    UE_LOG (LogTemp, Log,
        TEXT ("Receptacle accepted item %s ObjectId=%s Count=%d"),
        *ItemDefinitionId.ToString (),
        *ObjectId.ToString (),
        ContainedItems.Num ());

    return true;
}

bool AGridReceptacleActor::TryInsertItemInstanceFromCursor (
    const FGridItemInstance& CursorItem,
    FGridItemInstance& OutAcceptedItem)
{
    OutAcceptedItem = FGridItemInstance ();

    if (!CursorItem.IsValid () || CursorItem.ItemDefinitionId.IsNone ())
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("Receptacle cursor insert refused: ObjectId=%s Item=%s RuntimeId=%s Reason=invalid cursor item"),
            *ObjectId.ToString (),
            *CursorItem.ItemDefinitionId.ToString (),
            *CursorItem.RuntimeObjectId.ToString ());
        return false;
    }

    if (!bCanInsertItem)
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("Receptacle cursor insert refused: ObjectId=%s Item=%s RuntimeId=%s Reason=insert disabled"),
            *ObjectId.ToString (),
            *CursorItem.ItemDefinitionId.ToString (),
            *CursorItem.RuntimeObjectId.ToString ());
        return false;
    }

    if (IsFull ())
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("Receptacle cursor insert refused: ObjectId=%s Item=%s RuntimeId=%s Reason=full Count=%d Max=%d"),
            *ObjectId.ToString (),
            *CursorItem.ItemDefinitionId.ToString (),
            *CursorItem.RuntimeObjectId.ToString (),
            ContainedItems.Num (),
            MaxContainedItems);
        return false;
    }

    if (!CanAcceptItem (CursorItem.ItemDefinitionId))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("Receptacle cursor insert refused: ObjectId=%s Item=%s RuntimeId=%s Reason=%s"),
            *ObjectId.ToString (),
            *CursorItem.ItemDefinitionId.ToString (),
            *CursorItem.RuntimeObjectId.ToString (),
            *GetItemAcceptanceFailureReason (CursorItem.ItemDefinitionId));
        return false;
    }

    UGridItemDefinitionAsset* ItemDefinition = nullptr;
    if (const AGrimrockPartyPawn* PartyPawn = Cast<AGrimrockPartyPawn> (UGameplayStatics::GetPlayerPawn (this, 0)))
    {
        ItemDefinition = ResolveItemDefinition (
            PartyPawn->PartyInventoryComponent,
            nullptr,
            CursorItem.ItemDefinitionId);
    }
    if (!ItemDefinition)
    {
        if (const AGridLevelRuntimeActor* RuntimeActor = FindRuntimeActor (GetWorld ()))
        {
            ItemDefinition = RuntimeActor->ResolveRuntimeItemDefinition (CursorItem.ItemDefinitionId);
        }
    }

    const int32 NewIndex = AddContainedItem (
        CursorItem.ItemDefinitionId,
        ItemDefinition,
        nullptr,
        false,
        CursorItem.Quantity,
        CursorItem.RuntimeObjectId);

    if (!ContainedItems.IsValidIndex (NewIndex))
    {
        return false;
    }

    FGridContainedReceptacleItem& AcceptedReceptacleItem = ContainedItems[NewIndex];
    AcceptedReceptacleItem.Weight = CursorItem.Weight;
    AcceptedReceptacleItem.DisplayName = CursorItem.DisplayName;
    AcceptedReceptacleItem.bLightsEnabled =
        bUsePhysicalPlacement && bExtinguishItemOnPhysicalPlacement
            ? false
            : CursorItem.bLightsEnabled;
    if (AcceptedReceptacleItem.ItemActor)
    {
        AcceptedReceptacleItem.ItemActor->SetItemLightsEnabled (AcceptedReceptacleItem.bLightsEnabled);
    }

    OutAcceptedItem = CursorItem;
    OutAcceptedItem.OwnerType = EGridItemOwnerType::Receptacle;
    OutAcceptedItem.OwnerGuid = ObjectId;
    OutAcceptedItem.OwnerCharacterIndex = INDEX_NONE;
    OutAcceptedItem.EquipmentSlot = EGridEquipmentSlot::None;

    ExecuteInsertionLinks ();

    UE_LOG (LogTemp, Log,
        TEXT ("Receptacle accepted cursor item %s ObjectId=%s RuntimeId=%s Count=%d"),
        *CursorItem.ItemDefinitionId.ToString (),
        *ObjectId.ToString (),
        *CursorItem.RuntimeObjectId.ToString (),
        ContainedItems.Num ());

    return true;
}

bool AGridReceptacleActor::TryTakeFirstItem (AGrimrockPartyPawn* PartyPawn, FName& OutRemovedItemDefinitionId)
{
    return TryTakeItemAtIndex (0, PartyPawn, OutRemovedItemDefinitionId);
}

bool AGridReceptacleActor::TryTakeItemAtIndex (int32 ItemIndex, AGrimrockPartyPawn* PartyPawn, FName& OutRemovedItemDefinitionId)
{
    OutRemovedItemDefinitionId = NAME_None;

    if (!PartyPawn || !bCanRemoveItem || !ContainedItems.IsValidIndex (ItemIndex))
    {
        return false;
    }

    AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor (PartyPawn, this);
    if (!RuntimeActor || !RuntimeActor->CanPartyInteractWithEdgeObject (CellX, CellY, Edge, PartyPawn))
    {
        return false;
    }

    const FGridContainedReceptacleItem& Item = ContainedItems[ItemIndex];

    if (Item.ItemDefinitionId.IsNone ())
    {
        return false;
    }

    FGridItemInstance ItemInstance;
    ItemInstance.RuntimeObjectId = Item.RuntimeObjectId.IsValid () ? Item.RuntimeObjectId : FGuid::NewGuid ();
    ItemInstance.ItemDefinitionId = Item.ItemDefinitionId;
    ItemInstance.Quantity = FMath::Max (1, Item.Quantity);
    ItemInstance.Weight = Item.Weight;
    ItemInstance.DisplayName = Item.DisplayName;
    ItemInstance.bLightsEnabled = Item.bLightsEnabled;
    ItemInstance.OwnerType = EGridItemOwnerType::CharacterInventory;
    ItemInstance.OwnerCharacterIndex = PartyPawn->PartyInventoryComponent ? PartyPawn->PartyInventoryComponent->GetSelectedCharacterIndex () : INDEX_NONE;

    if (Item.ItemDefinition)
    {
        ItemInstance.Weight = Item.ItemDefinition->Weight;
        ItemInstance.DisplayName = Item.ItemDefinition->DisplayName;
    }
    if (IsValid (Item.ItemActor.Get ()))
    {
        ItemInstance.bLightsEnabled = Item.ItemActor->AreItemLightsEnabled ();
    }

    if (!PartyPawn->AddItemInstanceToSelectedCharacterInventory (ItemInstance))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("Receptacle take failed: ObjectId=%s Item=%s Reason=party inventory rejected"),
            *ObjectId.ToString (),
            *Item.ItemDefinitionId.ToString ());
        return false;
    }

    FGridContainedReceptacleItem RemovedItem;
    if (!RemoveContainedItemAtIndex (ItemIndex, RemovedItem))
    {
        return false;
    }
    OutRemovedItemDefinitionId = RemovedItem.ItemDefinitionId;
    ExecuteRemovalLinks ();
    UE_LOG (LogTemp, Log,
        TEXT ("Receptacle returned item %s ObjectId=%s Count=%d"),
        *RemovedItem.ItemDefinitionId.ToString (),
        *ObjectId.ToString (),
        ContainedItems.Num ());

    return true;
}

bool AGridReceptacleActor::TryInteractWithParty (AGrimrockPartyPawn* PartyPawn)
{
    if (!PartyPawn)
    {
        return false;
    }

    AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor (PartyPawn, this);
    if (!RuntimeActor || !RuntimeActor->CanPartyInteractWithEdgeObject (CellX, CellY, Edge, PartyPawn))
    {
        return false;
    }

    if (PartyPawn->PartyInventoryComponent && PartyPawn->PartyInventoryComponent->HasCursorItem ())
    {
        const FGridItemInstance& CursorItem = PartyPawn->PartyInventoryComponent->GetCursorItem ();
        UE_LOG (LogTemp, Log,
            TEXT ("Receptacle interact cursor item attempt ObjectId=%s Item=%s RuntimeId=%s"),
            *ObjectId.ToString (),
            *CursorItem.ItemDefinitionId.ToString (),
            *CursorItem.RuntimeObjectId.ToString ());
        return PartyPawn->TryPlaceCursorItemInReceptacle (this);
    }

    if (!bUsePhysicalPlacement && HasItem () && bCanRemoveItem)
    {
        FName RemovedItemId = NAME_None;
        return TryTakeFirstItem (PartyPawn, RemovedItemId);
    }

    FGridItemInstance HeldItem;
    EGridEquipmentSlot HeldSlot = EGridEquipmentSlot::None;
    if (ResolveHeldEquipmentItem (PartyPawn, HeldItem, HeldSlot))
    {
        if (!bCanInsertItem || IsFull () || !CanAcceptItem (HeldItem.ItemDefinitionId))
        {
            return false;
        }

        UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent;
        const int32 CharacterIndex = Inventory->GetSelectedCharacterIndex ();
        if (!Inventory->TryTakeEquipmentSlotToCursor (CharacterIndex, HeldSlot))
        {
            return false;
        }

        PartyPawn->SyncHeldVisualFromSelectedCharacterEquipment ();
        if (PartyPawn->TryPlaceCursorItemInReceptacle (this))
        {
            return true;
        }

        const bool bRestored = Inventory->TryEquipCursorItemToCharacterSlot (CharacterIndex, HeldSlot);
        PartyPawn->SyncHeldVisualFromSelectedCharacterEquipment ();
        UE_LOG (LogTemp, Warning,
            TEXT ("Receptacle held item insertion failed: ObjectId=%s Item=%s Restored=%s"),
            *ObjectId.ToString (),
            *HeldItem.ItemDefinitionId.ToString (),
            bRestored ? TEXT ("true") : TEXT ("false"));
        return false;
    }

    UE_LOG (LogTemp, Verbose,
        TEXT ("Receptacle interact ignored: ObjectId=%s no CursorItem and no contained item."),
        *ObjectId.ToString ());
    return false;
}

bool AGridReceptacleActor::TryPlaceCursorItemFromHit (
    AGrimrockPartyPawn* PartyPawn,
    const FHitResult& HitResult)
{
    if (bUsePhysicalPlacement)
    {
        PendingPlacementHitResult = HitResult;
    }
    const bool bPlaced = PartyPawn && PartyPawn->TryPlaceCursorItemInReceptacle (this);
    PendingPlacementHitResult.Reset ();
    return bPlaced;
}

void AGridReceptacleActor::CaptureRuntimeReceptacleState (FGridRuntimeReceptacleState& OutState) const
{
    OutState.ObjectId = ObjectId;
    OutState.ContainedItems.Reset ();

    for (const FGridContainedReceptacleItem& Item : ContainedItems)
    {
        FName ResolvedItemId = Item.ItemDefinitionId;
        if (ResolvedItemId.IsNone ())
        {
            ResolvedItemId = ResolveItemActorDefinitionOrArchetypeId (Item.ItemActor.Get ());
        }
        if (ResolvedItemId.IsNone () && Item.bWasInitialItem)
        {
            ResolvedItemId = ResolveDefinitionId (Item.ItemDefinition, Item.ItemArchetypeId.IsNone () ? InitialContainedItemArchetypeId : Item.ItemArchetypeId);
        }
        if (ResolvedItemId.IsNone ())
        {
            ResolvedItemId = ContainedItemArchetypeId;
        }
        if (ResolvedItemId.IsNone ())
        {
            UE_LOG (LogTemp, Warning,
                TEXT ("GridReceptacle Capture skipped contained item: ReceptacleId=%s RuntimeId=%s Actor=%s no ItemDefinitionId or legacy ArchetypeId resolved."),
                *ObjectId.ToString (),
                *Item.RuntimeObjectId.ToString (),
                *GetNameSafe (Item.ItemActor.Get ()));
            continue;
        }
        FGridRuntimeItemState ItemState;
        ItemState.ObjectId = Item.RuntimeObjectId.IsValid () ? Item.RuntimeObjectId : FGuid::NewGuid ();
        ItemState.ArchetypeId = !Item.ItemArchetypeId.IsNone () ? Item.ItemArchetypeId : ResolvedItemId;
        ItemState.ItemDefinitionId = ResolvedItemId;
        ItemState.bIsContainedInReceptacle = true;
        ItemState.ReceptacleObjectId = ObjectId;
        ItemState.bLightsEnabled = true;
        if (IsValid (Item.ItemActor.Get ()))
        {
            ItemState.Transform = Item.ItemActor->GetActorTransform ();
            ItemState.bLightsEnabled = Item.ItemActor->AreItemLightsEnabled ();
        } else
        {
            ItemState.Transform = ItemAttachPoint ? ItemAttachPoint->GetComponentTransform () : GetActorTransform ();
        }
        OutState.ContainedItems.Add (ItemState);
    }
}

int32 AGridReceptacleActor::ForceClearRuntimeContents (bool bMarkInitialItemsRemoved)
{
    const int32 RemovedCount = ContainedItems.Num ();

    if (bMarkInitialItemsRemoved)
    {
        for (const FGridContainedReceptacleItem& Item : ContainedItems)
        {
            if (Item.bWasInitialItem && !Item.ItemDefinitionId.IsNone ())
            {
                RemovedInitialItemDefinitionIds.Add (Item.ItemDefinitionId);
            }
        }
    }

    ClearAllContainedActors ();
    ContainedItems.Reset ();
    ContainedItemArchetypeId = NAME_None;

    if (ContainedItemMesh)
    {
        ContainedItemMesh->SetStaticMesh (nullptr);
        ContainedItemMesh->SetVisibility (false, true);
        ContainedItemMesh->SetCollisionEnabled (ECollisionEnabled::NoCollision);
    }

    UpdateContainedItemInteractionCollision ();

    return RemovedCount;
}

bool AGridReceptacleActor::RestoreRuntimeContainedItem (const FGridRuntimeItemState& ItemState, AGridItemActor* ItemActor)
{
    if (ItemState.ItemDefinitionId.IsNone ())
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridReceptacle Restore skipped contained item: ReceptacleId=%s RuntimeId=%s no ItemDefinitionId or legacy ArchetypeId resolved."),
            *ObjectId.ToString (),
            *ItemState.ObjectId.ToString ());
        return false;
    }
    const int32 ItemIndex = AddContainedItem (ItemState.ItemDefinitionId, nullptr, ItemActor, false, 1);

    if (ItemIndex == INDEX_NONE)
    {
        return false;
    }
    FGridContainedReceptacleItem& Item = ContainedItems[ItemIndex];
    Item.RuntimeObjectId = ItemState.ObjectId.IsValid () ? ItemState.ObjectId : FGuid::NewGuid ();
    Item.ItemArchetypeId = ItemState.ArchetypeId;
    Item.bLightsEnabled = ItemState.bLightsEnabled;
    if (IsValid (Item.ItemActor.Get ()))
    {
        Item.ItemActor->SetRuntimeObjectId (Item.RuntimeObjectId);
        Item.ItemActor->SetItemLightsEnabled (ItemState.bLightsEnabled);
    }
    return true;
}

bool AGridReceptacleActor::CanInteract_Implementation (APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const
{
    if (!InstigatorPawn || !HitComponent)
    {
        return false;
    }

    const AGrimrockPartyPawn* PartyPawn = GridInteractionUtils::ResolvePartyPawn (InstigatorPawn);
    AGridLevelRuntimeActor* RuntimeActor = GridInteractionUtils::ResolveRuntimeActor (InstigatorPawn, this);
    if (!PartyPawn || !RuntimeActor ||
        !RuntimeActor->CanPartyInteractWithEdgeObject (CellX, CellY, Edge, PartyPawn))
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
    if (PartyPawn->PartyInventoryComponent && PartyPawn->PartyInventoryComponent->HasCursorItem ())
    {
        return CanAcceptCursorItemFromParty (PartyPawn);
    }

    FGridItemInstance HeldItem;
    EGridEquipmentSlot HeldSlot = EGridEquipmentSlot::None;
    if (ResolveHeldEquipmentItem (PartyPawn, HeldItem, HeldSlot))
    {
        return bCanInsertItem && !IsFull () && CanAcceptItem (HeldItem.ItemDefinitionId);
    }

    return !bUsePhysicalPlacement && HasItem () && bCanRemoveItem;
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
    if (IsContainedItemHitComponent (HitComponent))
    {
        const int32 ItemIndex = FindContainedItemIndexForComponent (HitComponent);
        if (ItemIndex != INDEX_NONE)
        {
            FName RemovedItemId = NAME_None;
            TryTakeItemAtIndex (ItemIndex, PartyPawn, RemovedItemId);
            return;
        }
    }
    TryInteractWithParty (PartyPawn);
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
    if (HitComponent == MeshComponent && bCanInsertItem && !IsFull ())
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
    if (HitComponent == MeshComponent && bCanInsertItem && !IsFull ())
    {
        return FText::FromString (TEXT ("Place item"));
    }
    return FText::GetEmpty ();
}

int32 AGridReceptacleActor::AddContainedItem (
    FName ItemDefinitionId,
    UGridItemDefinitionAsset* ItemDefinition,
    AGridItemActor* ItemActor,
    bool bWasInitialItem,
    int32 Quantity,
    FGuid RuntimeObjectId)
{
    AGridLevelRuntimeActor* RuntimeActor = FindRuntimeActor (GetWorld ());
    if (!ItemDefinition && RuntimeActor)
    {
        ItemDefinition = RuntimeActor->ResolveRuntimeItemDefinition (ItemDefinitionId);
    }

    ItemDefinitionId = ResolveDefinitionId (ItemDefinition, ItemDefinitionId);
    if (ItemDefinitionId.IsNone ())
    {
        return INDEX_NONE;
    }
    if (IsFull ())
    {
        return INDEX_NONE;
    }
    FGridContainedReceptacleItem NewItem;
    NewItem.RuntimeObjectId = RuntimeObjectId.IsValid () ? RuntimeObjectId : FGuid::NewGuid ();
    NewItem.ItemDefinitionId = ItemDefinitionId;
    NewItem.ItemArchetypeId = IsValid (ItemActor) ? ItemActor->GetItemArchetypeId () : ItemDefinitionId;
    NewItem.ItemDefinition = ItemDefinition;
    NewItem.ItemActor = ItemActor;
    NewItem.bWasInitialItem = bWasInitialItem;
    NewItem.Quantity = FMath::Max (1, Quantity);
    if (ItemDefinition)
    {
        NewItem.Weight = ItemDefinition->Weight;
        NewItem.DisplayName = ItemDefinition->DisplayName;
        NewItem.bLightsEnabled = ItemDefinition->bCanEmitLight ? ItemDefinition->bDefaultLightEnabled : true;
    }

    const int32 NewIndex = ContainedItems.Add (NewItem);
    if (ContainedItemArchetypeId.IsNone ())
    {
        ContainedItemArchetypeId = NewItem.ItemArchetypeId;
    }

    if (!IsValid (ItemActor) && RuntimeActor)
    {
            ItemActor = RuntimeActor->SpawnItemActorForDefinition (
                ItemDefinition,
                ItemDefinitionId,
            this,
            ItemAttachPoint,
            ContainedItemActorClass);
        if (ItemActor)
        {
            if (ItemDefinition)
            {
                ItemActor->InitializeFromItemDefinition (ItemDefinition, NewItem.RuntimeObjectId);
            } else
            {
                ItemActor->InitializeFromItemDefinitionId (ItemDefinitionId, NewItem.RuntimeObjectId);
            }
            ContainedItems[NewIndex].ItemActor = ItemActor;
        }
    }
    if (!IsValid (ItemActor))
    {
        UWorld* World = GetWorld ();
        if (World)
        {
            const FTransform SpawnTransform = ItemAttachPoint ? ItemAttachPoint->GetComponentTransform () : GetActorTransform ();
            TSubclassOf<AGridItemActor> ItemActorClass = ContainedItemActorClass;
            if (!ItemActorClass)
            {
                ItemActorClass = AGridItemActor::StaticClass ();
            }
            AGridItemActor* SpawnedItemActor = World->SpawnActorDeferred<AGridItemActor> (ItemActorClass, SpawnTransform, this);
            if (SpawnedItemActor)
            {
                if (ItemDefinition)
                {
                    SpawnedItemActor->InitializeFromItemDefinition (ItemDefinition, NewItem.RuntimeObjectId);
                } else
                {
                    SpawnedItemActor->InitializeFromItemDefinitionId (ItemDefinitionId, NewItem.RuntimeObjectId);
                }
                UGameplayStatics::FinishSpawningActor (SpawnedItemActor, SpawnTransform);
                SpawnedItemActor->ConfigureAsAttachedItem ();
                ContainedItems[NewIndex].ItemActor = SpawnedItemActor;
            }
        }
    }
    if (IsValid (ContainedItems[NewIndex].ItemActor.Get ()))
    {
        AttachContainedItemActor (
            ContainedItems[NewIndex].ItemActor.Get (),
            NewIndex,
            PendingPlacementHitResult.IsSet () ? &PendingPlacementHitResult.GetValue () : nullptr);
        if (bUsePhysicalPlacement)
        {
            ContainedItems[NewIndex].bLightsEnabled =
                !bExtinguishItemOnPhysicalPlacement && ContainedItems[NewIndex].bLightsEnabled;
        }
        else
        {
            ContainedItems[NewIndex].ItemActor->OnPlacedInWorld ();
        }
        ContainedItems[NewIndex].ItemActor->SetItemLightsEnabled (ContainedItems[NewIndex].bLightsEnabled);
    }
    UpdateContainedItemInteractionCollision ();
    return NewIndex;
}

bool AGridReceptacleActor::RemoveContainedItemAtIndex (int32 ItemIndex, FGridContainedReceptacleItem& OutRemovedItem)
{
    if (!ContainedItems.IsValidIndex (ItemIndex))
    {
        return false;
    }
    OutRemovedItem = ContainedItems[ItemIndex];
    if (OutRemovedItem.bWasInitialItem && !OutRemovedItem.ItemDefinitionId.IsNone ())
    {
        RemovedInitialItemDefinitionIds.Add (OutRemovedItem.ItemDefinitionId);
    }
    ClearContainedActor (ContainedItems[ItemIndex]);
    ContainedItems.RemoveAt (ItemIndex);
    ContainedItemArchetypeId = ContainedItems.Num () > 0 ? ContainedItems[0].ItemArchetypeId : NAME_None;
    UpdateContainedItemInteractionCollision ();
    return true;
}

void AGridReceptacleActor::ClearContainedActor (FGridContainedReceptacleItem& Item)
{
    if (IsValid (Item.ItemActor.Get ()))
    {
        Item.ItemActor->OnRemovedFromWorld ();
        Item.ItemActor->DetachFromActor (FDetachmentTransformRules::KeepWorldTransform);
        Item.ItemActor->Destroy ();
    }
    Item.ItemActor = nullptr;
}

void AGridReceptacleActor::ClearAllContainedActors ()
{
    for (FGridContainedReceptacleItem& Item : ContainedItems)
    {
        ClearContainedActor (Item);
    }
}

void AGridReceptacleActor::AttachContainedItemActor (AGridItemActor* ItemActor, int32 ItemIndex, const FHitResult* PlacementHitResult)
{
    if (!IsValid (ItemActor) || !ItemAttachPoint)
    {
        return;
    }

    ItemActor->SetOwner (this);
    if (bUsePhysicalPlacement)
    {
        ItemActor->DetachFromActor (FDetachmentTransformRules::KeepWorldTransform);
        const FVector PlacementLocation = PlacementHitResult && PlacementHitResult->bBlockingHit
            ? PlacementHitResult->ImpactPoint + PlacementHitResult->ImpactNormal * PhysicalPlacementSurfaceOffset
            : ItemAttachPoint->GetComponentLocation ();
        const FQuat PlacementRotation =
            ItemAttachPoint->GetComponentQuat () * PhysicalPlacementInitialRotationOffset.Quaternion ();
        ItemActor->SetActorLocationAndRotation (
            PlacementLocation,
            PlacementRotation,
            false,
            nullptr,
            ETeleportType::TeleportPhysics);
        ItemActor->ConfigureAsWorldPickup ();
        return;
    }

    ItemActor->AttachToComponent (ItemAttachPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

    const FVector LocalOffset (0.f, MultiItemVisualSpacing * ItemIndex, 0.f);
    ItemActor->SetActorRelativeLocation (LocalOffset);
    ItemActor->SetActorRelativeRotation (FRotator::ZeroRotator);
    ItemActor->SetActorRelativeScale3D (FVector::OneVector);

    ItemActor->ConfigureAsAttachedItem ();
}

void AGridReceptacleActor::UpdateContainedItemInteractionCollision ()
{
    for (FGridContainedReceptacleItem& Item : ContainedItems)
    {
        if (IsValid (Item.ItemActor.Get ()) && Item.ItemActor->MeshComponent)
        {
            if (bUsePhysicalPlacement)
            {
                Item.ItemActor->MeshComponent->SetCollisionEnabled (ECollisionEnabled::QueryAndPhysics);
                Item.ItemActor->MeshComponent->SetCollisionProfileName (TEXT ("PhysicsActor"));
                Item.ItemActor->MeshComponent->SetCollisionResponseToChannel (ECC_Visibility, ECR_Block);
            }
            else
            {
                Item.ItemActor->MeshComponent->SetCollisionEnabled (ECollisionEnabled::QueryOnly);
                Item.ItemActor->MeshComponent->SetCollisionResponseToAllChannels (ECR_Ignore);
                Item.ItemActor->MeshComponent->SetCollisionResponseToChannel (ECC_Visibility, ECR_Block);
            }
        }
    }
}

bool AGridReceptacleActor::IsContainedItemHitComponent (UPrimitiveComponent* HitComponent) const
{
    return FindContainedItemIndexForComponent (HitComponent) != INDEX_NONE;
}

int32 AGridReceptacleActor::FindContainedItemIndexForComponent (UPrimitiveComponent* HitComponent) const
{
    if (!HitComponent)
    {
        return INDEX_NONE;
    }
    for (int32 Index = 0; Index < ContainedItems.Num (); ++Index)
    {
        const AGridItemActor* ItemActor = ContainedItems[Index].ItemActor.Get ();
        if (!IsValid (ItemActor))
        {
            continue;
        }
        if (HitComponent == ItemActor->MeshComponent)
        {
            return Index;
        }
        if (HitComponent->GetOwner () == ItemActor)
        {
            return Index;
        }
    }
    return INDEX_NONE;
}

FString AGridReceptacleActor::GetItemAcceptanceFailureReason (FName ItemDefinitionId) const
{
    if (ItemDefinitionId.IsNone ())
    {
        return TEXT ("empty item id");
    }
    if (RejectedItemDefinitionIds.Contains (ItemDefinitionId))
    {
        return TEXT ("item is rejected");
    }
    if (!bAcceptAnyItem && !AcceptedItemDefinitionIds.Contains (ItemDefinitionId))
    {
        return TEXT ("item not in accepted ids");
    }
    if (IsFull ())
    {
        return TEXT ("receptacle is full");
    }
    if (!bCanInsertItem)
    {
        return TEXT ("insertion disabled");
    }
    return TEXT ("unknown");
}

void AGridReceptacleActor::ExecuteInsertionLinks ()
{
    if (AGridLevelRuntimeActor* RuntimeActor = FindRuntimeActor (GetWorld ()))
    {
        RuntimeActor->ExecuteLinksFromRuntimeObject (ObjectId, EGridObjectEvent::ItemInserted);
        RuntimeActor->ExecuteLinksFromRuntimeObject (ObjectId, EGridObjectEvent::ItemChanged);
    }
}

void AGridReceptacleActor::ExecuteRemovalLinks ()
{
    if (AGridLevelRuntimeActor* RuntimeActor = FindRuntimeActor (GetWorld ()))
    {
        RuntimeActor->ExecuteLinksFromRuntimeObject (ObjectId, EGridObjectEvent::ItemRemoved);
        RuntimeActor->ExecuteLinksFromRuntimeObject (ObjectId, EGridObjectEvent::ItemChanged);
    }
}

void AGridReceptacleActor::InitializeInitialContainedItems ()
{
    bInitialItemsInitialized = true;
    UE_LOG (LogTemp, Log,
        TEXT ("GridReceptacle InitializeInitialContainedItems Start ObjectId=%s InitialItems=%d ContainedItems=%d InitialDefinitionId=%s InitialArchetypeId=%s"),
        *ObjectId.ToString (),
        InitialContainedItems.Num (),
        ContainedItems.Num (),
        InitialContainedItems.Num () > 0 ? *InitialContainedItems[0].ItemDefinitionId.ToString () : TEXT ("None"),
        *InitialContainedItemArchetypeId.ToString ());

    for (const FGridInitialReceptacleItem& InitialItem : InitialContainedItems)
    {
        const FName ItemDefinitionId = ResolveInitialItemDefinitionId (InitialItem);
        if (ItemDefinitionId.IsNone ())
        {
            UE_LOG (LogTemp, Warning,
                TEXT ("GridReceptacle InitialItem skipped: ObjectId=%s InitialItems=%d ContainedItems=%d InitialDefinitionId=%s InitialArchetypeId=%s"),
                *ObjectId.ToString (),
                InitialContainedItems.Num (),
                ContainedItems.Num (),
                *InitialItem.ItemDefinitionId.ToString (),
                *InitialContainedItemArchetypeId.ToString ());
            continue;
        }
        if (WasInitialItemRemoved (ItemDefinitionId))
        {
            continue;
        }
        AddContainedItem (ItemDefinitionId, InitialItem.ItemDefinition, nullptr, true, InitialItem.Quantity);
    }

    UE_LOG (LogTemp, Log,
        TEXT ("GridReceptacle InitializeInitialContainedItems Finish ObjectId=%s InitialItems=%d ContainedItems=%d InitialDefinitionId=%s InitialArchetypeId=%s"),
        *ObjectId.ToString (),
        InitialContainedItems.Num (),
        ContainedItems.Num (),
        InitialContainedItems.Num () > 0 ? *InitialContainedItems[0].ItemDefinitionId.ToString () : TEXT ("None"),
        *InitialContainedItemArchetypeId.ToString ());
}

FName AGridReceptacleActor::ResolveInitialItemDefinitionId (const FGridInitialReceptacleItem& InitialItem) const
{
    return ResolveDefinitionId (InitialItem.ItemDefinition,
        InitialItem.ItemDefinitionId.IsNone () ? InitialItem.ItemArchetypeId : InitialItem.ItemDefinitionId);
}

bool AGridReceptacleActor::WasInitialItemRemoved (FName ItemDefinitionId) const
{
    return RemovedInitialItemDefinitionIds.Contains (ItemDefinitionId);
}
