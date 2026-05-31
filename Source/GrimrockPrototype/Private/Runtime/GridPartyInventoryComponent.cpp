#include "Runtime/GridPartyInventoryComponent.h"

#include "Runtime/GridItemDefinitionAsset.h"

namespace
{
    constexpr float CarryWeightPerStrength = 5.0f;

    int32 CountOccupiedSlots (const FGridCharacterInventoryState& CharacterState)
    {
        int32 OccupiedCount = 0;
        for (const FGridInventorySlot& Slot : CharacterState.InventorySlots)
        {
            if (!Slot.IsEmpty ())
            {
                ++OccupiedCount;
            }
        }
        return OccupiedCount;
    }

    float GetItemTotalWeight (const FGridItemInstance& Item)
    {
        return Item.IsValid () ? Item.Weight * FMath::Max (1, Item.Quantity) : 0.0f;
    }

    bool IsSupportedEquipmentSlot (EGridEquipmentSlot Slot)
    {
        return Slot == EGridEquipmentSlot::MainHand || Slot == EGridEquipmentSlot::OffHand;
    }

    const TCHAR* GetEquipmentSlotName (EGridEquipmentSlot Slot)
    {
        switch (Slot)
        {
        case EGridEquipmentSlot::MainHand:
            return TEXT ("MainHand");
        case EGridEquipmentSlot::OffHand:
            return TEXT ("OffHand");
        default:
            return TEXT ("Unsupported");
        }
    }

    const TCHAR* GetOwnerTypeName (EGridItemOwnerType OwnerType)
    {
        switch (OwnerType)
        {
        case EGridItemOwnerType::World:
            return TEXT ("World");
        case EGridItemOwnerType::Receptacle:
            return TEXT ("Receptacle");
        case EGridItemOwnerType::CharacterInventory:
            return TEXT ("CharacterInventory");
        case EGridItemOwnerType::EquipmentSlot:
            return TEXT ("EquipmentSlot");
        case EGridItemOwnerType::Cursor:
            return TEXT ("Cursor");
        case EGridItemOwnerType::HeldBySelectedCharacter:
            return TEXT ("HeldBySelectedCharacter");
        case EGridItemOwnerType::Removed:
            return TEXT ("Removed");
        default:
            return TEXT ("None");
        }
    }

    const TCHAR* GetItemTypeName (EGridItemType ItemType)
    {
        switch (ItemType)
        {
        case EGridItemType::Torch:
            return TEXT ("Torch");
        case EGridItemType::Weapon:
            return TEXT ("Weapon");
        case EGridItemType::Shield:
            return TEXT ("Shield");
        case EGridItemType::Armor:
            return TEXT ("Armor");
        case EGridItemType::Jewelry:
            return TEXT ("Jewelry");
        case EGridItemType::Key:
            return TEXT ("Key");
        case EGridItemType::Gem:
            return TEXT ("Gem");
        case EGridItemType::Potion:
            return TEXT ("Potion");
        case EGridItemType::Scroll:
            return TEXT ("Scroll");
        case EGridItemType::Book:
            return TEXT ("Book");
        case EGridItemType::Food:
            return TEXT ("Food");
        case EGridItemType::Component:
            return TEXT ("Component");
        case EGridItemType::Quest:
            return TEXT ("Quest");
        case EGridItemType::Misc:
            return TEXT ("Misc");
        default:
            return TEXT ("None");
        }
    }

    FString GetEquipmentSlotsText (const TArray<EGridEquipmentSlot>& Slots)
    {
        if (Slots.Num () == 0)
        {
            return TEXT ("None");
        }

        FString Result;
        for (int32 Index = 0; Index < Slots.Num (); ++Index)
        {
            if (Index > 0)
            {
                Result += TEXT (",");
            }
            Result += GetEquipmentSlotName (Slots[Index]);
        }
        return Result;
    }

    int32 FindFreeInventorySlotIndex (const FGridCharacterInventoryState& CharacterState)
    {
        for (int32 SlotIndex = 0; SlotIndex < CharacterState.InventorySlots.Num (); ++SlotIndex)
        {
            if (CharacterState.InventorySlots[SlotIndex].IsEmpty ())
            {
                return SlotIndex;
            }
        }

        return INDEX_NONE;
    }
}

