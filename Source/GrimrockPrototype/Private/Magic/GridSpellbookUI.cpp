#include "Magic/GridSpellbookUI.h"

#include "Magic/GridProductionSpellLibrary.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace
{
    const FGridSpellDefinition* FindDefinition (
        const TArray<FGridSpellDefinition>& Definitions,
        FName SpellId)
    {
        return Definitions.FindByPredicate (
            [SpellId] (const FGridSpellDefinition& Definition)
            {
                return Definition.SpellId == SpellId;
            });
    }

    bool ResolveCharacterIdentity (
        const UGridPartyInventoryComponent* InventoryComponent,
        int32 CharacterIndex,
        FGuid& OutCharacterId)
    {
        OutCharacterId.Invalidate ();
        if (!InventoryComponent ||
            !InventoryComponent->IsValidCharacterIndex (CharacterIndex))
        {
            return false;
        }
        OutCharacterId = InventoryComponent->
            PartyInventoryState.ActiveCharacters[CharacterIndex].CharacterId;
        return OutCharacterId.IsValid ();
    }

    bool IsKnownProductionSpell (
        const FGridCharacterSpellbookState& Spellbook,
        FName SpellId)
    {
        if (!Spellbook.KnowsSpell (SpellId))
        {
            return false;
        }
        TArray<FGridSpellDefinition> Definitions;
        FGridProductionSpellLibrary::BuildAll (Definitions);
        const FGridSpellDefinition* Definition =
            FindDefinition (Definitions, SpellId);
        return Definition &&
            FGridSpellContract::ValidateDefinition (*Definition) ==
                EGridSpellValidationError::None;
    }
}

void UGridSpellbookUILibrary::BuildProductionSpellbookEntries (
    const FGridCharacterSpellbookState& Spellbook,
    const TArray<FGridCombatHotbarBinding>& HotbarBindings,
    TArray<FGridSpellbookEntryView>& OutEntries)
{
    OutEntries.Reset ();

    TArray<FGridSpellDefinition> Definitions;
    FGridProductionSpellLibrary::BuildAll (Definitions);

    for (const FName SpellId : Spellbook.KnownSpellIds)
    {
        FGridSpellbookEntryView Entry;
        Entry.SpellId = SpellId;
        Entry.AssignedHotbarSlotIndex =
            FindAssignedSpellSlot (HotbarBindings, SpellId);
        Entry.bAssignedToHotbar =
            Entry.AssignedHotbarSlotIndex != INDEX_NONE;

        const FGridSpellDefinition* Definition =
            FindDefinition (Definitions, SpellId);
        if (Definition &&
            FGridSpellContract::ValidateDefinition (*Definition) ==
                EGridSpellValidationError::None)
        {
            Entry.bDefinitionResolved = true;
            Entry.DisplayName = Definition->DisplayName;
            Entry.Description = Definition->Description;
            Entry.School = Definition->School;
            Entry.ManaCost = Definition->ManaCost;
            Entry.ActionPointCost = Definition->ActionPointCost;
            Entry.MinRangeCells = Definition->MinRangeCells;
            Entry.MaxRangeCells = Definition->MaxRangeCells;
            Entry.TargetingPolicy = Definition->TargetingPolicy;
            Entry.bRequiresLineOfSight =
                Definition->bRequiresLineOfSight;
            Entry.CombatActionDefinition =
                MakeSpellCombatActionDefinition (*Definition);
            Entry.bCanAssignToHotbar =
                Entry.CombatActionDefinition.IsValid ();
        }
        else
        {
            Entry.DisplayName =
                FText::FromString (SpellId.ToString ());
            Entry.Description = FText::FromString (
                TEXT ("Définition de sort indisponible."));
        }

        OutEntries.Add (MoveTemp (Entry));
    }
}

FGridCombatActionDefinition
UGridSpellbookUILibrary::MakeSpellCombatActionDefinition (
    const FGridSpellDefinition& SpellDefinition)
{
    FGridCombatActionDefinition Action;
    if (FGridSpellContract::ValidateDefinition (SpellDefinition) !=
        EGridSpellValidationError::None)
    {
        return Action;
    }

    Action.ActionId = SpellDefinition.SpellId;
    Action.DisplayName = SpellDefinition.DisplayName;
    Action.Description = SpellDefinition.Description;
    // SourcePolicy is the canonical spell discriminator. Keep ActionType on the
    // existing Ability visual family to avoid changing serialized enum values.
    Action.ActionType = EGridCombatActionType::Ability;
    Action.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
    Action.TargetingPolicy = SpellDefinition.TargetingPolicy;
    Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
    Action.ActionPointCost = SpellDefinition.ActionPointCost;
    Action.ResourceCosts.ManaCost = SpellDefinition.ManaCost;
    Action.RangeCells = SpellDefinition.MaxRangeCells;
    Action.AreaRadiusCells =
        SpellDefinition.TargetingPolicy == EGridCombatTargetingPolicy::Area
            ? 1
            : 0;
    Action.CooldownRounds = SpellDefinition.CooldownRounds;
    Action.PresentationProfileId = SpellDefinition.SpellId;
    return Action;
}

