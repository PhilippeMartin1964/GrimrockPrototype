#pragma once

#include "Runtime/GridInventoryTypes.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** Read-only loot validation and metadata projection from ItemSystem. */
struct FGridLootStaticMetadataReadView
{
	bool bCanonicalIdentity = false;
	bool bResolved = false;
	FText DisplayName;
	float UnitWeightKg = 0.0f;
	bool bLightsEnabled = false;
};

struct FGridLootStaticMetadataReadModel
{
	static FGridLootStaticMetadataReadView Resolve(const UGridItemSubsystem* Registry,
		const FGridItemInstance& Instance)
	{
		FGridLootStaticMetadataReadView Result;
		if (!Instance.IsValid() || !IsValid(Registry) || !Registry->HasCanonicalItem(Instance.ItemDefinitionId))
		{
			return Result;
		}
		Result.bCanonicalIdentity = true;
		FGridResolvedItem Canonical;
		if (!Registry->ResolveItem(Instance.ItemDefinitionId, Instance.ItemLevel, Canonical) ||
			!Canonical.IsValid() || !FMath::IsFinite(Canonical.WeightKg) || Canonical.WeightKg < 0.0f)
		{
			return Result;
		}
		Result.bResolved = true;
		Result.DisplayName = Canonical.DisplayName.IsEmpty()
			? FText::FromName(Instance.ItemDefinitionId) : Canonical.DisplayName;
		Result.UnitWeightKg = Canonical.WeightKg;
		Result.bLightsEnabled = Canonical.LightEmitter.HasEmitter() && Canonical.LightEmitter.bDefaultEnabled;
		return Result;
	}
};