UGridPartyInventoryComponent::UGridPartyInventoryComponent ()
{
    PrimaryComponentTick.bCanEverTick = false;
    PartyInventoryState.MaxActiveCharacters = DefaultMaxActiveCharacters;
}

void UGridPartyInventoryComponent::InitializeDefaultPartyIfNeeded ()
{
    PartyInventoryState.MaxActiveCharacters = FMath::Max (1, DefaultMaxActiveCharacters);

    if (PartyInventoryState.ActiveCharacters.Num () == 0)
    {
        FGridCharacterInventoryState DefaultCharacter;
        InitializeCharacterDefaults (DefaultCharacter, 0);
        PartyInventoryState.ActiveCharacters.Add (DefaultCharacter);
    }

    for (int32 CharacterIndex = 0; CharacterIndex < PartyInventoryState.ActiveCharacters.Num (); ++CharacterIndex)
    {
        InitializeCharacterDefaults (PartyInventoryState.ActiveCharacters[CharacterIndex], CharacterIndex);
    }

    EnsureEquipmentCountMatchesActiveCharacters ();

    if (!IsValidCharacterIndex (PartyInventoryState.SelectedCharacterIndex))
    {
        PartyInventoryState.SelectedCharacterIndex = PartyInventoryState.ActiveCharacters.Num () > 0 ? 0 : INDEX_NONE;
    }

    RecalculateAllWeights ();
}

int32 UGridPartyInventoryComponent::GetActiveCharacterCount () const
{
    return PartyInventoryState.ActiveCharacters.Num ();
}

int32 UGridPartyInventoryComponent::GetMaxActiveCharacters () const
{
    return PartyInventoryState.MaxActiveCharacters;
}

int32 UGridPartyInventoryComponent::GetSelectedCharacterIndex () const
{
    return PartyInventoryState.SelectedCharacterIndex;
}

bool UGridPartyInventoryComponent::SetSelectedCharacterIndex (int32 NewIndex)
{
    if (!IsValidCharacterIndex (NewIndex))
    {
        return false;
    }

    PartyInventoryState.SelectedCharacterIndex = NewIndex;
    return true;
}

bool UGridPartyInventoryComponent::IsValidCharacterIndex (int32 Index) const
{
    return PartyInventoryState.IsValidActiveCharacterIndex (Index);
}

bool UGridPartyInventoryComponent::CanAddItemToCharacterInventory (int32 CharacterIndex, const FGridItemInstance& Item) const
{
    if (!IsValidCharacterIndex (CharacterIndex) || !Item.IsValid ())
    {
        return false;
    }

    const FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
    for (const FGridInventorySlot& Slot : CharacterState.InventorySlots)
    {
        if (Slot.IsEmpty ())
        {
            return true;
        }
    }

    return false;
}

bool UGridPartyInventoryComponent::CanAddItemToSelectedCharacterInventory (const FGridItemInstance& Item) const
{
    return CanAddItemToCharacterInventory (PartyInventoryState.SelectedCharacterIndex, Item);
}

bool UGridPartyInventoryComponent::AddItemToCharacterInventory (int32 CharacterIndex, const FGridItemInstance& Item)
{
    FGridItemInstance ItemToAdd = Item;
    ApplyItemDefinitionToInstance (ItemToAdd);

    if (!CanAddItemToCharacterInventory (CharacterIndex, ItemToAdd))
    {
        return false;
    }

    FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
    for (FGridInventorySlot& Slot : CharacterState.InventorySlots)
    {
        if (!Slot.IsEmpty ())
        {
            continue;
        }

        Slot.bOccupied = true;
        Slot.Item = ItemToAdd;
        Slot.Item.OwnerType = EGridItemOwnerType::CharacterInventory;
        Slot.Item.OwnerGuid = CharacterState.CharacterId;
        Slot.Item.OwnerCharacterIndex = CharacterIndex;
        Slot.Item.EquipmentSlot = EGridEquipmentSlot::None;
        RecalculateCharacterWeight (CharacterIndex);
        return true;
    }

    return false;
}

bool UGridPartyInventoryComponent::AddItemToSelectedCharacterInventory (const FGridItemInstance& Item)
{
    return AddItemToCharacterInventory (PartyInventoryState.SelectedCharacterIndex, Item);
}

