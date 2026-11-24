#pragma once

#include "GridEquipmentArmorReadModel.h"

/**
 * One read-only equipment contribution model shared by gameplay and diagnostics.
 * ItemSystem owns the armor maximum of every registered ARM N1-N20.
 * Non-ARM equipment already owns its native bonuses and resistances.
 * Registered ARM have only catalogued armor contributions. The native
 * 22-source retirement gate proved that no authored extras are lost.
 */
struct FGridEquipmentContributionReadModel
{
	static FGridEquipmentStatBonus ResolveStatBonuses(
		const UGridItemSubsystem* Registry,
		const FGridItemInstance& Instance)
	{
		FGridEquipmentStatBonus Result;
		FGridEquipmentArmorReadView Armor;
		if (!FGridEquipmentArmorReadModel::TryResolve(Registry, Instance, Armor))
		{
			return Result; // Known invalid canonical ARM never falls back to old armor.
		}
		FGridResolvedItem Canonical;
		if (Registry && Registry->ContainsItem(Instance.ItemDefinitionId))
		{
			if (!Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Canonical))
			{
				return Result;
			}
			if (!Canonical.bIsArmament)
			{
				Result = Canonical.NonArmEquipmentBonuses;
			}
		}
		Result.ArmorBonus = Armor.NominalPhysical;
		Result.MagicalArmorBonus = Armor.NominalMagical;
		return Result;
	}

	static FGridDamageResistanceSet ResolveResistances(
		const UGridItemSubsystem* Registry,
		const FGridItemInstance& Instance)
	{
		if (Registry && Registry->ContainsItem(Instance.ItemDefinitionId))
		{
			FGridResolvedItem Resolved;
			if (!Instance.IsValid() ||
				!Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Resolved))
				return FGridDamageResistanceSet();
			if (!Resolved.bIsArmament)
				return Instance.ItemLevel == 0 && !Resolved.NonArmEquipmentSlots.IsEmpty()
					? Resolved.NonArmEquipmentResistances : FGridDamageResistanceSet();
			return FGridDamageResistanceSet(); // No historical resistance for registered ARM.
		}
		// No resistance from unknown identities in a configured game.
		return FGridDamageResistanceSet();
	}
};
