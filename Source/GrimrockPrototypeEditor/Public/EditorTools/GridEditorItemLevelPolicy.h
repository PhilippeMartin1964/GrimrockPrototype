#pragma once

#include "Runtime/Items/GridItemSubsystem.h"

/** Editor-only validation of NEW authored levels; never touches SaveGame instances. */
struct FGridEditorItemLevelPolicy
{
	/** Persisted ID/N-level authoring: no legacy definition or N1 default
	 * is ever inferred after the physical identity has been authored. */
	static bool CanAuthorItemLevel(
		const UGridItemSubsystem* Registry, FName ItemId, int32 PhysicalLevel)
	{
		if (!IsValid(Registry) || !Registry->IsConfigured() ||
			!Registry->ContainsItem(ItemId) ||
			PhysicalLevel < 0 || PhysicalLevel > 20)
		{
			return false;
		}
		FGridResolvedItem Resolved;
		return Registry->ResolveItem(ItemId, PhysicalLevel, Resolved) &&
			Resolved.IsValid() && Resolved.ItemId == ItemId &&
			Resolved.ItemLevel == PhysicalLevel &&
			(Resolved.bIsArmament || PhysicalLevel == 0);
	}

};
