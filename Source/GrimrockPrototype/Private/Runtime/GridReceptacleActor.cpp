#include "Runtime/GridReceptacleActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Runtime/GridInteractionUtils.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridItemTransferService.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    static constexpr float MultiItemVisualSpacing = 18.0f;

    FString JoinNames (const TArray<FName>& Names)
    {
        TArray<FString> Values;
        Values.Reserve (Names.Num ());
        for (const FName Name : Names)
        {
            Values.Add (Name.ToString ());
        }
        return FString::Join (Values, TEXT (","));
    }

    FString JoinItemTypes (const TArray<EGridItemType>& ItemTypes)
    {
        TArray<FString> Values;
        Values.Reserve (ItemTypes.Num ());
        for (const EGridItemType ItemType : ItemTypes)
        {
            Values.Add (UEnum::GetValueAsString (ItemType));
        }
        return FString::Join (Values, TEXT (","));
    }

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

    const TCHAR* GetRejectReasonName (EGridReceptacleRejectReason Reason)
    {
        switch (Reason)
        {
        case EGridReceptacleRejectReason::None:
            return TEXT ("none");
        case EGridReceptacleRejectReason::InvalidItem:
            return TEXT ("invalid item");
        case EGridReceptacleRejectReason::Full:
            return TEXT ("receptacle full");
        case EGridReceptacleRejectReason::InsertDisabled:
            return TEXT ("insert disabled");
        case EGridReceptacleRejectReason::ExplicitlyRejected:
            return TEXT ("rejected explicitly");
        case EGridReceptacleRejectReason::NoMatchingAcceptanceRule:
            return TEXT ("rejected because no rule matched");
        default:
            return TEXT ("unknown");
        }
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
    if (VisualPlacementMode == EGridReceptacleVisualPlacementMode::AttachedSocket)
    {
        VisualPlacementMode = bUsePhysicalPlacement
            ? EGridReceptacleVisualPlacementMode::PhysicalAtHit
            : EGridReceptacleVisualPlacementMode::AttachedSocket;
    }
    bUsePhysicalPlacement = VisualPlacementMode == EGridReceptacleVisualPlacementMode::PhysicalAtHit;
    StorageMode = MaxContainedItems == 1
        ? EGridReceptacleStorageMode::SingleSlot
        : EGridReceptacleStorageMode::MultiSlot;
    bExtinguishItemOnPhysicalPlacement = Params.bExtinguishItemOnPhysicalPlacement;
    PhysicalPlacementSurfaceOffset = Params.PhysicalPlacementSurfaceOffset;
    PhysicalPlacementInitialRotationOffset = Params.PhysicalPlacementInitialRotationOffset;

    if (MeshComponent && GetEffectiveVisualPlacementMode () == EGridReceptacleVisualPlacementMode::PhysicalAtHit)
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
    for (const FName AcceptedTag : Params.AcceptedItemTags)
    {
        AcceptedItemTags.AddUnique (AcceptedTag);
    }

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

    if (!bInitialItemsInitialized)
    {
        InitializeInitialContainedItems ();
    }
}

bool AGridReceptacleActor::HasItem () const
{
    return ContainedItems.Num () > 0;
}

