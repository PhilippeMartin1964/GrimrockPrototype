#pragma once

#include "GridInventoryItemMetadataReadModel.h"

/** Single item-level projection for sorting, independent of slot display order. */
struct FGridInventorySortReadView
{
	FString DisplayName;
	float TotalWeight = 0.0f;
};

struct FGridInventorySortReadModel
{
	static FGridInventorySortReadView Resolve(
		const UGridItemSubsystem* Registry, const UGridItemDefinitionAsset* LegacyDefinition,
		const FGridItemInstance& Instance)
	{
		FGridInventorySortReadView Result;
		if (!Instance.IsValid()) return Result;
		const FGridInventoryItemMetadata Meta =
			FGridInventoryItemMetadataReadModel::Resolve(Registry, LegacyDefinition, Instance);
		Result.DisplayName = Meta.DisplayName.IsEmpty() ? Instance.ItemDefinitionId.ToString()
			: Meta.DisplayName.ToString();
		Result.TotalWeight = FMath::Max(0.0f, Meta.UnitWeightKg) *
			static_cast<float>(FMath::Max(1, Instance.Quantity));
		return Result;
	}
};
