#pragma once

#include "CoreMinimal.h"
#include "ARMEMENT/GridArmClean06FullProductionCandidate.h"

/**
 * Editor-only projection for the existing production arrow definition.
 * AmmoKindId intentionally uses its already-authored ItemDefinitionId.
 * No new ammunition category, duplicate damage table or resource is created.
 */
namespace GridArmClean06ProductionAmmoCandidate
{
	struct FSummary
	{
		int32 CanonicalAmmoDefinitions = 0;
		int32 WeaponAmmoReferences = 0;
		FName SourceAmmoItemId = NAME_None;
		FName ResultAmmoKindId = NAME_None;
	};

	bool AddExistingArrows(
		const TArray<FGridWeaponPipelineRow>& Manifest,
		UGridItemSystemAsset& Root,
		GridArmClean06FullProductionCandidate::FSummary& RootSummary,
		FSummary& OutSummary);
}
