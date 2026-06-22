#include "Runtime/GridPartyInventoryComponent.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"

namespace
{
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

    bool HaveMatchingReadableContent (
        const FGridItemInstance& ExistingItem,
        const FGridItemInstance& IncomingItem)
    {
        return ExistingItem.ReadableContentAsset == IncomingItem.ReadableContentAsset &&
            ExistingItem.ReadableContentId == IncomingItem.ReadableContentId &&
            ExistingItem.ReadTitleOverride.EqualTo (IncomingItem.ReadTitleOverride) &&
            ExistingItem.ReadTextOverride.EqualTo (IncomingItem.ReadTextOverride);
    }

    bool IsSupportedEquipmentSlot (EGridEquipmentSlot Slot)
    {
        return Slot == EGridEquipmentSlot::MainHand || Slot == EGridEquipmentSlot::OffHand;
    }

    const TCHAR* GetEquipmentSlotName (EGridEquipmentSlot Slot)
    {
        switch (Slot)
        {
        case EGridEquipmentSlot::None:
            return TEXT ("None");
        case EGridEquipmentSlot::MainHand:
            return TEXT ("MainHand");
        case EGridEquipmentSlot::OffHand:
            return TEXT ("OffHand");
        case EGridEquipmentSlot::Head:
            return TEXT ("Head");
        case EGridEquipmentSlot::Chest:
            return TEXT ("Chest");
        case EGridEquipmentSlot::Legs:
            return TEXT ("Legs");
        case EGridEquipmentSlot::Feet:
            return TEXT ("Feet");
        case EGridEquipmentSlot::Amulet:
            return TEXT ("Amulet");
        case EGridEquipmentSlot::Ring1:
            return TEXT ("Ring1");
        case EGridEquipmentSlot::Ring2:
            return TEXT ("Ring2");
        case EGridEquipmentSlot::Shoulders:
            return TEXT ("Shoulders");
        case EGridEquipmentSlot::Gloves:
            return TEXT ("Gloves");
        case EGridEquipmentSlot::Belt:
            return TEXT ("Belt");
        case EGridEquipmentSlot::Cloak:
            return TEXT ("Cloak");
        case EGridEquipmentSlot::Talisman:
            return TEXT ("Talisman");
        case EGridEquipmentSlot::QuickSlot1:
            return TEXT ("QuickSlot1");
        case EGridEquipmentSlot::QuickSlot2:
            return TEXT ("QuickSlot2");
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

    void ForEachEquipmentItem (
        const FGridCharacterEquipmentState& EquipmentState,
        TFunctionRef<void (EGridEquipmentSlot, const FGridItemInstance&)> Visitor)
    {
        const EGridEquipmentSlot Slots[] = {
            EGridEquipmentSlot::MainHand,
            EGridEquipmentSlot::OffHand,
            EGridEquipmentSlot::Head,
            EGridEquipmentSlot::Chest,
            EGridEquipmentSlot::Legs,
            EGridEquipmentSlot::Feet,
            EGridEquipmentSlot::Amulet,
            EGridEquipmentSlot::Ring1,
            EGridEquipmentSlot::Ring2
        };

        for (const EGridEquipmentSlot Slot : Slots)
        {
            if (const FGridItemInstance* Item = EquipmentState.GetSlot (Slot))
            {
                Visitor (Slot, *Item);
            }
        }
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

bool UGridPartyInventoryComponent::HasCompletedInitialCharacterCreation () const
{
    return PartyInventoryState.bInitialCharacterCreationCompleted;
}

bool UGridPartyInventoryComponent::CreateInitialCharacter (
    const FRPGCharacterCreationRequest& Request,
    FText& OutError)
{
    OutError = FText::GetEmpty ();

    if (HasCompletedInitialCharacterCreation ())
    {
        OutError = FText::FromString (TEXT ("La création initiale du personnage est déjà terminée."));
        return false;
    }

    FString NormalizedName = Request.DisplayName.ToString ();
    NormalizedName.TrimStartAndEndInline ();
    if (NormalizedName.Len () < 1 || NormalizedName.Len () > 24)
    {
        OutError = FText::FromString (TEXT ("Le nom du personnage doit contenir entre 1 et 24 caractères."));
        return false;
    }

    if (!Request.RaceDefinition || !Request.RaceDefinition->IsValidDefinition ())
    {
        OutError = FText::FromString (TEXT ("Une définition de race valide est requise."));
        return false;
    }

    if (!Request.ClassDefinition || !Request.ClassDefinition->IsValidDefinition ())
    {
        OutError = FText::FromString (TEXT ("Une définition de classe valide est requise."));
        return false;
    }

    const FRPGAttributes FinalAttributes = URPGCharacterRulesLibrary::AddAttributes (
        Request.ClassDefinition->BaseAttributes,
        Request.RaceDefinition->AttributeBonuses);
    if (!URPGCharacterRulesLibrary::AreAttributesInRange (FinalAttributes))
    {
        OutError = FText::FromString (TEXT ("Toutes les caractéristiques initiales doivent être comprises entre 6 et 20."));
        return false;
    }

    FGridCharacterInventoryState NewCharacter;
    NewCharacter.CharacterId = FGuid::NewGuid ();
    NewCharacter.DisplayName = FText::FromString (NormalizedName);
    NewCharacter.RaceId = Request.RaceDefinition->RaceId;
    NewCharacter.ClassId = Request.ClassDefinition->ClassId;
    NewCharacter.Level = 1;
    NewCharacter.Experience = 0;
    NewCharacter.Attributes = FinalAttributes;
    NewCharacter.DerivedStats = URPGCharacterRulesLibrary::CalculateDerivedStats (
        FinalAttributes,
        Request.ClassDefinition,
        NewCharacter.Level);
    NewCharacter.Portrait = Request.Portrait;
    NewCharacter.bRPGAttributesInitialized = true;
    NewCharacter.Strength = static_cast<float> (FinalAttributes.Strength);
    NewCharacter.MaxCarryWeight = URPGCharacterRulesLibrary::CalculateMaxCarryWeight (FinalAttributes);
    NewCharacter.CurrentWeight = 0.0f;
    NewCharacter.InventorySlots.SetNum (FMath::Max (0, DefaultInventorySlotCountPerCharacter));

    FGridPartyInventoryState NewPartyState;
    NewPartyState.SelectedCharacterIndex = 0;
    NewPartyState.MaxActiveCharacters = FMath::Max (1, DefaultMaxActiveCharacters);
    NewPartyState.bInitialCharacterCreationCompleted = true;
    NewPartyState.ActiveCharacters.Add (MoveTemp (NewCharacter));
    NewPartyState.ActiveEquipment.SetNum (1);
    NewPartyState.bHasCursorItem = false;
    NewPartyState.CursorItem = FGridItemInstance ();

    const FGridPartyInventoryState PreviousPartyState = PartyInventoryState;
    PartyInventoryState = MoveTemp (NewPartyState);
    RecalculateAllWeights ();

    FString OwnershipError;
    if (!ValidateInventoryOwnership (OwnershipError))
    {
        PartyInventoryState = PreviousPartyState;
        OutError = FText::FromString (
            FString::Printf (TEXT ("La création du personnage a échoué lors de la validation de l'ownership : %s"), *OwnershipError));
        return false;
    }

    return true;
}

int32 UGridPartyInventoryComponent::GetActiveCharacterCount () const
{
    return PartyInventoryState.ActiveCharacters.Num ();
}

int32 UGridPartyInventoryComponent::GetMaxActiveCharacters () const
{
    return PartyInventoryState.MaxActiveCharacters;
}

int32 UGridPartyInventoryComponent::GetMaxActiveCharacterCount () const
{
    return GetMaxActiveCharacters ();
}

int32 UGridPartyInventoryComponent::GetSelectedCharacterIndex () const
{
    return PartyInventoryState.SelectedCharacterIndex;
}

bool UGridPartyInventoryComponent::SetSelectedCharacterIndex (int32 NewIndex)
{
    const int32 OldIndex = PartyInventoryState.SelectedCharacterIndex;
    if (!IsValidCharacterIndex (NewIndex))
    {
        UE_LOG (LogTemp, Log, TEXT ("GridInventory SelectedCharacter Changed Old=%d New=%d Result=false"),
            OldIndex,
            NewIndex);
        return false;
    }

    PartyInventoryState.SelectedCharacterIndex = NewIndex;
    UE_LOG (LogTemp, Log, TEXT ("GridInventory SelectedCharacter Changed Old=%d New=%d Result=true"),
        OldIndex,
        NewIndex);
    return true;
}

bool UGridPartyInventoryComponent::GetCharacterSummary (
    int32 CharacterIndex,
    FGridInventoryCharacterSummary& OutSummary) const
{
    OutSummary = FGridInventoryCharacterSummary ();
    if (!IsValidCharacterIndex (CharacterIndex))
    {
        return false;
    }

    const FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
    OutSummary.CharacterIndex = CharacterIndex;
    OutSummary.CharacterId = CharacterState.CharacterId.IsValid ()
        ? FName (*CharacterState.CharacterId.ToString (EGuidFormats::DigitsWithHyphens))
        : NAME_None;
    OutSummary.DisplayName = CharacterState.DisplayName.IsEmpty ()
        ? FText::FromString (CharacterIndex == 0 ? TEXT ("Hero_01") : FString::Printf (TEXT ("Hero_%02d"), CharacterIndex + 1))
        : CharacterState.DisplayName;
    OutSummary.ClassId = CharacterState.ClassId;
    OutSummary.Level = CharacterState.Level;
    OutSummary.UsedInventorySlots = CountOccupiedSlots (CharacterState);
    OutSummary.MaxInventorySlots = CharacterState.InventorySlots.Num ();
    OutSummary.CurrentWeight = CharacterState.CurrentWeight;
    OutSummary.MaxWeight = CharacterState.MaxCarryWeight;
    OutSummary.bOverloaded = CharacterState.IsOverloaded ();
    OutSummary.bIsSelected = CharacterIndex == PartyInventoryState.SelectedCharacterIndex;
    return true;
}

bool UGridPartyInventoryComponent::IsValidCharacterIndex (int32 Index) const
{
    return PartyInventoryState.IsValidActiveCharacterIndex (Index);
}

bool UGridPartyInventoryComponent::CanAddItemToCharacterInventory (int32 CharacterIndex, const FGridItemInstance& Item) const
{
    FGridItemInstance ItemToAdd = Item;
    const int32 InitialQuantity = ItemToAdd.Quantity;
    ApplyItemDefinitionToInstance (ItemToAdd);
    ItemToAdd.Quantity = InitialQuantity;

    if (!IsValidCharacterIndex (CharacterIndex) || !ItemToAdd.IsValid ())
    {
        return false;
    }

    const UGridItemDefinitionAsset* Definition = FindItemDefinition (ItemToAdd.ItemDefinitionId);
    const bool bStackable = Definition && Definition->bStackable;
    const int32 MaxStackSize = bStackable ? FMath::Max (1, Definition->MaxStackSize) : 1;

    int64 AvailableCapacity = 0;
    const FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
    for (const FGridInventorySlot& Slot : CharacterState.InventorySlots)
    {
        if (Slot.IsEmpty ())
        {
            AvailableCapacity += MaxStackSize;
        }
        else if (bStackable &&
            Slot.Item.ItemDefinitionId == ItemToAdd.ItemDefinitionId &&
            HaveMatchingReadableContent (Slot.Item, ItemToAdd))
        {
            AvailableCapacity += FMath::Max (0, MaxStackSize - FMath::Max (1, Slot.Item.Quantity));
        }

        if (AvailableCapacity >= InitialQuantity)
        {
            return true;
        }
    }

    return AvailableCapacity >= InitialQuantity;
}

bool UGridPartyInventoryComponent::CanAddItemToSelectedCharacterInventory (const FGridItemInstance& Item) const
{
    return CanAddItemToCharacterInventory (PartyInventoryState.SelectedCharacterIndex, Item);
}

bool UGridPartyInventoryComponent::AddItemToCharacterInventory (int32 CharacterIndex, const FGridItemInstance& Item)
{
    FGridItemInstance ItemToAdd = Item;
    const int32 InitialQuantity = ItemToAdd.Quantity;
    ApplyItemDefinitionToInstance (ItemToAdd);
    ItemToAdd.Quantity = InitialQuantity;

    if (!IsValidCharacterIndex (CharacterIndex) || !ItemToAdd.IsValid () ||
        !CanAddItemToCharacterInventory (CharacterIndex, ItemToAdd))
    {
        return false;
    }

    const UGridItemDefinitionAsset* Definition = FindItemDefinition (ItemToAdd.ItemDefinitionId);
    const bool bStackable = Definition && Definition->bStackable;
    const int32 MaxStackSize = bStackable ? FMath::Max (1, Definition->MaxStackSize) : 1;

    FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
    ItemToAdd.OwnerType = EGridItemOwnerType::CharacterInventory;
    ItemToAdd.OwnerGuid = CharacterState.CharacterId;
    ItemToAdd.OwnerCharacterIndex = CharacterIndex;
    ItemToAdd.EquipmentSlot = EGridEquipmentSlot::None;

    TArray<FGridInventorySlot> UpdatedInventorySlots = CharacterState.InventorySlots;
    int32 RemainingQuantity = InitialQuantity;
    if (bStackable)
    {
        for (FGridInventorySlot& Slot : UpdatedInventorySlots)
        {
            if (RemainingQuantity <= 0)
            {
                break;
            }
            if (Slot.IsEmpty () ||
                Slot.Item.ItemDefinitionId != ItemToAdd.ItemDefinitionId ||
                !HaveMatchingReadableContent (Slot.Item, ItemToAdd))
            {
                continue;
            }

            const int32 AvailableInStack = FMath::Max (0, MaxStackSize - FMath::Max (1, Slot.Item.Quantity));
            const int32 QuantityToStack = FMath::Min (RemainingQuantity, AvailableInStack);
            Slot.Item.Quantity += QuantityToStack;
            RemainingQuantity -= QuantityToStack;
        }
    }

    bool bUsedIncomingRuntimeObjectId = false;
    for (FGridInventorySlot& Slot : UpdatedInventorySlots)
    {
        if (RemainingQuantity <= 0)
        {
            break;
        }
        if (!Slot.IsEmpty ())
        {
            continue;
        }

        FGridItemInstance NewStack = ItemToAdd;
        NewStack.Quantity = FMath::Min (RemainingQuantity, MaxStackSize);
        if (bUsedIncomingRuntimeObjectId)
        {
            NewStack.RuntimeObjectId = FGuid::NewGuid ();
        }

        Slot.bOccupied = true;
        Slot.Item = NewStack;
        RemainingQuantity -= NewStack.Quantity;
        bUsedIncomingRuntimeObjectId = true;
    }

    if (RemainingQuantity != 0)
    {
        return false;
    }

    CharacterState.InventorySlots = MoveTemp (UpdatedInventorySlots);
    RecalculateCharacterWeight (CharacterIndex);
    return true;
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

bool UGridPartyInventoryComponent::RegisterItemDefinition (UGridItemDefinitionAsset* Definition)
{
    if (!Definition || Definition->ItemDefinitionId.IsNone ())
    {
        return false;
    }

    if (RuntimeItemDefinitionsById.Contains (Definition->ItemDefinitionId))
    {
        return true;
    }

    RuntimeItemDefinitionsById.Add (Definition->ItemDefinitionId, Definition);

    UE_LOG (LogTemp, Log, TEXT ("GridInventory Registered ItemDefinition=%s Asset=%s"),
        *Definition->ItemDefinitionId.ToString (),
        *Definition->GetPathName ());

    return true;
}

UGridItemDefinitionAsset* UGridPartyInventoryComponent::FindItemDefinition (FName ItemDefinitionId) const
{
    if (ItemDefinitionId.IsNone ())
    {
        return nullptr;
    }

    if (const TObjectPtr<UGridItemDefinitionAsset>* Definition =
        RuntimeItemDefinitionsById.Find (ItemDefinitionId))
    {
        return Definition->Get ();
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

bool UGridPartyInventoryComponent::TryTakeEquipmentSlotToCursor (
    int32 CharacterIndex,
    EGridEquipmentSlot SourceSlot)
{
    EnsureEquipmentCountMatchesActiveCharacters ();

    if (!IsValidCharacterIndex (CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex (CharacterIndex))
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Take FromEquipment Failed Character=%d Slot=%s Reason=InvalidCharacter"),
            CharacterIndex,
            GetEquipmentSlotName (SourceSlot));
        return false;
    }

    if (SourceSlot == EGridEquipmentSlot::None)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Take FromEquipment Failed Character=%d Slot=%s Reason=InvalidSlot"),
            CharacterIndex,
            GetEquipmentSlotName (SourceSlot));
        return false;
    }

    if (HasCursorItem ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Take FromEquipment Failed Character=%d Slot=%s Reason=CursorOccupied"),
            CharacterIndex,
            GetEquipmentSlotName (SourceSlot));
        return false;
    }

    FGridCharacterEquipmentState& EquipmentState = PartyInventoryState.ActiveEquipment[CharacterIndex];
    FGridItemInstance* EquippedItem = EquipmentState.GetMutableSlot (SourceSlot);
    if (!EquippedItem)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Take FromEquipment Failed Character=%d Slot=%s Reason=InvalidSlot"),
            CharacterIndex,
            GetEquipmentSlotName (SourceSlot));
        return false;
    }

    if (!EquippedItem->IsValid ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Take FromEquipment Failed Character=%d Slot=%s Reason=EmptySlot"),
            CharacterIndex,
            GetEquipmentSlotName (SourceSlot));
        return false;
    }

