#pragma once

#include "GridItemWeightReadModel.h"
#include "Runtime/GridItemDefinitionAsset.h"

/** Opt-in Chaos world mass is a unit mass, never the total stack weight. */
struct FGridWorldPhysicsMassReadModel
{
	static bool TryResolveCanonical(const UGridItemSubsystem* Registry,
		const FGridItemInstance& Instance, FGridResolvedItem& OutItem)
	{
		return Registry && Instance.IsValid() &&
			Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, OutItem) &&
			OutItem.IsValid() && OutItem.ItemId == Instance.ItemDefinitionId &&
			OutItem.ItemLevel == Instance.ItemLevel;
	}

	static bool TryResolveProjected(const FGridResolvedItem& Item, float& OutMassKg)
	{
		OutMassKg = 0.0f;
		if (!Item.IsValid() || !Item.bUseItemWeightAsWorldPhysicsMass ||
			!FMath::IsFinite(Item.WeightKg) || Item.WeightKg <= KINDA_SMALL_NUMBER)
		{
			return false;
		}
		OutMassKg = Item.WeightKg;
		return true;
	}

	static bool TryResolve(const UGridItemSubsystem* Registry,
		const UGridItemDefinitionAsset* LegacyDefinition,
		const FGridItemInstance& Instance, float& OutMassKg)
	{
		OutMassKg = 0.0f;
		if (Registry)
		{
			FGridResolvedItem Item;
			return TryResolveCanonical(Registry, Instance, Item) &&
				TryResolveProjected(Item, OutMassKg);
		}
#if WITH_DEV_AUTOMATION_TESTS
		// Isolated historical physics fixtures only.
		if (GIsAutomationTesting && Instance.IsValid() && IsValid(LegacyDefinition) &&
			LegacyDefinition->ItemDefinitionId == Instance.ItemDefinitionId &&
			LegacyDefinition->bUseItemWeightAsWorldPhysicsMass &&
			FMath::IsFinite(LegacyDefinition->Weight) && LegacyDefinition->Weight > KINDA_SMALL_NUMBER)
		{
			OutMassKg = LegacyDefinition->Weight;
			return true;
		}
#endif
		return false;
	}
};
