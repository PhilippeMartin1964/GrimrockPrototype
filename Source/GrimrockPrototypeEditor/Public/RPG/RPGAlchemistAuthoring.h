#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatTypes.h"

class URPGClassAsset;
class UGridItemDefinitionAsset;
class UGridStatusEffectDefinitionAsset;

/** RPG03.9.6 editor-only production authoring for the Alchemist class and consumables. */
struct GRIMROCKPROTOTYPEEDITOR_API FRPGAlchemistAuthoring
{
	static const TCHAR* AlchemistAssetPath();
	static FString GetItemObjectPath(FName ItemDefinitionId);
	static FString GetStatusObjectPath(FName EffectId);

	/** Authors the complete Grenadier, Apothecary and Transmuter talent tree. */
	static void ConfigureClass(URPGClassAsset& ClassAsset);

	/** Configures one Alchemist QuickItem by stable ItemDefinitionId. */
	static bool ConfigureItem(UGridItemDefinitionAsset& ItemAsset, FName ItemDefinitionId);

	/** Configures Alchemist-owned/shared statuses required by the authored QuickItems. */
	static bool ConfigureStatus(UGridStatusEffectDefinitionAsset& StatusAsset, FName EffectId);

	static void GetB1ItemIds(TArray<FName>& OutItemIds);
	static void GetB1StatusIds(TArray<FName>& OutStatusIds);
	static void GetB2ItemIds(TArray<FName>& OutItemIds);
	static void GetB2StatusIds(TArray<FName>& OutStatusIds);

	/**
	 * Recipe-facing definition for Transmutation majeure. The future crafting
	 * system selects exactly one output profile per use while Item_Catalyst_Rare
	 * remains the inventory source.
	 */
	static bool BuildMajorTransmutationRecipeAction(
		EGridCombatSurfaceType OutputSurfaceType, FGridCombatActionDefinition& OutAction);

	/** Materializes production class, QuickItems and owned statuses through Unreal Editor. */
	static bool AuthorProductionAssets(FString& OutError);
};
