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
        switch (Slot)
        {
        case EGridEquipmentSlot::MainHand:
        case EGridEquipmentSlot::OffHand:
        case EGridEquipmentSlot::Head:
        case EGridEquipmentSlot::Chest:
        case EGridEquipmentSlot::Legs:
        case EGridEquipmentSlot::Feet:
        case EGridEquipmentSlot::Amulet:
        case EGridEquipmentSlot::Ring1:
        case EGridEquipmentSlot::Ring2:
        case EGridEquipmentSlot::Shoulders:
        case EGridEquipmentSlot::Gloves:
        case EGridEquipmentSlot::Belt:
        case EGridEquipmentSlot::Cloak:
        case EGridEquipmentSlot::Talisman:
        case EGridEquipmentSlot::QuickSlot1:
        case EGridEquipmentSlot::QuickSlot2:
        case EGridEquipmentSlot::Face:
        case EGridEquipmentSlot::Shirt:
        case EGridEquipmentSlot::Bracers:
        case EGridEquipmentSlot::Earring1:
        case EGridEquipmentSlot::Earring2:
            return true;
        case EGridEquipmentSlot::None:
        default:
            return false;
        }
    }

    bool IsHandEquipmentSlot (EGridEquipmentSlot Slot)
    {
        return Slot == EGridEquipmentSlot::MainHand ||
            Slot == EGridEquipmentSlot::OffHand;
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
        case EGridEquipmentSlot::Face:
            return TEXT ("Visage");
        case EGridEquipmentSlot::Shirt:
            return TEXT ("Chemise");
        case EGridEquipmentSlot::Bracers:
            return TEXT ("Brassards");
        case EGridEquipmentSlot::Earring1:
            return TEXT ("Bijou d'oreille I");
        case EGridEquipmentSlot::Earring2:
            return TEXT ("Bijou d'oreille II");
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

    bool GridInventoryCompatibilityDiagnosticsIsHandSlot (EGridEquipmentSlot Slot)
    {
        return Slot == EGridEquipmentSlot::MainHand ||
            Slot == EGridEquipmentSlot::OffHand;
    }

    bool GridInventoryCompatibilityDiagnosticsIsExcludedPaperDollSlot (EGridEquipmentSlot Slot)
    {
        return Slot == EGridEquipmentSlot::Talisman ||
            Slot == EGridEquipmentSlot::QuickSlot1 ||
            Slot == EGridEquipmentSlot::QuickSlot2;
    }

    bool GridInventoryCompatibilityDiagnosticsIsNewPaperDollSlot (EGridEquipmentSlot Slot)
    {
        return Slot == EGridEquipmentSlot::Face ||
            Slot == EGridEquipmentSlot::Shirt ||
            Slot == EGridEquipmentSlot::Bracers ||
            Slot == EGridEquipmentSlot::Earring1 ||
            Slot == EGridEquipmentSlot::Earring2;
    }

    bool GridInventoryCompatibilityDiagnosticsLooksPotentiallyEquippable (
        const UGridItemDefinitionAsset* Definition)
    {
        if (!Definition)
        {
            return false;
        }

        switch (Definition->ItemType)
        {
        case EGridItemType::Torch:
        case EGridItemType::Weapon:
        case EGridItemType::Shield:
        case EGridItemType::Armor:
        case EGridItemType::Jewelry:
            return true;
        default:
            break;
        }

        return !Definition->EquippedMesh.IsNull ();
    }

    void AddEquipmentStatBonus (
        FGridEquipmentStatBonus& InOutTotal,
        const FGridEquipmentStatBonus& Bonus)
    {
        InOutTotal.StrengthBonus += Bonus.StrengthBonus;
        InOutTotal.DexterityBonus += Bonus.DexterityBonus;
        InOutTotal.ConstitutionBonus += Bonus.ConstitutionBonus;
        InOutTotal.IntelligenceBonus += Bonus.IntelligenceBonus;
        InOutTotal.WisdomBonus += Bonus.WisdomBonus;
        InOutTotal.CharismaBonus += Bonus.CharismaBonus;
        InOutTotal.MaxHealthBonus += Bonus.MaxHealthBonus;
        InOutTotal.MaxManaBonus += Bonus.MaxManaBonus;
        InOutTotal.CarryWeightBonus += Bonus.CarryWeightBonus;
        InOutTotal.ArmorBonus += Bonus.ArmorBonus;
    }

    FString GetEquipmentStatBonusText (const FGridEquipmentStatBonus& Bonus)
    {
        return FString::Printf (
            TEXT ("STR=%d DEX=%d CON=%d INT=%d WIS=%d CHA=%d MaxHealth=%d MaxMana=%d CarryWeight=%.1f Armor=%d"),
            Bonus.StrengthBonus,
            Bonus.DexterityBonus,
            Bonus.ConstitutionBonus,
            Bonus.IntelligenceBonus,
            Bonus.WisdomBonus,
            Bonus.CharismaBonus,
            Bonus.MaxHealthBonus,
            Bonus.MaxManaBonus,
            Bonus.CarryWeightBonus,
            Bonus.ArmorBonus);
    }

    FString GetDamageResistanceSetText (const FGridDamageResistanceSet& Resistances)
    {
        return FString::Printf (
            TEXT ("Physical=%d Fire=%d Ice=%d Lightning=%d Poison=%d Holy=%d Necrotic=%d Arcane=%d"),
            Resistances.PhysicalResistance,
            Resistances.FireResistance,
            Resistances.IceResistance,
            Resistances.LightningResistance,
            Resistances.PoisonResistance,
            Resistances.HolyResistance,
            Resistances.NecroticResistance,
            Resistances.ArcaneResistance);
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

    bool CharacterHasInventoryItemDefinition (
        const FGridCharacterInventoryState& CharacterState,
        FName ItemDefinitionId)
    {
        return !ItemDefinitionId.IsNone () &&
            CharacterState.InventorySlots.ContainsByPredicate (
                [ItemDefinitionId] (const FGridInventorySlot& Slot)
                {
                    return !Slot.IsEmpty () &&
                        Slot.Item.ItemDefinitionId == ItemDefinitionId &&
                        Slot.Item.Quantity > 0;
                });
    }

    void ClearQuickItemHotbarBindings (
        FGridCharacterInventoryState& CharacterState,
        FName ItemDefinitionId)
    {
        for (int32 SlotIndex = 0;
            SlotIndex < CharacterState.CombatHotbarSlots.Num ();
            ++SlotIndex)
        {
            FGridCombatHotbarBinding& Binding =
                CharacterState.CombatHotbarSlots[SlotIndex];
            if (Binding.SourcePolicy !=
                    EGridCombatActionSourcePolicy::QuickItem ||
                Binding.SourceDefinitionId != ItemDefinitionId)
            {
                continue;
            }

            Binding.Reset (SlotIndex);
        }
    }

    void SanitizeCombatHotbarBindings (
        FGridCharacterInventoryState& CharacterState)
    {
        TSet<FGuid> AssignedEquipmentRuntimeIds;
        TSet<FName> AssignedQuickItemDefinitionIds;
        for (int32 SlotIndex = 0;
            SlotIndex < CharacterState.CombatHotbarSlots.Num ();
            ++SlotIndex)
        {
            FGridCombatHotbarBinding& Binding =
                CharacterState.CombatHotbarSlots[SlotIndex];
            if (!Binding.IsValid () || Binding.IsEmpty ())
            {
                continue;
            }

            if (Binding.SourcePolicy ==
                    EGridCombatActionSourcePolicy::Equipment)
            {
                if (AssignedEquipmentRuntimeIds.Contains (
                        Binding.PreferredSourceRuntimeId))
                {
                    Binding.Reset (SlotIndex);
                }
                else
                {
                    AssignedEquipmentRuntimeIds.Add (
                        Binding.PreferredSourceRuntimeId);
                }
            }
            else if (Binding.SourcePolicy ==
                EGridCombatActionSourcePolicy::QuickItem)
            {
                if (!CharacterHasInventoryItemDefinition (
                        CharacterState,
                        Binding.SourceDefinitionId) ||
                    AssignedQuickItemDefinitionIds.Contains (
                        Binding.SourceDefinitionId))
                {
                    Binding.Reset (SlotIndex);
                }
                else
                {
                    AssignedQuickItemDefinitionIds.Add (
                        Binding.SourceDefinitionId);
                }
            }
        }
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
            EGridEquipmentSlot::Ring2,
            EGridEquipmentSlot::Shoulders,
            EGridEquipmentSlot::Gloves,
            EGridEquipmentSlot::Belt,
            EGridEquipmentSlot::Cloak,
            EGridEquipmentSlot::Talisman,
            EGridEquipmentSlot::QuickSlot1,
            EGridEquipmentSlot::QuickSlot2,
            EGridEquipmentSlot::Face,
            EGridEquipmentSlot::Shirt,
            EGridEquipmentSlot::Bracers,
            EGridEquipmentSlot::Earring1,
            EGridEquipmentSlot::Earring2
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

void UGridPartyInventoryComponent::NotifyPartyInventoryChanged (
    int32 CharacterIndex)
{
    OnPartyInventoryChanged.Broadcast (CharacterIndex);
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

    for (FGridCharacterInventoryState& CharacterState : PartyInventoryState.CharacterPool)
    {
        InitializeCombatHotbarDefaults (CharacterState);
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

void UGridPartyInventoryComponent::ResetPartyForNewGame ()
{
    PartyInventoryState = FGridPartyInventoryState ();
    InitializeDefaultPartyIfNeeded ();
}

bool UGridPartyInventoryComponent::RestorePartyInventoryState (
    const FGridPartyInventoryState& SavedState,
    FText& OutError)
{
    OutError = FText::GetEmpty ();

    if (!SavedState.bInitialCharacterCreationCompleted)
    {
        OutError = FText::FromString (TEXT ("La sauvegarde ne contient aucun personnage finalisé."));
        return false;
    }

    if (SavedState.ActiveCharacters.Num () < 1 ||
        SavedState.MaxActiveCharacters < SavedState.ActiveCharacters.Num ())
    {
        OutError = FText::FromString (TEXT ("Le groupe sauvegardé possède un nombre de personnages invalide."));
        return false;
    }

    if (SavedState.ActiveEquipment.Num () != SavedState.ActiveCharacters.Num ())
    {
        OutError = FText::FromString (TEXT ("Les personnages et leurs équipements sauvegardés ne sont pas alignés."));
        return false;
    }

    if (!SavedState.ActiveCharacters.IsValidIndex (SavedState.SelectedCharacterIndex))
    {
        OutError = FText::FromString (TEXT ("Le personnage sélectionné dans la sauvegarde est invalide."));
        return false;
    }

    FGridPartyInventoryState RestoredState = SavedState;
    for (FGridCharacterInventoryState& Character : RestoredState.ActiveCharacters)
    {
        if (!Character.CharacterId.IsValid ())
        {
            OutError = FText::FromString (TEXT ("Un personnage sauvegardé ne possède pas d'identifiant valide."));
            return false;
        }

        if (Character.CombatHotbarSlots.IsEmpty ())
        {
            InitializeCombatHotbarDefaults (Character);
        }
        else
        {
            SanitizeCombatHotbarBindings (Character);
        }

        FString HotbarError;
        if (!ValidateCombatHotbar (Character, HotbarError))
        {
            OutError = FText::FromString (
                FString::Printf (
                    TEXT ("La barre de raccourcis d'un personnage sauvegardé est invalide : %s"),
                    *HotbarError));
            return false;
        }
    }

    for (FGridCharacterInventoryState& Character : RestoredState.CharacterPool)
    {
        if (Character.CombatHotbarSlots.IsEmpty ())
        {
            InitializeCombatHotbarDefaults (Character);
        }
        else
        {
            SanitizeCombatHotbarBindings (Character);
        }

        FString HotbarError;
        if (!ValidateCombatHotbar (Character, HotbarError))
        {
            OutError = FText::FromString (
                FString::Printf (
                    TEXT ("La barre de raccourcis d'un personnage en réserve est invalide : %s"),
                    *HotbarError));
            return false;
        }
    }

    const FGridPartyInventoryState PreviousState = PartyInventoryState;
    PartyInventoryState = MoveTemp (RestoredState);
    RecalculateAllWeights ();

    FString OwnershipError;
    if (!ValidateInventoryOwnership (OwnershipError))
    {
        PartyInventoryState = PreviousState;
        NotifyPartyInventoryChanged (INDEX_NONE);
        OutError = FText::FromString (
            FString::Printf (
                TEXT ("L'ownership de la sauvegarde est invalide : %s"),
                *OwnershipError));
        return false;
    }

    return true;
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

    URPGClassAsset* CombatActionSourceClass =
        Request.CombatActionSourceClassDefinition
        ? Request.CombatActionSourceClassDefinition.Get ()
        : Request.ClassDefinition.Get ();
    if (!CombatActionSourceClass ||
        !CombatActionSourceClass->IsValidDefinition () ||
        CombatActionSourceClass->ClassId !=
            Request.ClassDefinition->ClassId)
    {
        OutError = FText::FromString (
            TEXT ("La source des actions de classe est invalide."));
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
    NewCharacter.RaceDisplayName = Request.RaceDefinition->DisplayName;
    NewCharacter.ClassId = Request.ClassDefinition->ClassId;
    NewCharacter.ClassDisplayName = Request.ClassDefinition->DisplayName;
    NewCharacter.ClassDefinition = CombatActionSourceClass;
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
    InitializeCombatHotbarDefaults (NewCharacter);

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
        NotifyPartyInventoryChanged (INDEX_NONE);
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
    if (OldIndex != NewIndex)
    {
        NotifyPartyInventoryChanged (INDEX_NONE);
    }
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
    OutSummary.ClassDisplayName = CharacterState.ClassDisplayName.IsEmpty ()
        ? FText::FromName (CharacterState.ClassId)
        : CharacterState.ClassDisplayName;
    OutSummary.RaceId = CharacterState.RaceId;
    OutSummary.RaceDisplayName = CharacterState.RaceDisplayName.IsEmpty ()
        ? FText::FromName (CharacterState.RaceId)
        : CharacterState.RaceDisplayName;
    OutSummary.Level = CharacterState.Level;
    OutSummary.Experience = CharacterState.Experience;
    OutSummary.BaseAttributes = CharacterState.Attributes;
    OutSummary.BaseDerivedStats = CharacterState.DerivedStats;
    OutSummary.BaseMaxWeight = CharacterState.MaxCarryWeight;
    OutSummary.EquipmentStatBonus = ComputeCharacterEquipmentStatBonus (CharacterIndex);
    OutSummary.EquipmentResistances = ComputeCharacterEquipmentResistances (CharacterIndex);
    OutSummary.FinalResistances = OutSummary.EquipmentResistances;
    OutSummary.Attributes = OutSummary.BaseAttributes;
    OutSummary.Attributes.Strength = FMath::Max (0, OutSummary.Attributes.Strength + OutSummary.EquipmentStatBonus.StrengthBonus);
    OutSummary.Attributes.Dexterity = FMath::Max (0, OutSummary.Attributes.Dexterity + OutSummary.EquipmentStatBonus.DexterityBonus);
    OutSummary.Attributes.Constitution = FMath::Max (0, OutSummary.Attributes.Constitution + OutSummary.EquipmentStatBonus.ConstitutionBonus);
    OutSummary.Attributes.Intelligence = FMath::Max (0, OutSummary.Attributes.Intelligence + OutSummary.EquipmentStatBonus.IntelligenceBonus);
    OutSummary.Attributes.Wisdom = FMath::Max (0, OutSummary.Attributes.Wisdom + OutSummary.EquipmentStatBonus.WisdomBonus);
    OutSummary.Attributes.Charisma = FMath::Max (0, OutSummary.Attributes.Charisma + OutSummary.EquipmentStatBonus.CharismaBonus);
    OutSummary.DerivedStats = OutSummary.BaseDerivedStats;
    OutSummary.DerivedStats.MaxHealth = FMath::Max (1, OutSummary.DerivedStats.MaxHealth + OutSummary.EquipmentStatBonus.MaxHealthBonus);
    OutSummary.DerivedStats.CurrentHealth = FMath::Clamp (
        OutSummary.DerivedStats.CurrentHealth,
        0,
        OutSummary.DerivedStats.MaxHealth);
    OutSummary.DerivedStats.MaxMana = FMath::Max (0, OutSummary.DerivedStats.MaxMana + OutSummary.EquipmentStatBonus.MaxManaBonus);
    OutSummary.DerivedStats.CurrentMana = FMath::Clamp (
        OutSummary.DerivedStats.CurrentMana,
        0,
        OutSummary.DerivedStats.MaxMana);
    OutSummary.DerivedStats.PhysicalArmor = FMath::Max (
        0,
        OutSummary.DerivedStats.PhysicalArmor + OutSummary.EquipmentStatBonus.ArmorBonus);
    OutSummary.Portrait = CharacterState.Portrait;
    OutSummary.UsedInventorySlots = CountOccupiedSlots (CharacterState);
    OutSummary.MaxInventorySlots = CharacterState.InventorySlots.Num ();
    OutSummary.CurrentWeight = CharacterState.CurrentWeight;
    OutSummary.MaxWeight = FMath::Max (0.0f, OutSummary.BaseMaxWeight + OutSummary.EquipmentStatBonus.CarryWeightBonus);
    OutSummary.bOverloaded = OutSummary.CurrentWeight > OutSummary.MaxWeight;
    OutSummary.bIsSelected = CharacterIndex == PartyInventoryState.SelectedCharacterIndex;
    return true;
}

int32 UGridPartyInventoryComponent::GetCombatHotbarSlotCount () const
{
    return FGridCombatHotbarBinding::SlotCount;
}

bool UGridPartyInventoryComponent::GetCharacterCombatHotbarBinding (
    int32 CharacterIndex,
    int32 SlotIndex,
    FGridCombatHotbarBinding& OutBinding) const
{
    OutBinding = FGridCombatHotbarBinding ();
    if (!IsValidCharacterIndex (CharacterIndex) ||
        SlotIndex < 0 ||
        SlotIndex >= FGridCombatHotbarBinding::SlotCount)
    {
        return false;
    }

    const FGridCharacterInventoryState& Character =
        PartyInventoryState.ActiveCharacters[CharacterIndex];
    if (!Character.CombatHotbarSlots.IsValidIndex (SlotIndex))
    {
        return false;
    }

    OutBinding = Character.CombatHotbarSlots[SlotIndex];
    return true;
}

bool UGridPartyInventoryComponent::SetCharacterCombatHotbarBinding (
    int32 CharacterIndex,
    int32 SlotIndex,
    const FGridCombatHotbarBinding& Binding)
{
    if (!IsValidCharacterIndex (CharacterIndex) ||
        SlotIndex < 0 ||
        SlotIndex >= FGridCombatHotbarBinding::SlotCount)
    {
        return false;
    }

    FGridCharacterInventoryState& Character =
        PartyInventoryState.ActiveCharacters[CharacterIndex];
    if (!Character.CombatHotbarSlots.IsValidIndex (SlotIndex))
    {
        return false;
    }

    FGridCombatHotbarBinding NormalizedBinding = Binding;
    if (NormalizedBinding.IsEmpty ())
    {
        NormalizedBinding.Reset (SlotIndex);
    }
    else
    {
        NormalizedBinding.SlotIndex = SlotIndex;
    }

    if (!NormalizedBinding.IsValid ())
    {
        return false;
    }

    if (NormalizedBinding.SourcePolicy ==
            EGridCombatActionSourcePolicy::QuickItem &&
        !CharacterHasInventoryItemDefinition (
            Character,
            NormalizedBinding.SourceDefinitionId))
    {
        return false;
    }

    if (NormalizedBinding.SourcePolicy ==
            EGridCombatActionSourcePolicy::Equipment ||
        NormalizedBinding.SourcePolicy ==
            EGridCombatActionSourcePolicy::QuickItem)
    {
        for (int32 ExistingSlotIndex = 0;
            ExistingSlotIndex < Character.CombatHotbarSlots.Num ();
            ++ExistingSlotIndex)
        {
            if (ExistingSlotIndex == SlotIndex)
            {
                continue;
            }

            FGridCombatHotbarBinding& ExistingBinding =
                Character.CombatHotbarSlots[ExistingSlotIndex];
            const bool bSameEquipmentItem =
                NormalizedBinding.SourcePolicy ==
                    EGridCombatActionSourcePolicy::Equipment &&
                ExistingBinding.SourcePolicy ==
                    EGridCombatActionSourcePolicy::Equipment &&
                ExistingBinding.PreferredSourceRuntimeId ==
                    NormalizedBinding.PreferredSourceRuntimeId;
            const bool bSameQuickItemDefinition =
                NormalizedBinding.SourcePolicy ==
                    EGridCombatActionSourcePolicy::QuickItem &&
                ExistingBinding.SourcePolicy ==
                    EGridCombatActionSourcePolicy::QuickItem &&
                ExistingBinding.SourceDefinitionId ==
                    NormalizedBinding.SourceDefinitionId;
            if (bSameEquipmentItem || bSameQuickItemDefinition)
            {
                ExistingBinding.Reset (ExistingSlotIndex);
            }
        }
    }

    Character.CombatHotbarSlots[SlotIndex] = MoveTemp (NormalizedBinding);
    NotifyPartyInventoryChanged (CharacterIndex);
    return true;
}

bool UGridPartyInventoryComponent::ClearCharacterCombatHotbarBinding (
    int32 CharacterIndex,
    int32 SlotIndex)
{
    FGridCombatHotbarBinding EmptyBinding;
    EmptyBinding.Reset (SlotIndex);
    return SetCharacterCombatHotbarBinding (
        CharacterIndex,
        SlotIndex,
        EmptyBinding);
}

bool UGridPartyInventoryComponent::SetCharacterCombatHotbarBindingFromItem (
    int32 CharacterIndex,
    int32 SlotIndex,
    const FGridItemInstance& SourceItem,
    EGridEquipmentSlot SourceEquipmentSlot)
{
    if (!IsValidCharacterIndex (CharacterIndex) ||
        SlotIndex < 0 ||
        SlotIndex >= FGridCombatHotbarBinding::SlotCount ||
        !SourceItem.IsValid ())
    {
        return false;
    }

    const UGridItemDefinitionAsset* Definition =
        FindItemDefinition (SourceItem.ItemDefinitionId);
    if (!IsValid (Definition) || !Definition->IsValidDefinition ())
    {
        return false;
    }

    FGridCombatHotbarBinding Binding;
    if (SourceEquipmentSlot != EGridEquipmentSlot::None)
    {
        if (SourceEquipmentSlot != EGridEquipmentSlot::MainHand &&
            SourceEquipmentSlot != EGridEquipmentSlot::OffHand)
        {
            return false;
        }
        if (!Definition->CompatibleEquipmentSlots.Contains (
            SourceEquipmentSlot))
        {
            return false;
        }

        FGridItemInstance EquippedItem;
        if (!GetEquippedItem (
                CharacterIndex,
                SourceEquipmentSlot,
                EquippedItem) ||
            EquippedItem.RuntimeObjectId != SourceItem.RuntimeObjectId ||
            EquippedItem.ItemDefinitionId != SourceItem.ItemDefinitionId)
        {
            return false;
        }

        FName PrimaryActionId = NAME_None;
        if (!Definition->CombatActions.IsEmpty ())
        {
            const FGridCombatActionDefinition* PrimaryAction =
                Definition->CombatActions.FindByPredicate (
                    [] (const FGridCombatActionDefinition& Candidate)
                    {
                        return Candidate.IsValid () &&
                            Candidate.SourcePolicy ==
                                EGridCombatActionSourcePolicy::Equipment;
                    });
            if (PrimaryAction)
            {
                PrimaryActionId = PrimaryAction->ActionId;
            }
        }
        else if (Definition->CanProvideAttackFromSlot (
            SourceEquipmentSlot))
        {
            PrimaryActionId = Definition->OffensiveProfile.AttackId;
        }

        if (PrimaryActionId.IsNone ())
        {
            return false;
        }

        Binding.ActionId = PrimaryActionId;
        Binding.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
        Binding.SourceDefinitionId = SourceItem.ItemDefinitionId;
        Binding.PreferredSourceRuntimeId = SourceItem.RuntimeObjectId;
        Binding.PreferredEquipmentSlot = SourceEquipmentSlot;
    }
    else
    {
        const FGridCharacterInventoryState& Character =
            PartyInventoryState.ActiveCharacters[CharacterIndex];
        const bool bItemStillOwned =
            Character.InventorySlots.ContainsByPredicate (
                [&SourceItem] (const FGridInventorySlot& Candidate)
                {
                    return !Candidate.IsEmpty () &&
                        Candidate.Item.RuntimeObjectId ==
                            SourceItem.RuntimeObjectId &&
                        Candidate.Item.ItemDefinitionId ==
                            SourceItem.ItemDefinitionId;
                });
        FGridCombatActionDefinition InventoryAction;
        if (!bItemStillOwned ||
            !Definition->BuildInventoryCombatActionDefinition (
                2,
                InventoryAction))
        {
            return false;
        }

        Binding.ActionId = InventoryAction.ActionId;
        Binding.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
        Binding.SourceDefinitionId = SourceItem.ItemDefinitionId;
    }

    return SetCharacterCombatHotbarBinding (
        CharacterIndex,
        SlotIndex,
        Binding);
}

bool UGridPartyInventoryComponent::MoveOrSwapCharacterCombatHotbarBinding (
    int32 CharacterIndex,
    int32 SourceSlotIndex,
    int32 TargetSlotIndex)
{
    if (!IsValidCharacterIndex (CharacterIndex) ||
        SourceSlotIndex < 0 ||
        SourceSlotIndex >= FGridCombatHotbarBinding::SlotCount ||
        TargetSlotIndex < 0 ||
        TargetSlotIndex >= FGridCombatHotbarBinding::SlotCount)
    {
        return false;
    }

    FGridCharacterInventoryState& Character =
        PartyInventoryState.ActiveCharacters[CharacterIndex];
    if (!Character.CombatHotbarSlots.IsValidIndex (SourceSlotIndex) ||
        !Character.CombatHotbarSlots.IsValidIndex (TargetSlotIndex) ||
        Character.CombatHotbarSlots[SourceSlotIndex].IsEmpty ())
    {
        return false;
    }
    if (SourceSlotIndex == TargetSlotIndex)
    {
        return true;
    }

    FGridCombatHotbarBinding SourceBinding =
        Character.CombatHotbarSlots[SourceSlotIndex];
    FGridCombatHotbarBinding TargetBinding =
        Character.CombatHotbarSlots[TargetSlotIndex];
    SourceBinding.SlotIndex = TargetSlotIndex;
    if (TargetBinding.IsEmpty ())
    {
        TargetBinding.Reset (SourceSlotIndex);
    }
    else
    {
        TargetBinding.SlotIndex = SourceSlotIndex;
    }

    if (!SourceBinding.IsValid () || !TargetBinding.IsValid ())
    {
        return false;
    }

    Character.CombatHotbarSlots[SourceSlotIndex] =
        MoveTemp (TargetBinding);
    Character.CombatHotbarSlots[TargetSlotIndex] =
        MoveTemp (SourceBinding);
    NotifyPartyInventoryChanged (CharacterIndex);
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

    if (RemainingToRemove != 0)
    {
        return false;
    }

    ClearQuickItemHotbarBindings (
        CharacterState,
        ItemDefinitionId);

    RecalculateCharacterWeight (CharacterIndex);
    return true;
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
    NotifyPartyInventoryChanged (INDEX_NONE);

    UE_LOG (LogTemp, Log, TEXT ("GridInventory Registered ItemDefinition=%s Asset=%s"),
        *Definition->ItemDefinitionId.ToString (),
        *Definition->GetPathName ());

    return true;
}

bool UGridPartyInventoryComponent::RehydrateOwnedItemDefinitions (
    TFunctionRef<UGridItemDefinitionAsset* (FName)> Resolver,
    FName& OutMissingDefinitionId)
{
    OutMissingDefinitionId = NAME_None;

    TSet<FName> DefinitionIds;
    auto CollectCharacterItems = [&DefinitionIds] (const FGridCharacterInventoryState& Character)
    {
        for (const FGridInventorySlot& Slot : Character.InventorySlots)
        {
            if (!Slot.IsEmpty ())
            {
                DefinitionIds.Add (Slot.Item.ItemDefinitionId);
            }
        }
        for (const FGridCombatHotbarBinding& Binding :
            Character.CombatHotbarSlots)
        {
            const bool bUsesItemDefinition =
                Binding.SourcePolicy ==
                    EGridCombatActionSourcePolicy::Equipment ||
                Binding.SourcePolicy ==
                    EGridCombatActionSourcePolicy::QuickItem;
            if (!Binding.IsEmpty () &&
                bUsesItemDefinition &&
                !Binding.SourceDefinitionId.IsNone ())
            {
                DefinitionIds.Add (Binding.SourceDefinitionId);
            }
        }
    };

    for (const FGridCharacterInventoryState& Character : PartyInventoryState.ActiveCharacters)
    {
        CollectCharacterItems (Character);
    }
    for (const FGridCharacterInventoryState& Character : PartyInventoryState.CharacterPool)
    {
        CollectCharacterItems (Character);
    }
    for (const FGridCharacterEquipmentState& Equipment : PartyInventoryState.ActiveEquipment)
    {
        ForEachEquipmentItem (
            Equipment,
            [&DefinitionIds] (EGridEquipmentSlot Slot, const FGridItemInstance& Item)
            {
                (void)Slot;
                if (Item.IsValid ())
                {
                    DefinitionIds.Add (Item.ItemDefinitionId);
                }
            });
    }
    if (PartyInventoryState.bHasCursorItem && PartyInventoryState.CursorItem.IsValid ())
    {
        DefinitionIds.Add (PartyInventoryState.CursorItem.ItemDefinitionId);
    }

    TArray<UGridItemDefinitionAsset*> ResolvedDefinitions;
    ResolvedDefinitions.Reserve (DefinitionIds.Num ());
    for (const FName DefinitionId : DefinitionIds)
    {
        UGridItemDefinitionAsset* Definition = Resolver (DefinitionId);
        if (!Definition ||
            Definition->ItemDefinitionId.IsNone () ||
            Definition->ItemDefinitionId != DefinitionId)
        {
            OutMissingDefinitionId = DefinitionId;
            return false;
        }
        ResolvedDefinitions.Add (Definition);
    }

    RuntimeItemDefinitionsById.Reset ();
    for (UGridItemDefinitionAsset* Definition : ResolvedDefinitions)
    {
        RegisterItemDefinition (Definition);
    }
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

bool UGridPartyInventoryComponent::
TryExtractOneEquippedItemForWorldTransfer (
    int32 CharacterIndex,
    EGridEquipmentSlot SourceSlot,
    FName ExpectedItemDefinitionId,
    FGridItemInstance& OutWorldItem)
{
    OutWorldItem = FGridItemInstance ();
    EnsureEquipmentCountMatchesActiveCharacters ();
    if (!IsValidCharacterIndex (CharacterIndex) ||
        !PartyInventoryState.ActiveEquipment.IsValidIndex (
            CharacterIndex) ||
        !IsHandEquipmentSlot (SourceSlot) ||
        ExpectedItemDefinitionId.IsNone ())
    {
        return false;
    }

    FGridItemInstance* EquippedItem =
        PartyInventoryState.ActiveEquipment[CharacterIndex]
            .GetMutableSlot (SourceSlot);
    if (!EquippedItem ||
        !EquippedItem->IsValid () ||
        EquippedItem->ItemDefinitionId !=
            ExpectedItemDefinitionId)
    {
        return false;
    }

    OutWorldItem = *EquippedItem;
    OutWorldItem.Quantity = 1;
    OutWorldItem.OwnerType = EGridItemOwnerType::World;
    OutWorldItem.OwnerGuid = FGuid ();
    OutWorldItem.OwnerCharacterIndex = INDEX_NONE;
    OutWorldItem.EquipmentSlot = EGridEquipmentSlot::None;

    const int32 QuantityBefore =
        FMath::Max (1, EquippedItem->Quantity);
    if (QuantityBefore > 1)
    {
        --EquippedItem->Quantity;
        OutWorldItem.RuntimeObjectId = FGuid::NewGuid ();
    }
    else
    {
        *EquippedItem = FGridItemInstance ();
    }
    RecalculateCharacterWeight (CharacterIndex);

    UE_LOG (
        LogTemp,
        Log,
        TEXT ("GridInventory EquipmentWorldTransfer Extract Character=%d Slot=%s Item=%s RuntimeId=%s Quantity=%d->%d Result=true"),
        CharacterIndex,
        GetEquipmentSlotName (SourceSlot),
        *OutWorldItem.ItemDefinitionId.ToString (),
        *OutWorldItem.RuntimeObjectId.ToString (),
        QuantityBefore,
        QuantityBefore - 1);
    return true;
}

bool UGridPartyInventoryComponent::
TryRestoreExtractedItemToEquipment (
    int32 CharacterIndex,
    EGridEquipmentSlot TargetSlot,
    const FGridItemInstance& WorldItem)
{
    EnsureEquipmentCountMatchesActiveCharacters ();
    if (!WorldItem.IsValid () ||
        WorldItem.Quantity != 1 ||
        !IsValidCharacterIndex (CharacterIndex) ||
        !PartyInventoryState.ActiveEquipment.IsValidIndex (
            CharacterIndex) ||
        !IsHandEquipmentSlot (TargetSlot))
    {
        return false;
    }

    FGridItemInstance* EquippedItem =
        PartyInventoryState.ActiveEquipment[CharacterIndex]
            .GetMutableSlot (TargetSlot);
    if (!EquippedItem)
    {
        return false;
    }

    const FGridCharacterInventoryState& CharacterState =
        PartyInventoryState.ActiveCharacters[CharacterIndex];
    if (EquippedItem->IsValid ())
    {
        if (EquippedItem->ItemDefinitionId !=
            WorldItem.ItemDefinitionId)
        {
            return false;
        }
        ++EquippedItem->Quantity;
    }
    else
    {
        *EquippedItem = WorldItem;
        EquippedItem->OwnerType =
            EGridItemOwnerType::EquipmentSlot;
        EquippedItem->OwnerGuid = CharacterState.CharacterId;
        EquippedItem->OwnerCharacterIndex = CharacterIndex;
        EquippedItem->EquipmentSlot = TargetSlot;
    }
    RecalculateCharacterWeight (CharacterIndex);

    UE_LOG (
        LogTemp,
        Warning,
        TEXT ("GridInventory EquipmentWorldTransfer Restored Character=%d Slot=%s Item=%s RuntimeId=%s Result=true"),
        CharacterIndex,
        GetEquipmentSlotName (TargetSlot),
        *WorldItem.ItemDefinitionId.ToString (),
        *WorldItem.RuntimeObjectId.ToString ());
    return true;
}

bool UGridPartyInventoryComponent::
TryConsumeEquippedItemQuantityForCombatAction (
    int32 CharacterIndex,
    EGridEquipmentSlot SourceSlot,
    FName ExpectedItemDefinitionId,
    const FGuid& ExpectedRuntimeObjectId,
    int32 Quantity)
{
    EnsureEquipmentCountMatchesActiveCharacters ();
    if (!IsValidCharacterIndex (CharacterIndex) ||
        !PartyInventoryState.ActiveEquipment.IsValidIndex (
            CharacterIndex) ||
        !IsHandEquipmentSlot (SourceSlot) ||
        ExpectedItemDefinitionId.IsNone () ||
        Quantity <= 0)
    {
        return false;
    }

    FGridItemInstance* EquippedItem =
        PartyInventoryState.ActiveEquipment[CharacterIndex]
            .GetMutableSlot (SourceSlot);
    if (!EquippedItem ||
        !EquippedItem->IsValid () ||
        EquippedItem->ItemDefinitionId != ExpectedItemDefinitionId ||
        (ExpectedRuntimeObjectId.IsValid () &&
            EquippedItem->RuntimeObjectId != ExpectedRuntimeObjectId) ||
        FMath::Max (1, EquippedItem->Quantity) < Quantity)
    {
        return false;
    }

    const int32 QuantityBefore = FMath::Max (1, EquippedItem->Quantity);
    if (QuantityBefore == Quantity)
    {
        *EquippedItem = FGridItemInstance ();
    }
    else
    {
        EquippedItem->Quantity = QuantityBefore - Quantity;
    }
    RecalculateCharacterWeight (CharacterIndex);
    return true;
}

FGridEquipmentStatBonus UGridPartyInventoryComponent::ComputeCharacterEquipmentStatBonus (
    int32 CharacterIndex) const
{
    FGridEquipmentStatBonus TotalBonus;
    if (!IsValidCharacterIndex (CharacterIndex) ||
        !PartyInventoryState.ActiveEquipment.IsValidIndex (CharacterIndex))
    {
        return TotalBonus;
    }

    ForEachEquipmentItem (
        PartyInventoryState.ActiveEquipment[CharacterIndex],
        [this, &TotalBonus] (EGridEquipmentSlot, const FGridItemInstance& Item)
        {
            if (!Item.IsValid ())
            {
                return;
            }

            const UGridItemDefinitionAsset* Definition = FindItemDefinition (Item.ItemDefinitionId);
            if (!Definition)
            {
                return;
            }

            AddEquipmentStatBonus (TotalBonus, Definition->EquipmentStatBonus);
        });

    return TotalBonus;
}

FGridDamageResistanceSet UGridPartyInventoryComponent::ComputeCharacterEquipmentResistances (
    int32 CharacterIndex) const
{
    FGridDamageResistanceSet TotalResistances;
    if (!IsValidCharacterIndex (CharacterIndex) ||
        !PartyInventoryState.ActiveEquipment.IsValidIndex (CharacterIndex))
    {
        return TotalResistances;
    }

    ForEachEquipmentItem (
        PartyInventoryState.ActiveEquipment[CharacterIndex],
        [this, &TotalResistances] (EGridEquipmentSlot, const FGridItemInstance& Item)
        {
            if (!Item.IsValid ())
            {
                return;
            }

            const UGridItemDefinitionAsset* Definition = FindItemDefinition (Item.ItemDefinitionId);
            if (!Definition)
            {
                return;
            }

            TotalResistances.Add (Definition->EquipmentResistanceBonus);
        });

    return TotalResistances;
}

FString UGridPartyInventoryComponent::GetEquipmentDiagnosticsForCharacter (int32 CharacterIndex) const
{
    if (!IsValidCharacterIndex (CharacterIndex) || !PartyInventoryState.ActiveEquipment.IsValidIndex (CharacterIndex))
    {
        return TEXT ("    Equipment: None");
    }

    const FGridCharacterEquipmentState& EquipmentState = PartyInventoryState.ActiveEquipment[CharacterIndex];
    TArray<FString> OccupiedSlots;
    ForEachEquipmentItem (
        EquipmentState,
        [&OccupiedSlots] (EGridEquipmentSlot Slot, const FGridItemInstance& Item)
        {
            if (Item.IsValid ())
            {
                OccupiedSlots.Add (FString::Printf (
                    TEXT ("%s=%s"),
                    GetEquipmentSlotName (Slot),
                    *Item.ItemDefinitionId.ToString ()));
            }
        });

    return OccupiedSlots.Num () > 0
        ? FString::Printf (TEXT ("    Equipment: %s"), *FString::Join (OccupiedSlots, TEXT (" ")))
        : TEXT ("    Equipment: None");
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
    NotifyPartyInventoryChanged (CharacterIndex);
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

void UGridPartyInventoryComponent::LogEquipmentCompatibilityDiagnostics () const
{
    UE_LOG (LogTemp, Log,
        TEXT ("GridEquipmentCompatibility Diagnostics RuntimeDefinitions=%d"),
        RuntimeItemDefinitionsById.Num ());

    TArray<FName> DefinitionIds;
    RuntimeItemDefinitionsById.GetKeys (DefinitionIds);
    DefinitionIds.Sort (FNameLexicalLess ());

    int32 PotentiallyEquippableWithoutSlotsCount = 0;
    int32 LightWithoutHandSlotCount = 0;
    int32 ExcludedPaperDollSlotCount = 0;
    int32 NewPaperDollSlotUsageCount = 0;

    for (const FName DefinitionId : DefinitionIds)
    {
        const TObjectPtr<UGridItemDefinitionAsset>* DefinitionEntry =
            RuntimeItemDefinitionsById.Find (DefinitionId);
        const UGridItemDefinitionAsset* Definition =
            DefinitionEntry ? DefinitionEntry->Get () : nullptr;
        if (!Definition)
        {
            UE_LOG (LogTemp, Warning,
                TEXT ("GridEquipmentCompatibility Item=%s Warning=NullDefinition"),
                *DefinitionId.ToString ());
            continue;
        }

        const FString SlotsText = GetEquipmentSlotsText (Definition->CompatibleEquipmentSlots);
        if (Definition->CompatibleEquipmentSlots.Num () == 0 &&
            GridInventoryCompatibilityDiagnosticsLooksPotentiallyEquippable (Definition))
        {
            ++PotentiallyEquippableWithoutSlotsCount;
            UE_LOG (LogTemp, Warning,
                TEXT ("GridEquipmentCompatibility Item=%s Warning=PotentiallyEquippableWithoutSlots Type=%s Slots=%s"),
                *Definition->ItemDefinitionId.ToString (),
                GetItemTypeName (Definition->ItemType),
                *SlotsText);
        }

        if (Definition->bCanEmitLight)
        {
            bool bHasHandSlot = false;
            for (const EGridEquipmentSlot Slot : Definition->CompatibleEquipmentSlots)
            {
                if (GridInventoryCompatibilityDiagnosticsIsHandSlot (Slot))
                {
                    bHasHandSlot = true;
                    break;
                }
            }

            if (!bHasHandSlot)
            {
                ++LightWithoutHandSlotCount;
                UE_LOG (LogTemp, Warning,
                    TEXT ("GridEquipmentCompatibility Item=%s Warning=LightWithoutMainHandOrOffHand Slots=%s"),
                    *Definition->ItemDefinitionId.ToString (),
                    *SlotsText);
            }
        }

        for (const EGridEquipmentSlot Slot : Definition->CompatibleEquipmentSlots)
        {
            if (GridInventoryCompatibilityDiagnosticsIsExcludedPaperDollSlot (Slot))
            {
                ++ExcludedPaperDollSlotCount;
                UE_LOG (LogTemp, Warning,
                    TEXT ("GridEquipmentCompatibility Item=%s Warning=PaperDollExcludedSlot Slot=%s Slots=%s"),
                    *Definition->ItemDefinitionId.ToString (),
                    GetEquipmentSlotName (Slot),
                    *SlotsText);
            }
            else if (GridInventoryCompatibilityDiagnosticsIsNewPaperDollSlot (Slot))
            {
                ++NewPaperDollSlotUsageCount;
                UE_LOG (LogTemp, Log,
                    TEXT ("GridEquipmentCompatibility Item=%s UsesNewPaperDollSlot=%s Slots=%s"),
                    *Definition->ItemDefinitionId.ToString (),
                    GetEquipmentSlotName (Slot),
                    *SlotsText);
            }
        }
    }

    UE_LOG (LogTemp, Log,
        TEXT ("GridEquipmentCompatibility Summary PotentiallyEquippableWithoutSlots=%d LightWithoutHandSlot=%d ExcludedPaperDollSlots=%d NewPaperDollSlotUses=%d"),
        PotentiallyEquippableWithoutSlotsCount,
        LightWithoutHandSlotCount,
        ExcludedPaperDollSlotCount,
        NewPaperDollSlotUsageCount);
}

void UGridPartyInventoryComponent::LogSelectedCharacterEquipmentStatBonusDiagnostics () const
{
    const int32 CharacterIndex = PartyInventoryState.SelectedCharacterIndex;
    if (!IsValidCharacterIndex (CharacterIndex) ||
        !PartyInventoryState.ActiveEquipment.IsValidIndex (CharacterIndex))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridEquipmentStatBonus Diagnostics Character=%d Result=false Reason=InvalidCharacterOrEquipment"),
            CharacterIndex);
        return;
    }

    const FGridEquipmentStatBonus TotalBonus =
        ComputeCharacterEquipmentStatBonus (CharacterIndex);
    UE_LOG (LogTemp, Log,
        TEXT ("GridEquipmentStatBonus Diagnostics Character=%d Total=%s"),
        CharacterIndex,
        *GetEquipmentStatBonusText (TotalBonus));

    ForEachEquipmentItem (
        PartyInventoryState.ActiveEquipment[CharacterIndex],
        [this] (EGridEquipmentSlot Slot, const FGridItemInstance& Item)
        {
            if (!Item.IsValid ())
            {
                return;
            }

            const UGridItemDefinitionAsset* Definition = FindItemDefinition (Item.ItemDefinitionId);
            if (!Definition)
            {
                UE_LOG (LogTemp, Warning,
                    TEXT ("GridEquipmentStatBonus Item=%s Slot=%s Warning=MissingDefinition"),
                    *Item.ItemDefinitionId.ToString (),
                    GetEquipmentSlotName (Slot));
                return;
            }

            if (!Definition->EquipmentStatBonus.HasAnyBonus ())
            {
                return;
            }

            UE_LOG (LogTemp, Log,
                TEXT ("GridEquipmentStatBonus Item=%s Slot=%s Bonus=%s"),
                *Definition->ItemDefinitionId.ToString (),
                GetEquipmentSlotName (Slot),
                *GetEquipmentStatBonusText (Definition->EquipmentStatBonus));
        });
}

void UGridPartyInventoryComponent::LogSelectedCharacterResistanceDiagnostics () const
{
    const int32 CharacterIndex = PartyInventoryState.SelectedCharacterIndex;
    if (!IsValidCharacterIndex (CharacterIndex) ||
        !PartyInventoryState.ActiveEquipment.IsValidIndex (CharacterIndex))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("GridDamageResistance Diagnostics Character=%d Result=false Reason=InvalidCharacterOrEquipment"),
            CharacterIndex);
        return;
    }

    const FGridDamageResistanceSet TotalResistances =
        ComputeCharacterEquipmentResistances (CharacterIndex);
    UE_LOG (LogTemp, Log,
        TEXT ("GridDamageResistance Diagnostics Character=%d Total=%s"),
        CharacterIndex,
        *GetDamageResistanceSetText (TotalResistances));

    ForEachEquipmentItem (
        PartyInventoryState.ActiveEquipment[CharacterIndex],
        [this] (EGridEquipmentSlot Slot, const FGridItemInstance& Item)
        {
            if (!Item.IsValid ())
            {
                return;
            }

            const UGridItemDefinitionAsset* Definition = FindItemDefinition (Item.ItemDefinitionId);
            if (!Definition)
            {
                UE_LOG (LogTemp, Warning,
                    TEXT ("GridDamageResistance Item=%s Slot=%s Warning=MissingDefinition"),
                    *Item.ItemDefinitionId.ToString (),
                    GetEquipmentSlotName (Slot));
                return;
            }

            if (Definition->EquipmentResistanceBonus.IsEmpty ())
            {
                return;
            }

            UE_LOG (LogTemp, Log,
                TEXT ("GridDamageResistance Item=%s Slot=%s Resistances=%s"),
                *Definition->ItemDefinitionId.ToString (),
                GetEquipmentSlotName (Slot),
                *GetDamageResistanceSetText (Definition->EquipmentResistanceBonus));
        });
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

    if (CharacterState.ClassDisplayName.IsEmpty ())
    {
        CharacterState.ClassDisplayName = FText::FromName (CharacterState.ClassId);
    }

    if (CharacterState.RaceDisplayName.IsEmpty ())
    {
        CharacterState.RaceDisplayName = FText::FromName (CharacterState.RaceId);
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

    InitializeCombatHotbarDefaults (CharacterState);
}

void UGridPartyInventoryComponent::InitializeCombatHotbarDefaults (
    FGridCharacterInventoryState& CharacterState) const
{
    TArray<FGridCombatHotbarBinding> PreviousBindings =
        MoveTemp (CharacterState.CombatHotbarSlots);
    CharacterState.CombatHotbarSlots.SetNum (
        FGridCombatHotbarBinding::SlotCount);

    for (int32 SlotIndex = 0;
        SlotIndex < FGridCombatHotbarBinding::SlotCount;
        ++SlotIndex)
    {
        FGridCombatHotbarBinding& Binding =
            CharacterState.CombatHotbarSlots[SlotIndex];
        Binding.Reset (SlotIndex);

        if (PreviousBindings.IsValidIndex (SlotIndex))
        {
            FGridCombatHotbarBinding PreviousBinding =
                PreviousBindings[SlotIndex];
            PreviousBinding.SlotIndex = SlotIndex;
            if (PreviousBinding.IsValid ())
            {
                Binding = MoveTemp (PreviousBinding);
            }
        }
    }

    SanitizeCombatHotbarBindings (CharacterState);
}

bool UGridPartyInventoryComponent::ValidateCombatHotbar (
    const FGridCharacterInventoryState& CharacterState,
    FString& OutError) const
{
    OutError.Empty ();
    if (CharacterState.CombatHotbarSlots.Num () !=
        FGridCombatHotbarBinding::SlotCount)
    {
        OutError = FString::Printf (
            TEXT ("SlotCount=%d Expected=%d"),
            CharacterState.CombatHotbarSlots.Num (),
            FGridCombatHotbarBinding::SlotCount);
        return false;
    }

    TSet<FGuid> AssignedEquipmentRuntimeIds;
    TSet<FName> AssignedQuickItemDefinitionIds;
    for (int32 SlotIndex = 0;
        SlotIndex < CharacterState.CombatHotbarSlots.Num ();
        ++SlotIndex)
    {
        const FGridCombatHotbarBinding& Binding =
            CharacterState.CombatHotbarSlots[SlotIndex];
        if (Binding.SlotIndex != SlotIndex || !Binding.IsValid ())
        {
            OutError = FString::Printf (
                TEXT ("InvalidSlot=%d StoredIndex=%d Action=%s SourcePolicy=%s"),
                SlotIndex,
                Binding.SlotIndex,
                *Binding.ActionId.ToString (),
                *UEnum::GetValueAsString (Binding.SourcePolicy));
            return false;
        }

        if (Binding.SourcePolicy ==
            EGridCombatActionSourcePolicy::Equipment)
        {
            if (AssignedEquipmentRuntimeIds.Contains (
                    Binding.PreferredSourceRuntimeId))
            {
                OutError = FString::Printf (
                    TEXT ("DuplicateEquipmentSource Slot=%d RuntimeId=%s"),
                    SlotIndex,
                    *Binding.PreferredSourceRuntimeId.ToString ());
                return false;
            }
            AssignedEquipmentRuntimeIds.Add (
                Binding.PreferredSourceRuntimeId);
        }
        else if (Binding.SourcePolicy ==
            EGridCombatActionSourcePolicy::QuickItem)
        {
            if (!CharacterHasInventoryItemDefinition (
                    CharacterState,
                    Binding.SourceDefinitionId))
            {
                OutError = FString::Printf (
                    TEXT ("MissingQuickItemSource Slot=%d Definition=%s"),
                    SlotIndex,
                    *Binding.SourceDefinitionId.ToString ());
                return false;
            }
            if (AssignedQuickItemDefinitionIds.Contains (
                    Binding.SourceDefinitionId))
            {
                OutError = FString::Printf (
                    TEXT ("DuplicateQuickItemSource Slot=%d Definition=%s"),
                    SlotIndex,
                    *Binding.SourceDefinitionId.ToString ());
                return false;
            }
            AssignedQuickItemDefinitionIds.Add (
                Binding.SourceDefinitionId);
        }
    }

    return true;
}

float UGridPartyInventoryComponent::CalculateEquipmentWeight (const FGridCharacterEquipmentState& EquipmentState) const
{
    float TotalWeight = 0.0f;
    ForEachEquipmentItem (
        EquipmentState,
        [&TotalWeight] (EGridEquipmentSlot, const FGridItemInstance& Item)
        {
            TotalWeight += GetItemTotalWeight (Item);
        });
    return TotalWeight;
}
