#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "CoreGlobals.h"

/** The equipment hand is a canonical item rule, never copied into a kit. */
struct FGridStartingKitEquipmentPolicyReadView
{
	bool bRegisteredIdentity = false;
	bool bResolved = false;
	bool bTwoHanded = false;
	EGridEquipmentSlot PhysicalSlot = EGridEquipmentSlot::None;
};

struct FGridStartingKitEquipmentPolicyReadModel
{
	/** No legacy UGridItemDefinitionAsset required for an explicitly authored
	 * ItemId + physical N-level. Rejects unsupported non-ARM hand slots. */
	static FGridStartingKitEquipmentPolicyReadView ResolveCanonical(
		const UGridItemSubsystem* Registry,
		FName ItemId, int32 FixedItemLevel, EGridEquipmentSlot AuthoredSlot)
	{
		FGridStartingKitEquipmentPolicyReadView View;
		View.bRegisteredIdentity = Registry && Registry->ContainsItem(ItemId);
		if (!View.bRegisteredIdentity || AuthoredSlot == EGridEquipmentSlot::None)
		{
			return View;
		}
		FGridResolvedItem Item;
		if (!Registry->ResolveItem(ItemId, FixedItemLevel, Item) || !Item.IsValid() ||
			(!Item.bIsArmament && FixedItemLevel != 0))
		{
			return View;
		}
		if (Item.bIsArmament)
		{
			if (!Item.Weapon.CanEquipInHand(AuthoredSlot))
			{
				return View;
			}
			View.bTwoHanded = Item.Weapon.HandPolicy == EGridCanonicalHandPolicy::TwoHanded;
		}
		else
		{
			if (!Item.NonArmEquipmentSlots.Contains(AuthoredSlot))
			{
				return View;
			}
			View.bTwoHanded = Item.bNonArmTwoHanded;
		}
		View.PhysicalSlot = View.bTwoHanded
			? EGridEquipmentSlot::MainHand : AuthoredSlot;
		View.bResolved = true;
		return View;
	}

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

		if (Registry)
		{
			return ResolveCanonical(Registry, LegacyDefinition->ItemDefinitionId, FixedItemLevel, AuthoredSlot);
		}
#if WITH_DEV_AUTOMATION_TESTS
		if (!GIsAutomationTesting || !LegacyDefinition->CanEquipToSlot(AuthoredSlot))
		{
			return View;
		}
		View.bTwoHanded =
			LegacyDefinition->GetEffectiveHandUsage() == EGridItemHandUsage::TwoHanded;

		if (View.bTwoHanded &&
			AuthoredSlot != EGridEquipmentSlot::MainHand &&
			AuthoredSlot != EGridEquipmentSlot::OffHand)
		{
			return View;
		}
		View.PhysicalSlot = View.bTwoHanded
			? EGridEquipmentSlot::MainHand : AuthoredSlot;
		View.bResolved = true;
#endif // WITH_DEV_AUTOMATION_TESTS
		return View;
	}
};