bool AGridReceptacleActor::HasAnyItem () const
{
    return HasItem ();
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

bool AGridReceptacleActor::ContainsItemDefinition (FName ItemDefinitionId) const
{
    if (ItemDefinitionId.IsNone ())
    {
        return false;
    }

    return ContainedItems.ContainsByPredicate (
        [ItemDefinitionId] (const FGridContainedReceptacleItem& Item)
        {
            return Item.ItemDefinitionId == ItemDefinitionId;
        });
}

bool AGridReceptacleActor::ContainsItemTag (FName ItemTag) const
{
    if (ItemTag.IsNone ())
    {
        return false;
    }

    return ContainedItems.ContainsByPredicate (
        [ItemTag] (const FGridContainedReceptacleItem& Item)
        {
            return Item.ItemDefinition && Item.ItemDefinition->ItemTags.Contains (ItemTag);
        });
}

bool AGridReceptacleActor::ContainsItemType (EGridItemType ItemType) const
{
    return ContainedItems.ContainsByPredicate (
        [ItemType] (const FGridContainedReceptacleItem& Item)
        {
            return Item.ItemDefinition && Item.ItemDefinition->ItemType == ItemType;
        });
}

float AGridReceptacleActor::GetContainedTotalWeight () const
{
    float TotalWeight = 0.0f;
    for (const FGridContainedReceptacleItem& Item : ContainedItems)
    {
        TotalWeight += Item.Weight * FMath::Max (1, Item.Quantity);
    }
    return TotalWeight;
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
    FGridReceptacleAcceptanceResult Result;
    return EvaluateItemAcceptance (Item, Result);
}

bool AGridReceptacleActor::EvaluateItemAcceptance (
    const FGridItemInstance& Item,
    FGridReceptacleAcceptanceResult& OutResult,
    bool bLogDiagnostics) const
{
    OutResult = FGridReceptacleAcceptanceResult ();

    const AGridLevelRuntimeActor* RuntimeActor = FindRuntimeActor (GetWorld ());
    const UGridItemDefinitionAsset* ItemDefinition = RuntimeActor
        ? RuntimeActor->ResolveRuntimeItemDefinition (Item.ItemDefinitionId)
        : nullptr;
    if (!ItemDefinition)
    {
        const AGrimrockPartyPawn* PartyPawn = Cast<AGrimrockPartyPawn> (UGameplayStatics::GetPlayerPawn (this, 0));
        if (PartyPawn && PartyPawn->PartyInventoryComponent)
        {
            ItemDefinition = PartyPawn->PartyInventoryComponent->FindItemDefinition (Item.ItemDefinitionId);
        }
    }

    auto LogDiagnostic = [this, &Item, ItemDefinition, bLogDiagnostics] (const TCHAR* Outcome, const FString& Decision)
    {
        if (!bLogDiagnostics)
        {
            return;
        }

        UE_LOG (LogTemp, VeryVerbose,
            TEXT ("GridReceptacle Diagnostic Evaluate Outcome=%s Receptacle=%s ObjectId=%s ActorClass=%s "
                "AcceptAny=%s CanInsert=%s Count=%d Max=%d ItemPolicy=%s "
                "ItemDefinitionId=%s RuntimeObjectId=%s ItemDefinitionResolved=%s ItemDefinition=%s "
                "ItemTags=[%s] ItemType=%s AcceptedItemTags=[%s] AcceptedItemTypes=[%s] "
                "AcceptedItemDefinitionIds=[%s] RejectedItemDefinitionIds=[%s] %s"),
            Outcome,
            *GetName (),
            *ObjectId.ToString (),
            *GetClass ()->GetPathName (),
            bAcceptAnyItem ? TEXT ("true") : TEXT ("false"),
            bCanInsertItem ? TEXT ("true") : TEXT ("false"),
            ContainedItems.Num (),
            MaxContainedItems,
            *UEnum::GetValueAsString (ItemPolicy),
            *Item.ItemDefinitionId.ToString (),
            *Item.RuntimeObjectId.ToString (),
            ItemDefinition ? TEXT ("true") : TEXT ("false"),
            ItemDefinition ? *ItemDefinition->GetPathName () : TEXT ("None"),
            ItemDefinition ? *JoinNames (ItemDefinition->ItemTags) : TEXT (""),
            ItemDefinition ? *UEnum::GetValueAsString (ItemDefinition->ItemType) : TEXT ("Unresolved"),
            *JoinNames (AcceptedItemTags),
            *JoinItemTypes (AcceptedItemTypes),
            *JoinNames (AcceptedItemDefinitionIds),
            *JoinNames (RejectedItemDefinitionIds),
            *Decision);
    };

    auto Reject = [&OutResult, &LogDiagnostic, bLogDiagnostics] (EGridReceptacleRejectReason Reason)
    {
        OutResult.bAccepted = false;
        OutResult.RejectReason = Reason;
        if (bLogDiagnostics)
        {
            LogDiagnostic (
                TEXT ("Rejected"),
                FString::Printf (TEXT ("RejectReason=%s"), GetRejectReasonName (Reason)));
        }
        return false;
    };

    auto Accept = [&OutResult, &LogDiagnostic, bLogDiagnostics] (FName MatchedRule, const FString& MatchedValue)
    {
        OutResult.bAccepted = true;
        OutResult.RejectReason = EGridReceptacleRejectReason::None;
        OutResult.MatchedRule = MatchedRule;
        if (bLogDiagnostics)
        {
            LogDiagnostic (
                TEXT ("Accepted"),
                FString::Printf (
                    TEXT ("MatchedRule=\"%s\" MatchedValue=%s"),
                    *MatchedRule.ToString (),
                    *MatchedValue));
        }
        return true;
    };

    if (!Item.IsValid ())
    {
        return Reject (EGridReceptacleRejectReason::InvalidItem);
    }
    if (IsFull ())
    {
        return Reject (EGridReceptacleRejectReason::Full);
    }
    if (!bCanInsertItem)
    {
        return Reject (EGridReceptacleRejectReason::InsertDisabled);
    }
    if (RejectedItemDefinitionIds.Contains (Item.ItemDefinitionId))
    {
        return Reject (EGridReceptacleRejectReason::ExplicitlyRejected);
    }

    const bool bEffectiveAcceptAny =
        ItemPolicy == EGridReceptacleItemPolicy::AcceptAny ||
        (ItemPolicy != EGridReceptacleItemPolicy::Filtered && bAcceptAnyItem);
    if (bEffectiveAcceptAny)
    {
        return Accept (TEXT ("accepted by accept any"), TEXT ("true"));
    }
    if (AcceptedItemDefinitionIds.Contains (Item.ItemDefinitionId))
    {
        return Accept (TEXT ("accepted by definition id"), Item.ItemDefinitionId.ToString ());
    }

    if (ItemDefinition)
    {
        for (const FName AcceptedTag : AcceptedItemTags)
        {
            if (!AcceptedTag.IsNone () && ItemDefinition->ItemTags.Contains (AcceptedTag))
            {
                return Accept (TEXT ("accepted by tag"), AcceptedTag.ToString ());
            }
        }
        if (AcceptedItemTypes.Contains (ItemDefinition->ItemType))
        {
            return Accept (TEXT ("accepted by type"), UEnum::GetValueAsString (ItemDefinition->ItemType));
        }
    }

    return Reject (EGridReceptacleRejectReason::NoMatchingAcceptanceRule);
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
    if (ItemPolicy == EGridReceptacleItemPolicy::ConsumeOnInsert)
    {
        ConsumeItemAtIndex (NewIndex);
    }

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

    FGridReceptacleAcceptanceResult AcceptanceResult;
    if (!EvaluateItemAcceptance (CursorItem, AcceptanceResult))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("Receptacle cursor insert refused: ObjectId=%s Item=%s RuntimeId=%s Reason=%s"),
            *ObjectId.ToString (),
            *CursorItem.ItemDefinitionId.ToString (),
            *CursorItem.RuntimeObjectId.ToString (),
            GetRejectReasonName (AcceptanceResult.RejectReason));
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
        GetEffectiveVisualPlacementMode () == EGridReceptacleVisualPlacementMode::PhysicalAtHit &&
        bExtinguishItemOnPhysicalPlacement
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
    if (ItemPolicy == EGridReceptacleItemPolicy::ConsumeOnInsert)
    {
        ConsumeItemAtIndex (NewIndex);
    }

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

    if (!PartyPawn || !ContainedItems.IsValidIndex (ItemIndex))
    {
        return false;
    }
    if (!IsItemRemovalAllowed ())
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridReceptacle RemoveRejected ObjectId=%s ItemIndex=%d Policy=%s CanRemove=%s"),
            *ObjectId.ToString (),
            ItemIndex,
            *UEnum::GetValueAsString (ItemPolicy),
            bCanRemoveItem ? TEXT ("true") : TEXT ("false"));
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

