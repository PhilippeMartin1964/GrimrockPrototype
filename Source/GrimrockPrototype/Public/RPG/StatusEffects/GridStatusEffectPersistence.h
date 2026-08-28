#pragma once

#include "CoreMinimal.h"
#include "RPG/StatusEffects/GridStatusEffectTypes.h"

class UGridStatusEffectDefinitionAsset;
struct FGridPartyInventoryState;

/**
 * Status Effect persistence / rehydration boundary.
 *
 * TD07.3.3.8 makes Character.StatusEffects durable directly. The snapshot
 * conversion methods remain because FGridRuntimeMonsterState still persists
 * monster status effects as FGridStatusEffectSaveState.
 */
struct GRIMROCKPROTOTYPE_API FGridStatusEffectPersistence
{
	/** Stable snapshot validation used by monster persistence. */
	static bool ValidateSavedCollection(const TArray<FGridStatusEffectSaveState>& SavedStates, FString& OutError);

	/** Durable character collection validation; DefinitionAsset is deliberately ignored. */
	static bool ValidateDurableCollection(const FGridStatusEffectCollection& Collection, FString& OutError);

	/** Runtime validation additionally requires a matching canonical DefinitionAsset cache. */
	static bool ValidateRuntimeCollection(const FGridStatusEffectCollection& Collection, FString& OutError);

	/** Structural current-schema validation for every Active + Pool character status collection. */
	static bool ValidatePartyStatusEffects(const FGridPartyInventoryState& PartyState, FString& OutError);

	/** Save-time validation requiring valid runtime DefinitionAsset caches. */
	static bool ValidateRuntimePartyStatusEffects(const FGridPartyInventoryState& PartyState, FString& OutError);

	/**
	 * Atomically rehydrate every character DefinitionAsset cache from EffectId.
	 * Stable fields remain unchanged; deterministic collection ordering is rebuilt.
	 */
	static bool RehydratePartyStatusEffects(
		FGridPartyInventoryState& PartyState, TFunctionRef<UGridStatusEffectDefinitionAsset*(FName)> DefinitionResolver, FString& OutError);
	static bool RehydratePartyStatusEffects(FGridPartyInventoryState& PartyState, FString& OutError);

	/** MON16.7 snapshot conversion retained for monster runtime persistence. */
	static bool CaptureCollection(const FGridStatusEffectCollection& RuntimeCollection, TArray<FGridStatusEffectSaveState>& OutSavedStates, FString& OutError);

	/** MON16.7 snapshot restore retained for monster runtime persistence. */
	static bool RestoreCollection(const TArray<FGridStatusEffectSaveState>& SavedStates,
		TFunctionRef<UGridStatusEffectDefinitionAsset*(FName)> DefinitionResolver, FGridStatusEffectCollection& OutRuntimeCollection, FString& OutError);
	static bool RestoreCollection(const TArray<FGridStatusEffectSaveState>& SavedStates, FGridStatusEffectCollection& OutRuntimeCollection, FString& OutError);

	/** Resolves the canonical GridStatusEffect:EffectId primary asset. */
	static UGridStatusEffectDefinitionAsset* ResolveDefinitionByEffectId(FName EffectId);
};
