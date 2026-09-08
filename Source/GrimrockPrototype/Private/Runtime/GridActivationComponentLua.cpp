#include "Runtime/GridActivationComponent.h"

#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLevelVariableStore.h"
#include "Core/GridLevelAsset.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridActivation, Log, All);

namespace
{
	FString GridObjectEventToString(EGridObjectEvent EventType)
	{
		if (const UEnum* Enum = StaticEnum<EGridObjectEvent>())
		{
			return Enum->GetNameStringByValue(static_cast<int64>(EventType));
		}
		return FString::Printf(TEXT("%d"), static_cast<int32>(EventType));
	}
}

bool UGridActivationComponent::ReloadLuaRuntime(FString* OutError)
{
	FString Error;
	if (!RuntimeActor || !RuntimeActor->LevelAsset)
	{
		LuaVm.Reset();
		Error = TEXT("Cannot load Lua runtime without a current LevelAsset.");
		if (OutError)
		{
			*OutError = Error;
		}
		return false;
	}

	const bool bSuccess = LuaVm.Reload(RuntimeActor->LevelAsset->LuaScripts, FGridLuaVmConfig(), Error);
	if (OutError)
	{
		*OutError = Error;
	}
	return bSuccess;
}

bool UGridActivationComponent::ConsumeRuntimeActionBudget(const TCHAR* ActionLabel)
{
	if (RuntimeDispatchDepth <= 0)
	{
		return true;
	}
	if (RuntimeActionBudgetRemaining <= 0)
	{
		UE_LOG(LogGridActivation, Warning, TEXT("Grid runtime action rejected: Action=%s Reason=shared Event/Command/Lua budget exhausted"),
			ActionLabel ? ActionLabel : TEXT("Unknown"));
		return false;
	}

	--RuntimeActionBudgetRemaining;
	return true;
}

bool UGridActivationComponent::ExecuteLuaIssuedCommand(FGuid SourceObjectId, const FString& TargetObjectId, const FString& CommandName, FString& OutError)
{
	const FString TargetReference = TargetObjectId.TrimStartAndEnd();
	if (TargetReference.IsEmpty())
	{
		OutError = TEXT("grid.command target reference cannot be empty.");
		return false;
	}

	FGuid TargetId;
	if (!FGuid::Parse(TargetReference, TargetId) || !TargetId.IsValid())
	{
		if (!RuntimeActor || !RuntimeActor->LevelAsset)
		{
			OutError = TEXT("grid.command cannot resolve LogicId without a current LevelAsset.");
			return false;
		}

		TArray<FGuid> MatchingIds;
		const int32 MatchCount = RuntimeActor->LevelAsset->FindTypedPlacementIdsByLogicId(FName(*TargetReference), MatchingIds);
		if (MatchCount == 0)
		{
			OutError = FString::Printf(TEXT("grid.command target '%s' is neither a valid ObjectId nor a declared LogicId."), *TargetReference);
			return false;
		}
		if (MatchCount > 1)
		{
			OutError = FString::Printf(TEXT("grid.command LogicId '%s' is ambiguous (%d objects)."), *TargetReference, MatchCount);
			return false;
		}

		TargetId = MatchingIds[0];
		if (!TargetId.IsValid())
		{
			OutError = FString::Printf(TEXT("grid.command LogicId '%s' resolves to an object without a valid ObjectId."), *TargetReference);
			return false;
		}
	}

	const UEnum* CommandEnum = StaticEnum<EGridObjectCommand>();
	const int64 CommandValue = CommandEnum ? CommandEnum->GetValueByNameString(CommandName) : INDEX_NONE;
	if (CommandValue == INDEX_NONE)
	{
		OutError = FString::Printf(TEXT("grid.command command '%s' is unknown."), *CommandName);
		return false;
	}

	const EGridObjectCommand Command = static_cast<EGridObjectCommand>(CommandValue);
	if (Command == EGridObjectCommand::LuaCallback)
	{
		OutError = TEXT("grid.command cannot invoke LuaCallback directly.");
		return false;
	}

	FGridObjectLink SyntheticLink;
	SyntheticLink.SourceObjectId = SourceObjectId;
	SyntheticLink.TargetObjectId = TargetId;
	SyntheticLink.SourceEvent = EGridObjectEvent::Activated;
	SyntheticLink.Command = Command;
	SyntheticLink.Condition = EGridObjectCondition::None;

	if (!ApplyLinkCommand(SyntheticLink))
	{
		OutError = FString::Printf(TEXT("grid.command failed: Target=%s Command=%s"), *TargetReference, *CommandName);
		return false;
	}

	OutError.Reset();
	return true;
}