    FGridItemInstance ItemToCursor = *EquippedItem;
    ItemToCursor.OwnerType = EGridItemOwnerType::Cursor;
    ItemToCursor.OwnerGuid = FGuid ();
    ItemToCursor.OwnerCharacterIndex = INDEX_NONE;
    ItemToCursor.EquipmentSlot = EGridEquipmentSlot::None;

    *EquippedItem = FGridItemInstance ();
    PartyInventoryState.CursorItem = ItemToCursor;
    PartyInventoryState.bHasCursorItem = true;
    RecalculateCharacterWeight (CharacterIndex);

    UE_LOG (LogTemp, Log, TEXT ("GridInventory Cursor Take FromEquipment Character=%d Slot=%s Item=%s RuntimeId=%s Result=true"),
        CharacterIndex,
        GetEquipmentSlotName (SourceSlot),
        *ItemToCursor.ItemDefinitionId.ToString (),
        *ItemToCursor.RuntimeObjectId.ToString ());
    return true;
}

bool UGridPartyInventoryComponent::TryTakeSelectedCharacterEquipmentSlotToCursor (EGridEquipmentSlot SourceSlot)
{
    return TryTakeEquipmentSlotToCursor (PartyInventoryState.SelectedCharacterIndex, SourceSlot);
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
    PartyInventoryState.CursorItem.OwnerGuid = FGuid ();
    PartyInventoryState.CursorItem.OwnerCharacterIndex = INDEX_NONE;
    PartyInventoryState.CursorItem.EquipmentSlot = EGridEquipmentSlot::None;
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

const FGridItemInstance& UGridPartyInventoryComponent::GetCursorItem () const
{
    return PartyInventoryState.CursorItem;
}

bool UGridPartyInventoryComponent::TryTakeInventorySlotToCursor (int32 CharacterIndex, int32 InventorySlotIndex)
{
    return TryTakeInventorySlotQuantityToCursor (CharacterIndex, InventorySlotIndex, MAX_int32);
}

bool UGridPartyInventoryComponent::TryTakeInventorySlotQuantityToCursor (
    int32 CharacterIndex,
    int32 InventorySlotIndex,
    int32 Quantity)
{
    if (HasCursorItem ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Take Failed Character=%d Slot=%d Reason=CursorOccupied"),
            CharacterIndex,
            InventorySlotIndex);
        return false;
    }

    if (!IsValidCharacterIndex (CharacterIndex))
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Take Failed Character=%d Slot=%d Reason=InvalidCharacter"),
            CharacterIndex,
            InventorySlotIndex);
        return false;
    }

    if (Quantity <= 0)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Take Failed Character=%d Slot=%d Reason=InvalidQuantity Quantity=%d"),
            CharacterIndex,
            InventorySlotIndex,
            Quantity);
        return false;
    }

    FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
    if (!CharacterState.InventorySlots.IsValidIndex (InventorySlotIndex) || CharacterState.InventorySlots[InventorySlotIndex].IsEmpty ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Take Failed Character=%d Slot=%d Reason=InvalidInventorySlot"),
            CharacterIndex,
            InventorySlotIndex);
        return false;
    }

    FGridInventorySlot& InventorySlot = CharacterState.InventorySlots[InventorySlotIndex];
    const UGridItemDefinitionAsset* Definition = FindItemDefinition (InventorySlot.Item.ItemDefinitionId);
    const bool bCanSplitStack =
        Definition &&
        Definition->bStackable &&
        InventorySlot.Item.Quantity > 1 &&
        Quantity < InventorySlot.Item.Quantity;

    FGridItemInstance ItemToCursor = InventorySlot.Item;
    if (bCanSplitStack)
    {
        InventorySlot.Item.Quantity -= Quantity;
        ItemToCursor.Quantity = Quantity;
        ItemToCursor.RuntimeObjectId = FGuid::NewGuid ();
    }
    else
    {
        InventorySlot = FGridInventorySlot ();
    }

    ItemToCursor.OwnerType = EGridItemOwnerType::Cursor;
    ItemToCursor.OwnerGuid = FGuid ();
    ItemToCursor.OwnerCharacterIndex = INDEX_NONE;
    ItemToCursor.EquipmentSlot = EGridEquipmentSlot::None;

    PartyInventoryState.CursorItem = ItemToCursor;
    PartyInventoryState.bHasCursorItem = true;
    RecalculateCharacterWeight (CharacterIndex);

    UE_LOG (LogTemp, Log, TEXT ("GridInventory Cursor Take FromInventory Character=%d Slot=%d Item=%s RuntimeId=%s Quantity=%d Split=%s"),
        CharacterIndex,
        InventorySlotIndex,
        *ItemToCursor.ItemDefinitionId.ToString (),
        *ItemToCursor.RuntimeObjectId.ToString (),
        ItemToCursor.Quantity,
        bCanSplitStack ? TEXT ("true") : TEXT ("false"));
    return true;
}

