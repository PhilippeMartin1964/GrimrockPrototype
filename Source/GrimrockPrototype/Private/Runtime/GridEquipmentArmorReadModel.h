#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** Runtime-only projection, not a persisted or authorable armor authority. */
struct FGridEquipmentArmorReadView
{
	int32 NominalPhysical = 0;
	int32 NominalMagical = 0;
	int32 RemainingPhysical = 0;
	int32 RemainingMagical = 0;
};

/** Select exactly one armor source per item; keep the legacy reader only for IDs outside ItemSystem. */
struct FGridEquipmentArmorReadModel
{
	static bool TryResolve(const UGridItemSubsystem* Registry,
		const UGridItemDefinitionAsset* LegacyDefinition,
		const FGridItemInstance& Instance, FGridEquipmentArmorReadView& Out)
	{
		Out = FGridEquipmentArmorReadView();
		if (!Instance.IsValid())
		{
			return false;
		}
		if (Registry && Registry->HasCanonicalItem(Instance.ItemDefinitionId))
		{
			FGridCanonicalArmorReserveView Canonical;
			if (!Registry->ResolveArmorReserveForInstance(Instance, Canonical))
			{
				return false; // Never reinterpret an invalid canonical level via DA_Weapon_*.
			}
			Out.NominalPhysical = Canonical.NominalPhysicalArmor;
			Out.NominalMagical = Canonical.NominalMagicalArmor;
			Out.RemainingPhysical = Canonical.RemainingPhysicalArmor;
			Out.RemainingMagical = Canonical.RemainingMagicalArmor;
			return true;
		}
		if (!IsValid(LegacyDefinition) ||
			LegacyDefinition->ItemDefinitionId != Instance.ItemDefinitionId)
		{
			return false;
		}
		const FGridEquipmentStatBonus Legacy =
			LegacyDefinition->GetEffectiveEquipmentStatBonusForInstance(Instance);
		Out.NominalPhysical = Legacy.ArmorBonus;
		Out.NominalMagical = Legacy.MagicalArmorBonus;
		Out.RemainingPhysical = Legacy.ArmorBonus > 0
			? FMath::Max(0, Legacy.ArmorBonus - FMath::Max(0, Instance.AbsorbedEquipmentPhysicalArmor))
			: Legacy.ArmorBonus;
		Out.RemainingMagical = Legacy.MagicalArmorBonus > 0
			? FMath::Max(0, Legacy.MagicalArmorBonus - FMath::Max(0, Instance.AbsorbedEquipmentMagicalArmor))
			: Legacy.MagicalArmorBonus;
		return true;
	}
};
