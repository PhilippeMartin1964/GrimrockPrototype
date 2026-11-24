#pragma once

#include "Runtime/Items/GridItemSubsystem.h"
#include "GridItemWeightReadModel.h"

/**
 * Diagnostics-only formatting; never supplies gameplay state.
 * A slot carries a real level; a definition does not, so list authored
 * progression endpoints rather than fabricating a default item level.
 */
struct FGridInventoryDiagnosticWeightReadModel
{
	static FString FormatInstanceUnitWeight(
		const UGridItemSubsystem* Registry, const FGridItemInstance& Instance)
	{
		float UnitWeight = 0.0f;
		if (!FGridItemWeightReadModel::TryResolveUnitWeight(Registry, Instance, UnitWeight))
		{
			return TEXT("INVALID"); // Known invalid N0 cannot reuse an old snapshot.
		}
		return FString::Printf(TEXT("%.1f"), UnitWeight);
	}

	static FString FormatDefinitionWeight(
		const UGridItemSubsystem* Registry, FName ItemId)
	{
		if (!Registry || !Registry->ContainsItem(ItemId))
		{
			return TEXT("INVALID");
		}
		FGridResolvedItem Unlevelled;
		if (Registry->ResolveItem(ItemId, 0, Unlevelled))
		{
			return FString::Printf(TEXT("N0:%.1f"), Unlevelled.WeightKg);
		}
		FGridResolvedItem First, Last;
		if (Registry->ResolveItem(ItemId, 1, First) &&
			Registry->ResolveItem(ItemId, 20, Last))
		{
			return FString::Printf(TEXT("N1:%.1f,N20:%.1f"), First.WeightKg, Last.WeightKg);
		}
		return TEXT("INVALID"); // Registered but unusable, no old definition fallback.
	}
};