bool UGridPartyInventoryComponent::TryPlaceCursorItemInCharacterInventorySlot (
    int32 CharacterIndex,
    int32 TargetSlotIndex)
{
    if (!HasCursorItem ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Place ToInventorySlot Failed Character=%d Slot=%d Reason=NoCursorItem"),
            CharacterIndex,
            TargetSlotIndex);
        return false;
    }

    if (!IsValidCharacterIndex (CharacterIndex))
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Place ToInventorySlot Failed Character=%d Slot=%d Reason=InvalidCharacter"),
            CharacterIndex,
            TargetSlotIndex);
        return false;
    }

    FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
    if (!CharacterState.InventorySlots.IsValidIndex (TargetSlotIndex))
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Place ToInventorySlot Failed Character=%d Slot=%d Reason=InvalidTargetSlot"),
            CharacterIndex,
            TargetSlotIndex);
        return false;
    }

    FGridInventorySlot& TargetSlot = CharacterState.InventorySlots[TargetSlotIndex];
    FGridItemInstance CursorItem = PartyInventoryState.CursorItem;
    ApplyItemDefinitionToInstance (CursorItem);
    CursorItem.OwnerType = EGridItemOwnerType::CharacterInventory;
    CursorItem.OwnerGuid = CharacterState.CharacterId;
    CursorItem.OwnerCharacterIndex = CharacterIndex;
    CursorItem.EquipmentSlot = EGridEquipmentSlot::None;

    if (TargetSlot.IsEmpty ())
    {
        TargetSlot.bOccupied = true;
        TargetSlot.Item = CursorItem;
        PartyInventoryState.CursorItem = FGridItemInstance ();
        PartyInventoryState.bHasCursorItem = false;
        RecalculateCharacterWeight (CharacterIndex);

        UE_LOG (LogTemp, Log, TEXT ("GridInventory Cursor Place ToInventorySlot Character=%d Slot=%d Item=%s Result=true"),
            CharacterIndex,
            TargetSlotIndex,
            *CursorItem.ItemDefinitionId.ToString ());
        return true;
    }

    FGridItemInstance SlotItem = TargetSlot.Item;
    SlotItem.OwnerType = EGridItemOwnerType::Cursor;
    SlotItem.OwnerGuid = FGuid ();
    SlotItem.OwnerCharacterIndex = INDEX_NONE;
    SlotItem.EquipmentSlot = EGridEquipmentSlot::None;

    TargetSlot.Item = CursorItem;
    TargetSlot.bOccupied = true;
    PartyInventoryState.CursorItem = SlotItem;
    PartyInventoryState.bHasCursorItem = true;
    RecalculateCharacterWeight (CharacterIndex);

    UE_LOG (LogTemp, Log, TEXT ("GridInventory Cursor Swap WithInventorySlot Character=%d Slot=%d CursorItem=%s SlotItem=%s Result=true"),
        CharacterIndex,
        TargetSlotIndex,
        *CursorItem.ItemDefinitionId.ToString (),
        *SlotItem.ItemDefinitionId.ToString ());
    return true;
}

