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

bool UGridInventoryWidget::HandleInventorySlotClicked (int32 SlotIndex)
{
    if (!InventoryComponent)
    {
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

    if (bResult)
    {
        RefreshInventory ();
    }
    return bResult;
}

bool UGridInventoryWidget::HandleMainHandClicked ()
{
    if (!OwningPartyPawn || !InventoryComponent)
    {
        return false;
    }

    const bool bCursorBefore = InventoryComponent->HasCursorItem ();
    const bool bResult = bCursorBefore
        ? OwningPartyPawn->TryEquipCursorItemToSelectedCharacterMainHand ()
        : OwningPartyPawn->TryTakeSelectedCharacterMainHandToCursor ();

    UE_LOG (LogTemp, Log, TEXT ("GridInventory UI MainHandClicked CursorBefore=%s Result=%s"),
        bCursorBefore ? TEXT ("true") : TEXT ("false"),
        bResult ? TEXT ("true") : TEXT ("false"));

    if (bResult)
    {
        RefreshInventory ();
    }
    return bResult;
}

bool UGridInventoryWidget::HandleOffHandClicked ()
{
    if (!OwningPartyPawn || !InventoryComponent)
    {
        return false;
    }

    const bool bCursorBefore = InventoryComponent->HasCursorItem ();
    const bool bResult = bCursorBefore
        ? OwningPartyPawn->TryEquipCursorItemToSelectedCharacterOffHand ()
        : OwningPartyPawn->TryTakeSelectedCharacterOffHandToCursor ();

    UE_LOG (LogTemp, Log, TEXT ("GridInventory UI OffHandClicked CursorBefore=%s Result=%s"),
        bCursorBefore ? TEXT ("true") : TEXT ("false"),
        bResult ? TEXT ("true") : TEXT ("false"));

    if (bResult)
    {
        RefreshInventory ();
    }
    return bResult;
}

bool UGridInventoryWidget::HandleCursorReturnToInventoryClicked ()
{
    if (!OwningPartyPawn)
    {
        return false;
    }

    const bool bResult = OwningPartyPawn->DebugPlaceCursorItemInSelectedInventory ();
    UE_LOG (LogTemp, Log, TEXT ("GridInventory UI CursorReturnToInventory Result=%s"),
        bResult ? TEXT ("true") : TEXT ("false"));

    if (bResult)
    {
        RefreshInventory ();
    }
    return bResult;
}
