#include "Runtime/GridLogicRuntime.h"

#include "Core/GridLevelAsset.h"
#include "Core/GridLevelVariableTypes.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridLevelVariableStore.h"

namespace
{
	const FGridLevelVariableDefinition* FindTypedLogicVariable(const UGridLevelAsset& LevelAsset, FName VariableId)
	{
		return LevelAsset.LevelVariables.FindByPredicate(
			[VariableId](const FGridLevelVariableDefinition& Definition)
			{
				return Definition.VariableId == VariableId;
			});
	}

	bool IsSupportedTypedNodeType(EGridLogicNodeType NodeType)
	{
		switch (NodeType)
		{
			case EGridLogicNodeType::Relay:
			case EGridLogicNodeType::SetBool:
			case EGridLogicNodeType::ToggleBool:
			case EGridLogicNodeType::SetInt:
			case EGridLogicNodeType::AddInt:
			case EGridLogicNodeType::SubtractInt:
			case EGridLogicNodeType::ResetVariable:
			case EGridLogicNodeType::CompareBool:
			case EGridLogicNodeType::CompareInt:
			case EGridLogicNodeType::Latch:
				return true;
			default:
				return false;
		}
	}

	bool IsSupportedTypedIntComparison(EGridLogicIntComparison Comparison)
	{
		switch (Comparison)
		{
			case EGridLogicIntComparison::Equal:
			case EGridLogicIntComparison::NotEqual:
			case EGridLogicIntComparison::Less:
			case EGridLogicIntComparison::LessOrEqual:
			case EGridLogicIntComparison::Greater:
			case EGridLogicIntComparison::GreaterOrEqual:
				return true;
			default:
				return false;
		}
	}

	bool TypedLogicRequiresBool(EGridLogicNodeType NodeType)
	{
		return NodeType == EGridLogicNodeType::SetBool || NodeType == EGridLogicNodeType::ToggleBool || NodeType == EGridLogicNodeType::CompareBool ||
			NodeType == EGridLogicNodeType::Latch;
	}

	bool TypedLogicRequiresInt32(EGridLogicNodeType NodeType)
	{
		return NodeType == EGridLogicNodeType::SetInt || NodeType == EGridLogicNodeType::AddInt || NodeType == EGridLogicNodeType::SubtractInt ||
			NodeType == EGridLogicNodeType::CompareInt;
	}

	bool CompareTypedInt32(int32 Left, EGridLogicIntComparison Comparison, int32 Right)
	{
		switch (Comparison)
		{
			case EGridLogicIntComparison::Equal: return Left == Right;
			case EGridLogicIntComparison::NotEqual: return Left != Right;
			case EGridLogicIntComparison::Less: return Left < Right;
			case EGridLogicIntComparison::LessOrEqual: return Left <= Right;
			case EGridLogicIntComparison::Greater: return Left > Right;
			case EGridLogicIntComparison::GreaterOrEqual: return Left >= Right;
			default: return false;
		}
	}

	bool TryTypedCheckedInt32(int64 Candidate, int32& OutValue)
	{
		if (Candidate < static_cast<int64>(MIN_int32) || Candidate > static_cast<int64>(MAX_int32))
		{
			return false;
		}
		OutValue = static_cast<int32>(Candidate);
		return true;
	}

	void EmitTypedLogic(FGridLogicExecutionResult& Result, EGridObjectEvent Event)
	{
		Result.bEmitEvent = true;
		Result.EmittedEvent = Event;
	}
}

namespace GridLogicRuntime
{
	bool ValidateNode(const UGridLevelAsset& LevelAsset, const FGridLogicObjectInstance& ObjectData, FString& OutError)
	{
		OutError.Reset();

		if (ObjectData.Type != EGridLevelObjectType::Logic)
		{
			OutError = TEXT("Object is not a Logic node.");
			return false;
		}
		if (!ObjectData.InstanceId.IsValid())
		{
			OutError = TEXT("Logic node requires a valid ObjectId.");
			return false;
		}
		if (!IsSupportedTypedNodeType(ObjectData.Logic.NodeType))
		{
			OutError = TEXT("Logic node type is unsupported.");
			return false;
		}
		if (ObjectData.Logic.NodeType == EGridLogicNodeType::CompareInt && !IsSupportedTypedIntComparison(ObjectData.Logic.IntComparison))
		{
			OutError = TEXT("Logic CompareInt uses an unsupported comparison.");
			return false;
		}
		if (!GridLevelVariableStore::ValidateDefinitions(LevelAsset, OutError))
		{
			return false;
		}

		const EGridLogicNodeType NodeType = ObjectData.Logic.NodeType;
		if (NodeType == EGridLogicNodeType::Relay)
		{
			return true;
		}
		if (ObjectData.Logic.VariableId.IsNone())
		{
			OutError = TEXT("Logic node requires VariableId.");
			return false;
		}

		const FGridLevelVariableDefinition* Definition = FindTypedLogicVariable(LevelAsset, ObjectData.Logic.VariableId);
		if (!Definition)
		{
			OutError = FString::Printf(TEXT("Logic node references undeclared variable '%s'."), *ObjectData.Logic.VariableId.ToString());
			return false;
		}
		if (TypedLogicRequiresBool(NodeType) && Definition->Type != EGridLevelVariableType::Bool)
		{
			OutError = FString::Printf(TEXT("Logic node requires Bool variable '%s'."), *ObjectData.Logic.VariableId.ToString());
			return false;
		}
		if (TypedLogicRequiresInt32(NodeType) && Definition->Type != EGridLevelVariableType::Int32)
		{
			OutError = FString::Printf(TEXT("Logic node requires Int32 variable '%s'."), *ObjectData.Logic.VariableId.ToString());
			return false;
		}
		return true;
	}