bool UGridActivationComponent::ExecuteLuaCallbackLink(const FGridObjectLink& LinkData)
{
	if (!RuntimeActor || !RuntimeActor->LevelAsset)
	{
		return false;
	}
	if (LinkData.LuaScriptId.IsNone() || LinkData.LuaCallbackName.IsNone())
	{
		UE_LOG(LogGridActivation, Warning, TEXT("Grid Lua callback rejected: Source=%s Script=%s Callback=%s Reason=missing ScriptId or CallbackName"),
			*LinkData.SourceObjectId.ToString(), *LinkData.LuaScriptId.ToString(), *LinkData.LuaCallbackName.ToString());
		return false;
	}
	if (bExecutingLuaCallback)
	{
		UE_LOG(LogGridActivation, Warning, TEXT("Grid Lua callback rejected: Source=%s Script=%s Callback=%s Reason=nested Lua callback dispatch"),
			*LinkData.SourceObjectId.ToString(), *LinkData.LuaScriptId.ToString(), *LinkData.LuaCallbackName.ToString());
		return false;
	}

	if (!LuaVm.IsReady())
	{
		FString ReloadError;
		if (!ReloadLuaRuntime(&ReloadError))
		{
			UE_LOG(LogGridActivation, Warning, TEXT("Grid Lua callback rejected: Script=%s Callback=%s Reason=%s"), *LinkData.LuaScriptId.ToString(),
				*LinkData.LuaCallbackName.ToString(), *ReloadError);
			return false;
		}
	}

	FGridLevelRuntimeState* RuntimeState = RuntimeActor->GetOrCreateRuntimeStateForCurrentLevel();
	if (!RuntimeState)
	{
		UE_LOG(LogGridActivation, Warning, TEXT("Grid Lua callback rejected: Script=%s Callback=%s Reason=missing current-level runtime state"),
			*LinkData.LuaScriptId.ToString(), *LinkData.LuaCallbackName.ToString());
		return false;
	}

	FGridLuaHostApi HostApi;
	HostApi.GetBool = [this, RuntimeState](FName VariableId, bool& OutValue, FString& OutHostError)
	{
		return GridLevelVariableStore::TryGetBool(*RuntimeActor->LevelAsset, *RuntimeState, VariableId, OutValue, OutHostError);
	};
	HostApi.SetBool = [this, RuntimeState](FName VariableId, bool bValue, FString& OutHostError)
	{
		return GridLevelVariableStore::SetBool(*RuntimeActor->LevelAsset, *RuntimeState, VariableId, bValue, OutHostError);
	};
	HostApi.GetInt32 = [this, RuntimeState](FName VariableId, int32& OutValue, FString& OutHostError)
	{
		return GridLevelVariableStore::TryGetInt32(*RuntimeActor->LevelAsset, *RuntimeState, VariableId, OutValue, OutHostError);
	};
	HostApi.SetInt32 = [this, RuntimeState](FName VariableId, int32 Value, FString& OutHostError)
	{
		return GridLevelVariableStore::SetInt32(*RuntimeActor->LevelAsset, *RuntimeState, VariableId, Value, OutHostError);
	};
	HostApi.Command = [this, SourceObjectId = LinkData.SourceObjectId](const FString& TargetId, const FString& CommandName, FString& OutHostError)
	{
		return ExecuteLuaIssuedCommand(SourceObjectId, TargetId, CommandName, OutHostError);
	};
	HostApi.Log = [ScriptId = LinkData.LuaScriptId](const FString& Message)
	{
		UE_LOG(LogGridActivation, Log, TEXT("[GridLua:%s] %s"), *ScriptId.ToString(), *Message);
	};

	FGridLuaEventContext EventContext;
	EventContext.SourceObjectId = LinkData.SourceObjectId.ToString();
	EventContext.EventName = GridObjectEventToString(LinkData.SourceEvent);

	bExecutingLuaCallback = true;
	FString LuaError;
	const bool bSuccess = LuaVm.CallEventFunction(LinkData.LuaScriptId, LinkData.LuaCallbackName, EventContext, HostApi, LuaError);
	bExecutingLuaCallback = false;

	if (!bSuccess)
	{
		UE_LOG(LogGridActivation, Warning, TEXT("Grid Lua callback failed: Source=%s Event=%s Script=%s Callback=%s Reason=%s"),
			*LinkData.SourceObjectId.ToString(), *GridObjectEventToString(LinkData.SourceEvent), *LinkData.LuaScriptId.ToString(),
			*LinkData.LuaCallbackName.ToString(), *LuaError);
		return false;
	}

	UE_LOG(LogGridActivation, Log, TEXT("Grid Lua callback executed: Source=%s Event=%s Script=%s Callback=%s"), *LinkData.SourceObjectId.ToString(),
		*GridObjectEventToString(LinkData.SourceEvent), *LinkData.LuaScriptId.ToString(), *LinkData.LuaCallbackName.ToString());
	return true;
}