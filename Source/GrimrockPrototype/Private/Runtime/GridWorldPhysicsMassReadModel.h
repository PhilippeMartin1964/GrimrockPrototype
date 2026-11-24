#pragma once

#include "GridItemWeightReadModel.h"

/** Opt-in Chaos world mass is a unit mass, never the total stack weight. */
struct FGridWorldPhysicsMassReadModel
{
	static bool TryResolve(const UGridItemSubsystem* Registry,
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
		const FGridItemInstance& Instance, float& OutMassKg)
	{
		OutMassKg = 0.0f;
		if (Registry)
		{
			FGridResolvedItem Item;
			return TryResolve(Registry, Instance, Item) &&
				TryResolveProjected(Item, OutMassKg);
		}
		return false;
	}
};
