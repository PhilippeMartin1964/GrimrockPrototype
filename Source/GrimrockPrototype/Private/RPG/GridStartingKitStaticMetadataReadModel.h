#pragma once

#include "Runtime/Items/GridItemSubsystem.h"

/** Read-only starter metadata projection; historical fixture values are not serialized. */
struct FGridStartingKitStaticMetadataReadView
{
	bool bRegisteredIdentity = false;
	bool bResolved = false;
	FText DisplayName;
	float UnitWeightKg = 0.0f;
};

struct FGridStartingKitStaticMetadataReadModel
{
	static FGridStartingKitStaticMetadataReadView Resolve(
		const UGridItemSubsystem* Registry, FName ItemId, int32 FixedItemLevel)
	{
		FGridStartingKitStaticMetadataReadView Result;
		if (ItemId.IsNone()) return Result;
		if (Registry && Registry->ContainsItem(ItemId))
		{
			Result.bRegisteredIdentity = true;
			FGridResolvedItem Canonical;
			if (!Registry->ResolveItem(ItemId, FixedItemLevel, Canonical) ||
				!Canonical.IsValid() || (!Canonical.bIsArmament && FixedItemLevel != 0) ||
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
		return Result;
	}
};