bool UGridPartyInventoryComponent::TryMoveCharacterInventorySlot (
    int32 CharacterIndex,
    int32 SourceSlotIndex,
    int32 TargetSlotIndex)
{
    if (!IsValidCharacterIndex (CharacterIndex))
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Move InventorySlot Failed Character=%d Source=%d Target=%d Reason=InvalidCharacter"),
            CharacterIndex,
            SourceSlotIndex,
            TargetSlotIndex);
        return false;
    }

    FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
    if (!CharacterState.InventorySlots.IsValidIndex (SourceSlotIndex))
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Move InventorySlot Failed Character=%d Source=%d Target=%d Reason=InvalidSourceSlot"),
            CharacterIndex,
            SourceSlotIndex,
            TargetSlotIndex);
        return false;
    }

    if (!CharacterState.InventorySlots.IsValidIndex (TargetSlotIndex))
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Move InventorySlot Failed Character=%d Source=%d Target=%d Reason=InvalidTargetSlot"),
            CharacterIndex,
            SourceSlotIndex,
            TargetSlotIndex);
        return false;
    }

    if (SourceSlotIndex == TargetSlotIndex)
    {
        UE_LOG (LogTemp, Log, TEXT ("GridInventory Move InventorySlot Character=%d Source=%d Target=%d Result=true Reason=SameSlot"),
            CharacterIndex,
            SourceSlotIndex,
            TargetSlotIndex);
        return true;
    }

    FGridInventorySlot& SourceSlot = CharacterState.InventorySlots[SourceSlotIndex];
    FGridInventorySlot& TargetSlot = CharacterState.InventorySlots[TargetSlotIndex];
    if (SourceSlot.IsEmpty ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Move InventorySlot Failed Character=%d Source=%d Target=%d Reason=SourceEmpty"),
            CharacterIndex,
            SourceSlotIndex,
            TargetSlotIndex);
        return false;
    }

    FGridItemInstance SourceItem = SourceSlot.Item;
    SourceItem.OwnerType = EGridItemOwnerType::CharacterInventory;
    SourceItem.OwnerGuid = CharacterState.CharacterId;
    SourceItem.OwnerCharacterIndex = CharacterIndex;
    SourceItem.EquipmentSlot = EGridEquipmentSlot::None;

    if (TargetSlot.IsEmpty ())
    {
        TargetSlot.bOccupied = true;
        TargetSlot.Item = SourceItem;
        SourceSlot = FGridInventorySlot ();
        RecalculateCharacterWeight (CharacterIndex);

        UE_LOG (LogTemp, Log, TEXT ("GridInventory Move InventorySlot Character=%d Source=%d Target=%d Item=%s Result=true"),
            CharacterIndex,
            SourceSlotIndex,
            TargetSlotIndex,
            *SourceItem.ItemDefinitionId.ToString ());
        return true;
    }

    FGridItemInstance TargetItem = TargetSlot.Item;
    TargetItem.OwnerType = EGridItemOwnerType::CharacterInventory;
    TargetItem.OwnerGuid = CharacterState.CharacterId;
    TargetItem.OwnerCharacterIndex = CharacterIndex;
    TargetItem.EquipmentSlot = EGridEquipmentSlot::None;

    SourceSlot.bOccupied = true;
    SourceSlot.Item = TargetItem;
    TargetSlot.bOccupied = true;
    TargetSlot.Item = SourceItem;
    RecalculateCharacterWeight (CharacterIndex);

    UE_LOG (LogTemp, Log, TEXT ("GridInventory Swap InventorySlots Character=%d A=%d B=%d ItemA=%s ItemB=%s Result=true"),
        CharacterIndex,
        SourceSlotIndex,
        TargetSlotIndex,
        *SourceItem.ItemDefinitionId.ToString (),
        *TargetItem.ItemDefinitionId.ToString ());
    return true;
}

