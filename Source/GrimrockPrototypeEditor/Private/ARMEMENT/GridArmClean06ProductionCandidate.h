#pragma once

#include "CoreMinimal.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "UI/GridEquippedWeaponActionIconPalette.h"

/**
 * Build only concrete identities and visual bindings from the 22 ARM manifest
 * into a transient UGridItemSystemAsset. This is NOT a fully authored registry:
 * numeric, actions and presentation catalogues must be constructed separately.
 */
namespace GridArmClean06ProductionCandidate
{
	struct FSummary
	{
		int32 ItemDefinitions = 0;
		int32 UniqueVisualProfiles = 0;
		int32 PassiveDefinitions = 0;
		int32 NonCanonicalIdentities = 0;
	};

	bool BuildDefinitionsAndVisuals(
		const TArray<FGridWeaponPipelineRow>& Manifest,
		const UGridWeaponVisualCatalogueAsset& LegacyVisuals,
		const UGridEquippedWeaponActionIconPalette& LegacyIcons,
		UGridItemSystemAsset& Destination,
		FSummary& OutSummary);
}
