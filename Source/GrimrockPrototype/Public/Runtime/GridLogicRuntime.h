#pragma once

#include "CoreMinimal.h"
#include "Core/GridTypes.h"

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
	GRIMROCKPROTOTYPE_API bool ValidateNode(const UGridLevelAsset& LevelAsset, const FGridLevelObjectData& ObjectData, FString& OutError);

	GRIMROCKPROTOTYPE_API bool ExecuteNode(const UGridLevelAsset& LevelAsset, const FGridLevelObjectData& ObjectData, FGridLevelRuntimeState& RuntimeState,
		EGridObjectCommand Command, FGridLogicExecutionResult& OutResult);
}