bool UGridPartyInventoryComponent::RemoveItemFromCharacterInventoryByRuntimeId (
    int32 CharacterIndex,
    FGuid RuntimeObjectId,
    FGridItemInstance& OutRemovedItem)
{
    OutRemovedItem = FGridItemInstance ();
    if (!IsValidCharacterIndex (CharacterIndex) || !RuntimeObjectId.IsValid ())
    {
        return false;
    }

    FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
    for (FGridInventorySlot& Slot : CharacterState.InventorySlots)
    {
        if (Slot.IsEmpty () || Slot.Item.RuntimeObjectId != RuntimeObjectId)
        {
            continue;
        }

        OutRemovedItem = Slot.Item;
        Slot = FGridInventorySlot ();
        RecalculateCharacterWeight (CharacterIndex);
        return true;
    }

    return false;
}

bool UGridPartyInventoryComponent::RemoveFirstItemFromCharacterInventoryByDefinitionId (
    int32 CharacterIndex,
    FName ItemDefinitionId,
    FGridItemInstance& OutRemovedItem)
{
    OutRemovedItem = FGridItemInstance ();
    if (!IsValidCharacterIndex (CharacterIndex) || ItemDefinitionId.IsNone ())
    {
        return false;
    }

    FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
    for (FGridInventorySlot& Slot : CharacterState.InventorySlots)
    {
        if (Slot.IsEmpty () || Slot.Item.ItemDefinitionId != ItemDefinitionId)
        {
            continue;
        }

        OutRemovedItem = Slot.Item;
        Slot = FGridInventorySlot ();
        RecalculateCharacterWeight (CharacterIndex);
        return true;
    }

    return false;
}

bool UGridPartyInventoryComponent::RemoveFirstItemFromSelectedCharacterInventoryByDefinitionId (
    FName ItemDefinitionId,
    FGridItemInstance& OutRemovedItem)
{
    return RemoveFirstItemFromCharacterInventoryByDefinitionId (
        PartyInventoryState.SelectedCharacterIndex,
        ItemDefinitionId,
        OutRemovedItem);
}

bool UGridPartyInventoryComponent::HasItemDefinitionInCharacterInventory (int32 CharacterIndex, FName ItemDefinitionId) const
{
    return CountItemDefinitionInCharacterInventory (CharacterIndex, ItemDefinitionId) > 0;
}

bool UGridPartyInventoryComponent::HasItemDefinitionInSelectedCharacterInventory (FName ItemDefinitionId) const
{
    return HasItemDefinitionInCharacterInventory (PartyInventoryState.SelectedCharacterIndex, ItemDefinitionId);
}

int32 UGridPartyInventoryComponent::CountItemDefinitionInCharacterInventory (int32 CharacterIndex, FName ItemDefinitionId) const
{
    if (!IsValidCharacterIndex (CharacterIndex) || ItemDefinitionId.IsNone ())
    {
        return 0;
    }

    int32 Count = 0;
    const FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
    for (const FGridInventorySlot& Slot : CharacterState.InventorySlots)
    {
        if (!Slot.IsEmpty () && Slot.Item.ItemDefinitionId == ItemDefinitionId)
        {
            Count += FMath::Max (1, Slot.Item.Quantity);
        }
    }
    return Count;
}

int32 UGridPartyInventoryComponent::CountItemDefinitionInSelectedCharacterInventory (FName ItemDefinitionId) const
{
    return CountItemDefinitionInCharacterInventory (PartyInventoryState.SelectedCharacterIndex, ItemDefinitionId);
}

bool UGridPartyInventoryComponent::RemoveItemDefinitionFromCharacterInventory (int32 CharacterIndex, FName ItemDefinitionId, int32 Quantity)
{
    if (!IsValidCharacterIndex (CharacterIndex) || ItemDefinitionId.IsNone () || Quantity <= 0)
    {
        return false;
    }

    if (CountItemDefinitionInCharacterInventory (CharacterIndex, ItemDefinitionId) < Quantity)
    {
        return false;
    }

    int32 RemainingToRemove = Quantity;
    FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
    for (FGridInventorySlot& Slot : CharacterState.InventorySlots)
    {
        if (RemainingToRemove <= 0)
        {
            break;
        }
        if (Slot.IsEmpty () || Slot.Item.ItemDefinitionId != ItemDefinitionId)
        {
            continue;
        }

        const int32 SlotQuantity = FMath::Max (1, Slot.Item.Quantity);
        if (SlotQuantity > RemainingToRemove)
        {
            Slot.Item.Quantity = SlotQuantity - RemainingToRemove;
            RemainingToRemove = 0;
        }
        else
        {
            RemainingToRemove -= SlotQuantity;
            Slot = FGridInventorySlot ();
        }
    }

    RecalculateCharacterWeight (CharacterIndex);
    return RemainingToRemove == 0;
}