	bool ExecuteNode(const UGridLevelAsset& LevelAsset, const FGridLogicObjectInstance& ObjectData, FGridLevelRuntimeState& RuntimeState,
		EGridObjectCommand Command, FGridLogicExecutionResult& OutResult)
	{
		OutResult = FGridLogicExecutionResult();

		FString ValidationError;
		if (!ValidateNode(LevelAsset, ObjectData, ValidationError))
		{
			OutResult.Error = MoveTemp(ValidationError);
			return false;
		}
		if (Command != EGridObjectCommand::LogicExecute && Command != EGridObjectCommand::LogicReset)
		{
			OutResult.Error = TEXT("Logic node received an unsupported command.");
			return false;
		}

		const FGridLogicNodeParams& Logic = ObjectData.Logic;
		if (Command == EGridObjectCommand::LogicReset)
		{
			if (Logic.NodeType != EGridLogicNodeType::Latch)
			{
				OutResult.Error = TEXT("Logic Reset is only supported by Latch nodes.");
				return false;
			}

			bool bCurrent = false;
			FString Error;
			if (!GridLevelVariableStore::TryGetBool(LevelAsset, RuntimeState, Logic.VariableId, bCurrent, Error))
			{
				OutResult.Error = MoveTemp(Error);
				return false;
			}
			if (!bCurrent)
			{
				return true;
			}
			if (!GridLevelVariableStore::SetBool(LevelAsset, RuntimeState, Logic.VariableId, false, Error))
			{
				OutResult.Error = MoveTemp(Error);
				return false;
			}
			OutResult.bStateChanged = true;
			EmitTypedLogic(OutResult, EGridObjectEvent::Deactivated);
			return true;
		}

		FString Error;
		switch (Logic.NodeType)
		{
			case EGridLogicNodeType::Relay:
				EmitTypedLogic(OutResult, EGridObjectEvent::Activated);
				return true;

			case EGridLogicNodeType::SetBool:
			{
				bool bCurrent = false;
				if (!GridLevelVariableStore::TryGetBool(LevelAsset, RuntimeState, Logic.VariableId, bCurrent, Error) ||
					!GridLevelVariableStore::SetBool(LevelAsset, RuntimeState, Logic.VariableId, Logic.bBoolValue, Error))
				{
					OutResult.Error = MoveTemp(Error);
					return false;
				}
				OutResult.bStateChanged = bCurrent != Logic.bBoolValue;
				EmitTypedLogic(OutResult, EGridObjectEvent::Activated);
				return true;
			}

			case EGridLogicNodeType::ToggleBool:
			{
				bool bCurrent = false;
				if (!GridLevelVariableStore::TryGetBool(LevelAsset, RuntimeState, Logic.VariableId, bCurrent, Error) ||
					!GridLevelVariableStore::SetBool(LevelAsset, RuntimeState, Logic.VariableId, !bCurrent, Error))
				{
					OutResult.Error = MoveTemp(Error);
					return false;
				}
				OutResult.bStateChanged = true;
				EmitTypedLogic(OutResult, EGridObjectEvent::Activated);
				return true;
			}

			case EGridLogicNodeType::SetInt:
			{
				int32 Current = 0;
				if (!GridLevelVariableStore::TryGetInt32(LevelAsset, RuntimeState, Logic.VariableId, Current, Error) ||
					!GridLevelVariableStore::SetInt32(LevelAsset, RuntimeState, Logic.VariableId, Logic.IntValue, Error))
				{
					OutResult.Error = MoveTemp(Error);
					return false;
				}
				OutResult.bStateChanged = Current != Logic.IntValue;
				EmitTypedLogic(OutResult, EGridObjectEvent::Activated);
				return true;
			}

			case EGridLogicNodeType::AddInt:
			case EGridLogicNodeType::SubtractInt:
			{
				int32 Current = 0;
				if (!GridLevelVariableStore::TryGetInt32(LevelAsset, RuntimeState, Logic.VariableId, Current, Error))
				{
					OutResult.Error = MoveTemp(Error);
					return false;
				}
				const int64 Candidate = Logic.NodeType == EGridLogicNodeType::AddInt
					? static_cast<int64>(Current) + static_cast<int64>(Logic.IntValue)
					: static_cast<int64>(Current) - static_cast<int64>(Logic.IntValue);
				int32 NewValue = 0;
				if (!TryTypedCheckedInt32(Candidate, NewValue))
				{
					OutResult.Error = FString::Printf(TEXT("Int32 overflow for variable '%s'."), *Logic.VariableId.ToString());
					return false;
				}
				if (!GridLevelVariableStore::SetInt32(LevelAsset, RuntimeState, Logic.VariableId, NewValue, Error))
				{
					OutResult.Error = MoveTemp(Error);
					return false;
				}
				OutResult.bStateChanged = Current != NewValue;
				EmitTypedLogic(OutResult, EGridObjectEvent::Activated);
				return true;
			}

			case EGridLogicNodeType::ResetVariable:
			{
				const FGridLevelVariableDefinition* Definition = FindTypedLogicVariable(LevelAsset, Logic.VariableId);
				if (!Definition)
				{
					OutResult.Error = FString::Printf(TEXT("Variable '%s' is not declared."), *Logic.VariableId.ToString());
					return false;
				}
				if (Definition->Type == EGridLevelVariableType::Bool)
				{
					bool bCurrent = false;
					if (!GridLevelVariableStore::TryGetBool(LevelAsset, RuntimeState, Logic.VariableId, bCurrent, Error) ||
						!GridLevelVariableStore::SetBool(LevelAsset, RuntimeState, Logic.VariableId, Definition->bDefaultBoolValue, Error))
					{
						OutResult.Error = MoveTemp(Error);
						return false;
					}
					OutResult.bStateChanged = bCurrent != Definition->bDefaultBoolValue;
				}
				else if (Definition->Type == EGridLevelVariableType::Int32)
				{
					int32 Current = 0;
					if (!GridLevelVariableStore::TryGetInt32(LevelAsset, RuntimeState, Logic.VariableId, Current, Error) ||
						!GridLevelVariableStore::SetInt32(LevelAsset, RuntimeState, Logic.VariableId, Definition->DefaultInt32Value, Error))
					{
						OutResult.Error = MoveTemp(Error);
						return false;
					}
					OutResult.bStateChanged = Current != Definition->DefaultInt32Value;
				}
				else
				{
					OutResult.Error = TEXT("ResetVariable uses an unsupported variable type.");
					return false;
				}
				EmitTypedLogic(OutResult, EGridObjectEvent::Activated);
				return true;
			}

			case EGridLogicNodeType::CompareBool:
			{
				bool bCurrent = false;
				if (!GridLevelVariableStore::TryGetBool(LevelAsset, RuntimeState, Logic.VariableId, bCurrent, Error))
				{
					OutResult.Error = MoveTemp(Error);
					return false;
				}
				EmitTypedLogic(OutResult, bCurrent == Logic.bBoolValue ? EGridObjectEvent::Activated : EGridObjectEvent::Deactivated);
				return true;
			}

			case EGridLogicNodeType::CompareInt:
			{
				int32 Current = 0;
				if (!GridLevelVariableStore::TryGetInt32(LevelAsset, RuntimeState, Logic.VariableId, Current, Error))
				{
					OutResult.Error = MoveTemp(Error);
					return false;
				}
				EmitTypedLogic(OutResult, CompareTypedInt32(Current, Logic.IntComparison, Logic.IntValue) ? EGridObjectEvent::Activated : EGridObjectEvent::Deactivated);
				return true;
			}

			case EGridLogicNodeType::Latch:
			{
				bool bLatched = false;
				if (!GridLevelVariableStore::TryGetBool(LevelAsset, RuntimeState, Logic.VariableId, bLatched, Error))
				{
					OutResult.Error = MoveTemp(Error);
					return false;
				}
				if (bLatched)
				{
					return true;
				}
				if (!GridLevelVariableStore::SetBool(LevelAsset, RuntimeState, Logic.VariableId, true, Error))
				{
					OutResult.Error = MoveTemp(Error);
					return false;
				}
				OutResult.bStateChanged = true;
				EmitTypedLogic(OutResult, EGridObjectEvent::Activated);
				return true;
			}

			default:
				OutResult.Error = TEXT("Unsupported logic node type.");
				return false;
		}
	}
}
