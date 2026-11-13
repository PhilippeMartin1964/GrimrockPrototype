#pragma once

#include "CoreMinimal.h"
#include "ARMEMENT/GridArmClean06ProductionAmmoCandidate.h"
#include "ARMEMENT/GridArmClean06FullProductionCandidate.h"

/**
 * Editor-only staging: compose the canonical root and all known production
 * ammunition/throw data, while refusing unresolved combat authoring.
 * No asset package is ever saved.
 */
namespace GridArmClean06CompleteProductionPreview
{
	struct FSummary
	{
		GridArmClean06FullProductionCandidate::FSummary Core;
		GridArmClean06ProductionAmmoCandidate::FSummary Ammo;
		int32 PhysicalThrowProfiles = 0;
		int32 VisualThrowProfiles = 0;
		int32 UnresolvedSourceActions = 0;
		bool bVisualCatalogueValid = false;
		bool bWeaponCatalogueValid = false;
		bool bRegistryValidAfterProjection = false;

		bool ReadyForBinaryCutover() const
		{
			return bRegistryValidAfterProjection &&
				bVisualCatalogueValid && bWeaponCatalogueValid &&
				Core.ReadyForBinaryCutover() &&
				Core.UnboundOffensiveArmaments == 0 &&
				Core.UnmappedAmmunition == 0 &&
				Core.UnconvertedThrowPolicies == 0 &&
				UnresolvedSourceActions == 0;
		}
	};

	bool Build(const TArray<FGridWeaponPipelineRow>& Manifest,
		const UGridWeaponCatalogueAsset& NumericSource,
		const UGridWeaponVisualCatalogueAsset& VisualSource,
		const UGridEquippedWeaponActionIconPalette& IconSource,
		UGridItemSystemAsset& TransientRoot, FSummary& OutSummary);
}
