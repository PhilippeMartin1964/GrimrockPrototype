#pragma once

#include "Runtime/Inspection/GridItemInspectionTypes.h"

/** The editor may choose an initial level; all explicit or physical reads are strict. */
struct GRIMROCKPROTOTYPE_API FGridItemInspectionReadModel
{
	static bool ResolveDefault(const UGridItemSubsystem* Registry, FName ItemId, FGridItemInspectionView& Out);
	static bool ResolveExact(const UGridItemSubsystem* Registry, FName ItemId, int32 ItemLevel, FGridItemInspectionView& Out);
	/** The runtime caller is responsible for revalidating the actual source slot. */
	static bool ResolvePhysical(const UGridItemSubsystem* Registry, const FGridItemInstance& Instance, FGridItemInspectionView& Out);
};
