#include "Runtime/GridItemDefinitionAsset.h"

bool UGridItemDefinitionAsset::IsValidDefinition () const
{
    return !ItemDefinitionId.IsNone ();
}

bool UGridItemDefinitionAsset::CanEquipToSlot (EGridEquipmentSlot Slot) const
{
    return Slot != EGridEquipmentSlot::None && CompatibleEquipmentSlots.Contains (Slot);
}
