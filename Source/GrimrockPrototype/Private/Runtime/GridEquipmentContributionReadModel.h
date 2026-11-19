#pragma once

#include "GridEquipmentArmorReadModel.h"

/**
 * One read-only equipment contribution model shared by gameplay and diagnostics.
 * ItemSystem owns the armor maximum of every registered ARM N1-N20.
 * Non-ARM equipment already owns its native bonuses and resistances.
 * ARM attributes other than armor and elemental resistances retain the
 * historical source temporarily, pending their own lossless retirement.
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
		FGridResolvedItem Canonical;
		if (Registry && Registry->HasCanonicalItem(Instance.ItemDefinitionId) &&
			Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Canonical) &&
			!Canonical.bIsArmament)
		{
			Result = Canonical.NonArmEquipmentBonuses;
		}
		else if (IsValid(LegacyDefinition) && LegacyDefinition->ItemDefinitionId == Instance.ItemDefinitionId)
		{
			Result = LegacyDefinition->EquipmentStatBonus;
		}
		Result.ArmorBonus = Armor.NominalPhysical;
		Result.MagicalArmorBonus = Armor.NominalMagical;
		return Result;
	}

	static FGridDamageResistanceSet ResolveResistances(
		const UGridItemSubsystem* Registry,
		const UGridItemDefinitionAsset* LegacyDefinition,
		const FGridItemInstance& Instance)
	{
		if (Registry && Registry->HasCanonicalItem(Instance.ItemDefinitionId))
		{
			FGridResolvedItem Resolved;
			if (!Instance.IsValid() ||
				!Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Resolved))
				return FGridDamageResistanceSet();
			if (!Resolved.bIsArmament)
				return Instance.ItemLevel == 0 && !Resolved.NonArmEquipmentSlots.IsEmpty()
					? Resolved.NonArmEquipmentResistances : FGridDamageResistanceSet();
		}
		// ARM non-armor contributions are still gated by their exact
		// historical identity; no fallback for registered non-ARM equipment.
		if (!Instance.IsValid() || !IsValid(LegacyDefinition) ||
			LegacyDefinition->ItemDefinitionId != Instance.ItemDefinitionId)
		{
			return FGridDamageResistanceSet();
		}
		return LegacyDefinition->EquipmentResistanceBonus;
	}
};
