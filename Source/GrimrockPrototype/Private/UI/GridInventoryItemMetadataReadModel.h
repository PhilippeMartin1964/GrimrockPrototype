#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** Temporary C++ projection of identity and mass for an inventory tooltip. */
struct FGridInventoryItemMetadata
{
	bool bCanonicalIdentity = false;
	bool bResolved = false;
	FText DisplayName;
	FText Description;
	float UnitWeightKg = 0.0f;
};

struct FGridInventoryItemMetadataReadModel
{
	static FGridInventoryItemMetadata Resolve(const UGridItemSubsystem* Registry,
		const UGridItemDefinitionAsset* LegacyDefinition, const FGridItemInstance& Instance)
	{
		FGridInventoryItemMetadata Result;
		if (!Instance.IsValid())
		{
			return Result;
		}
		if (Registry && Registry->HasCanonicalItem(Instance.ItemDefinitionId))
		{
			Result.bCanonicalIdentity = true;
			// Never display historical text or weight for a registered invalid N0.
			Result.DisplayName = FText::FromName(Instance.ItemDefinitionId);
			FGridResolvedItem Canonical;
			if (Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Canonical) &&
				FMath::IsFinite(Canonical.WeightKg) && Canonical.WeightKg >= 0.0f)
			{
				Result.bResolved = true;
				if (!Canonical.DisplayName.IsEmpty()) Result.DisplayName = Canonical.DisplayName;
				Result.Description = Canonical.Description;
				Result.UnitWeightKg = Canonical.WeightKg;
			}
			return Result;
		}
		// Items outside the canonical registry continue to use their old authored
		// definition, then the persisted item snapshot, until the 06.4 conversion.
		if (IsValid(LegacyDefinition) && LegacyDefinition->ItemDefinitionId == Instance.ItemDefinitionId)
		{
			Result.bResolved = true;
			Result.DisplayName = LegacyDefinition->DisplayName;
			Result.Description = LegacyDefinition->Description;
			Result.UnitWeightKg = LegacyDefinition->Weight;
		}
		else
		{
			Result.UnitWeightKg = Instance.Weight;
		}
		if (Result.DisplayName.IsEmpty())
		{
			Result.DisplayName = Instance.DisplayName.IsEmpty()
				? FText::FromName(Instance.ItemDefinitionId) : Instance.DisplayName;
		}
		return Result;
	}
};
