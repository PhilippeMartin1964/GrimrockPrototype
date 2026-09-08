#include "Runtime/GridLogicRuntime.h"

namespace GridLogicRuntime
{
	bool ValidateNode(const UGridLevelAsset& LevelAsset, const FGridLevelObjectData& ObjectData, FString& OutError)
	{
		// WORLDOBJ-MIG09-E2C: preserve the historical data-only invariant while
		// Editor/tests still submit the legacy DTO. The actual logic validation
		// remains owned by the native typed implementation.
		if (!ObjectData.ArchetypeId.IsNone())
		{
			OutError = TEXT("Logic node must remain data-only and cannot reference an ArchetypeId.");
			return false;
		}

		const FGridLogicObjectInstance TypedObject = GridLevelPlacementConversion::ToLogicObject(ObjectData);
		return ValidateNode(LevelAsset, TypedObject, OutError);
	}

	bool ExecuteNode(const UGridLevelAsset& LevelAsset, const FGridLevelObjectData& ObjectData, FGridLevelRuntimeState& RuntimeState,
		EGridObjectCommand Command, FGridLogicExecutionResult& OutResult)
	{
		FString ValidationError;
		if (!ValidateNode(LevelAsset, ObjectData, ValidationError))
		{
			OutResult = FGridLogicExecutionResult();
			OutResult.Error = MoveTemp(ValidationError);
			return false;
		}

		const FGridLogicObjectInstance TypedObject = GridLevelPlacementConversion::ToLogicObject(ObjectData);
		return ExecuteNode(LevelAsset, TypedObject, RuntimeState, Command, OutResult);
	}
}
