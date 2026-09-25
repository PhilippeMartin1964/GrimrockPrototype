#pragma once

#include "Runtime/GridInventoryTypes.h"

namespace GridEquipmentSlotUtils
{
	constexpr uint8 FirstSlotValue = static_cast<uint8>(EGridEquipmentSlot::MainHand);
	constexpr uint8 LastSlotValue = static_cast<uint8>(EGridEquipmentSlot::Bracers);

	inline bool IsSupportedSlot(EGridEquipmentSlot Slot)
	{
		const uint8 SlotValue = static_cast<uint8>(Slot);
		return SlotValue >= FirstSlotValue && SlotValue <= LastSlotValue;
	}

	inline bool IsHandSlot(EGridEquipmentSlot Slot)
	{
		return Slot == EGridEquipmentSlot::MainHand || Slot == EGridEquipmentSlot::OffHand;
	}

	inline const TCHAR* GetLogName(EGridEquipmentSlot Slot)
	{
		static const TCHAR* const Names[] = { TEXT("None"), TEXT("MainHand"), TEXT("OffHand"), TEXT("Head"), TEXT("Chest"), TEXT("Legs"),
			TEXT("Feet"), TEXT("Amulet"), TEXT("Ring1"), TEXT("Ring2"), TEXT("Shoulders"), TEXT("Gloves"), TEXT("Belt"), TEXT("Cloak"),
			TEXT("Talisman"), TEXT("QuickSlot1"), TEXT("QuickSlot2"), TEXT("Chemise"), TEXT("Brassards") };
		static_assert(UE_ARRAY_COUNT(Names) == LastSlotValue + 1, "Equipment slot log names must stay aligned with EGridEquipmentSlot.");

		const uint8 SlotValue = static_cast<uint8>(Slot);
		return SlotValue < UE_ARRAY_COUNT(Names) ? Names[SlotValue] : TEXT("Unsupported");
	}

	template <typename VisitorType>
	void ForEachEquipmentItem(const FGridCharacterEquipmentState& EquipmentState, VisitorType&& Visitor)
	{
		for (uint8 SlotValue = FirstSlotValue; SlotValue <= LastSlotValue; ++SlotValue)
		{
			const EGridEquipmentSlot Slot = static_cast<EGridEquipmentSlot>(SlotValue);
			if (const FGridItemInstance* Item = EquipmentState.GetSlot(Slot))
			{
				Visitor(Slot, *Item);
			}
		}
	}
}
