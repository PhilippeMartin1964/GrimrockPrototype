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
	static FGridInventoryItemMetadata Project(const FGridItemInstance& Instance,
		const FGridResolvedItem* Item, bool bRegisteredIdentity)
	{
		FGridInventoryItemMetadata Result;
		if (!Instance.IsValid())
		{
			return Result;
		}
		Result.bRegisteredIdentity = bRegisteredIdentity;
		Result.DisplayName = FText::FromName(Instance.ItemDefinitionId);
		if (Item && FMath::IsFinite(Item->WeightKg) && Item->WeightKg >= 0.0f)
		{
			Result.bResolved = true;
			if (!Item->DisplayName.IsEmpty()) Result.DisplayName = Item->DisplayName;
			Result.Description = Item->Description;
			Result.UnitWeightKg = Item->WeightKg;
		}
		return Result;
	}

	static FGridInventoryItemMetadata Resolve(const UGridItemSubsystem* Registry, const FGridItemInstance& Instance)
	{
		FGridResolvedItem Item;
		const bool bResolved = Registry && Registry->ResolvePhysicalItem(Instance, Item);
		return Project(Instance, bResolved ? &Item : nullptr,
			Registry && Registry->ContainsItem(Instance.ItemDefinitionId));
	}
};