FGridCombatHotbarBinding UGridSpellbookUILibrary::MakeSpellHotbarBinding (
    FName SpellId,
    int32 SlotIndex)
{
    FGridCombatHotbarBinding Binding;
    Binding.SlotIndex = SlotIndex;
    Binding.ActionId = SpellId;
    Binding.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
    Binding.SourceDefinitionId = SpellId;
    return Binding;
}

bool UGridSpellbookUILibrary::IsSpellHotbarBinding (
    const FGridCombatHotbarBinding& Binding,
    FName SpellId)
{
    return !SpellId.IsNone () &&
        !Binding.IsEmpty () &&
        Binding.ActionId == SpellId &&
        Binding.SourcePolicy == EGridCombatActionSourcePolicy::Spell &&
        Binding.SourceDefinitionId == SpellId &&
        !Binding.PreferredSourceRuntimeId.IsValid () &&
        Binding.PreferredEquipmentSlot == EGridEquipmentSlot::None;
}

int32 UGridSpellbookUILibrary::FindAssignedSpellSlot (
    const TArray<FGridCombatHotbarBinding>& HotbarBindings,
    FName SpellId)
{
    for (int32 Index = 0; Index < HotbarBindings.Num (); ++Index)
    {
        if (IsSpellHotbarBinding (HotbarBindings[Index], SpellId))
        {
            return Index;
        }
    }
    return INDEX_NONE;
}

EGridSpellHotbarAssignmentResult
UGridSpellbookUILibrary::AssignKnownSpellToHotbar (
    UGridPartyInventoryComponent* InventoryComponent,
    int32 CharacterIndex,
    const FGridCharacterSpellbookState& Spellbook,
    FName SpellId,
    int32 TargetSlotIndex)
{
    FGuid CharacterId;
    if (!ResolveCharacterIdentity (
            InventoryComponent,
            CharacterIndex,
            CharacterId) ||
        CharacterId != Spellbook.CharacterId)
    {
        return EGridSpellHotbarAssignmentResult::InvalidCharacter;
    }
    if (TargetSlotIndex < 0 ||
        TargetSlotIndex >= InventoryComponent->GetCombatHotbarSlotCount ())
    {
        return EGridSpellHotbarAssignmentResult::InvalidSlot;
    }
    if (!Spellbook.KnowsSpell (SpellId))
    {
        return EGridSpellHotbarAssignmentResult::UnknownSpell;
    }
    if (!IsKnownProductionSpell (Spellbook, SpellId))
    {
        return EGridSpellHotbarAssignmentResult::InvalidDefinition;
    }

    TArray<FGridCombatHotbarBinding> CurrentBindings;
    CurrentBindings.SetNum (InventoryComponent->GetCombatHotbarSlotCount ());
    for (int32 SlotIndex = 0;
        SlotIndex < CurrentBindings.Num ();
        ++SlotIndex)
    {
        CurrentBindings[SlotIndex].Reset (SlotIndex);
        InventoryComponent->GetCharacterCombatHotbarBinding (
            CharacterIndex,
            SlotIndex,
            CurrentBindings[SlotIndex]);
    }

    const int32 ExistingSlot =
        FindAssignedSpellSlot (CurrentBindings, SpellId);
    if (ExistingSlot == TargetSlotIndex)
    {
        return EGridSpellHotbarAssignmentResult::Success;
    }
    if (ExistingSlot != INDEX_NONE)
    {
        return InventoryComponent->MoveOrSwapCharacterCombatHotbarBinding (
                CharacterIndex,
                ExistingSlot,
                TargetSlotIndex)
            ? EGridSpellHotbarAssignmentResult::Success
            : EGridSpellHotbarAssignmentResult::HotbarRejected;
    }

    const FGridCombatHotbarBinding Binding =
        MakeSpellHotbarBinding (SpellId, TargetSlotIndex);
    return Binding.IsValid () &&
            InventoryComponent->SetCharacterCombatHotbarBinding (
                CharacterIndex,
                TargetSlotIndex,
                Binding)
        ? EGridSpellHotbarAssignmentResult::Success
        : EGridSpellHotbarAssignmentResult::HotbarRejected;
}

EGridSpellHotbarAssignmentResult
UGridSpellbookUILibrary::UnassignSpellFromHotbar (
    UGridPartyInventoryComponent* InventoryComponent,
    int32 CharacterIndex,
    const FGridCharacterSpellbookState& Spellbook,
    FName SpellId)
{
    FGuid CharacterId;
    if (!ResolveCharacterIdentity (
            InventoryComponent,
            CharacterIndex,
            CharacterId) ||
        CharacterId != Spellbook.CharacterId)
    {
        return EGridSpellHotbarAssignmentResult::InvalidCharacter;
    }

    for (int32 SlotIndex = 0;
        SlotIndex < InventoryComponent->GetCombatHotbarSlotCount ();
        ++SlotIndex)
    {
        FGridCombatHotbarBinding Binding;
        if (InventoryComponent->GetCharacterCombatHotbarBinding (
                CharacterIndex,
                SlotIndex,
                Binding) &&
            IsSpellHotbarBinding (Binding, SpellId))
        {
            return InventoryComponent->ClearCharacterCombatHotbarBinding (
                    CharacterIndex,
                    SlotIndex)
                ? EGridSpellHotbarAssignmentResult::Success
                : EGridSpellHotbarAssignmentResult::HotbarRejected;
        }
    }
    return EGridSpellHotbarAssignmentResult::NotAssigned;
}
