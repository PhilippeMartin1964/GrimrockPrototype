#include "Runtime/GridItemDefinitionAsset.h"

bool UGridItemDefinitionAsset::IsValidDefinition () const
{
    if (ItemDefinitionId.IsNone ())
    {
        return false;
    }

    if (!bProvidesAttack)
    {
        return true;
    }

    return HasValidOffensiveProfile () &&
        (CompatibleEquipmentSlots.Contains (EGridEquipmentSlot::MainHand) ||
            CompatibleEquipmentSlots.Contains (EGridEquipmentSlot::OffHand));
}

bool UGridItemDefinitionAsset::CanEquipToSlot (EGridEquipmentSlot Slot) const
{
    return Slot != EGridEquipmentSlot::None && CompatibleEquipmentSlots.Contains (Slot);
}

bool UGridItemDefinitionAsset::HasValidOffensiveProfile () const
{
    return bProvidesAttack && OffensiveProfile.IsValid ();
}

bool UGridItemDefinitionAsset::CanProvideAttackFromSlot (
    EGridEquipmentSlot Slot) const
{
    return HasValidOffensiveProfile () &&
        (Slot == EGridEquipmentSlot::MainHand ||
            Slot == EGridEquipmentSlot::OffHand) &&
        CompatibleEquipmentSlots.Contains (Slot);
}
