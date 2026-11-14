#pragma once

#include "GridEquipmentArmorReadModel.h"

/**
 * One read-only equipment contribution model shared by gameplay and diagnostics.
 * ItemSystem owns the armor maximum of every registered ARM N1-N20.
 * Non-armor attributes and resistances have no canonical fields yet and
 * deliberately keep their current item-definition authority until schema migration.
 */
struct FGridEquipmentContributionReadModel
{
	static FGridEquipmentStatBonus ResolveStatBonuses(
		const UGridItemSubsystem* Registry,
		const UGridItemDefinitionAsset* LegacyDefinition,
		const FGridItemInstance& Instance)
	{
		FGridEquipmentStatBonus Result;
		FGridEquipmentArmorReadView Armor;
		if (!FGridEquipmentArmorReadModel::TryResolve(Registry, LegacyDefinition, Instance, Armor))
		{
			return Result; // Known invalid canonical ARM never falls back to old armor.
		}
		if (IsValid(LegacyDefinition))
		{
			Result = LegacyDefinition->EquipmentStatBonus;
		}
		Result.ArmorBonus = Armor.NominalPhysical;
		Result.MagicalArmorBonus = Armor.NominalMagical;
		return Result;
	}

	static FGridDamageResistanceSet ResolveResistances(
		const UGridItemDefinitionAsset* LegacyDefinition,
		const FGridItemInstance& Instance)
	{
		// Keep exactly the original combat semantics: no invented resistance
		// data in WeaponCatalogue, and none from an absent definition.
		if (!Instance.IsValid() || !IsValid(LegacyDefinition) ||
			LegacyDefinition->ItemDefinitionId != Instance.ItemDefinitionId)
		{
			return FGridDamageResistanceSet();
		}
		return LegacyDefinition->EquipmentResistanceBonus;
	}
};
