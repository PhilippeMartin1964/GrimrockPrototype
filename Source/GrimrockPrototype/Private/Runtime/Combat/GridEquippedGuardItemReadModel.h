#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** Transient defensive identity: no new chance, reaction or armor reservoir. */
struct FGridEquippedGuardItemReadView
{
	bool bCanonicalIdentity = false;
	bool bResolved = false;
	bool bCanParry = false;
	bool bIsShield = false;
};

/** A registered ItemId is authoritative regardless of what old DA_Weapon_* says. */
struct FGridEquippedGuardItemReadModel
{
	static FGridEquippedGuardItemReadView Resolve(
		const UGridItemSubsystem* Registry, const UGridItemDefinitionAsset* LegacyDefinition,
		const FGridItemInstance& Item)
	{
		FGridEquippedGuardItemReadView View;
		if (Registry && Registry->HasCanonicalItem(Item.ItemDefinitionId))
		{
			View.bCanonicalIdentity = true;
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
				Canonical.Weapon.Kind == EGridCanonicalArmamentKind::Weapon &&
				Canonical.ArchetypeId == FName(TEXT("ARM013"));
			View.bIsShield = bAllowedOffHand &&
				Canonical.Weapon.Kind == EGridCanonicalArmamentKind::Shield;
			return View;
		}
		if (!Item.IsValid() || Item.EquipmentSlot != EGridEquipmentSlot::OffHand ||
			!IsValid(LegacyDefinition) ||
			LegacyDefinition->ItemDefinitionId != Item.ItemDefinitionId)
		{
			return View;
		}
		// Transitional compatibility for objects not registered in ItemSystem.
		View.bResolved = true;
		View.bCanParry = LegacyDefinition->ItemType == EGridItemType::Weapon &&
			LegacyDefinition->bUseWeaponCatalogue &&
			LegacyDefinition->WeaponArchetypeId == FName(TEXT("ARM013"));
		View.bIsShield = LegacyDefinition->ItemType == EGridItemType::Shield;
		return View;
	}
};
