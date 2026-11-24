#pragma once

#include "Runtime/Items/GridItemSubsystem.h"

/**
 * Read-only equipment-slot view for menus and tooltips.
 * Registered ItemIds always obey their REAL fixed-level hand policy.
 * Historical equipment policies exist only in explicit Automation fixtures.
 * Two-handed gear offers one MainHand storage action, reserving OffHand virtually.
 */
struct FGridEquipmentSlotsView
{
	bool bRegisteredIdentity = false;
	bool bResolved = false;
	bool bTwoHanded = false;
	TArray<EGridEquipmentSlot> AllowedSlots;
};

struct FGridEquipmentSlots
{
	static FGridEquipmentSlotsView Resolve(
		const UGridItemSubsystem* Registry,
		const FGridItemInstance& Item)
	{
		FGridEquipmentSlotsView View;
		if (!Item.IsValid() || Item.ItemDefinitionId.IsNone())
		{
			return View;
		}

		if (Registry && Registry->ContainsItem(Item.ItemDefinitionId))
		{
			View.bRegisteredIdentity = true;
			FGridResolvedItem Canonical;
			if (!Registry->ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, Canonical))
			{
				return View; // Persisted N0/invalid N21: no legacy slot fallback.
			}
			View.bResolved = true;
			if (!Canonical.bIsArmament)
			{
				// Only genuine canonical equipment carries explicit slots.
				// Ammo, potions and generic stacks have none, regardless
				// of obsolete legacy hand flags. Physical non-ARM is N0.
				if (Item.ItemLevel != 0) { View.bResolved = false; return View; }
				View.bTwoHanded = Canonical.bNonArmTwoHanded;
				View.AllowedSlots = Canonical.NonArmEquipmentSlots;
				return View;
			}

			View.bTwoHanded =
				Canonical.Weapon.HandPolicy == EGridHandPolicy::TwoHanded;
			if (Canonical.Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand))
			{
				View.AllowedSlots.Add(EGridEquipmentSlot::MainHand);
			}
			if (!View.bTwoHanded &&
				Canonical.Weapon.CanEquipInHand(EGridEquipmentSlot::OffHand))
			{
				View.AllowedSlots.Add(EGridEquipmentSlot::OffHand);
			}
			return View;
		}

		return View;
	}
};
