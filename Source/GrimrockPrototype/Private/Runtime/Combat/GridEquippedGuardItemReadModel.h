#pragma once

#include "Runtime/Items/GridItemSubsystem.h"

/** Transient defensive identity: no new chance, reaction or armor reservoir. */
struct FGridEquippedGuardItemReadView
{
	bool bRegisteredIdentity = false;
	bool bResolved = false;
	bool bCanParry = false;
	bool bIsShield = false;
};

/** A registered ItemId is authoritative regardless of what old DA_Weapon_* says. */
struct FGridEquippedGuardItemReadModel
{
	static FGridEquippedGuardItemReadView Resolve(
		const UGridItemSubsystem* Registry,
		const FGridItemInstance& Item)
	{
		FGridEquippedGuardItemReadView View;
		if (Registry && Registry->ContainsItem(Item.ItemDefinitionId))
		{
			View.bRegisteredIdentity = true;
			if (!Item.IsValid() || Item.EquipmentSlot != EGridEquipmentSlot::OffHand)
			{
				return View;
			}
			FGridResolvedItem Canonical;
			if (!Registry->ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, Canonical) ||
				!Canonical.bIsArmament)
			{
				return View; // Known invalid level cannot borrow old guard properties.
			}
			View.bResolved = true;
			const bool bAllowedOffHand =
				Canonical.Weapon.CanEquipInHand(EGridEquipmentSlot::OffHand);
			View.bCanParry = bAllowedOffHand &&
				Canonical.Weapon.Kind == EGridArmamentKind::Weapon &&
				Canonical.ArchetypeId == FName(TEXT("ARM013"));
			View.bIsShield = bAllowedOffHand &&
				Canonical.Weapon.Kind == EGridArmamentKind::Shield;
			return View;
		}
		// No unknown identity may borrow an obsolete guard rule in a configured game.
		return View;
	}
};
