#include "UI/GridInventoryWidget.h"

#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

void UGridInventoryWidget::InitializeInventoryWidget (AGrimrockPartyPawn* InPartyPawn)
{
    OwningPartyPawn = InPartyPawn;
    InventoryComponent = InPartyPawn ? InPartyPawn->PartyInventoryComponent : nullptr;
    RefreshInventory ();
}

void UGridInventoryWidget::RefreshInventory_Implementation ()
{
    UE_LOG (LogTemp, Log, TEXT ("GridInventory UI Refresh Pawn=%s InventoryComponent=%s"),
        *GetNameSafe (OwningPartyPawn),
        *GetNameSafe (InventoryComponent));
}

int32 UGridInventoryWidget::GetSelectedCharacterIndex () const
{
    return InventoryComponent ? InventoryComponent->GetSelectedCharacterIndex () : INDEX_NONE;
}

int32 UGridInventoryWidget::GetInventorySlotCount () const
{
    if (!InventoryComponent)
    {
        return 0;
    }

    const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex ();
    const FGridPartyInventoryState& State = InventoryComponent->PartyInventoryState;
    return State.ActiveCharacters.IsValidIndex (CharacterIndex)
        ? State.ActiveCharacters[CharacterIndex].InventorySlots.Num ()
        : 0;
}

bool UGridInventoryWidget::GetInventoryItemAtSlot (int32 SlotIndex, FGridItemInstance& OutItem) const
{
    OutItem = FGridItemInstance ();
    if (!InventoryComponent)
    {
        return false;
    }

    const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex ();
    const FGridPartyInventoryState& State = InventoryComponent->PartyInventoryState;
    if (!State.ActiveCharacters.IsValidIndex (CharacterIndex))
    {
        return false;
    }

    const FGridCharacterInventoryState& CharacterState = State.ActiveCharacters[CharacterIndex];
    if (!CharacterState.InventorySlots.IsValidIndex (SlotIndex) ||
        CharacterState.InventorySlots[SlotIndex].IsEmpty ())
    {
        return false;
    }

    OutItem = CharacterState.InventorySlots[SlotIndex].Item;
    return true;
}

bool UGridInventoryWidget::GetMainHandItem (FGridItemInstance& OutItem) const
{
    OutItem = FGridItemInstance ();
    if (!InventoryComponent)
    {
        return false;
    }
    return InventoryComponent->GetEquippedItem (
        InventoryComponent->GetSelectedCharacterIndex (),
        EGridEquipmentSlot::MainHand,
        OutItem);
}

bool UGridInventoryWidget::GetOffHandItem (FGridItemInstance& OutItem) const
{
    OutItem = FGridItemInstance ();
    if (!InventoryComponent)
    {
        return false;
    }
    return InventoryComponent->GetEquippedItem (
        InventoryComponent->GetSelectedCharacterIndex (),
        EGridEquipmentSlot::OffHand,
        OutItem);
}

bool UGridInventoryWidget::GetCursorItem (FGridItemInstance& OutItem) const
{
    OutItem = FGridItemInstance ();
    if (!InventoryComponent || !InventoryComponent->HasCursorItem ())
    {
        return false;
    }

    OutItem = InventoryComponent->GetCursorItem ();
    return true;
}

bool UGridInventoryWidget::HasCursorItem () const
{
    return InventoryComponent && InventoryComponent->HasCursorItem ();
}

FString UGridInventoryWidget::GetItemDisplayString (const FGridItemInstance& Item) const
{
    return Item.ItemDefinitionId.IsNone () ? FString (TEXT ("Empty")) : Item.ItemDefinitionId.ToString ();
}

FString UGridInventoryWidget::GetCursorItemDisplayText () const
{
    FGridItemInstance Item;
    GetCursorItem (Item);
    return FString::Printf (TEXT ("Cursor: %s"), *GetItemDisplayString (Item));
}

FString UGridInventoryWidget::GetMainHandDisplayText () const
{
    FGridItemInstance Item;
    GetMainHandItem (Item);
    return FString::Printf (TEXT ("MainHand: %s"), *GetItemDisplayString (Item));
}

FString UGridInventoryWidget::GetOffHandDisplayText () const
{
    FGridItemInstance Item;
    GetOffHandItem (Item);
    return FString::Printf (TEXT ("OffHand: %s"), *GetItemDisplayString (Item));
}