bool AGridReceptacleActor::ConsumeItemAtIndex (int32 ItemIndex)
{
    if (!ContainedItems.IsValidIndex (ItemIndex))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridReceptacle ConsumeFailed ObjectId=%s ItemIndex=%d Reason=InvalidIndex Count=%d"),
            *ObjectId.ToString (),
            ItemIndex,
            ContainedItems.Num ());
        return false;
    }

    const FName ItemDefinitionId = ContainedItems[ItemIndex].ItemDefinitionId;
    const FGuid RuntimeObjectId = ContainedItems[ItemIndex].RuntimeObjectId;
    FGridContainedReceptacleItem ConsumedItem;
    if (!RemoveContainedItemAtIndex (ItemIndex, ConsumedItem))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridReceptacle ConsumeFailed ObjectId=%s Item=%s RuntimeId=%s Reason=RemoveFailed"),
            *ObjectId.ToString (),
            *ItemDefinitionId.ToString (),
            *RuntimeObjectId.ToString ());
        return false;
    }

    if (AGridLevelRuntimeActor* RuntimeActor = FindRuntimeActor (GetWorld ()))
    {
        RuntimeActor->ExecuteLinksFromRuntimeObject (ObjectId, EGridObjectEvent::ItemChanged);
    }
    UE_LOG (LogTemp, Log,
        TEXT ("GridReceptacle ItemConsumed ObjectId=%s Item=%s RuntimeId=%s Remaining=%d Policy=%s"),
        *ObjectId.ToString (),
        *ItemDefinitionId.ToString (),
        *RuntimeObjectId.ToString (),
        ContainedItems.Num (),
        *UEnum::GetValueAsString (ItemPolicy));
    return true;
}

