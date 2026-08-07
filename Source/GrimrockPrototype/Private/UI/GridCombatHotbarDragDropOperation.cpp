#include "UI/GridCombatHotbarDragDropOperation.h"

void UGridCombatHotbarDragDropOperation::InitializeFromHotbarSlot (
    int32 InCharacterIndex,
    int32 InSourceSlotIndex,
    const FGridCombatHotbarBinding& InBinding)
{
    CharacterIndex = InCharacterIndex;
    SourceSlotIndex = InSourceSlotIndex;
    Binding = InBinding;
    bFromActionPalette = false;
}

void UGridCombatHotbarDragDropOperation::InitializeFromActionPalette (
    int32 InCharacterIndex,
    const FGridAvailableCombatAction& InAction)
{
    CharacterIndex = InCharacterIndex;
    SourceSlotIndex = INDEX_NONE;
    bFromActionPalette = true;
    Binding = FGridCombatHotbarBinding ();
    Binding.ActionId = InAction.Definition.ActionId;
    Binding.SourcePolicy = InAction.Definition.SourcePolicy;
    Binding.SourceDefinitionId = InAction.SourceDefinitionId;
}
