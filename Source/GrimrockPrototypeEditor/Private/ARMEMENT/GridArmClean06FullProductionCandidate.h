#pragma once

#include "CoreMinimal.h"
#include "ARMEMENT/GridArmClean06ProductionActionsCandidate.h"
#include "ARMEMENT/GridArmClean06ProductionNumericsCandidate.h"
#include "ARMEMENT/GridArmClean06ProductionCandidate.h"

/**
 * Read-only assembly of all four canonical catalogues and 22 concrete ARM.
 * Missing authoring stays unresolved: this is NOT a runtime cutover.
 */
namespace GridArmClean06FullProductionCandidate
{
	struct FSummary
	{
		GridArmClean06ProductionCandidate::FSummary Items;
		GridArmClean06ProductionActionsCandidate::FSummary Actions;
		GridArmClean06ProductionNumericsCandidate::FSummary Numerics;
		int32 AttacksMainHand = 0;
		int32 AttacksOffHand = 0;
		int32 UnboundOffensiveArmaments = 0;
		int32 UnmappedAmmunition = 0;
		int32 UnconvertedThrowPolicies = 0;
		int32 InvalidCanonicalIdentities = 0;
		bool bRegistryValid = false;

		bool ReadyForBinaryCutover() const
		{
			return bRegistryValid && UnboundOffensiveArmaments == 0 &&
				UnmappedAmmunition == 0 && UnconvertedThrowPolicies == 0 &&
				InvalidCanonicalIdentities == 0 &&
				Actions.CustomizedActionsUnconverted == 0 &&
				Actions.OtherLocalActionsUnconverted == 0;
		}
	};

	bool Build(const TArray<FGridWeaponPipelineRow>& Manifest,
		const UGridWeaponCatalogueAsset& NumericSource,
		const UGridWeaponVisualCatalogueAsset& VisualSource,
		const UGridEquippedWeaponActionIconPalette& IconSource,
		UGridItemSystemAsset& TransientRoot, FSummary& OutSummary);
}
