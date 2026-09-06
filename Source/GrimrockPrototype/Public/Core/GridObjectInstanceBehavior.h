#pragma once

#include "Core/GridLevelAsset.h"
#include "Core/GridObjectArchetypeAsset.h"

/**
 * WORLDOBJ-MIG09 definition/instance behavior resolver.
 *
 * MIG08 re-saved production level assets into typed placement storage. A world
 * object therefore resolves shared behavior from its definition and overlays
 * only the instance-owned payload carried by the compatibility object view.
 * The old per-object sparse migration marker no longer participates in runtime
 * behavior resolution.
 *
 * A missing definition still returns the raw object behavior temporarily for
 * direct legacy actor/test callers. Those direct initializer paths are removed
 * later in MIG09 before FGridLevelObjectData itself is deleted.
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
		const FGridLevelObjectData& ObjectData, const UGridObjectArchetypeAsset* Archetype)
	{
		// Temporary direct-call compatibility only. Production world objects have a
		// definition after MIG08; this branch disappears with the old initializers.
		if (!Archetype)
		{
			return ObjectData.Behavior;
		}

		FGridObjectBehaviorParams Resolved = Archetype->DefaultBehavior;
		ApplyInstanceOwnedOverrides(ObjectData.Behavior, Resolved);
		return Resolved;
	}

	inline FGridObjectBehaviorParams Resolve(
		const UGridLevelAsset* LevelAsset, const FGridLevelObjectData& ObjectData, const UGridObjectArchetypeAsset* Archetype)
	{
		// LevelAsset is retained in the signature only while consumers are still
		// typed against FGridLevelObjectData. MIG09 no longer consults migration
		// markers to choose definition authority.
		(void)LevelAsset;
		return Resolve(ObjectData, Archetype);
	}
}