bool UGridPartyInventoryComponent::RemoveItemDefinitionFromSelectedCharacterInventory (FName ItemDefinitionId, int32 Quantity)
{
    return RemoveItemDefinitionFromCharacterInventory (PartyInventoryState.SelectedCharacterIndex, ItemDefinitionId, Quantity);
}

UGridItemDefinitionAsset* UGridPartyInventoryComponent::FindItemDefinition (FName ItemDefinitionId) const
{
    if (ItemDefinitionId.IsNone ())
    {
        return nullptr;
    }

    for (const TObjectPtr<UGridItemDefinitionAsset>& Definition : ItemDefinitions)
    {
        if (Definition && Definition->ItemDefinitionId == ItemDefinitionId)
        {
            return Definition.Get ();
        }
    }

    return nullptr;
}

bool UGridPartyInventoryComponent::ApplyItemDefinitionToInstance (FGridItemInstance& ItemInstance) const
{
    UGridItemDefinitionAsset* Definition = FindItemDefinition (ItemInstance.ItemDefinitionId);
    if (!Definition)
    {
        UE_LOG (LogTemp, Verbose, TEXT ("GridInventory ItemDefinition Missing ItemDefinitionId=%s"),
            *ItemInstance.ItemDefinitionId.ToString ());
        return false;
    }

    ItemInstance.Weight = Definition->Weight;
    if (ItemInstance.DisplayName.IsEmpty ())
    {
        ItemInstance.DisplayName = Definition->DisplayName;
    }
    if (Definition->bCanEmitLight)
    {
        ItemInstance.bLightsEnabled = Definition->bDefaultLightEnabled;
    }
    if (!Definition->bStackable)
    {
        ItemInstance.Quantity = 1;
    }
    else
    {
        ItemInstance.Quantity = FMath::Clamp (ItemInstance.Quantity, 1, FMath::Max (1, Definition->MaxStackSize));
    }

    UE_LOG (LogTemp, Verbose, TEXT ("GridInventory ItemDefinition Applied Item=%s Weight=%.2f Type=%d"),
        *ItemInstance.ItemDefinitionId.ToString (),
        ItemInstance.Weight,
        static_cast<int32> (Definition->ItemType));
    return true;
}

