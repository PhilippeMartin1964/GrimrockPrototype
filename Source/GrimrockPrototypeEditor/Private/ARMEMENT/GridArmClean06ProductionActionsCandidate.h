#pragma once

#include "CoreMinimal.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"

/**
 * Populate the ACTION and PRESENTATION candidates for the 22 concrete ARM
 * definitions. Only fully proven standard actions are projected. Authored
 * PresentationIds are mandatory for bindings; no synthetic IDs are created.
 */
namespace GridArmClean06ProductionActionsCandidate
{
	struct FSummary
	{
		int32 PassiveDefinitions = 0;
		int32 StandardActions = 0;
		int32 CustomizedActionsUnconverted = 0;
		int32 OtherLocalActionsUnconverted = 0;
		int32 NoAuthoredPresentationId = 0;
		int32 NoLocalPresentation = 0;
		int32 ConflictingPresentationId = 0;
		int32 BoundActions = 0;
		/** Existing runtime resolves unconfigured weapons to the same default profile. */
		int32 NeutralDefaultBindings = 0;
		/** Transient proposed links, not installed in an ARM catalogue. */
		TMap<FName, FGridItemActionBinding> BindingsByArchetype;
	};

	bool AddProvenActionsAndPresentations(
		const TArray<FGridWeaponPipelineRow>& Manifest,
		const UGridWeaponCatalogueAsset& SourceWeaponCatalogue,
		UGridItemSystemAsset& TransientRoot,
		FSummary& OutSummary);
}
