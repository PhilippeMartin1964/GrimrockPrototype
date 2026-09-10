#pragma once

#include "Core/GridLevelPlacementTypes.h"
#include "Core/GridWorldObjectVisual.h"

/**
 * Resolves sparse per-instance visual overrides without mutating the shared definition.
 *
 * The array is intentionally sparse and keyed by PartIndex (0..1). If multiple entries
 * target the same part, they are applied in authored order so later entries win only for
 * the fields they explicitly override.
 */
namespace GridWorldObjectInstanceVisual
{
	inline FGridWorldObjectMovingPart ResolveMovingPart(
		const FGridWorldObjectMovingPart& DefinitionPart,
		const TArray<FGridWorldObjectMovingPartInstanceOverride>& Overrides,
		int32 PartIndex)
	{
		FGridWorldObjectMovingPart Resolved = DefinitionPart;

		for (const FGridWorldObjectMovingPartInstanceOverride& Override : Overrides)
		{
			if (Override.PartIndex != PartIndex)
			{
				continue;
			}

			if (Override.bOverrideLocalTransform)
			{
				Resolved.LocalTransform = Override.LocalTransform;
			}
			if (Override.bOverrideMotionAmount)
			{
				Resolved.Motion.Amount = Override.MotionAmount;
			}
			if (Override.bOverrideMotionDuration)
			{
				Resolved.Motion.Duration = FMath::Max(0.0f, Override.MotionDuration);
			}
		}

		return Resolved;
	}
}
