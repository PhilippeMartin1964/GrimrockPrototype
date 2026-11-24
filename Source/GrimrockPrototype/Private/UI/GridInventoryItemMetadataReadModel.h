#pragma once

#include "Runtime/Items/GridItemSubsystem.h"

/** Temporary C++ projection of identity and mass for an inventory tooltip. */
struct FGridInventoryItemMetadata
{
	bool bRegisteredIdentity = false;
	bool bResolved = false;
	FText DisplayName;
	FText Description;
	float UnitWeightKg = 0.0f;
};

struct FGridInventoryItemMetadataReadModel
{
	static FGridInventoryItemMetadata Resolve(const UGridItemSubsystem* Registry, const FGridItemInstance& Instance)
	{
		FGridInventoryItemMetadata Result;
		if (!Instance.IsValid())
		{
			return Result;
		}
		if (Registry && Registry->ContainsItem(Instance.ItemDefinitionId))
		{
			Result.bRegisteredIdentity = true;
			// Never display historical text or weight for a registered invalid N0.
			Result.DisplayName = FText::FromName(Instance.ItemDefinitionId);
			FGridResolvedItem Canonical;
			if (Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Canonical) &&
				Canonical.IsValid() && Canonical.ItemId == Instance.ItemDefinitionId &&
				Canonical.ItemLevel == Instance.ItemLevel &&
				(Canonical.bIsArmament ? Instance.ItemLevel >= 1 : Instance.ItemLevel == 0) &&
				FMath::IsFinite(Canonical.WeightKg) && Canonical.WeightKg >= 0.0f)
			{
				Result.bResolved = true;
				if (!Canonical.DisplayName.IsEmpty()) Result.DisplayName = Canonical.DisplayName;
				Result.Description = Canonical.Description;
				Result.UnitWeightKg = Canonical.WeightKg;
			}
			return Result;
		}
		// Unknown identities expose only their technical ID, not a cached label
		// or mass. Historical fixture data is isolated from the production UI.
		Result.DisplayName = FText::FromName(Instance.ItemDefinitionId);
		return Result;
	}
};