bool AGridReceptacleActor::ConsumeAllItems ()
{
    bool bConsumedAny = false;
    while (ContainedItems.Num () > 0)
    {
        if (!ConsumeItemAtIndex (ContainedItems.Num () - 1))
        {
            return false;
        }
        bConsumedAny = true;
    }
    return bConsumedAny;
}

void AGridReceptacleActor::SetReceptacleItemPolicy (EGridReceptacleItemPolicy NewPolicy)
{
    const EGridReceptacleItemPolicy PreviousPolicy = ItemPolicy;
    ItemPolicy = NewPolicy;
    UE_LOG (LogTemp, Log,
        TEXT ("GridReceptacle PolicyChanged ObjectId=%s Previous=%s New=%s"),
        *ObjectId.ToString (),
        *UEnum::GetValueAsString (PreviousPolicy),
        *UEnum::GetValueAsString (ItemPolicy));
}

void AGridReceptacleActor::SetCanRemoveItem (bool bNewCanRemoveItem)
{
    const bool bPreviousCanRemoveItem = bCanRemoveItem;
    bCanRemoveItem = bNewCanRemoveItem;
    UE_LOG (LogTemp, Log,
        TEXT ("GridReceptacle RemovalChanged ObjectId=%s Previous=%s New=%s"),
        *ObjectId.ToString (),
        bPreviousCanRemoveItem ? TEXT ("true") : TEXT ("false"),
        bCanRemoveItem ? TEXT ("true") : TEXT ("false"));
}

