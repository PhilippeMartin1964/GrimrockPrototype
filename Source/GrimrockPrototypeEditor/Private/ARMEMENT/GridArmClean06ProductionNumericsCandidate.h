#pragma once

#include "CoreMinimal.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"

/** Native, Editor-only numeric projection of all 82 ARM while proving
 * hand/type semantics for precisely the 22 concrete manifest entries.
 * Uninstantiated ARM rows retain bHandAndKindProven=false.
 */
namespace GridArmClean06ProductionNumericsCandidate
{
	struct FSummary
	{
		int32 Archetypes = 0;
		int32 ComparedProfiles = 0;
		int32 PassiveProfiles = 0;
		int32 ProvenConcreteArchetypes = 0;
		int32 UnprovenUninstantiatedArchetypes = 0;
		int32 MainHandAttacks = 0;
		int32 OffHandAttacks = 0;
	};

	bool Build(const UGridWeaponCatalogueAsset& Source,
		const TArray<FGridWeaponPipelineRow>& Manifest,
		UGridCanonicalWeaponCatalogueAsset& Candidate, FSummary& OutSummary);
}
