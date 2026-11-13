#pragma once

#include "CoreMinimal.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"

/**
 * Editor-only whitelist for the genuinely generated N1 standard action.
 * Any local change or additional field causes rejection, never data loss.
 * Caller owns all source objects and no package is modified.
 */
namespace GridArmClean06ActionProjection
{
	enum class EResult : uint8
	{
		ProvenStandard,
		InvalidInput,
		NonStandardAction
	};

	EResult TryProjectGeneratedAttack(
		const FGridWeaponPipelineRow& Manifest,
		const UGridItemDefinitionAsset& Item,
		const FGridWeaponLevelProfile& LevelOne,
		const FGridCombatActionDefinition& SourceAction,
		FGridCanonicalAction& OutAction);
}
