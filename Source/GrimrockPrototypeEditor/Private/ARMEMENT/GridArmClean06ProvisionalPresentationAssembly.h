#pragma once

#include "CoreMinimal.h"
#include "ARMEMENT/GridArmClean06CompleteProductionPreview.h"
#include "ARMEMENT/GridArmClean06PresentationKeyPreview.h"

/**
 * Temporary projection ONLY. Existing, already-authored ActionIds may be
 * provisionally used as presentation keys after the strict 06.2Z proof.
 * This does not authorize binary authoring or become a runtime fallback.
 */
namespace GridArmClean06ProvisionalPresentationAssembly
{
	struct FSummary
	{
		GridArmClean06PresentationKeyPreview::FSummary Source;
		int32 AppliedProvisionalBindings = 0;
		int32 RemainingUnboundAttacks = 0;
		bool bContainsUnapprovedPresentationKeys = false;
		bool bPresentationCatalogueValid = false;
	};

	bool Apply(const TArray<FGridWeaponPipelineRow>& Manifest,
		UGridItemSystemAsset& TransientRoot,
		const GridArmClean06CompleteProductionPreview::FSummary& Baseline,
		FSummary& OutSummary);
}
