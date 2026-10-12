#pragma once

#include "CoreMinimal.h"
#include "Magic/GridSpellPresentation.h"
#include "Magic/GridSpellTypes.h"

class UGridSpellDefinitionAsset;

/**
 * Temporary Editor-only materialization seed derived from the existing
 * FGridProductionSpellLibrary. It introduces no second production authority.
 */
struct GRIMROCKPROTOTYPEEDITOR_API FGridProductionSpellAuthoringSeed
{
	FGridSpellDefinition Definition;
	FGridSpellPresentationProfile Presentation;
};

/**
 * Safe Editor-only materialization of the four current production Spells.
 *
 * TEXT05.5 deliberately keeps FGridProductionSpellLibrary as the source used
 * for first materialization. Existing same-SpellId assets preserve authored
 * DisplayName, Description and Presentation exactly while mechanics are rebuilt.
 */
struct GRIMROCKPROTOTYPEEDITOR_API FGridSpellCatalogAuthoring
{
	static const TCHAR* ProductionFolder();

	static bool BuildProductionSeeds(
		TArray<FGridProductionSpellAuthoringSeed>& OutSeeds,
		FString& OutError);

	static bool ConfigureSpellAsset(
		UGridSpellDefinitionAsset& SpellAsset,
		const FGridProductionSpellAuthoringSeed& Seed,
		FString& OutError);

	/** Compare only gameplay identity/mechanics; authored text/presentation are excluded. */
	static bool IsCanonicalMechanics(
		const UGridSpellDefinitionAsset& SpellAsset,
		const FGridProductionSpellAuthoringSeed& Seed);

	/** Creates/updates and saves the four DA_Spell_* assets through Unreal. */
	static bool AuthorProductionAssets(FString& OutError);
};
