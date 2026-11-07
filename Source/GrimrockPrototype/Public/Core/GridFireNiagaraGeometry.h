#pragma once

#include "CoreMinimal.h"
#include "Core/GridVfxProfileTypes.h"

/** Game-owned transform adapter for Niagara packs that expose no User.* controls.
 * Provides deterministic, independent horizontal coverage and vertical height.
 * Values are relative to the vendor system's measured, unscaled size.
 */
struct FGridFireNiagaraGeometry
{
	static FVector ComponentScale(const FGridFireVfxSettings& RawSettings, float CellSizeCm, bool bSpawnBoxIsAuthoritative = false)
	{
		const FGridFireVfxSettings Vfx = RawSettings.Sanitized();
		const float CellSize = FMath::Max(1.f, CellSizeCm);
		// If Niagara exposes a writable SpawnBox, that parameter defines XY in cm.
		// Avoid applying GroundFireCoverage twice through transform + SpawnBox.
		const float XY = bSpawnBoxIsAuthoritative ? Vfx.GroundFireScale :
			Vfx.GroundFireScale *
			(CellSize * Vfx.GroundFireCoverage / Vfx.NiagaraReferenceWidthCm);
		const float Z = Vfx.GroundFireScale *
			(Vfx.FlameHeightCm / Vfx.NiagaraReferenceHeightCm);
		return FVector(XY, XY, Z);
	}
};
