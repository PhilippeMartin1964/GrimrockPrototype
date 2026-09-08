#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"
#include "Core/GridLevelPlacementTypes.h"

class UGridLevelAsset;
struct FGridLevelRuntimeState;

/** Result of one data-only logic-node execution. */
struct GRIMROCKPROTOTYPE_API FGridLogicExecutionResult
{
	bool bEmitEvent = false;
	EGridObjectEvent EmittedEvent = EGridObjectEvent::Activated;
	bool bStateChanged = false;
	FString Error;
};

/**
 * MON19.2.3 deterministic execution of data-only logic primitives.
 * No Actor, Tick, World or Lua dependency.
 */
namespace GridLogicRuntime
{
	/** WORLDOBJ-MIG09-E2C-FINAL-A native typed Logic placement contract. */
	GRIMROCKPROTOTYPE_API bool ValidateNode(const UGridLevelAsset& LevelAsset, const FGridLogicObjectInstance& ObjectData, FString& OutError);

	GRIMROCKPROTOTYPE_API bool ExecuteNode(const UGridLevelAsset& LevelAsset, const FGridLogicObjectInstance& ObjectData, FGridLevelRuntimeState& RuntimeState,
		EGridObjectCommand Command, FGridLogicExecutionResult& OutResult);

	/** Temporary legacy adapters for Editor/tests until E2C-FINAL-B/C. */
	GRIMROCKPROTOTYPE_API bool ValidateNode(const UGridLevelAsset& LevelAsset, const FGridLevelObjectData& ObjectData, FString& OutError);

	GRIMROCKPROTOTYPE_API bool ExecuteNode(const UGridLevelAsset& LevelAsset, const FGridLevelObjectData& ObjectData, FGridLevelRuntimeState& RuntimeState,
		EGridObjectCommand Command, FGridLogicExecutionResult& OutResult);
}