void AGridReceptacleActor::SetCanInsertItem (bool bNewCanInsertItem)
{
    const bool bPreviousCanInsertItem = bCanInsertItem;
    bCanInsertItem = bNewCanInsertItem;
    UE_LOG (LogTemp, Log,
        TEXT ("GridReceptacle InsertionChanged ObjectId=%s Previous=%s New=%s"),
        *ObjectId.ToString (),
        bPreviousCanInsertItem ? TEXT ("true") : TEXT ("false"),
        bCanInsertItem ? TEXT ("true") : TEXT ("false"));
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
            TEXT ("GridReceptacle Interaction Route=\"fallback legacy cursor -> receptacle\" ObjectId=%s Item=%s RuntimeId=%s"),
            *ObjectId.ToString (),
            *CursorItem.ItemDefinitionId.ToString (),
            *CursorItem.RuntimeObjectId.ToString ());
        return PartyPawn->TryPlaceCursorItemInReceptacle (this);
    }

    if (GetEffectiveVisualPlacementMode () != EGridReceptacleVisualPlacementMode::PhysicalAtHit &&
        HasItem ())
    {
        if (!IsItemRemovalAllowed ())
        {
            UE_LOG (LogTemp, Warning,
                TEXT ("GridReceptacle Interaction RemoveRejected ObjectId=%s Policy=%s CanRemove=%s"),
                *ObjectId.ToString (),
                *UEnum::GetValueAsString (ItemPolicy),
                bCanRemoveItem ? TEXT ("true") : TEXT ("false"));
            return false;
        }
        UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent;
        const int32 CharacterIndex = Inventory ? Inventory->GetSelectedCharacterIndex () : INDEX_NONE;
        UE_LOG (LogTemp, Log,
            TEXT ("GridReceptacle Interaction Route=\"service transfer receptacle -> inventory\" ObjectId=%s ItemIndex=0 Character=%d"),
            *ObjectId.ToString (),
            CharacterIndex);

        const FGridItemTransferResult TransferResult =
            UGridItemTransferService::TransferReceptacleItemToInventory (
                this,
                0,
                Inventory,
                CharacterIndex);
        if (!TransferResult.bSuccess)
        {
            UE_LOG (LogTemp, Warning,
                TEXT ("GridReceptacle Interaction ServiceFailed Route=\"receptacle -> inventory\" ObjectId=%s Result=%s Message=%s"),
                *ObjectId.ToString (),
                *UEnum::GetValueAsString (TransferResult.Result),
                *TransferResult.Message.ToString ());
        }
        return TransferResult.bSuccess;
    }

    FGridItemInstance HeldItem;
    EGridEquipmentSlot HeldSlot = EGridEquipmentSlot::None;
    if (ResolveHeldEquipmentItem (PartyPawn, HeldItem, HeldSlot))
    {
        UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent;
        const int32 CharacterIndex = Inventory->GetSelectedCharacterIndex ();
        UE_LOG (LogTemp, Log,
            TEXT ("GridReceptacle Interaction Route=\"service transfer equipment -> receptacle\" ObjectId=%s Character=%d Slot=%d Item=%s RuntimeId=%s"),
            *ObjectId.ToString (),
            CharacterIndex,
            static_cast<int32> (HeldSlot),
            *HeldItem.ItemDefinitionId.ToString (),
            *HeldItem.RuntimeObjectId.ToString ());

        const FGridItemTransferResult TransferResult =
            UGridItemTransferService::TransferEquipmentSlotToReceptacle (
                Inventory,
                CharacterIndex,
                HeldSlot,
                this);
        if (!TransferResult.bSuccess)
        {
            UE_LOG (LogTemp, Warning,
                TEXT ("GridReceptacle Interaction ServiceFailed Route=\"equipment -> receptacle\" ObjectId=%s Result=%s Message=%s"),
                *ObjectId.ToString (),
                *UEnum::GetValueAsString (TransferResult.Result),
                *TransferResult.Message.ToString ());
        }
        return TransferResult.bSuccess;
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
    if (GetEffectiveVisualPlacementMode () == EGridReceptacleVisualPlacementMode::PhysicalAtHit)
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
        return HasItem () && IsItemRemovalAllowed ();
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
        return CanAcceptItemInstance (HeldItem);
    }

    return GetEffectiveVisualPlacementMode () != EGridReceptacleVisualPlacementMode::PhysicalAtHit &&
        HasItem () && IsItemRemovalAllowed ();
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
    if (GetEffectiveVisualPlacementMode () == EGridReceptacleVisualPlacementMode::PhysicalAtHit &&
        HitComponent == MeshComponent)
    {
        PendingPlacementHitResult = HitResult;
    }
    Interact_Implementation (InstigatorPawn, HitComponent);
    PendingPlacementHitResult.Reset ();
}

