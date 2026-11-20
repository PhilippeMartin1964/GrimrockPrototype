#pragma once

#include "Runtime/GridInventoryTypes.h"
#include "Runtime/Items/GridItemSubsystem.h"

/**
 * ARM-CLEAN06.3I: Read-only unit mass for an actual persisted item.
 * Canonical ItemIds always read WeightKg from the single registry at ItemLevel.
 * Copies of Weight on instances remain serialized only for 06.4 migration.
 */
struct FGridItemWeightReadModel
{
	static bool TryResolveUnitWeight(const UGridItemSubsystem* Registry,
		const FGridItemInstance& Instance, float& OutUnitWeight)
	{
		OutUnitWeight = 0.0f;
		if (!Instance.IsValid())
		{
			return false;
		}
		if (Registry && Registry->HasCanonicalItem(Instance.ItemDefinitionId))
		{
			FGridResolvedItem Resolved;
			if (!Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Resolved) ||
				!Resolved.IsValid() || Resolved.ItemId != Instance.ItemDefinitionId ||
				Resolved.ItemLevel != Instance.ItemLevel ||
				!FMath::IsFinite(Resolved.WeightKg) || Resolved.WeightKg < 0.0f)
			{
				return false; // No legacy fallback for a known but invalid canonical ID.
			}
			OutUnitWeight = Resolved.WeightKg;
			return true;
		}
		// Only isolated historical Automation fixtures may read copied mass.
#if WITH_DEV_AUTOMATION_TESTS
		if (GIsAutomationTesting && !Registry && FMath::IsFinite(Instance.Weight) && Instance.Weight >= 0.0f)
		{
			OutUnitWeight = Instance.Weight;
			return true;
		}
#endif
		return false;
	}

	static float GetTotalWeight(const UGridItemSubsystem* Registry, const FGridItemInstance& Instance)
	{
		float UnitWeight = 0.0f;
		return TryResolveUnitWeight(Registry, Instance, UnitWeight)
			? UnitWeight * FMath::Max(1, Instance.Quantity) : 0.0f;
	}
};
