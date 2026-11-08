#pragma once

#include "CoreMinimal.h"
#include "Core/GridVfxProfileTypes.h"

/** Component transform. SpawnBox controls physical XY footprint when available.
 * Without a matching SpawnBox, coverage is only a proportional XY scale.
 * Height is calibrated relative to the source Niagara's apparent height.
 */
struct FGridFireNiagaraGeometry
{
	static FVector ComponentScale(const FGridFireVfxSettings& RawSettings, bool bSpawnBoxAvailable)
	{
		const FGridFireVfxSettings Vfx = RawSettings.Sanitized();
		// Exact cell footprint is controlled by SpawnBox if the system exposes it.
		// Otherwise XY is only a relative scale; never claim an exact width.
		const float XY = Vfx.GroundFireScale *
			(bSpawnBoxAvailable ? 1.f : Vfx.GroundFireCoverage);
		const float Z = Vfx.GroundFireScale *
			(Vfx.FlameHeightCm / Vfx.NiagaraReferenceHeightCm);
		return FVector(XY, XY, Z);
	}
};
