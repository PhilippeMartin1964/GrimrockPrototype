#include "UI/GridInventoryDragDropOperation.h"

void UGridInventoryDragDropOperation::InitializeFromSlot(EGridInventoryUiSlotType InSlotType, int32 InSlotIndex, const FGridItemInstance& InItem)
{
	SourceSlotType = InSlotType;
	SourceSlotIndex = InSlotIndex;
	SourceItem = InItem;
	SourceItemDefinitionId = InItem.ItemDefinitionId;
	SourceRuntimeObjectId = InItem.RuntimeObjectId;
	bHasItem = InItem.IsValid();
}