bool UGridPartyInventoryComponent::CanEquipItemToSlot (
    int32 CharacterIndex,
    const FGridItemInstance& Item,
    EGridEquipmentSlot TargetSlot) const
{
    if (!IsValidCharacterIndex (CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex (CharacterIndex))
    {
        return false;
    }

    if (!Item.IsValid ())
    {
        return false;
    }

    if (const UGridItemDefinitionAsset* Definition = FindItemDefinition (Item.ItemDefinitionId))
    {
        return Definition->CanEquipToSlot (TargetSlot);
    }

    if (!IsSupportedEquipmentSlot (TargetSlot))
    {
        return false;
    }

    UE_LOG (LogTemp, Verbose, TEXT ("GridInventory Equip Compatibility Fallback Item=%s Slot=%s"),
        *Item.ItemDefinitionId.ToString (),
        GetEquipmentSlotName (TargetSlot));

    return true;
}

bool UGridPartyInventoryComponent::EquipItemFromInventorySlot (
    int32 CharacterIndex,
    int32 InventorySlotIndex,
    EGridEquipmentSlot TargetSlot)
{
    EnsureEquipmentCountMatchesActiveCharacters ();

    if (!IsValidCharacterIndex (CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex (CharacterIndex))
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Equip Failed Character=%d Slot=%s Reason=InvalidCharacter"),
            CharacterIndex,
            GetEquipmentSlotName (TargetSlot));
        return false;
    }

    FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
    if (!CharacterState.InventorySlots.IsValidIndex (InventorySlotIndex) || CharacterState.InventorySlots[InventorySlotIndex].IsEmpty ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Equip Failed Character=%d Slot=%s Reason=InvalidInventorySlot"),
            CharacterIndex,
            GetEquipmentSlotName (TargetSlot));
        return false;
    }

    FGridInventorySlot& InventorySlot = CharacterState.InventorySlots[InventorySlotIndex];
    FGridItemInstance ItemToEquip = InventorySlot.Item;
    if (!CanEquipItemToSlot (CharacterIndex, ItemToEquip, TargetSlot))
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Equip Failed Character=%d Slot=%s Reason=UnsupportedSlot Item=%s"),
            CharacterIndex,
            GetEquipmentSlotName (TargetSlot),
            *ItemToEquip.ItemDefinitionId.ToString ());
        return false;
    }

    FGridCharacterEquipmentState& EquipmentState = PartyInventoryState.ActiveEquipment[CharacterIndex];
    FGridItemInstance* TargetItem = EquipmentState.GetMutableSlot (TargetSlot);
    if (!TargetItem)
    {
        return false;
    }

    FGridItemInstance PreviouslyEquippedItem = *TargetItem;
    ItemToEquip.OwnerType = EGridItemOwnerType::EquipmentSlot;
    ItemToEquip.OwnerGuid = CharacterState.CharacterId;
    ItemToEquip.OwnerCharacterIndex = CharacterIndex;
    ItemToEquip.EquipmentSlot = TargetSlot;

    if (PreviouslyEquippedItem.IsValid ())
    {
        PreviouslyEquippedItem.OwnerType = EGridItemOwnerType::CharacterInventory;
        PreviouslyEquippedItem.OwnerGuid = CharacterState.CharacterId;
        PreviouslyEquippedItem.OwnerCharacterIndex = CharacterIndex;
        PreviouslyEquippedItem.EquipmentSlot = EGridEquipmentSlot::None;
        InventorySlot.Item = PreviouslyEquippedItem;
        InventorySlot.bOccupied = true;
    }
    else
    {
        InventorySlot = FGridInventorySlot ();
    }

    *TargetItem = ItemToEquip;
    RecalculateCharacterWeight (CharacterIndex);

    UE_LOG (LogTemp, Log, TEXT ("GridInventory Equip Character=%d Slot=%s Item=%s RuntimeId=%s Result=true"),
        CharacterIndex,
        GetEquipmentSlotName (TargetSlot),
        *ItemToEquip.ItemDefinitionId.ToString (),
        *ItemToEquip.RuntimeObjectId.ToString ());
    return true;
}

bool UGridPartyInventoryComponent::UnequipItemToInventory (int32 CharacterIndex, EGridEquipmentSlot SourceSlot)
{
    EnsureEquipmentCountMatchesActiveCharacters ();

    if (!IsValidCharacterIndex (CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex (CharacterIndex))
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Unequip Failed Character=%d Slot=%s Reason=InvalidCharacter"),
            CharacterIndex,
            GetEquipmentSlotName (SourceSlot));
        return false;
    }

    if (!IsSupportedEquipmentSlot (SourceSlot))
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Unequip Failed Character=%d Slot=%s Reason=UnsupportedSlot"),
            CharacterIndex,
            GetEquipmentSlotName (SourceSlot));
        return false;
    }

    FGridCharacterEquipmentState& EquipmentState = PartyInventoryState.ActiveEquipment[CharacterIndex];
    FGridItemInstance* EquippedItem = EquipmentState.GetMutableSlot (SourceSlot);
    if (!EquippedItem || !EquippedItem->IsValid ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Unequip Failed Character=%d Slot=%s Reason=EmptySlot"),
            CharacterIndex,
            GetEquipmentSlotName (SourceSlot));
        return false;
    }

    FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
    const int32 FreeSlotIndex = FindFreeInventorySlotIndex (CharacterState);
    if (FreeSlotIndex == INDEX_NONE)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Unequip Failed Character=%d Slot=%s Reason=InventoryFull"),
            CharacterIndex,
            GetEquipmentSlotName (SourceSlot));
        return false;
    }

    FGridItemInstance ItemToInventory = *EquippedItem;
    ItemToInventory.OwnerType = EGridItemOwnerType::CharacterInventory;
    ItemToInventory.OwnerGuid = CharacterState.CharacterId;
    ItemToInventory.OwnerCharacterIndex = CharacterIndex;
    ItemToInventory.EquipmentSlot = EGridEquipmentSlot::None;

    CharacterState.InventorySlots[FreeSlotIndex].bOccupied = true;
    CharacterState.InventorySlots[FreeSlotIndex].Item = ItemToInventory;
    *EquippedItem = FGridItemInstance ();
    RecalculateCharacterWeight (CharacterIndex);

    UE_LOG (LogTemp, Log, TEXT ("GridInventory Unequip Character=%d Slot=%s Item=%s RuntimeId=%s Result=true"),
        CharacterIndex,
        GetEquipmentSlotName (SourceSlot),
        *ItemToInventory.ItemDefinitionId.ToString (),
        *ItemToInventory.RuntimeObjectId.ToString ());
    return true;
}

