#pragma once

#include "Runtime/Items/GridItemSubsystem.h"

/** Starter kit may save old snapshots, but their value comes from ItemSystem. */
struct FGridStartingKitStaticMetadataReadView
{
	bool bCanonicalIdentity = false;
	bool bResolved = false;
	FText DisplayName;
	float UnitWeightKg = 0.0f;
};

struct FGridStartingKitStaticMetadataReadModel
{
	static FGridStartingKitStaticMetadataReadView Resolve(
		const UGridItemSubsystem* Registry, FName ItemId, int32 FixedItemLevel,
		const FText& LegacyName, float LegacyWeight)
	{
		FGridStartingKitStaticMetadataReadView Result;
		if (ItemId.IsNone()) return Result;
		if (Registry && Registry->HasCanonicalItem(ItemId))
		{
			Result.bCanonicalIdentity = true;
			FGridResolvedItem Canonical;
			if (!Registry->ResolveItem(ItemId, FixedItemLevel, Canonical) ||
				!FMath::IsFinite(Canonical.WeightKg) || Canonical.WeightKg < 0.0f)
			{
				return Result; // No N1 promotion or legacy fallback for known invalid item.
			}
			Result.bResolved = true;
			Result.DisplayName = Canonical.DisplayName.IsEmpty()
				? FText::FromName(ItemId) : Canonical.DisplayName;
			Result.UnitWeightKg = Canonical.WeightKg;
			return Result;
		}
		Result.bResolved = true;
		Result.DisplayName = LegacyName;
		Result.UnitWeightKg = LegacyWeight;
		return Result;
	}
};
