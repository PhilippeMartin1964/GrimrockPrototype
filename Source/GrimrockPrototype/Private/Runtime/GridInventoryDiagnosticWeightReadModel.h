#pragma once

#include "Runtime/GridItemDefinitionAsset.h"
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
		const UGridItemSubsystem* Registry, const UGridItemDefinitionAsset* Definition)
	{
		if (!IsValid(Definition))
		{
			return TEXT("INVALID");
		}
		if (!Registry || !Registry->HasCanonicalItem(Definition->ItemDefinitionId))
		{
			// No ItemSystem for legacy items or standalone diagnostics.
			return FString::Printf(TEXT("%.1f"), Definition->Weight);
		}
		FGridResolvedItem Unlevelled;
		if (Registry->ResolveItem(Definition->ItemDefinitionId, 0, Unlevelled))
		{
			return FString::Printf(TEXT("N0:%.1f"), Unlevelled.WeightKg);
		}
		FGridResolvedItem First, Last;
		if (Registry->ResolveItem(Definition->ItemDefinitionId, 1, First) &&
			Registry->ResolveItem(Definition->ItemDefinitionId, 20, Last))
		{
			return FString::Printf(TEXT("N1:%.1f,N20:%.1f"), First.WeightKg, Last.WeightKg);
		}
		return TEXT("INVALID"); // Registered but unusable, no old definition fallback.
	}
};
