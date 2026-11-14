#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** The equipment hand is a canonical item rule, never copied into a kit. */
struct FGridStartingKitEquipmentPolicyReadView
{
	bool bCanonicalIdentity = false;
	bool bResolved = false;
	bool bTwoHanded = false;
	EGridEquipmentSlot PhysicalSlot = EGridEquipmentSlot::None;
};

struct FGridStartingKitEquipmentPolicyReadModel
{
	static FGridStartingKitEquipmentPolicyReadView Resolve(
		const UGridItemSubsystem* Registry,
		const UGridItemDefinitionAsset* LegacyDefinition,
		int32 FixedItemLevel,
		EGridEquipmentSlot AuthoredSlot)
	{
		FGridStartingKitEquipmentPolicyReadView View;
		if (!IsValid(LegacyDefinition) || LegacyDefinition->ItemDefinitionId.IsNone() ||
			AuthoredSlot == EGridEquipmentSlot::None)
		{
			return View;
		}

		if (Registry && Registry->HasCanonicalItem(LegacyDefinition->ItemDefinitionId))
		{
			View.bCanonicalIdentity = true;
			FGridResolvedItem Canonical;
			if (!Registry->ResolveItem(LegacyDefinition->ItemDefinitionId, FixedItemLevel, Canonical))
			{
				return View; // Known but invalid: no fallback to the old hand flags.
			}
			if (Canonical.bIsArmament)
			{
				if (!Canonical.Weapon.CanEquipInHand(AuthoredSlot))
				{
					return View; // OffHand-only shields must never equip MainHand.
				}
				View.bTwoHanded =
					Canonical.Weapon.HandPolicy == EGridCanonicalHandPolicy::TwoHanded;
			}
			else
			{
				// Non-ARM equipment is not yet converted in 06.4. In
				// particular, registered ammunition is NEVER wearable.
				if (!Canonical.AmmoKindId.IsNone() ||
					!LegacyDefinition->CanEquipToSlot(AuthoredSlot))
				{
					return View;
				}
			}
		}
		else
		{
			// Only identities absent from the configured register retain the
			// transitional asset rules until their binary conversion.
			if (!LegacyDefinition->CanEquipToSlot(AuthoredSlot))
			{
				return View;
			}
			View.bTwoHanded =
				LegacyDefinition->GetEffectiveHandUsage() == EGridItemHandUsage::TwoHanded;
		}

		if (View.bTwoHanded &&
			AuthoredSlot != EGridEquipmentSlot::MainHand &&
			AuthoredSlot != EGridEquipmentSlot::OffHand)
		{
			return View;
		}
		View.PhysicalSlot = View.bTwoHanded
			? EGridEquipmentSlot::MainHand : AuthoredSlot;
		View.bResolved = true;
		return View;
	}
};
