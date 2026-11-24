#pragma once

#include "Runtime/Items/GridItemSubsystem.h"

/** Runtime-only projection, not a persisted or authorable armor authority. */
struct FGridEquipmentArmorReadView
{
	int32 NominalPhysical = 0;
	int32 NominalMagical = 0;
	int32 RemainingPhysical = 0;
	int32 RemainingMagical = 0;
};

/** Project the registered item's nominal and remaining armor. */
struct FGridEquipmentArmorReadModel
{
	static bool TryResolve(const UGridItemSubsystem* Registry,
		const FGridItemInstance& Instance, FGridEquipmentArmorReadView& Out)
	{
		Out = FGridEquipmentArmorReadView();
		if (!Instance.IsValid())
		{
			return false;
		}
		if (Registry && Registry->ContainsItem(Instance.ItemDefinitionId))
		{
			FGridArmorReserveView Canonical;
			if (!Registry->ResolveArmorReserveForInstance(Instance, Canonical))
			{
				return false; // Never reinterpret an invalid physical level via an old definition.
			}
			Out.NominalPhysical = Canonical.NominalPhysicalArmor;
			Out.NominalMagical = Canonical.NominalMagicalArmor;
			Out.RemainingPhysical = Canonical.RemainingPhysicalArmor;
			Out.RemainingMagical = Canonical.RemainingMagicalArmor;
			return true;
		}
		return false;
	}
};