bool UGridPartyInventoryComponent::TryPlaceCursorItemInCharacterInventory (int32 CharacterIndex)
{
    if (!HasCursorItem ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Place Failed Character=%d Reason=NoCursorItem"),
            CharacterIndex);
        return false;
    }

    if (!IsValidCharacterIndex (CharacterIndex))
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Place Failed Character=%d Reason=InvalidCharacter"),
            CharacterIndex);
        return false;
    }

    FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
    const int32 FreeSlotIndex = FindFreeInventorySlotIndex (CharacterState);
    if (FreeSlotIndex == INDEX_NONE)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Place Failed Character=%d Item=%s RuntimeId=%s Reason=InventoryFull"),
            CharacterIndex,
            *PartyInventoryState.CursorItem.ItemDefinitionId.ToString (),
            *PartyInventoryState.CursorItem.RuntimeObjectId.ToString ());
        return false;
    }

    FGridItemInstance ItemToInventory = PartyInventoryState.CursorItem;
    ApplyItemDefinitionToInstance (ItemToInventory);
    ItemToInventory.OwnerType = EGridItemOwnerType::CharacterInventory;
    ItemToInventory.OwnerGuid = CharacterState.CharacterId;
    ItemToInventory.OwnerCharacterIndex = CharacterIndex;
    ItemToInventory.EquipmentSlot = EGridEquipmentSlot::None;

    CharacterState.InventorySlots[FreeSlotIndex].bOccupied = true;
    CharacterState.InventorySlots[FreeSlotIndex].Item = ItemToInventory;
    PartyInventoryState.CursorItem = FGridItemInstance ();
    PartyInventoryState.bHasCursorItem = false;
    RecalculateCharacterWeight (CharacterIndex);

    UE_LOG (LogTemp, Log, TEXT ("GridInventory Cursor Place ToInventory Character=%d Slot=%d Item=%s RuntimeId=%s"),
        CharacterIndex,
        FreeSlotIndex,
        *ItemToInventory.ItemDefinitionId.ToString (),
        *ItemToInventory.RuntimeObjectId.ToString ());
    return true;
}

