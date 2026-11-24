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
		FGridResolvedItem Resolved;
		FGridArmorReserveView Armor;
		return Registry && Registry->ResolvePhysicalItem(Instance, Resolved) &&
			UGridItemSubsystem::ProjectArmorReserve(Instance, Resolved, Armor)
			? ProjectStatBonuses(Resolved, Armor) : FGridEquipmentStatBonus();
	}

	static FGridEquipmentStatBonus ProjectStatBonuses(
		const FGridResolvedItem& Item, const FGridArmorReserveView& Armor)
	{
		FGridEquipmentStatBonus Result;
		if (!Item.bIsArmament)
		{
			Result = Item.NonArmEquipmentBonuses;
		}
		Result.ArmorBonus = Armor.NominalPhysicalArmor;
		Result.MagicalArmorBonus = Armor.NominalMagicalArmor;
		return Result;
	}

	static FGridDamageResistanceSet ResolveResistances(
		const UGridItemSubsystem* Registry,
		const FGridItemInstance& Instance)
	{
		FGridResolvedItem Resolved;
		return Registry && Registry->ResolvePhysicalItem(Instance, Resolved)
			? ProjectResistances(Resolved) : FGridDamageResistanceSet();
	}

	static FGridDamageResistanceSet ProjectResistances(const FGridResolvedItem& Item)
	{
		return !Item.bIsArmament && !Item.NonArmEquipmentSlots.IsEmpty()
			? Item.NonArmEquipmentResistances : FGridDamageResistanceSet();
	}
};