EGridInteractionCursor AGridReceptacleActor::GetInteractionCursor_Implementation (UPrimitiveComponent* HitComponent) const
{
    if (IsContainedItemHitComponent (HitComponent) && HasItem () && IsItemRemovalAllowed ())
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
    if (IsContainedItemHitComponent (HitComponent) && HasItem () && IsItemRemovalAllowed ())
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

    const EGridReceptacleVisualPlacementMode PlacementMode = GetEffectiveVisualPlacementMode ();
    if (PlacementMode == EGridReceptacleVisualPlacementMode::ContainerOnly)
    {
        if (IsValid (ItemActor))
        {
            ItemActor->Destroy ();
        }
        ItemActor = nullptr;
        ContainedItems[NewIndex].ItemActor = nullptr;
    }
    else if (!IsValid (ItemActor) && RuntimeActor)
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
    if (PlacementMode != EGridReceptacleVisualPlacementMode::ContainerOnly && !IsValid (ItemActor))
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
        FGridItemInstance PlacementItem;
        PlacementItem.RuntimeObjectId = ContainedItems[NewIndex].RuntimeObjectId;
        PlacementItem.ItemDefinitionId = ContainedItems[NewIndex].ItemDefinitionId;
        PlacementItem.Quantity = ContainedItems[NewIndex].Quantity;
        ApplyVisualPlacement (
            ContainedItems[NewIndex].ItemActor.Get (),
            PlacementItem,
            PendingPlacementHitResult.IsSet () ? &PendingPlacementHitResult.GetValue () : nullptr);
        if (PlacementMode == EGridReceptacleVisualPlacementMode::PhysicalAtHit)
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

EGridReceptacleVisualPlacementMode AGridReceptacleActor::GetEffectiveVisualPlacementMode () const
{
    if (VisualPlacementMode == EGridReceptacleVisualPlacementMode::AttachedSocket && bUsePhysicalPlacement)
    {
        return EGridReceptacleVisualPlacementMode::PhysicalAtHit;
    }

    return VisualPlacementMode;
}

bool AGridReceptacleActor::IsItemRemovalAllowed () const
{
    return bCanRemoveItem && ItemPolicy != EGridReceptacleItemPolicy::Locked;
}

void AGridReceptacleActor::ApplyVisualPlacement (
    AGridItemActor* ItemActor,
    const FGridItemInstance& Item,
    const FHitResult* OptionalHit)
{
    if (!IsValid (ItemActor) || !ItemAttachPoint)
    {
        return;
    }

    const EGridReceptacleVisualPlacementMode PlacementMode = GetEffectiveVisualPlacementMode ();
    ItemActor->SetOwner (this);
    if (PlacementMode == EGridReceptacleVisualPlacementMode::ContainerOnly)
    {
        ItemActor->SetActorHiddenInGame (true);
        ItemActor->SetActorEnableCollision (false);
        ItemActor->ConfigureAsAttachedItem ();
        return;
    }

    ItemActor->SetActorHiddenInGame (false);
    ItemActor->SetActorEnableCollision (true);

    if (PlacementMode == EGridReceptacleVisualPlacementMode::PhysicalAtHit)
    {
        ItemActor->DetachFromActor (FDetachmentTransformRules::KeepWorldTransform);
        const FVector PlacementLocation = OptionalHit && OptionalHit->bBlockingHit
            ? OptionalHit->ImpactPoint + OptionalHit->ImpactNormal * PhysicalPlacementSurfaceOffset
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
        if (ItemActor->MeshComponent && !bSimulatePhysicsWhenPlaced)
        {
            ItemActor->MeshComponent->SetSimulatePhysics (false);
            ItemActor->MeshComponent->SetEnableGravity (false);
            ItemActor->MeshComponent->SetCollisionEnabled (ECollisionEnabled::QueryOnly);
            ItemActor->MeshComponent->SetCollisionResponseToAllChannels (ECR_Ignore);
            ItemActor->MeshComponent->SetCollisionResponseToChannel (ECC_Visibility, ECR_Block);
        }
        return;
    }

    if (PlacementMode == EGridReceptacleVisualPlacementMode::DisplaySlots)
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridReceptacle DisplaySlots fallback to AttachedSocket Receptacle=%s ObjectId=%s Item=%s RuntimeId=%s"),
            *GetName (),
            *ObjectId.ToString (),
            *Item.ItemDefinitionId.ToString (),
            *Item.RuntimeObjectId.ToString ());
    }

    ItemActor->AttachToComponent (ItemAttachPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

    const int32 ItemIndex = FMath::Max (0, ContainedItems.IndexOfByPredicate (
        [&Item] (const FGridContainedReceptacleItem& ContainedItem)
        {
            return ContainedItem.RuntimeObjectId == Item.RuntimeObjectId;
        }));
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
            const EGridReceptacleVisualPlacementMode PlacementMode = GetEffectiveVisualPlacementMode ();
            if (PlacementMode == EGridReceptacleVisualPlacementMode::PhysicalAtHit)
            {
                Item.ItemActor->MeshComponent->SetCollisionEnabled (
                    bSimulatePhysicsWhenPlaced
                        ? ECollisionEnabled::QueryAndPhysics
                        : ECollisionEnabled::QueryOnly);
                if (bSimulatePhysicsWhenPlaced)
                {
                    Item.ItemActor->MeshComponent->SetCollisionProfileName (TEXT ("PhysicsActor"));
                }
                Item.ItemActor->MeshComponent->SetCollisionResponseToChannel (ECC_Visibility, ECR_Block);
            }
            else if (PlacementMode == EGridReceptacleVisualPlacementMode::ContainerOnly)
            {
                Item.ItemActor->SetActorHiddenInGame (true);
                Item.ItemActor->MeshComponent->SetCollisionEnabled (ECollisionEnabled::NoCollision);
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