bool UGridPartyInventoryComponent::TryPlaceCursorItemInSelectedCharacterInventory ()
{
    return TryPlaceCursorItemInCharacterInventory (PartyInventoryState.SelectedCharacterIndex);
}

bool UGridPartyInventoryComponent::TryClearCursorToSelectedCharacterInventory ()
{
    return TryPlaceCursorItemInSelectedCharacterInventory ();
}

bool UGridPartyInventoryComponent::TryDropCursorItem ()
{
    if (!HasCursorItem ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Drop Failed Reason=NoCursorItem"));
        return false;
    }

    UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Drop Failed Item=%s RuntimeId=%s Reason=NotImplemented"),
        *PartyInventoryState.CursorItem.ItemDefinitionId.ToString (),
        *PartyInventoryState.CursorItem.RuntimeObjectId.ToString ());
    return false;
}

bool UGridPartyInventoryComponent::CanEquipCursorItemToCharacterSlot (
    int32 CharacterIndex,
    EGridEquipmentSlot TargetSlot) const
{
    if (!HasCursorItem () || !IsValidCharacterIndex (CharacterIndex) || TargetSlot == EGridEquipmentSlot::None)
    {
        return false;
    }

    return CanEquipItemToSlot (CharacterIndex, PartyInventoryState.CursorItem, TargetSlot);
}