bool UGridPartyInventoryComponent::GetEquippedItem (
    int32 CharacterIndex,
    EGridEquipmentSlot Slot,
    FGridItemInstance& OutItem) const
{
    OutItem = FGridItemInstance ();
    if (!IsValidCharacterIndex (CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex (CharacterIndex))
    {
        return false;
    }

    const FGridItemInstance* Item = PartyInventoryState.ActiveEquipment[CharacterIndex].GetSlot (Slot);
    if (!Item || !Item->IsValid ())
    {
        return false;
    }

    OutItem = *Item;
    return true;
}

bool UGridPartyInventoryComponent::IsEquipmentSlotOccupied (int32 CharacterIndex, EGridEquipmentSlot Slot) const
{
    if (!IsValidCharacterIndex (CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex (CharacterIndex))
    {
        return false;
    }

    return PartyInventoryState.ActiveEquipment[CharacterIndex].IsSlotOccupied (Slot);
}

FString UGridPartyInventoryComponent::GetEquipmentDiagnosticsForCharacter (int32 CharacterIndex) const
{
    if (!IsValidCharacterIndex (CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex (CharacterIndex))
    {
        return TEXT ("    Equipment: MainHand=None OffHand=None");
    }

    const FGridCharacterEquipmentState& EquipmentState = PartyInventoryState.ActiveEquipment[CharacterIndex];
    return FString::Printf (
        TEXT ("    Equipment: MainHand=%s OffHand=%s"),
        EquipmentState.MainHand.IsValid () ? *EquipmentState.MainHand.ItemDefinitionId.ToString () : TEXT ("None"),
        EquipmentState.OffHand.IsValid () ? *EquipmentState.OffHand.ItemDefinitionId.ToString () : TEXT ("None"));
}

bool UGridPartyInventoryComponent::SetCursorItem (const FGridItemInstance& Item)
{
    if (!Item.IsValid ())
    {
        return false;
    }

    PartyInventoryState.CursorItem = Item;
    PartyInventoryState.CursorItem.OwnerType = EGridItemOwnerType::Cursor;
    PartyInventoryState.CursorItem.OwnerCharacterIndex = INDEX_NONE;
    PartyInventoryState.bHasCursorItem = true;
    return true;
}

bool UGridPartyInventoryComponent::ClearCursorItem ()
{
    if (!PartyInventoryState.bHasCursorItem)
    {
        return false;
    }

    PartyInventoryState.CursorItem = FGridItemInstance ();
    PartyInventoryState.bHasCursorItem = false;
    return true;
}

bool UGridPartyInventoryComponent::HasCursorItem () const
{
    return PartyInventoryState.bHasCursorItem && PartyInventoryState.CursorItem.IsValid ();
}

void UGridPartyInventoryComponent::RecalculateCharacterWeight (int32 CharacterIndex)
{
    if (!IsValidCharacterIndex (CharacterIndex))
    {
        return;
    }

    FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
    float TotalWeight = 0.0f;

    for (const FGridInventorySlot& Slot : CharacterState.InventorySlots)
    {
        if (!Slot.IsEmpty ())
        {
            TotalWeight += GetItemTotalWeight (Slot.Item);
        }
    }

    if (PartyInventoryState.ActiveEquipment.IsValidIndex (CharacterIndex))
    {
        TotalWeight += CalculateEquipmentWeight (PartyInventoryState.ActiveEquipment[CharacterIndex]);
    }

    CharacterState.MaxCarryWeight = CharacterState.Strength * CarryWeightPerStrength;
    CharacterState.CurrentWeight = TotalWeight;
}

void UGridPartyInventoryComponent::RecalculateAllWeights ()
{
    for (int32 CharacterIndex = 0; CharacterIndex < PartyInventoryState.ActiveCharacters.Num (); ++CharacterIndex)
    {
        RecalculateCharacterWeight (CharacterIndex);
    }
}

FString UGridPartyInventoryComponent::GetPartyInventoryDiagnostics () const
{
    FString Result;
    Result += TEXT ("GridPartyInventory Diagnostics\n");
    Result += FString::Printf (
        TEXT ("ActiveCharacters=%d MaxActiveCharacters=%d SelectedCharacter=%d CursorItem=%s CharacterPool=%d\n"),
        PartyInventoryState.ActiveCharacters.Num (),
        PartyInventoryState.MaxActiveCharacters,
        PartyInventoryState.SelectedCharacterIndex,
        HasCursorItem () ? *PartyInventoryState.CursorItem.ItemDefinitionId.ToString () : TEXT ("None"),
        PartyInventoryState.CharacterPool.Num ());

    for (int32 CharacterIndex = 0; CharacterIndex < PartyInventoryState.ActiveCharacters.Num (); ++CharacterIndex)
    {
        const FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
        Result += FString::Printf (
            TEXT ("[%d] Name=%s Class=%s Level=%d Slots=%d/%d Weight=%.1f/%.1f Overloaded=%s\n"),
            CharacterIndex,
            *CharacterState.DisplayName.ToString (),
            *CharacterState.ClassId.ToString (),
            CharacterState.Level,
            CountOccupiedSlots (CharacterState),
            CharacterState.InventorySlots.Num (),
            CharacterState.CurrentWeight,
            CharacterState.MaxCarryWeight,
            CharacterState.IsOverloaded () ? TEXT ("true") : TEXT ("false"));
        Result += GetEquipmentDiagnosticsForCharacter (CharacterIndex);
        Result += TEXT ("\n");

        bool bWroteInventoryHeader = false;
        for (int32 SlotIndex = 0; SlotIndex < CharacterState.InventorySlots.Num (); ++SlotIndex)
        {
            const FGridInventorySlot& Slot = CharacterState.InventorySlots[SlotIndex];
            if (Slot.IsEmpty ())
            {
                continue;
            }

            if (!bWroteInventoryHeader)
            {
                Result += TEXT ("    Inventory:\n");
                bWroteInventoryHeader = true;
            }

            Result += FString::Printf (
                TEXT ("      [%d] Item=%s Qty=%d Weight=%.1f Owner=%s\n"),
                SlotIndex,
                *Slot.Item.ItemDefinitionId.ToString (),
                Slot.Item.Quantity,
                Slot.Item.Weight,
                GetOwnerTypeName (Slot.Item.OwnerType));
        }
    }

    return Result;
}

void UGridPartyInventoryComponent::LogPartyInventoryDiagnostics () const
{
    UE_LOG (LogTemp, Log, TEXT ("%s"), *GetPartyInventoryDiagnostics ());
}

FString UGridPartyInventoryComponent::GetItemDefinitionDiagnostics () const
{
    FString Result;
    Result += TEXT ("GridItemDefinition Diagnostics\n");
    Result += FString::Printf (TEXT ("Definitions=%d\n"), ItemDefinitions.Num ());

    TMap<FName, int32> DefinitionIdCounts;
    int32 DuplicateCount = 0;
    for (const TObjectPtr<UGridItemDefinitionAsset>& Definition : ItemDefinitions)
    {
        if (!Definition || Definition->ItemDefinitionId.IsNone ())
        {
            continue;
        }
        int32& Count = DefinitionIdCounts.FindOrAdd (Definition->ItemDefinitionId);
        ++Count;
        if (Count == 2)
        {
            ++DuplicateCount;
        }
    }

    for (int32 Index = 0; Index < ItemDefinitions.Num (); ++Index)
    {
        const UGridItemDefinitionAsset* Definition = ItemDefinitions[Index];
        if (!Definition)
        {
            Result += FString::Printf (TEXT ("[%d] Asset=None Warning=NullDefinition\n"), Index);
            continue;
        }

        Result += FString::Printf (
            TEXT ("[%d] Asset=%s Id=%s Type=%s Weight=%.1f Slots=%s\n"),
            Index,
            *Definition->GetName (),
            *Definition->ItemDefinitionId.ToString (),
            GetItemTypeName (Definition->ItemType),
            Definition->Weight,
            *GetEquipmentSlotsText (Definition->CompatibleEquipmentSlots));

        if (Definition->ItemDefinitionId.IsNone ())
        {
            Result += FString::Printf (TEXT ("Warning Asset=%s ItemDefinitionId=None\n"), *Definition->GetName ());
        }
        else if (DefinitionIdCounts.FindRef (Definition->ItemDefinitionId) > 1)
        {
            Result += FString::Printf (
                TEXT ("Warning Duplicate ItemDefinitionId=%s Asset=%s\n"),
                *Definition->ItemDefinitionId.ToString (),
                *Definition->GetName ());
        }
    }

    Result += FString::Printf (TEXT ("Duplicates=%d\n"), DuplicateCount);
    return Result;
}

void UGridPartyInventoryComponent::LogItemDefinitionDiagnostics () const
{
    UE_LOG (LogTemp, Log, TEXT ("%s"), *GetItemDefinitionDiagnostics ());
}

void UGridPartyInventoryComponent::EnsureEquipmentCountMatchesActiveCharacters ()
{
    const int32 CharacterCount = PartyInventoryState.ActiveCharacters.Num ();
    if (PartyInventoryState.ActiveEquipment.Num () < CharacterCount)
    {
        PartyInventoryState.ActiveEquipment.SetNum (CharacterCount);
    }
    else if (PartyInventoryState.ActiveEquipment.Num () > CharacterCount)
    {
        PartyInventoryState.ActiveEquipment.SetNum (CharacterCount);
    }
}

void UGridPartyInventoryComponent::InitializeCharacterDefaults (
    FGridCharacterInventoryState& CharacterState,
    int32 CharacterIndex) const
{
    if (!CharacterState.CharacterId.IsValid ())
    {
        CharacterState.CharacterId = FGuid::NewGuid ();
    }

    if (CharacterState.DisplayName.IsEmpty ())
    {
        CharacterState.DisplayName = FText::FromString (
            CharacterIndex == 0 ? TEXT ("Hero_01") : FString::Printf (TEXT ("Hero_%02d"), CharacterIndex + 1));
    }

    if (CharacterState.ClassId.IsNone ())
    {
        CharacterState.ClassId = TEXT ("Warrior");
    }

    CharacterState.Level = FMath::Max (1, CharacterState.Level);
    CharacterState.Strength = FMath::Max (0.0f, CharacterState.Strength);
    CharacterState.MaxCarryWeight = CharacterState.Strength * CarryWeightPerStrength;

    if (CharacterState.InventorySlots.Num () == 0)
    {
        CharacterState.InventorySlots.SetNum (FMath::Max (0, DefaultInventorySlotCountPerCharacter));
    }
}

float UGridPartyInventoryComponent::CalculateEquipmentWeight (const FGridCharacterEquipmentState& EquipmentState) const
{
    float TotalWeight = 0.0f;
    TotalWeight += GetItemTotalWeight (EquipmentState.MainHand);
    TotalWeight += GetItemTotalWeight (EquipmentState.OffHand);
    TotalWeight += GetItemTotalWeight (EquipmentState.Head);
    TotalWeight += GetItemTotalWeight (EquipmentState.Chest);
    TotalWeight += GetItemTotalWeight (EquipmentState.Legs);
    TotalWeight += GetItemTotalWeight (EquipmentState.Feet);
    TotalWeight += GetItemTotalWeight (EquipmentState.Amulet);
    TotalWeight += GetItemTotalWeight (EquipmentState.Ring1);
    TotalWeight += GetItemTotalWeight (EquipmentState.Ring2);
    return TotalWeight;
}