FString UGridInventoryWidget::GetInventorySlotDisplayText (int32 SlotIndex) const
{
    FGridItemInstance Item;
    GetInventoryItemAtSlot (SlotIndex, Item);
    return FString::Printf (TEXT ("Slot %d: %s"), SlotIndex, *GetItemDisplayString (Item));
}

bool UGridInventoryWidget::HandleInventorySlotClicked (int32 SlotIndex)
{
    if (!InventoryComponent)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory UI SlotClicked Slot=%d CursorBefore=false Result=false Reason=NoInventoryComponent"),
            SlotIndex);
        RefreshInventory ();
        return false;
    }

    const bool bCursorBefore = InventoryComponent->HasCursorItem ();
    const int32 CharacterIndex = InventoryComponent->GetSelectedCharacterIndex ();
    const bool bResult = bCursorBefore
        ? InventoryComponent->TryPlaceCursorItemInSelectedCharacterInventory ()
        : InventoryComponent->TryTakeInventorySlotToCursor (CharacterIndex, SlotIndex);

    UE_LOG (LogTemp, Log, TEXT ("GridInventory UI SlotClicked Slot=%d CursorBefore=%s Result=%s"),
        SlotIndex,
        bCursorBefore ? TEXT ("true") : TEXT ("false"),
        bResult ? TEXT ("true") : TEXT ("false"));

    RefreshInventory ();
    return bResult;
}

bool UGridInventoryWidget::HandleMainHandClicked ()
{
    if (!OwningPartyPawn || !InventoryComponent)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory UI MainHandClicked CursorBefore=false Result=false Reason=MissingPawnOrInventoryComponent"));
        RefreshInventory ();
        return false;
    }

    const bool bCursorBefore = InventoryComponent->HasCursorItem ();
    const bool bResult = bCursorBefore
        ? OwningPartyPawn->TryEquipCursorItemToSelectedCharacterMainHand ()
        : OwningPartyPawn->TryTakeSelectedCharacterMainHandToCursor ();

    UE_LOG (LogTemp, Log, TEXT ("GridInventory UI MainHandClicked CursorBefore=%s Result=%s"),
        bCursorBefore ? TEXT ("true") : TEXT ("false"),
        bResult ? TEXT ("true") : TEXT ("false"));

    RefreshInventory ();
    return bResult;
}

bool UGridInventoryWidget::HandleOffHandClicked ()
{
    if (!OwningPartyPawn || !InventoryComponent)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory UI OffHandClicked CursorBefore=false Result=false Reason=MissingPawnOrInventoryComponent"));
        RefreshInventory ();
        return false;
    }

    const bool bCursorBefore = InventoryComponent->HasCursorItem ();
    const bool bResult = bCursorBefore
        ? OwningPartyPawn->TryEquipCursorItemToSelectedCharacterOffHand ()
        : OwningPartyPawn->TryTakeSelectedCharacterOffHandToCursor ();

    UE_LOG (LogTemp, Log, TEXT ("GridInventory UI OffHandClicked CursorBefore=%s Result=%s"),
        bCursorBefore ? TEXT ("true") : TEXT ("false"),
        bResult ? TEXT ("true") : TEXT ("false"));

    RefreshInventory ();
    return bResult;
}

bool UGridInventoryWidget::HandleCursorReturnToInventoryClicked ()
{
    if (!OwningPartyPawn || !InventoryComponent)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory UI CursorReturnToInventory CursorBefore=false Result=false Reason=MissingPawnOrInventoryComponent"));
        RefreshInventory ();
        return false;
    }

    const bool bCursorBefore = InventoryComponent->HasCursorItem ();
    if (!bCursorBefore)
    {
        UE_LOG (LogTemp, Log, TEXT ("GridInventory UI CursorReturnToInventory CursorBefore=false Result=false"));
        RefreshInventory ();
        return false;
    }

    const bool bResult = OwningPartyPawn->DebugPlaceCursorItemInSelectedInventory ();
    UE_LOG (LogTemp, Log, TEXT ("GridInventory UI CursorReturnToInventory CursorBefore=true Result=%s"),
        bResult ? TEXT ("true") : TEXT ("false"));

    RefreshInventory ();
    return bResult;
}
