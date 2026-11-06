#pragma once

#include "Core/GridLevelPlacementTypes.h"
#include "Core/GridWorldObjectVisual.h"

/**
 * Resolves sparse per-instance visual overrides without mutating the shared definition.
 *
 * The array is intentionally sparse and keyed by PartIndex. If multiple entries
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
	/** Sparse instance overrides only; does not mutate the shared Definition. */
	inline FGridWorldObjectStaticPart ResolveStaticPart(
		const FGridWorldObjectStaticPart& Definition,
		const FGridWorldObjectNiagaraInstanceOverride& Overrides)
	{
		FGridWorldObjectStaticPart Resolved = Definition;
		if (Overrides.bOverrideSystem)
		{
			// Empty reference is a deliberate "disable for this instance".
			Resolved.ActiveNiagaraSystem = Overrides.NiagaraSystem.LoadSynchronous();
		}
		FTransform Transform = Resolved.ActiveNiagaraLocalTransform;
		if (Overrides.bOverrideOffset)
		{
			const FVector Offset = Overrides.LocalOffsetCm;
			if (!Offset.ContainsNaN() &&
				FMath::Abs(Offset.X) <= 10000.f &&
				FMath::Abs(Offset.Y) <= 10000.f &&
				FMath::Abs(Offset.Z) <= 10000.f)
			{
				Transform.AddToTranslation(Offset);
			}
		}
		if (Overrides.bOverrideScale)
		{
			const float Multiplier = FMath::IsFinite(Overrides.ScaleMultiplier)
				? FMath::Clamp(Overrides.ScaleMultiplier, .01f, 5.f) : 1.f;
			Transform.SetScale3D(Transform.GetScale3D() * Multiplier);
		}
		Resolved.ActiveNiagaraLocalTransform = Transform;
		return Resolved;
	}

}
