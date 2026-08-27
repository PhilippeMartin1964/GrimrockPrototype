#pragma once

#include "CoreMinimal.h"

class UGridLevelAsset;
struct FGridDungeonRuntimeState;
struct FGridLevelRuntimeState;

/**
 * MON19.2.2 typed logical variable store.
 *
 * UGridLevelAsset owns declarations/defaults. FGridLevelRuntimeState owns the
 * mutable values and is already part of the durable dungeon SaveGame.
 */
namespace GridLevelVariableStore
{
	GRIMROCKPROTOTYPE_API bool ValidateDefinitions(const UGridLevelAsset& LevelAsset, FString& OutError);

	/**
     * Builds or reconciles the runtime snapshot with the current declarations.
     * Existing values of the same declared type are preserved; new/type-changed
     * variables receive their current level defaults and stale values are removed.
     */
	GRIMROCKPROTOTYPE_API bool EnsureInitialized(const UGridLevelAsset& LevelAsset, FGridLevelRuntimeState& RuntimeState, FString& OutError);

	GRIMROCKPROTOTYPE_API bool ResetToDefaults(const UGridLevelAsset& LevelAsset, FGridLevelRuntimeState& RuntimeState, FString& OutError);

	GRIMROCKPROTOTYPE_API bool TryGetBool(
		const UGridLevelAsset& LevelAsset, FGridLevelRuntimeState& RuntimeState, FName VariableId, bool& OutValue, FString& OutError);

	GRIMROCKPROTOTYPE_API bool SetBool(
		const UGridLevelAsset& LevelAsset, FGridLevelRuntimeState& RuntimeState, FName VariableId, bool bValue, FString& OutError);

	GRIMROCKPROTOTYPE_API bool TryGetInt32(
		const UGridLevelAsset& LevelAsset, FGridLevelRuntimeState& RuntimeState, FName VariableId, int32& OutValue, FString& OutError);

	GRIMROCKPROTOTYPE_API bool SetInt32(
		const UGridLevelAsset& LevelAsset, FGridLevelRuntimeState& RuntimeState, FName VariableId, int32 Value, FString& OutError);

	/** Structural validation possible without loading a UGridLevelAsset. */
	GRIMROCKPROTOTYPE_API bool ValidateDungeonSnapshots(const FGridDungeonRuntimeState& DungeonState, FString& OutError);
}