bool UGridPartyInventoryComponent::TryEquipCursorItemToCharacterSlot (
    int32 CharacterIndex,
    EGridEquipmentSlot TargetSlot)
{
    EnsureEquipmentCountMatchesActiveCharacters ();

    if (!CanEquipCursorItemToCharacterSlot (CharacterIndex, TargetSlot) ||
        !PartyInventoryState.ActiveEquipment.IsValidIndex (CharacterIndex))
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Equip Failed Character=%d Slot=%s Reason=InvalidOrIncompatible"),
            CharacterIndex,
            GetEquipmentSlotName (TargetSlot));
        return false;
    }

    FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
    FGridCharacterEquipmentState& EquipmentState = PartyInventoryState.ActiveEquipment[CharacterIndex];
    FGridItemInstance* TargetItem = EquipmentState.GetMutableSlot (TargetSlot);
    if (!TargetItem)
    {
        UE_LOG (LogTemp, Warning, TEXT ("GridInventory Cursor Equip Failed Character=%d Slot=%s Reason=InvalidEquipmentSlot"),
            CharacterIndex,
            GetEquipmentSlotName (TargetSlot));
        return false;
    }

    FGridItemInstance ItemToEquip = PartyInventoryState.CursorItem;
    ItemToEquip.OwnerType = EGridItemOwnerType::EquipmentSlot;
    ItemToEquip.OwnerGuid = CharacterState.CharacterId;
    ItemToEquip.OwnerCharacterIndex = CharacterIndex;
    ItemToEquip.EquipmentSlot = TargetSlot;

    FGridItemInstance PreviouslyEquippedItem = *TargetItem;
    const bool bWasOccupied = PreviouslyEquippedItem.IsValid ();
    if (bWasOccupied)
    {
        PreviouslyEquippedItem.OwnerType = EGridItemOwnerType::Cursor;
        PreviouslyEquippedItem.OwnerGuid = FGuid ();
        PreviouslyEquippedItem.OwnerCharacterIndex = INDEX_NONE;
        PreviouslyEquippedItem.EquipmentSlot = EGridEquipmentSlot::None;
        PartyInventoryState.CursorItem = PreviouslyEquippedItem;
    }
    else
    {
        PartyInventoryState.CursorItem = FGridItemInstance ();
        PartyInventoryState.bHasCursorItem = false;
    }

    *TargetItem = ItemToEquip;
    RecalculateCharacterWeight (CharacterIndex);

    if (bWasOccupied)
    {
        UE_LOG (LogTemp, Log, TEXT ("GridInventory Cursor Equip Swap Character=%d Slot=%s NewItem=%s OldItem=%s Result=true"),
            CharacterIndex,
            GetEquipmentSlotName (TargetSlot),
            *ItemToEquip.ItemDefinitionId.ToString (),
            *PreviouslyEquippedItem.ItemDefinitionId.ToString ());
    }
    else
    {
        UE_LOG (LogTemp, Log, TEXT ("GridInventory Cursor Equip Character=%d Slot=%s Item=%s RuntimeId=%s Result=true"),
            CharacterIndex,
            GetEquipmentSlotName (TargetSlot),
            *ItemToEquip.ItemDefinitionId.ToString (),
            *ItemToEquip.RuntimeObjectId.ToString ());
    }

    return true;
}

