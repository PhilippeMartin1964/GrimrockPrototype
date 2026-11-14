#pragma once

#include "GridEquipmentContributionReadModel.h"
#include "../UI/GridInventoryItemMetadataReadModel.h"

/**
 * Transient read-only per-item state shared by hypothetical equipment previews
 * and inline inventory tooltip comparisons. All values use the actual level.
 */
struct FGridEquipmentPreviewItemReadView
{
	FGridEquipmentStatBonus Bonuses;
	FGridDamageResistanceSet Resistances;
	int32 RemainingPhysicalArmor = 0;
	FText DisplayName;
};

/** Only the ItemSystem owns registered ARM identity, armor and display name. */
struct FGridEquipmentPreviewItemReadModel
{
	static FGridEquipmentPreviewItemReadView Resolve(
		const UGridItemSubsystem* Registry,
		const UGridItemDefinitionAsset* LegacyDefinition,
		const FGridItemInstance& Item)
	{
		FGridEquipmentPreviewItemReadView View;
		if (!Item.IsValid())
		{
			return View;
		}
		View.Bonuses = FGridEquipmentContributionReadModel::ResolveStatBonuses(
			Registry, LegacyDefinition, Item);
		View.Resistances = FGridEquipmentContributionReadModel::ResolveResistances(
			LegacyDefinition, Item);
		FGridEquipmentArmorReadView Armor;
		if (FGridEquipmentArmorReadModel::TryResolve(
			Registry, LegacyDefinition, Item, Armor))
		{
			View.RemainingPhysicalArmor = FMath::Max(0, Armor.RemainingPhysical);
		}
		const FGridInventoryItemMetadata Metadata =
			FGridInventoryItemMetadataReadModel::Resolve(Registry, LegacyDefinition, Item);
		View.DisplayName = Metadata.DisplayName.IsEmpty()
			? FText::FromName(Item.ItemDefinitionId) : Metadata.DisplayName;
		return View;
	}
};
