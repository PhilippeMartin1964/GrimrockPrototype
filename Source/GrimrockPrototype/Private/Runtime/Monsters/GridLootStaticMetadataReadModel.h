#pragma once

#include "Runtime/GridInventoryTypes.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** Temporary copies for 06.4 SaveGame compatibility; resolved from one source. */
struct FGridLootStaticMetadataReadView
{
	bool bCanonicalIdentity = false;
	bool bResolved = false;
	FText DisplayName;
	float UnitWeightKg = 0.0f;
};

struct FGridLootStaticMetadataReadModel
{
	static FGridLootStaticMetadataReadView Resolve(const UGridItemSubsystem* Registry,
		const FGridItemInstance& Instance, const FText& LegacyName, float LegacyUnitWeightKg)
	{
		FGridLootStaticMetadataReadView Result;
		if (!Instance.IsValid()) return Result;
		if (Registry && Registry->HasCanonicalItem(Instance.ItemDefinitionId))
		{
			Result.bCanonicalIdentity = true;
			FGridResolvedItem Canonical;
			if (!Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Canonical) ||
				!FMath::IsFinite(Canonical.WeightKg) || Canonical.WeightKg < 0.0f)
			{
				return Result; // Known invalid canonical item must not borrow old metadata.
			}
			Result.bResolved = true;
			Result.DisplayName = Canonical.DisplayName.IsEmpty()
				? FText::FromName(Instance.ItemDefinitionId) : Canonical.DisplayName;
			Result.UnitWeightKg = Canonical.WeightKg;
			return Result;
		}
		// Unknown ID retains exactly its historical snapshot semantics.
		Result.bResolved = true;
		Result.DisplayName = LegacyName;
		Result.UnitWeightKg = LegacyUnitWeightKg;
		return Result;
	}
};