bool UGridPartyInventoryComponent::TryEquipCursorItemToSelectedCharacterSlot (EGridEquipmentSlot TargetSlot)
{
    return TryEquipCursorItemToCharacterSlot (PartyInventoryState.SelectedCharacterIndex, TargetSlot);
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

    CharacterState.MaxCarryWeight =
        URPGCharacterRulesLibrary::CalculateMaxCarryWeight (CharacterState.Attributes);
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
    Result += FString::Printf (TEXT ("RuntimeDefinitions=%d\n"), RuntimeItemDefinitionsById.Num ());

    TArray<FName> DefinitionIds;
    RuntimeItemDefinitionsById.GetKeys (DefinitionIds);
    DefinitionIds.Sort (FNameLexicalLess ());
    for (int32 Index = 0; Index < DefinitionIds.Num (); ++Index)
    {
        const FName DefinitionId = DefinitionIds[Index];
        const TObjectPtr<UGridItemDefinitionAsset>* DefinitionEntry =
            RuntimeItemDefinitionsById.Find (DefinitionId);
        const UGridItemDefinitionAsset* Definition =
            DefinitionEntry ? DefinitionEntry->Get () : nullptr;
        if (!Definition)
        {
            Result += FString::Printf (
                TEXT ("[%d] Id=%s Asset=None Warning=NullDefinition\n"),
                Index,
                *DefinitionId.ToString ());
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
    }

    return Result;
}

void UGridPartyInventoryComponent::LogItemDefinitionDiagnostics () const
{
    UE_LOG (LogTemp, Log, TEXT ("%s"), *GetItemDefinitionDiagnostics ());
}

void UGridPartyInventoryComponent::LogInventoryOwnershipDiagnostics () const
{
    FString Error;
    if (ValidateInventoryOwnership (Error))
    {
        UE_LOG (LogTemp, Log, TEXT ("GridInventory Ownership OK"));
        return;
    }

    UE_LOG (LogTemp, Error, TEXT ("GridInventory Ownership ERROR %s"), *Error);
}

bool UGridPartyInventoryComponent::ValidateInventoryOwnership (FString& OutError) const
{
    OutError.Empty ();

    TMap<FGuid, FString> RuntimeOwners;
    auto RegisterRuntimeOwner = [&RuntimeOwners, &OutError] (const FGridItemInstance& Item, const FString& Location) -> bool
    {
        if (!Item.IsValid ())
        {
            return true;
        }

        if (const FString* ExistingOwner = RuntimeOwners.Find (Item.RuntimeObjectId))
        {
            OutError = FString::Printf (
                TEXT ("Duplicate RuntimeObjectId=%s Existing=%s Duplicate=%s"),
                *Item.RuntimeObjectId.ToString (),
                **ExistingOwner,
                *Location);
            return false;
        }

        RuntimeOwners.Add (Item.RuntimeObjectId, Location);
        return true;
    };

    if (PartyInventoryState.bHasCursorItem)
    {
        if (!PartyInventoryState.CursorItem.IsValid ())
        {
            OutError = TEXT ("CursorItem flag is set but CursorItem is invalid");
            return false;
        }

        if (PartyInventoryState.CursorItem.OwnerType != EGridItemOwnerType::Cursor ||
            PartyInventoryState.CursorItem.OwnerCharacterIndex != INDEX_NONE ||
            PartyInventoryState.CursorItem.EquipmentSlot != EGridEquipmentSlot::None)
        {
            OutError = FString::Printf (
                TEXT ("CursorItem has invalid ownership Owner=%s Character=%d Slot=%s"),
                GetOwnerTypeName (PartyInventoryState.CursorItem.OwnerType),
                PartyInventoryState.CursorItem.OwnerCharacterIndex,
                GetEquipmentSlotName (PartyInventoryState.CursorItem.EquipmentSlot));
            return false;
        }

        if (!RegisterRuntimeOwner (PartyInventoryState.CursorItem, TEXT ("CursorItem")))
        {
            return false;
        }
    }
    else if (PartyInventoryState.CursorItem.IsValid ())
    {
        OutError = TEXT ("CursorItem is valid while cursor flag is false");
        return false;
    }

    for (int32 CharacterIndex = 0; CharacterIndex < PartyInventoryState.ActiveCharacters.Num (); ++CharacterIndex)
    {
        const FGridCharacterInventoryState& CharacterState = PartyInventoryState.ActiveCharacters[CharacterIndex];
        for (int32 SlotIndex = 0; SlotIndex < CharacterState.InventorySlots.Num (); ++SlotIndex)
        {
            const FGridInventorySlot& InventorySlot = CharacterState.InventorySlots[SlotIndex];
            if (InventorySlot.IsEmpty ())
            {
                continue;
            }

            const FGridItemInstance& Item = InventorySlot.Item;
            if (Item.OwnerType != EGridItemOwnerType::CharacterInventory ||
                Item.OwnerCharacterIndex != CharacterIndex ||
                Item.EquipmentSlot != EGridEquipmentSlot::None)
            {
                OutError = FString::Printf (
                    TEXT ("Inventory item has invalid ownership Character=%d Slot=%d Item=%s Owner=%s OwnerCharacter=%d EquipmentSlot=%s"),
                    CharacterIndex,
                    SlotIndex,
                    *Item.ItemDefinitionId.ToString (),
                    GetOwnerTypeName (Item.OwnerType),
                    Item.OwnerCharacterIndex,
                    GetEquipmentSlotName (Item.EquipmentSlot));
                return false;
            }

            const FString Location = FString::Printf (TEXT ("Inventory Character=%d Slot=%d"), CharacterIndex, SlotIndex);
            if (!RegisterRuntimeOwner (Item, Location))
            {
                return false;
            }
        }

        if (!PartyInventoryState.ActiveEquipment.IsValidIndex (CharacterIndex))
        {
            continue;
        }

        ForEachEquipmentItem (
            PartyInventoryState.ActiveEquipment[CharacterIndex],
            [CharacterIndex, &RegisterRuntimeOwner, &OutError] (EGridEquipmentSlot Slot, const FGridItemInstance& Item)
            {
                if (!Item.IsValid ())
                {
                    return;
                }

                if (Item.OwnerType != EGridItemOwnerType::EquipmentSlot ||
                    Item.OwnerCharacterIndex != CharacterIndex ||
                    Item.EquipmentSlot != Slot)
                {
                    OutError = FString::Printf (
                        TEXT ("Equipment item has invalid ownership Character=%d Slot=%s Item=%s Owner=%s OwnerCharacter=%d EquipmentSlot=%s"),
                        CharacterIndex,
                        GetEquipmentSlotName (Slot),
                        *Item.ItemDefinitionId.ToString (),
                        GetOwnerTypeName (Item.OwnerType),
                        Item.OwnerCharacterIndex,
                        GetEquipmentSlotName (Item.EquipmentSlot));
                    return;
                }

                const FString Location = FString::Printf (TEXT ("Equipment Character=%d Slot=%s"), CharacterIndex, GetEquipmentSlotName (Slot));
                RegisterRuntimeOwner (Item, Location);
            });

        if (!OutError.IsEmpty ())
        {
            return false;
        }
    }

    return true;
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

    if (CharacterState.RaceId.IsNone ())
    {
        CharacterState.RaceId = TEXT ("Human");
    }

    CharacterState.Level = FMath::Max (1, CharacterState.Level);
    CharacterState.Experience = FMath::Max (0, CharacterState.Experience);

    if (!CharacterState.bRPGAttributesInitialized)
    {
        CharacterState.Attributes.Strength = FMath::RoundToInt (FMath::Max (0.0f, CharacterState.Strength));
        CharacterState.bRPGAttributesInitialized = true;
    }

    CharacterState.Attributes.Strength = FMath::Max (0, CharacterState.Attributes.Strength);
    CharacterState.Attributes.Dexterity = FMath::Max (0, CharacterState.Attributes.Dexterity);
    CharacterState.Attributes.Constitution = FMath::Max (0, CharacterState.Attributes.Constitution);
    CharacterState.Attributes.Intelligence = FMath::Max (0, CharacterState.Attributes.Intelligence);
    CharacterState.Attributes.Wisdom = FMath::Max (0, CharacterState.Attributes.Wisdom);
    CharacterState.Attributes.Charisma = FMath::Max (0, CharacterState.Attributes.Charisma);
    CharacterState.MaxCarryWeight =
        URPGCharacterRulesLibrary::CalculateMaxCarryWeight (CharacterState.Attributes);

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
