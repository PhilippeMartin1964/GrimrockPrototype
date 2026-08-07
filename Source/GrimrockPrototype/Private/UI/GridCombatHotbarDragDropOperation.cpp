#include "UI/GridCombatHotbarDragDropOperation.h"

void UGridCombatHotbarDragDropOperation::InitializeFromHotbarSlot (
    int32 InCharacterIndex,
    int32 InSourceSlotIndex,
    const FGridCombatHotbarBinding& InBinding)
{
    CharacterIndex = InCharacterIndex;
    SourceSlotIndex = InSourceSlotIndex;
    Binding = InBinding;
}
