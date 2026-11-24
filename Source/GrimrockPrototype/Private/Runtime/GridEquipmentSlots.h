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
	static FGridEquipmentSlotsView Project(const FGridResolvedItem* Item, bool bRegisteredIdentity)
	{
		FGridEquipmentSlotsView View;
		View.bRegisteredIdentity = bRegisteredIdentity;
		if (!Item)
		{
			return View;
		}

		View.bResolved = true;
		if (!Item->bIsArmament)
		{
			// Only genuine canonical equipment carries explicit slots.
			// Ammo, potions and generic stacks have none, regardless
			// of obsolete legacy hand flags. Physical non-ARM is N0.
			View.bTwoHanded = Item->bNonArmTwoHanded;
			View.AllowedSlots = Item->NonArmEquipmentSlots;
			return View;
		}

		View.bTwoHanded =
			Item->Weapon.HandPolicy == EGridHandPolicy::TwoHanded;
		if (Item->Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand))
		{
			View.AllowedSlots.Add(EGridEquipmentSlot::MainHand);
		}
		if (!View.bTwoHanded &&
			Item->Weapon.CanEquipInHand(EGridEquipmentSlot::OffHand))
		{
			View.AllowedSlots.Add(EGridEquipmentSlot::OffHand);
		}
		return View;
	}

	static FGridEquipmentSlotsView Resolve(
		const UGridItemSubsystem* Registry, const FGridItemInstance& Item)
	{
		FGridResolvedItem Resolved;
		const bool bResolved = Registry && Registry->ResolvePhysicalItem(Item, Resolved);
		return Project(bResolved ? &Resolved : nullptr,
			Item.IsValid() && Registry && Registry->ContainsItem(Item.ItemDefinitionId));
	}
};
