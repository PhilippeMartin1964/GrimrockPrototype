#pragma once

#include "GridItemWeightReadModel.h"
#include "Runtime/GridItemDefinitionAsset.h"

/** Opt-in Chaos world mass is a unit mass, never the total stack weight. */
struct FGridWorldPhysicsMassReadModel
{
	static bool TryResolve(const UGridItemSubsystem* Registry,
		const UGridItemDefinitionAsset* LegacyDefinition,
		const FGridItemInstance& Instance, float& OutMassKg)
	{
		OutMassKg = 0.0f;
		// The existing definition retains the *policy* to opt into Chaos mass;
		// it does not retain canonical ownership of the numerical weight.
		if (!IsValid(LegacyDefinition) || !LegacyDefinition->bUseItemWeightAsWorldPhysicsMass ||
			!Instance.IsValid() || Instance.ItemDefinitionId != LegacyDefinition->ItemDefinitionId)
		{
			return false;
		}
		float UnitWeight = LegacyDefinition->Weight;
		if (Registry && Registry->HasCanonicalItem(Instance.ItemDefinitionId))
		{
			if (!FGridItemWeightReadModel::TryResolveUnitWeight(Registry, Instance, UnitWeight))
			{
				return false; // A known invalid canonical level may not use legacy Weight.
			}
		}
		if (!FMath::IsFinite(UnitWeight) || UnitWeight <= KINDA_SMALL_NUMBER)
		{
			return false;
		}
		OutMassKg = UnitWeight;
		return true;
	}
};
