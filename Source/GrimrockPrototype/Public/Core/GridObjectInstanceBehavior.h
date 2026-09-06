#pragma once

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"

/**
 * WORLDOBJ-MIG06 definition/instance behavior resolver.
 *
 * New placements store only instance-owned values in FGridLevelObjectData::Behavior.
 * Existing pre-MIG06 assets keep their historical full snapshot until MIG08 and
 * are identified by the absence of their ObjectId from the level migration set.
 */
namespace GridObjectInstanceBehavior
{
	inline FGridObjectBehaviorParams BuildSparseOverrides(const FGridObjectBehaviorParams& Source)
	{
		FGridObjectBehaviorParams Overrides;

		// True level-instance data only. Shared rules stay in the definition.
		Overrides.Teleporter = Source.Teleporter;
		Overrides.Transition = Source.Transition;
		Overrides.Pit = Source.Pit;
		Overrides.Receptacle.InitialContent = Source.Receptacle.InitialContent;
		Overrides.Lock.bStartsUnlocked = Source.Lock.bStartsUnlocked;

		return Overrides;
	}

	inline void ApplyInstanceOwnedOverrides(const FGridObjectBehaviorParams& Overrides, FGridObjectBehaviorParams& InOutBehavior)
	{
		InOutBehavior.Teleporter = Overrides.Teleporter;
		InOutBehavior.Transition = Overrides.Transition;
		InOutBehavior.Pit = Overrides.Pit;
		InOutBehavior.Receptacle.InitialContent = Overrides.Receptacle.InitialContent;
		InOutBehavior.Lock.bStartsUnlocked = Overrides.Lock.bStartsUnlocked;
	}

	inline FGridObjectBehaviorParams Resolve(
		const FGridLevelObjectData& ObjectData, const UGridObjectArchetypeAsset* Archetype, bool bUsesSparseOverrides)
	{
		// Pre-MIG06 assets retain their historical full instance snapshot exactly.
		if (!bUsesSparseOverrides)
		{
			return ObjectData.Behavior;
		}

		FGridObjectBehaviorParams Resolved = Archetype ? Archetype->DefaultBehavior : FGridObjectBehaviorParams();
		ApplyInstanceOwnedOverrides(ObjectData.Behavior, Resolved);
		return Resolved;
	}

	inline FGridObjectBehaviorParams Resolve(
		const UGridLevelAsset* LevelAsset, const FGridLevelObjectData& ObjectData, const UGridObjectArchetypeAsset* Archetype)
	{
		return Resolve(ObjectData, Archetype, LevelAsset && LevelAsset->UsesSparseBehaviorOverrides(ObjectData.ObjectId));
	}
}
