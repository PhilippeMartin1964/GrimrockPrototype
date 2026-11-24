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
	static FGridEquipmentPreviewItemReadView Project(const FGridItemInstance& Instance,
		const FGridResolvedItem* Item)
	{
		FGridEquipmentPreviewItemReadView View;
		if (!Instance.IsValid())
		{
			return View;
		}
		if (Item)
		{
			View.Resistances = FGridEquipmentContributionReadModel::ProjectResistances(*Item);
			FGridArmorReserveView Armor;
			if (UGridItemSubsystem::ProjectArmorReserve(Instance, *Item, Armor))
			{
				View.Bonuses = FGridEquipmentContributionReadModel::ProjectStatBonuses(*Item, Armor);
				View.RemainingPhysicalArmor = FMath::Max(0, Armor.RemainingPhysicalArmor);
			}
		}
		View.DisplayName = FGridInventoryItemMetadataReadModel::Project(
			Instance, Item, Item != nullptr).DisplayName;
		return View;
	}

	static FGridEquipmentPreviewItemReadView Resolve(
		const UGridItemSubsystem* Registry, const FGridItemInstance& Item)
	{
		FGridResolvedItem Resolved;
		const bool bResolved = Registry && Registry->ResolvePhysicalItem(Item, Resolved);
		return Project(Item, bResolved ? &Resolved : nullptr);
	}
};
