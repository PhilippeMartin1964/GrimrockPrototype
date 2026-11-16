#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/**
 * Read-only equipment-slot view for menus and tooltips.
 * Registered ItemIds always obey their REAL fixed-level hand policy; legacy
 * definitions can supply slots only for identities absent from DA_ItemSystem.
 * Two-handed gear offers one MainHand storage action, reserving OffHand virtually.
 */
struct FGridCanonicalEquipmentSlotsReadView
{
	bool bCanonicalIdentity = false;
	bool bResolved = false;
	bool bTwoHanded = false;
	TArray<EGridEquipmentSlot> AllowedSlots;
};

struct FGridCanonicalEquipmentSlotsReadModel
{
	static FGridCanonicalEquipmentSlotsReadView Resolve(
		const UGridItemSubsystem* Registry,
		const UGridItemDefinitionAsset* LegacyDefinition,
		const FGridItemInstance& Item)
	{
		FGridCanonicalEquipmentSlotsReadView View;
		if (!Item.IsValid() || Item.ItemDefinitionId.IsNone())
		{
			return View;
		}

		if (Registry && Registry->HasCanonicalItem(Item.ItemDefinitionId))
		{
			View.bCanonicalIdentity = true;
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
				Canonical.Weapon.HandPolicy == EGridCanonicalHandPolicy::TwoHanded;
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

		if (!IsValid(LegacyDefinition) ||
			LegacyDefinition->ItemDefinitionId != Item.ItemDefinitionId)
		{
			return View;
		}
		View.bResolved = true;
		View.bTwoHanded =
			LegacyDefinition->GetEffectiveHandUsage() == EGridItemHandUsage::TwoHanded &&
			LegacyDefinition->CanEquipToSlot(EGridEquipmentSlot::MainHand);
		if (LegacyDefinition->CanEquipToSlot(EGridEquipmentSlot::MainHand))
		{
			View.AllowedSlots.Add(EGridEquipmentSlot::MainHand);
		}
		if (!View.bTwoHanded &&
			LegacyDefinition->CanEquipToSlot(EGridEquipmentSlot::OffHand))
		{
			View.AllowedSlots.Add(EGridEquipmentSlot::OffHand);
		}
		for (const EGridEquipmentSlot Slot : LegacyDefinition->CompatibleEquipmentSlots)
		{
			if (Slot == EGridEquipmentSlot::None ||
				Slot == EGridEquipmentSlot::MainHand ||
				Slot == EGridEquipmentSlot::OffHand ||
				View.AllowedSlots.Contains(Slot) ||
				!LegacyDefinition->CanEquipToSlot(Slot))
			{
				continue;
			}
			View.AllowedSlots.Add(Slot);
		}
		return View;
	}
};
