#include "Runtime/GridActivationComponent.h"

#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridButtonActor.h"
#include "Runtime/GridLeverActor.h"
#include "Runtime/GridPressurePlateActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridWallLockActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridGenericObjectActor.h"
#include "Runtime/GridLevelVariableStore.h"
#include "Runtime/GridLogicRuntime.h"
#include "Runtime/Monsters/GridAutomaticPerceptionEngagementSubsystem.h"
#include "Core/GridLevelAsset.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Quests/GridQuestDefinitionAsset.h"
#include "Quests/GridQuestSubsystem.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGStoryCompanionAsset.h"
#include "UI/RPGStoryCompanionRecruitmentWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridActivation, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogGridRecruitmentOffer, Log, All);

namespace
{
	FString GridObjectCommandToString(EGridObjectCommand Command)
	{
		if (const UEnum* Enum = StaticEnum<EGridObjectCommand>())
		{
			return Enum->GetNameStringByValue(static_cast<int64>(Command));
		}
		return FString::Printf(TEXT("%d"), static_cast<int32>(Command));
	}

	FString GridObjectTypeToString(EGridLevelObjectType Type)
	{
		if (const UEnum* Enum = StaticEnum<EGridLevelObjectType>())
		{
			return Enum->GetNameStringByValue(static_cast<int64>(Type));
		}
		return FString::Printf(TEXT("%d"), static_cast<int32>(Type));
	}

	FString GridObjectEventToString(EGridObjectEvent EventType)
	{
		if (const UEnum* Enum = StaticEnum<EGridObjectEvent>())
		{
			return Enum->GetNameStringByValue(static_cast<int64>(EventType));
		}
		return FString::Printf(TEXT("%d"), static_cast<int32>(EventType));
	}

	FString GridObjectConditionToString(EGridObjectCondition Condition)
	{
		if (const UEnum* Enum = StaticEnum<EGridObjectCondition>())
		{
			return Enum->GetNameStringByValue(static_cast<int64>(Condition));
		}
		return FString::Printf(TEXT("%d"), static_cast<int32>(Condition));
	}

	bool TryEvaluateGridIntComparison(int32 Left, EGridLogicIntComparison Comparison, int32 Right, bool& OutResult)
	{
		switch (Comparison)
		{
			case EGridLogicIntComparison::Equal:
				OutResult = Left == Right;
				return true;
			case EGridLogicIntComparison::NotEqual:
				OutResult = Left != Right;
				return true;
			case EGridLogicIntComparison::Less:
				OutResult = Left < Right;
				return true;
			case EGridLogicIntComparison::LessOrEqual:
				OutResult = Left <= Right;
				return true;
			case EGridLogicIntComparison::Greater:
				OutResult = Left > Right;
				return true;
			case EGridLogicIntComparison::GreaterOrEqual:
				OutResult = Left >= Right;
				return true;
			default:
				return false;
		}
	}

	bool IsReceptacleCommand(EGridObjectCommand Command)
	{
		switch (Command)
		{
			case EGridObjectCommand::ReceptacleConsumeItem:
			case EGridObjectCommand::ReceptacleConsumeAllItems:
			case EGridObjectCommand::ReceptacleEnableRemoval:
			case EGridObjectCommand::ReceptacleDisableRemoval:
				return true;

			default:
				return false;
		}
	}

	bool IsQuestCommand(EGridObjectCommand Command)
	{
		switch (Command)
		{
			case EGridObjectCommand::QuestStart:
			case EGridObjectCommand::QuestCompleteObjective:
			case EGridObjectCommand::QuestComplete:
			case EGridObjectCommand::QuestFail:
				return true;

			default:
				return false;
		}
	}

	bool IsReadableGenericObject(const FGridLevelObjectData& ObjectData)
	{
		return ObjectData.Type == EGridLevelObjectType::Decoration || ObjectData.Type == EGridLevelObjectType::Light;
	}
}

UGridActivationComponent::UGridActivationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGridActivationComponent::Initialize(AGridLevelRuntimeActor* InRuntime)
{
	RuntimeActor = InRuntime;

	FString LuaError;
	if (!ReloadLuaRuntime(&LuaError) && !LuaError.IsEmpty())
	{
		UE_LOG(LogGridActivation, Warning, TEXT("Grid Lua runtime initialization failed: %s"), *LuaError);
	}
}

void UGridActivationComponent::ResetRuntimeState()
{
	ActiveObjectIds.Reset();
	DeclinedStoryCompanionOfferKeys.Reset();
	DispatchingSourceObjectIds.Reset();
	RuntimeDispatchDepth = 0;
	RuntimeActionBudgetRemaining = 0;
	bExecutingLuaCallback = false;
	LuaVm.Reset();
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

void UGridActivationComponent::SetActiveObjectIds(const TSet<FGuid>& InActiveObjectIds)
{
	ActiveObjectIds = InActiveObjectIds;
}

FString UGridActivationComponent::BuildStoryCompanionOfferKey(FGuid SourceObjectId, FGuid CharacterId)
{
	if (!SourceObjectId.IsValid() || !CharacterId.IsValid())
	{
		return FString();
	}

	return FString::Printf(TEXT("%s:%s"), *SourceObjectId.ToString(EGuidFormats::Digits), *CharacterId.ToString(EGuidFormats::Digits));
}

bool UGridActivationComponent::IsStoryCompanionOfferDeclined(FGuid SourceObjectId, FGuid CharacterId) const
{
	const FString Key = BuildStoryCompanionOfferKey(SourceObjectId, CharacterId);
	return !Key.IsEmpty() && DeclinedStoryCompanionOfferKeys.Contains(Key);
}

void UGridActivationComponent::RememberStoryCompanionOfferDeclined(FGuid SourceObjectId, FGuid CharacterId)
{
	const FString Key = BuildStoryCompanionOfferKey(SourceObjectId, CharacterId);
	if (!Key.IsEmpty())
	{
		DeclinedStoryCompanionOfferKeys.Add(Key);
	}
}

bool UGridActivationComponent::IsStoryCompanionAlreadyActive(const FGridPartyInventoryState& PartyState, const URPGStoryCompanionAsset& CompanionDefinition)
{
	if (!CompanionDefinition.CharacterId.IsValid() || !CompanionDefinition.RaceDefinition || !CompanionDefinition.ClassDefinition)
	{
		return false;
	}

	return PartyState.ActiveCharacters.ContainsByPredicate(
		[&CompanionDefinition](const FGridCharacterInventoryState& Character)
		{
			return Character.CharacterId == CompanionDefinition.CharacterId && Character.RaceId == CompanionDefinition.RaceDefinition->RaceId &&
				Character.ClassId == CompanionDefinition.ClassDefinition->ClassId;
		});
}

bool UGridActivationComponent::TryInteractAtEdge(int32 FromCellX, int32 FromCellY, EGridEdge Edge, AGrimrockPartyPawn* PartyPawn)
{
	const FGridLevelObjectData* ObjectData = FindInteractableObjectOnEdge(FromCellX, FromCellY, Edge);
	return ObjectData ? ActivateObject(*ObjectData, PartyPawn) : false;
}

AGridReceptacleActor* UGridActivationComponent::FindReceptacleAtEdge(int32 FromCellX, int32 FromCellY, EGridEdge Edge) const
{
	const FGridLevelObjectData* ObjectData = FindInteractableObjectOnEdge(FromCellX, FromCellY, Edge);
	if (!RuntimeActor || !ObjectData || ObjectData->Type != EGridLevelObjectType::Receptacle)
	{
		return nullptr;
	}

	return RuntimeActor->FindRuntimeObjectActor<AGridReceptacleActor>(ObjectData->ObjectId);
}

void UGridActivationComponent::HandlePartyCellChanged(int32 OldCellX, int32 OldCellY, int32 NewCellX, int32 NewCellY)
{
	if (OldCellX == NewCellX && OldCellY == NewCellY)
	{
		RefreshPressurePlatesAtCell(NewCellX, NewCellY);
		NotifyPawnEnteredCell(NewCellX, NewCellY);
		GridAutomaticPerceptionEngagement::Request(RuntimeActor, TEXT("PartyCellStable"));
		return;
	}

	RefreshPressurePlatesAtCell(OldCellX, OldCellY);
	RefreshPressurePlatesAtCell(NewCellX, NewCellY);

	NotifyPawnExitedCell(OldCellX, OldCellY);
	NotifyPawnEnteredCell(NewCellX, NewCellY);
	GridAutomaticPerceptionEngagement::Request(RuntimeActor, TEXT("PartyTranslationCompleted"));
}

void UGridActivationComponent::NotifyPawnEnteredCell(int32 CellX, int32 CellY)
{
	ProcessTriggersAtCell(CellX, CellY, true);
}

void UGridActivationComponent::NotifyPawnExitedCell(int32 CellX, int32 CellY)
{
	ProcessTriggersAtCell(CellX, CellY, false);
}

const FGridLevelObjectData* UGridActivationComponent::FindObjectById(FGuid ObjectId) const
{
	const int32* ObjectIndex = ObjectIndexById.Find(ObjectId);
	return ObjectIndex ? GetObjectByIndex(*ObjectIndex) : nullptr;
}

const FGridLevelObjectData* UGridActivationComponent::FindInteractableObjectOnEdge(int32 X, int32 Y, EGridEdge Edge) const
{
	const int32* ObjectIndex = InteractableObjectIndexByEdge.Find(FGridEdgeKey(X, Y, Edge));
	return ObjectIndex ? GetObjectByIndex(*ObjectIndex) : nullptr;
}

bool UGridActivationComponent::ActivateObject(const FGridLevelObjectData& ObjectData, AGrimrockPartyPawn* PartyPawn)
{
	if (!RuntimeActor)
	{
		return false;
	}
	switch (ObjectData.Type)
	{
		case EGridLevelObjectType::Button:
		{
			if (AGridButtonActor* ButtonActor = RuntimeActor->FindRuntimeObjectActor<AGridButtonActor>(ObjectData.ObjectId))
			{
				ButtonActor->TriggerPress();
			}
			return ExecuteLinksFromObjectForEvent(ObjectData.ObjectId, EGridObjectEvent::Activated);
		}
		case EGridLevelObjectType::Lever:
		{
			const bool bWasActive = ActiveObjectIds.Contains(ObjectData.ObjectId);
			const bool bNewActive = !bWasActive;
			if (bNewActive)
			{
				ActiveObjectIds.Add(ObjectData.ObjectId);
			}
			else
			{
				ActiveObjectIds.Remove(ObjectData.ObjectId);
			}
			if (AGridLeverActor* LeverActor = RuntimeActor->FindRuntimeObjectActor<AGridLeverActor>(ObjectData.ObjectId))
			{
				LeverActor->SetLeverState(bNewActive);
			}

			const EGridObjectEvent LeverEvent = bNewActive ? EGridObjectEvent::Activated : EGridObjectEvent::Deactivated;
			return ExecuteLinksFromObjectForEvent(ObjectData.ObjectId, LeverEvent);
		}
		case EGridLevelObjectType::Receptacle:
		{
			if (AGridWallLockActor* WallLockActor = RuntimeActor->FindRuntimeObjectActor<AGridWallLockActor>(ObjectData.ObjectId))
			{
				return WallLockActor->TryInteractWithParty(PartyPawn);
			}
			return ActivateReceptacle(ObjectData, PartyPawn);
		}
		case EGridLevelObjectType::Decoration:
		case EGridLevelObjectType::Light:
		{
			return ActivateReadableObject(ObjectData);
		}
		default:
			return false;
	}
}

bool UGridActivationComponent::RefreshPressurePlatesAtCell(int32 X, int32 Y)
{
	if (!RuntimeActor)
	{
		return false;
	}

	TArray<int32> PlateIndexes;
	PressurePlateIndexesByCell.MultiFind(FIntPoint(X, Y), PlateIndexes);
	bool bAnyStateChanged = false;

	for (const int32 PlateIndex : PlateIndexes)
	{
		const FGridLevelObjectData* PlateData = GetObjectByIndex(PlateIndex);
		if (!PlateData || !PlateData->ObjectId.IsValid())
		{
			continue;
		}

		const FGridPressurePlateWeightParams& WeightParams = PlateData->Behavior.PressurePlateWeight;
		const float CurrentItemWeight = RuntimeActor->GetWorldItemWeightAtCell(X, Y, WeightParams.bCountEdgeItems);
		const bool bPartyActivates = WeightParams.bActivateWhenPartyPresent && RuntimeActor->IsPartyOnCell(X, Y);
		const bool bWeightActivates = WeightParams.bUseItemWeight && CurrentItemWeight >= FMath::Max(0.0f, WeightParams.RequiredItemWeight);
		const bool bShouldBePressed = bPartyActivates || bWeightActivates;
		const bool bWasPressed = ActiveObjectIds.Contains(PlateData->ObjectId);

		if (AGridPressurePlateActor* PlateActor = RuntimeActor->FindRuntimeObjectActor<AGridPressurePlateActor>(PlateData->ObjectId))
		{
			PlateActor->SetWeightState(CurrentItemWeight, WeightParams.RequiredItemWeight, WeightParams.bUseItemWeight, WeightParams.bActivateWhenPartyPresent);
		}

		if (bWasPressed == bShouldBePressed)
		{
			continue;
		}

		if (bShouldBePressed)
		{
			ActiveObjectIds.Add(PlateData->ObjectId);
		}
		else
		{
			ActiveObjectIds.Remove(PlateData->ObjectId);
		}

		if (AGridPressurePlateActor* PlateActor = RuntimeActor->FindRuntimeObjectActor<AGridPressurePlateActor>(PlateData->ObjectId))
		{
			PlateActor->SetPressed(bShouldBePressed);
		}

		const EGridObjectEvent StateEvent = bShouldBePressed ? EGridObjectEvent::Activated : EGridObjectEvent::Deactivated;
		UE_LOG(LogGridActivation, Log, TEXT("GridPressurePlate StateChanged Id=%s Cell=(%d,%d) Party=%s ItemWeight=%.2f RequiredWeight=%.2f Pressed=%s"),
			*PlateData->ObjectId.ToString(), X, Y, bPartyActivates ? TEXT("true") : TEXT("false"), CurrentItemWeight, WeightParams.RequiredItemWeight,
			bShouldBePressed ? TEXT("true") : TEXT("false"));
		ExecuteLinksFromObjectForEvent(PlateData->ObjectId, StateEvent);
		bAnyStateChanged = true;
	}

	return bAnyStateChanged;
}

bool UGridActivationComponent::RefreshAllPressurePlates()
{
	TArray<FIntPoint> PlateCells;
	PressurePlateIndexesByCell.GetKeys(PlateCells);

	TSet<FIntPoint> UniquePlateCells;
	UniquePlateCells.Append(PlateCells);

	bool bAnyStateChanged = false;
	for (const FIntPoint& PlateCell : UniquePlateCells)
	{
		bAnyStateChanged |= RefreshPressurePlatesAtCell(PlateCell.X, PlateCell.Y);
	}
	return bAnyStateChanged;
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

		const FName RequestedLogicId(*TargetReference);
		const FGridLevelObjectData* ResolvedObject = nullptr;
		int32 MatchCount = 0;
		for (const FGridLevelObjectData& Object : RuntimeActor->LevelAsset->Objects)
		{
			if (!Object.LogicId.IsNone() && Object.LogicId == RequestedLogicId)
			{
				ResolvedObject = &Object;
				++MatchCount;
			}
		}

		if (MatchCount == 0 || !ResolvedObject)
		{
			OutError = FString::Printf(TEXT("grid.command target '%s' is neither a valid ObjectId nor a declared LogicId."), *TargetReference);
			return false;
		}
		if (MatchCount > 1)
		{
			OutError = FString::Printf(TEXT("grid.command LogicId '%s' is ambiguous (%d objects)."), *TargetReference, MatchCount);
			return false;
		}
		TargetId = ResolvedObject->ObjectId;
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
	HostApi.GetBool = [this, RuntimeState](FName VariableId, bool& OutValue, FString& OutError)
	{
		return GridLevelVariableStore::TryGetBool(*RuntimeActor->LevelAsset, *RuntimeState, VariableId, OutValue, OutError);
	};
	HostApi.SetBool = [this, RuntimeState](FName VariableId, bool bValue, FString& OutError)
	{
		return GridLevelVariableStore::SetBool(*RuntimeActor->LevelAsset, *RuntimeState, VariableId, bValue, OutError);
	};
	HostApi.GetInt32 = [this, RuntimeState](FName VariableId, int32& OutValue, FString& OutError)
	{
		return GridLevelVariableStore::TryGetInt32(*RuntimeActor->LevelAsset, *RuntimeState, VariableId, OutValue, OutError);
	};
	HostApi.SetInt32 = [this, RuntimeState](FName VariableId, int32 Value, FString& OutError)
	{
		return GridLevelVariableStore::SetInt32(*RuntimeActor->LevelAsset, *RuntimeState, VariableId, Value, OutError);
	};
	HostApi.Command = [this, SourceObjectId = LinkData.SourceObjectId](const FString& TargetObjectId, const FString& CommandName, FString& OutError)
	{
		return ExecuteLuaIssuedCommand(SourceObjectId, TargetObjectId, CommandName, OutError);
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

void UGridActivationComponent::RegisterCurrentLevelQuestDefinitions()
{
	if (!RuntimeActor || !RuntimeActor->LevelAsset)
	{
		return;
	}

	UWorld* World = RuntimeActor->GetWorld();
	if (!World || !World->IsGameWorld() || RuntimeActor->LevelAsset->QuestDefinitions.IsEmpty())
	{
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	UGridQuestSubsystem* QuestSubsystem = GameInstance ? GameInstance->GetSubsystem<UGridQuestSubsystem>() : nullptr;
	if (!QuestSubsystem)
	{
		UE_LOG(LogGridActivation, Warning, TEXT("Grid quest definitions were not registered: missing UGridQuestSubsystem."));
		return;
	}

	for (UGridQuestDefinitionAsset* Definition : RuntimeActor->LevelAsset->QuestDefinitions)
	{
		FString Error;
		if (!QuestSubsystem->RegisterQuestDefinition(Definition, Error))
		{
			UE_LOG(LogGridActivation, Warning, TEXT("Grid quest definition registration failed: Definition=%s Reason=%s"), *GetNameSafe(Definition), *Error);
		}
	}
}

bool UGridActivationComponent::ApplyQuestLinkCommand(const FGridObjectLink& LinkData)
{
	if (!RuntimeActor || !IsQuestCommand(LinkData.Command) || LinkData.QuestId.IsNone())
	{
		return false;
	}

	UWorld* World = RuntimeActor->GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UGridQuestSubsystem* QuestSubsystem = GameInstance ? GameInstance->GetSubsystem<UGridQuestSubsystem>() : nullptr;
	if (!QuestSubsystem)
	{
		return false;
	}

	EGridQuestMutationResult Result = EGridQuestMutationResult::InvalidTransition;
	switch (LinkData.Command)
	{
		case EGridObjectCommand::QuestStart:
			Result = QuestSubsystem->StartQuest(LinkData.QuestId);
			break;

		case EGridObjectCommand::QuestCompleteObjective:
			Result = QuestSubsystem->CompleteObjective(LinkData.QuestId, LinkData.QuestObjectiveId);
			break;

		case EGridObjectCommand::QuestComplete:
			Result = QuestSubsystem->CompleteQuest(LinkData.QuestId);
			break;

		case EGridObjectCommand::QuestFail:
			Result = QuestSubsystem->FailQuest(LinkData.QuestId);
			break;

		default:
			return false;
	}

	const bool bSuccess = Result == EGridQuestMutationResult::Success || Result == EGridQuestMutationResult::AlreadyInState;
	const FString ResultText = UEnum::GetValueAsString(Result);
	if (bSuccess)
	{
		UE_LOG(LogGridActivation, Log, TEXT("Grid quest command result: Quest=%s Objective=%s Command=%s Result=%s"), *LinkData.QuestId.ToString(),
			*LinkData.QuestObjectiveId.ToString(), *GridObjectCommandToString(LinkData.Command), *ResultText);
	}
	else
	{
		UE_LOG(LogGridActivation, Warning, TEXT("Grid quest command result: Quest=%s Objective=%s Command=%s Result=%s"), *LinkData.QuestId.ToString(),
			*LinkData.QuestObjectiveId.ToString(), *GridObjectCommandToString(LinkData.Command), *ResultText);
	}
	return bSuccess;
}
bool UGridActivationComponent::ApplyLinkCommand(const FGridObjectLink& LinkData)
{
	if (!RuntimeActor)
	{
		LogLinkResult(LinkData, LinkData.Command, false, TEXT("missing runtime actor"));
		return false;
	}
	if (!ConsumeRuntimeActionBudget(LinkData.Command == EGridObjectCommand::LuaCallback ? TEXT("LuaCallback") : TEXT("ObjectCommand")))
	{
		LogLinkResult(LinkData, LinkData.Command, false, TEXT("shared runtime action budget exhausted"));
		return false;
	}

	AActor* SourceActor = RuntimeActor->FindRuntimeObjectActor<AActor>(LinkData.SourceObjectId);

	if (LinkData.Command == EGridObjectCommand::LuaCallback)
	{
		if (!EvaluateGridObjectLinkCondition(LinkData, SourceActor, nullptr))
		{
			return false;
		}
		return ExecuteLuaCallbackLink(LinkData);
	}

	if (IsQuestCommand(LinkData.Command))
	{
		if (!EvaluateGridObjectLinkCondition(LinkData, SourceActor, nullptr))
		{
			return false;
		}

		const bool bSuccess = ApplyQuestLinkCommand(LinkData);
		LogLinkResult(LinkData, LinkData.Command, bSuccess, bSuccess ? nullptr : TEXT("quest command failed"));
		return bSuccess;
	}

	const FGridLevelObjectData* TargetObject = FindObjectById(LinkData.TargetObjectId);
	if (!TargetObject)
	{
		LogLinkResult(LinkData, LinkData.Command, false, TEXT("target object not found"));
		return false;
	}

	AActor* TargetActor = RuntimeActor->FindRuntimeObjectActor<AActor>(LinkData.TargetObjectId);
	if (!EvaluateGridObjectLinkCondition(LinkData, SourceActor, TargetActor))
	{
		return false;
	}

	const EGridObjectCommand ResolvedCommand = LinkData.Command;
	bool bSuccess = false;
	const TCHAR* FailureReason = TEXT("unsupported target type or command");

	if (TargetObject->Type == EGridLevelObjectType::StoryCompanion)
	{
		if (ResolvedCommand != EGridObjectCommand::OfferRecruitment)
		{
			LogLinkResult(LinkData, ResolvedCommand, false, TEXT("story companion only supports OfferRecruitment"));
			return false;
		}

		URPGStoryCompanionAsset* CompanionDefinition = TargetObject->StoryCompanionDefinition.Get();
		if (!IsValid(CompanionDefinition) || !CompanionDefinition->IsValidDefinition())
		{
			LogLinkResult(LinkData, ResolvedCommand, false, TEXT("missing or invalid story companion definition"));
			return false;
		}

		AGrimrockPartyPawn* PartyPawn =
			RuntimeActor->GetWorld() ? Cast<AGrimrockPartyPawn>(UGameplayStatics::GetPlayerPawn(RuntimeActor->GetWorld(), 0)) : nullptr;
		if (!IsValid(PartyPawn))
		{
			LogLinkResult(LinkData, ResolvedCommand, false, TEXT("missing player party pawn"));
			return false;
		}

		if (IsValid(PartyPawn->PartyInventoryComponent) &&
			IsStoryCompanionAlreadyActive(PartyPawn->PartyInventoryComponent->PartyInventoryState, *CompanionDefinition))
		{
			UE_LOG(LogGridRecruitmentOffer, Log, TEXT("[GridRecruitmentOffer] Skipped Source=%s Companion=%s Reason=AlreadyActive"),
				*LinkData.SourceObjectId.ToString(EGuidFormats::Digits), *CompanionDefinition->CompanionId.ToString());
			LogLinkResult(LinkData, ResolvedCommand, true, nullptr);
			return true;
		}

		if (IsStoryCompanionOfferDeclined(LinkData.SourceObjectId, CompanionDefinition->CharacterId))
		{
			UE_LOG(LogGridRecruitmentOffer, Log, TEXT("[GridRecruitmentOffer] Skipped Source=%s Companion=%s Reason=DeclinedFromSource"),
				*LinkData.SourceObjectId.ToString(EGuidFormats::Digits), *CompanionDefinition->CompanionId.ToString());
			LogLinkResult(LinkData, ResolvedCommand, true, nullptr);
			return true;
		}

		bSuccess = PartyPawn->ShowStoryCompanionRecruitmentWidget(CompanionDefinition);
		if (bSuccess)
		{
			if (URPGStoryCompanionRecruitmentWidget* Widget = PartyPawn->GetStoryCompanionRecruitmentWidget())
			{
				const FGuid OfferSourceObjectId = LinkData.SourceObjectId;
				const FGuid CharacterId = CompanionDefinition->CharacterId;
				const FName CompanionId = CompanionDefinition->CompanionId;
				const TWeakObjectPtr<UGridActivationComponent> WeakThis(this);

				Widget->OnDeclined().AddLambda(
					[WeakThis, OfferSourceObjectId, CharacterId, CompanionId](URPGStoryCompanionRecruitmentWidget* /*DeclinedWidget*/)
					{
						UGridActivationComponent* Activation = WeakThis.Get();
						if (!Activation)
						{
							return;
						}

						Activation->RememberStoryCompanionOfferDeclined(OfferSourceObjectId, CharacterId);
						UE_LOG(LogGridRecruitmentOffer, Log, TEXT("[GridRecruitmentOffer] Suppressed Source=%s Companion=%s Reason=Declined"),
							*OfferSourceObjectId.ToString(EGuidFormats::Digits), *CompanionId.ToString());
					});
			}
		}

		LogLinkResult(LinkData, ResolvedCommand, bSuccess, bSuccess ? nullptr : TEXT("recruitment modal rejected"));
		return bSuccess;
	}

	if (TargetObject->Type == EGridLevelObjectType::CustomRecruiter)
	{
		if (ResolvedCommand != EGridObjectCommand::OpenCustomRecruit)
		{
			LogLinkResult(LinkData, ResolvedCommand, false, TEXT("custom recruiter only supports OpenCustomRecruit"));
			return false;
		}

		AGrimrockPartyPawn* PartyPawn =
			RuntimeActor->GetWorld() ? Cast<AGrimrockPartyPawn>(UGameplayStatics::GetPlayerPawn(RuntimeActor->GetWorld(), 0)) : nullptr;
		if (!IsValid(PartyPawn))
		{
			LogLinkResult(LinkData, ResolvedCommand, false, TEXT("missing player party pawn"));
			return false;
		}

		bSuccess = PartyPawn->ShowCustomRecruitCharacterCreationWidget();
		LogLinkResult(LinkData, ResolvedCommand, bSuccess, bSuccess ? nullptr : TEXT("custom recruit modal rejected"));
		return bSuccess;
	}

	if (TargetObject->Type == EGridLevelObjectType::Logic)
	{
		if (DispatchingSourceObjectIds.Contains(TargetObject->ObjectId))
		{
			LogLinkResult(LinkData, ResolvedCommand, false, TEXT("cyclic logic target dispatch"));
			return false;
		}

		if (!RuntimeActor->LevelAsset)
		{
			LogLinkResult(LinkData, ResolvedCommand, false, TEXT("missing level asset for logic command"));
			return false;
		}

		FGridLevelRuntimeState* RuntimeState = RuntimeActor->GetOrCreateRuntimeStateForCurrentLevel();
		if (!RuntimeState)
		{
			LogLinkResult(LinkData, ResolvedCommand, false, TEXT("missing current-level runtime state"));
			return false;
		}

		FGridLogicExecutionResult LogicResult;
		bSuccess = GridLogicRuntime::ExecuteNode(*RuntimeActor->LevelAsset, *TargetObject, *RuntimeState, ResolvedCommand, LogicResult);

		if (bSuccess && LogicResult.bEmitEvent)
		{
			ExecuteLinksFromObjectForEvent(TargetObject->ObjectId, LogicResult.EmittedEvent);
		}

		const FString LogicFailureReason = LogicResult.Error.IsEmpty() ? TEXT("logic command failed") : LogicResult.Error;
		LogLinkResult(LinkData, ResolvedCommand, bSuccess, bSuccess ? nullptr : *LogicFailureReason);
		return bSuccess;
	}

	if (TargetObject->Type == EGridLevelObjectType::MonsterSpawn)
	{
		switch (ResolvedCommand)
		{
			case EGridObjectCommand::StartEncounter:
				bSuccess = RuntimeActor->StartMonsterEncounter(TargetObject->ObjectId);
				break;

			case EGridObjectCommand::Spawn:
			case EGridObjectCommand::Despawn:
			case EGridObjectCommand::Teleport:
			case EGridObjectCommand::Activate:
			case EGridObjectCommand::Deactivate:
			case EGridObjectCommand::Enable:
			case EGridObjectCommand::Disable:
			case EGridObjectCommand::Toggle:
				bSuccess = RuntimeActor->ExecuteMonsterSpawnCommand(TargetObject->ObjectId, ResolvedCommand);
				break;

			default:
				break;
		}

		if (bSuccess)
		{
			if (RuntimeActor->FindSpawnedMonsterActor(TargetObject->ObjectId))
			{
				ActiveObjectIds.Add(TargetObject->ObjectId);
			}
			else
			{
				ActiveObjectIds.Remove(TargetObject->ObjectId);
			}

			GridAutomaticPerceptionEngagement::Request(
				RuntimeActor, ResolvedCommand == EGridObjectCommand::StartEncounter ? TEXT("EncounterStarted") : TEXT("MonsterLifecycleCommand"));
		}
		LogLinkResult(LinkData, ResolvedCommand, bSuccess, bSuccess ? nullptr : TEXT("monster lifecycle or encounter command failed"));
		return bSuccess;
	}

	if (IsReceptacleCommand(ResolvedCommand))
	{
		bSuccess = ApplyReceptacleLinkCommand(*TargetObject, ResolvedCommand);
		FailureReason = bSuccess ? nullptr : TEXT("receptacle command failed");
		LogLinkResult(LinkData, ResolvedCommand, bSuccess, FailureReason);
		return bSuccess;
	}

	switch (TargetObject->Type)
	{
		case EGridLevelObjectType::Door:
		{
			bSuccess = ApplyDoorLinkCommand(*TargetObject, ResolvedCommand);
			FailureReason = bSuccess ? nullptr : TEXT("door command failed");
			break;
		}

		case EGridLevelObjectType::PressurePlate:
		case EGridLevelObjectType::Lever:
		{
			bSuccess = ApplyStatefulLinkCommand(*TargetObject, ResolvedCommand);
			FailureReason = bSuccess ? nullptr : TEXT("stateful gameplay command failed");
			break;
		}

		case EGridLevelObjectType::Pit:
		{
			bSuccess = ApplyPitLinkCommand(*TargetObject, ResolvedCommand);
			FailureReason = bSuccess ? nullptr : TEXT("pit command failed");
			break;
		}

		case EGridLevelObjectType::Button:
		case EGridLevelObjectType::Decoration:
		case EGridLevelObjectType::ItemSpawn:
		case EGridLevelObjectType::Item:
		case EGridLevelObjectType::Light:
		case EGridLevelObjectType::Teleporter:
		case EGridLevelObjectType::Trigger:
		case EGridLevelObjectType::Receptacle:
		{
			FailureReason = TEXT("target type has no gameplay command handler");
			break;
		}

		case EGridLevelObjectType::MonsterSpawn:
		case EGridLevelObjectType::CustomRecruiter:
		case EGridLevelObjectType::StoryCompanion:
		case EGridLevelObjectType::Logic:
		case EGridLevelObjectType::None:
		default:
			break;
	}

	LogLinkResult(LinkData, ResolvedCommand, bSuccess, FailureReason);
	return bSuccess;
}

bool UGridActivationComponent::EvaluateGridObjectLinkCondition(const FGridObjectLink& LinkData, AActor* SourceActor, AActor* TargetActor) const
{
	if (LinkData.Condition == EGridObjectCondition::None)
	{
		return true;
	}

	bool bConditionResult = false;

	if (LinkData.Condition == EGridObjectCondition::LevelVariableBoolEquals || LinkData.Condition == EGridObjectCondition::LevelVariableIntCompare)
	{
		if (!RuntimeActor || !RuntimeActor->LevelAsset || LinkData.ConditionVariableId.IsNone())
		{
			UE_LOG(LogGridActivation, Warning,
				TEXT("Grid link variable condition rejected: Source=%s Target=%s Condition=%s Variable=%s Reason=missing runtime, level asset or variable id"),
				*LinkData.SourceObjectId.ToString(), *LinkData.TargetObjectId.ToString(), *GridObjectConditionToString(LinkData.Condition),
				*LinkData.ConditionVariableId.ToString());
			return false;
		}

		FGridLevelRuntimeState* RuntimeState = RuntimeActor->GetOrCreateRuntimeStateForCurrentLevel();
		if (!RuntimeState)
		{
			UE_LOG(LogGridActivation, Warning,
				TEXT("Grid link variable condition rejected: Source=%s Target=%s Condition=%s Variable=%s Reason=missing current-level runtime state"),
				*LinkData.SourceObjectId.ToString(), *LinkData.TargetObjectId.ToString(), *GridObjectConditionToString(LinkData.Condition),
				*LinkData.ConditionVariableId.ToString());
			return false;
		}

		FString VariableError;
		if (LinkData.Condition == EGridObjectCondition::LevelVariableBoolEquals)
		{
			bool bCurrentValue = false;
			if (!GridLevelVariableStore::TryGetBool(*RuntimeActor->LevelAsset, *RuntimeState, LinkData.ConditionVariableId, bCurrentValue, VariableError))
			{
				UE_LOG(LogGridActivation, Warning, TEXT("Grid link variable condition rejected: Source=%s Target=%s Condition=%s Variable=%s Reason=%s"),
					*LinkData.SourceObjectId.ToString(), *LinkData.TargetObjectId.ToString(), *GridObjectConditionToString(LinkData.Condition),
					*LinkData.ConditionVariableId.ToString(), *VariableError);
				return false;
			}
			bConditionResult = bCurrentValue == LinkData.ConditionBoolValue;
		}
		else
		{
			int32 CurrentValue = 0;
			if (!GridLevelVariableStore::TryGetInt32(*RuntimeActor->LevelAsset, *RuntimeState, LinkData.ConditionVariableId, CurrentValue, VariableError))
			{
				UE_LOG(LogGridActivation, Warning, TEXT("Grid link variable condition rejected: Source=%s Target=%s Condition=%s Variable=%s Reason=%s"),
					*LinkData.SourceObjectId.ToString(), *LinkData.TargetObjectId.ToString(), *GridObjectConditionToString(LinkData.Condition),
					*LinkData.ConditionVariableId.ToString(), *VariableError);
				return false;
			}

			if (!TryEvaluateGridIntComparison(CurrentValue, LinkData.ConditionIntComparison, LinkData.ConditionIntValue, bConditionResult))
			{
				UE_LOG(LogGridActivation, Warning,
					TEXT("Grid link variable condition rejected: Source=%s Target=%s Condition=%s Variable=%s Reason=invalid Int comparison"),
					*LinkData.SourceObjectId.ToString(), *LinkData.TargetObjectId.ToString(), *GridObjectConditionToString(LinkData.Condition),
					*LinkData.ConditionVariableId.ToString());
				return false;
			}
		}

		const bool bFinalResult = LinkData.bInvertCondition ? !bConditionResult : bConditionResult;
		if (!bFinalResult)
		{
			UE_LOG(LogGridActivation, Verbose, TEXT("Grid link variable condition failed: Source=%s Target=%s Condition=%s Variable=%s Inverted=%s"),
				*LinkData.SourceObjectId.ToString(), *LinkData.TargetObjectId.ToString(), *GridObjectConditionToString(LinkData.Condition),
				*LinkData.ConditionVariableId.ToString(), LinkData.bInvertCondition ? TEXT("true") : TEXT("false"));
		}
		return bFinalResult;
	}

	const AGridReceptacleActor* ReceptacleActor = Cast<AGridReceptacleActor>(TargetActor);
	if (!ReceptacleActor)
	{
		UE_LOG(LogGridActivation, Warning,
			TEXT("Grid link condition rejected: Source=%s SourceActor=%s Target=%s TargetActor=%s Condition=%s Reason=target is not a spawned receptacle"),
			*LinkData.SourceObjectId.ToString(), *GetNameSafe(SourceActor), *LinkData.TargetObjectId.ToString(), *GetNameSafe(TargetActor),
			*GridObjectConditionToString(LinkData.Condition));
		return false;
	}

	const auto RejectMissingConditionParameter = [&LinkData, SourceActor, TargetActor](const TCHAR* ParameterName)
	{
		UE_LOG(LogGridActivation, Warning,
			TEXT("Grid link condition rejected: Source=%s SourceActor=%s Target=%s TargetActor=%s Condition=%s Reason=missing or invalid %s"),
			*LinkData.SourceObjectId.ToString(), *GetNameSafe(SourceActor), *LinkData.TargetObjectId.ToString(), *GetNameSafe(TargetActor),
			*GridObjectConditionToString(LinkData.Condition), ParameterName);
		return false;
	};

	switch (LinkData.Condition)
	{
		case EGridObjectCondition::ReceptacleIsEmpty:
			bConditionResult = ReceptacleActor->IsEmpty();
			break;

		case EGridObjectCondition::ReceptacleHasAnyItem:
			bConditionResult = ReceptacleActor->HasAnyItem();
			break;

		case EGridObjectCondition::ReceptacleContainsItemDefinition:
			if (LinkData.ConditionItemDefinitionId.IsNone())
			{
				return RejectMissingConditionParameter(TEXT("ConditionItemDefinitionId"));
			}
			bConditionResult = ReceptacleActor->ContainsItemDefinition(LinkData.ConditionItemDefinitionId);
			break;

		case EGridObjectCondition::ReceptacleContainsItemTag:
			if (LinkData.ConditionItemTag.IsNone())
			{
				return RejectMissingConditionParameter(TEXT("ConditionItemTag"));
			}
			bConditionResult = ReceptacleActor->ContainsItemTag(LinkData.ConditionItemTag);
			break;

		case EGridObjectCondition::ReceptacleContainsItemType:
			if (LinkData.ConditionItemType == EGridItemType::None)
			{
				return RejectMissingConditionParameter(TEXT("ConditionItemType"));
			}
			bConditionResult = ReceptacleActor->ContainsItemType(LinkData.ConditionItemType);
			break;

		case EGridObjectCondition::ReceptacleItemCountAtLeast:
			if (LinkData.ConditionCount <= 0)
			{
				return RejectMissingConditionParameter(TEXT("ConditionCount"));
			}
			bConditionResult = ReceptacleActor->GetContainedItemCount() >= LinkData.ConditionCount;
			break;

		case EGridObjectCondition::ReceptacleWeightAtLeast:
			if (LinkData.ConditionWeight <= 0.0f)
			{
				return RejectMissingConditionParameter(TEXT("ConditionWeight"));
			}
			bConditionResult = ReceptacleActor->GetContainedTotalWeight() >= LinkData.ConditionWeight;
			break;

		case EGridObjectCondition::LevelVariableBoolEquals:
		case EGridObjectCondition::LevelVariableIntCompare:
		case EGridObjectCondition::None:
		default:
			return false;
	}

	const bool bFinalResult = LinkData.bInvertCondition ? !bConditionResult : bConditionResult;
	if (!bFinalResult)
	{
		UE_LOG(LogGridActivation, Verbose, TEXT("Grid link condition failed: Source=%s SourceActor=%s Target=%s TargetActor=%s Condition=%s Inverted=%s"),
			*LinkData.SourceObjectId.ToString(), *GetNameSafe(SourceActor), *LinkData.TargetObjectId.ToString(), *GetNameSafe(TargetActor),
			*GridObjectConditionToString(LinkData.Condition), LinkData.bInvertCondition ? TEXT("true") : TEXT("false"));
	}
	return bFinalResult;
}

bool UGridActivationComponent::ExecuteLinksFromObjectForEvent(FGuid SourceObjectId, EGridObjectEvent SourceEvent)
{
	const bool bRootDispatch = RuntimeDispatchDepth == 0;
	if (bRootDispatch)
	{
		RuntimeActionBudgetRemaining = MaxRuntimeActionBudget;
	}

	++RuntimeDispatchDepth;
	const bool bResult = ExecuteLinksFromObjectForEventInternal(SourceObjectId, SourceEvent);
	--RuntimeDispatchDepth;

	if (bRootDispatch)
	{
		RuntimeActionBudgetRemaining = 0;
	}
	return bResult;
}

bool UGridActivationComponent::ExecuteLinksFromObjectForEventInternal(FGuid SourceObjectId, EGridObjectEvent SourceEvent)
{
	if (!RuntimeActor || !RuntimeActor->LevelAsset || !SourceObjectId.IsValid())
	{
		return false;
	}
	if (!FindObjectById(SourceObjectId))
	{
		UE_LOG(LogGridActivation, Warning, TEXT("Grid object event rejected: Source=%s Event=%s Reason=source object not found"), *SourceObjectId.ToString(),
			*GridObjectEventToString(SourceEvent));
		return false;
	}
	if (DispatchingSourceObjectIds.Contains(SourceObjectId))
	{
		UE_LOG(LogGridActivation, Warning, TEXT("Grid object event rejected: Source=%s Event=%s Reason=cyclic link dispatch"), *SourceObjectId.ToString(),
			*GridObjectEventToString(SourceEvent));
		return false;
	}

	DispatchingSourceObjectIds.Add(SourceObjectId);
	bool bAnyApplied = false;
	int32 ExecutedLinkCount = 0;

	TArray<int32> LinkIndexes;
	LinkIndexesBySource.MultiFind(SourceObjectId, LinkIndexes);

	for (const int32 LinkIndex : LinkIndexes)
	{
		if (!RuntimeActor->LevelAsset->Links.IsValidIndex(LinkIndex))
		{
			continue;
		}

		const FGridObjectLink& LinkData = RuntimeActor->LevelAsset->Links[LinkIndex];
		if (LinkData.SourceEvent != SourceEvent)
		{
			continue;
		}

		++ExecutedLinkCount;
		bAnyApplied |= ApplyLinkCommand(LinkData);
	}

	UE_LOG(LogGridActivation, Log, TEXT("Grid object event %s: Event=%s LinksExecuted=%d AnyApplied=%s"), *SourceObjectId.ToString(),
		*GridObjectEventToString(SourceEvent), ExecutedLinkCount, bAnyApplied ? TEXT("true") : TEXT("false"));

	DispatchingSourceObjectIds.Remove(SourceObjectId);
	return bAnyApplied;
}

bool UGridActivationComponent::ApplyDoorLinkCommand(const FGridLevelObjectData& TargetObject, EGridObjectCommand Command)
{
	if (!RuntimeActor)
	{
		return false;
	}

	switch (Command)
	{
		case EGridObjectCommand::Toggle:
			return RuntimeActor->ToggleDoorOnEdge(TargetObject.CellX, TargetObject.CellY, TargetObject.Edge);

		case EGridObjectCommand::Open:
		case EGridObjectCommand::Activate:
			return RuntimeActor->OpenDoorOnEdge(TargetObject.CellX, TargetObject.CellY, TargetObject.Edge);

		case EGridObjectCommand::Close:
		case EGridObjectCommand::Deactivate:
			return RuntimeActor->CloseDoorOnEdge(TargetObject.CellX, TargetObject.CellY, TargetObject.Edge);

		default:
			return false;
	}
}

bool UGridActivationComponent::ApplyPitLinkCommand(const FGridLevelObjectData& TargetObject, EGridObjectCommand Command)
{
	if (!RuntimeActor || TargetObject.Type != EGridLevelObjectType::Pit)
	{
		return false;
	}

	switch (Command)
	{
		case EGridObjectCommand::Toggle:
			return RuntimeActor->TogglePit(TargetObject.ObjectId);

		case EGridObjectCommand::Open:
		case EGridObjectCommand::Activate:
			return RuntimeActor->SetPitOpen(TargetObject.ObjectId, true);

		case EGridObjectCommand::Close:
		case EGridObjectCommand::Deactivate:
			return RuntimeActor->SetPitOpen(TargetObject.ObjectId, false);

		default:
			return false;
	}
}

bool UGridActivationComponent::ApplyReceptacleLinkCommand(const FGridLevelObjectData& TargetObject, EGridObjectCommand Command)
{
	UE_LOG(LogGridActivation, Log, TEXT("Grid receptacle command received: Command=%s Target=%s TargetType=%s"), *GridObjectCommandToString(Command),
		*TargetObject.ObjectId.ToString(), *GridObjectTypeToString(TargetObject.Type));

	AGridReceptacleActor* ReceptacleActor = RuntimeActor ? RuntimeActor->FindRuntimeObjectActor<AGridReceptacleActor>(TargetObject.ObjectId) : nullptr;
	if (!ReceptacleActor)
	{
		UE_LOG(LogGridActivation, Warning, TEXT("Grid receptacle command failed: Command=%s Target=%s Reason=target is not a receptacle"),
			*GridObjectCommandToString(Command), *TargetObject.ObjectId.ToString());
		return false;
	}

	bool bSuccess = true;
	switch (Command)
	{
		case EGridObjectCommand::ReceptacleConsumeItem:
			if (!ReceptacleActor->HasItem())
			{
				UE_LOG(LogGridActivation, Warning, TEXT("Grid receptacle command failed: Command=%s Target=%s Actor=%s Reason=no item present"),
					*GridObjectCommandToString(Command), *TargetObject.ObjectId.ToString(), *GetNameSafe(ReceptacleActor));
				return false;
			}
			bSuccess = ReceptacleActor->ConsumeItemAtIndex(0);
			break;

		case EGridObjectCommand::ReceptacleConsumeAllItems:
			if (!ReceptacleActor->HasItem())
			{
				UE_LOG(LogGridActivation, Warning, TEXT("Grid receptacle command failed: Command=%s Target=%s Actor=%s Reason=no item present"),
					*GridObjectCommandToString(Command), *TargetObject.ObjectId.ToString(), *GetNameSafe(ReceptacleActor));
				return false;
			}
			bSuccess = ReceptacleActor->ConsumeAllItems();
			break;

		case EGridObjectCommand::ReceptacleEnableRemoval:
			ReceptacleActor->SetCanRemoveItem(true);
			break;

		case EGridObjectCommand::ReceptacleDisableRemoval:
			ReceptacleActor->SetCanRemoveItem(false);
			break;

		default:
			bSuccess = false;
			break;
	}

	if (bSuccess)
	{
		UE_LOG(LogGridActivation, Log, TEXT("Grid receptacle command result: Command=%s Target=%s Actor=%s Success=true"), *GridObjectCommandToString(Command),
			*TargetObject.ObjectId.ToString(), *GetNameSafe(ReceptacleActor));
	}
	else
	{
		UE_LOG(LogGridActivation, Warning, TEXT("Grid receptacle command result: Command=%s Target=%s Actor=%s Success=false"),
			*GridObjectCommandToString(Command), *TargetObject.ObjectId.ToString(), *GetNameSafe(ReceptacleActor));
	}
	return bSuccess;
}

bool UGridActivationComponent::ApplyStatefulLinkCommand(const FGridLevelObjectData& TargetObject, EGridObjectCommand Command)
{
	switch (Command)
	{
		case EGridObjectCommand::Open:
		case EGridObjectCommand::Activate:
			return SetTargetActiveState(TargetObject, true);

		case EGridObjectCommand::Close:
		case EGridObjectCommand::Deactivate:
			return SetTargetActiveState(TargetObject, false);

		case EGridObjectCommand::Toggle:
			return SetTargetActiveState(TargetObject, !IsTargetActive(TargetObject.ObjectId));

		default:
			return false;
	}
}

bool UGridActivationComponent::SetTargetActiveState(const FGridLevelObjectData& TargetObject, bool bActive)
{
	if (!TargetObject.ObjectId.IsValid() || !RuntimeActor)
	{
		return false;
	}

	AGridLeverActor* LeverActor = nullptr;
	AGridPressurePlateActor* PlateActor = nullptr;

	switch (TargetObject.Type)
	{
		case EGridLevelObjectType::Lever:
			LeverActor = RuntimeActor->FindRuntimeObjectActor<AGridLeverActor>(TargetObject.ObjectId);
			if (!LeverActor)
			{
				return false;
			}
			break;

		case EGridLevelObjectType::PressurePlate:
			PlateActor = RuntimeActor->FindRuntimeObjectActor<AGridPressurePlateActor>(TargetObject.ObjectId);
			if (!PlateActor)
			{
				return false;
			}
			break;

		default:
			return false;
	}

	const bool bWasActive = IsTargetActive(TargetObject.ObjectId);
	const bool bStateChanged = bWasActive != bActive;

	if (bActive)
	{
		ActiveObjectIds.Add(TargetObject.ObjectId);
	}
	else
	{
		ActiveObjectIds.Remove(TargetObject.ObjectId);
	}

	if (LeverActor)
	{
		LeverActor->SetLeverState(bActive);
	}
	else if (PlateActor)
	{
		PlateActor->SetPressed(bActive);
	}

	if (bStateChanged)
	{
		const EGridObjectEvent StateEvent = bActive ? EGridObjectEvent::Activated : EGridObjectEvent::Deactivated;
		UE_LOG(LogGridActivation, Log, TEXT("Grid mechanism state changed by link command: Target=%s Type=%s PreviousActive=%s NewActive=%s Event=%s"),
			*TargetObject.ObjectId.ToString(), *GridObjectTypeToString(TargetObject.Type), bWasActive ? TEXT("true") : TEXT("false"),
			bActive ? TEXT("true") : TEXT("false"), *GridObjectEventToString(StateEvent));
		ExecuteLinksFromObjectForEvent(TargetObject.ObjectId, StateEvent);
	}

	return true;
}

bool UGridActivationComponent::IsTargetActive(FGuid ObjectId) const
{
	return ObjectId.IsValid() && ActiveObjectIds.Contains(ObjectId);
}

void UGridActivationComponent::LogLinkResult(
	const FGridObjectLink& LinkData, EGridObjectCommand ResolvedCommand, bool bSuccess, const TCHAR* FailureReason) const
{
	if (bSuccess)
	{
		UE_LOG(LogGridActivation, Log, TEXT("Grid link executed: Source=%s Target=%s Command=%s Success=true"), *LinkData.SourceObjectId.ToString(),
			*LinkData.TargetObjectId.ToString(), *GridObjectCommandToString(ResolvedCommand));
		return;
	}

	UE_LOG(LogGridActivation, Warning, TEXT("Grid link failed: Source=%s Target=%s Command=%s Reason=%s"), *LinkData.SourceObjectId.ToString(),
		*LinkData.TargetObjectId.ToString(), *GridObjectCommandToString(ResolvedCommand), FailureReason ? FailureReason : TEXT("unknown"));
}

bool UGridActivationComponent::ProcessTriggersAtCell(int32 X, int32 Y, bool bEntering)
{
	TArray<int32> TriggerIndexes;
	TriggerIndexesByCell.MultiFind(FIntPoint(X, Y), TriggerIndexes);
	bool bAnyTriggered = false;

	for (const int32 TriggerIndex : TriggerIndexes)
	{
		const FGridLevelObjectData* TriggerData = GetObjectByIndex(TriggerIndex);
		if (!TriggerData)
		{
			continue;
		}

		bAnyTriggered |= ProcessTriggerEvent(*TriggerData, bEntering);
	}

	return bAnyTriggered;
}

bool UGridActivationComponent::ProcessTriggerEvent(const FGridLevelObjectData& TriggerData, bool bEntering)
{
	if (TriggerData.Type != EGridLevelObjectType::Trigger || !TriggerData.ObjectId.IsValid())
	{
		return false;
	}

	const TCHAR* EventLabel = bEntering ? TEXT("Enter") : TEXT("Exit");
	const EGridObjectEvent SourceEvent = bEntering ? EGridObjectEvent::Activated : EGridObjectEvent::Deactivated;

	UE_LOG(LogGridActivation, Log, TEXT("Grid trigger detected: Id=%s Cell=(%d,%d) Event=%s SourceEvent=%s"), *TriggerData.ObjectId.ToString(),
		TriggerData.CellX, TriggerData.CellY, EventLabel, *GridObjectEventToString(SourceEvent));

	return ExecuteLinksFromObjectForEvent(TriggerData.ObjectId, SourceEvent);
}

void UGridActivationComponent::RegisterInitialObjectState(const FGridLevelObjectData& ObjectData)
{
	if (ObjectData.bInitiallyActive)
	{
		ActiveObjectIds.Add(ObjectData.ObjectId);
	}
}

void UGridActivationComponent::RebuildIndexes()
{
	ObjectIndexById.Reset();
	LinkIndexesBySource.Reset();
	InteractableObjectIndexByEdge.Reset();
	PressurePlateIndexesByCell.Reset();
	TriggerIndexesByCell.Reset();

	if (!RuntimeActor || !RuntimeActor->LevelAsset)
	{
		return;
	}

	RegisterCurrentLevelQuestDefinitions();

	const TArray<FGridLevelObjectData>& Objects = RuntimeActor->LevelAsset->Objects;

	for (int32 Index = 0; Index < Objects.Num(); ++Index)
	{
		const FGridLevelObjectData& ObjectData = Objects[Index];

		if (ObjectData.ObjectId.IsValid())
		{
			ObjectIndexById.Add(ObjectData.ObjectId, Index);
		}
		if (ObjectData.Type == EGridLevelObjectType::Button || ObjectData.Type == EGridLevelObjectType::Lever ||
			ObjectData.Type == EGridLevelObjectType::Receptacle || IsReadableGenericObject(ObjectData))
		{
			if (ObjectData.Edge != EGridEdge::None)
			{
				InteractableObjectIndexByEdge.Add(FGridEdgeKey(ObjectData.CellX, ObjectData.CellY, ObjectData.Edge), Index);
			}
		}
		else if (ObjectData.Type == EGridLevelObjectType::PressurePlate)
		{
			PressurePlateIndexesByCell.Add(FIntPoint(ObjectData.CellX, ObjectData.CellY), Index);
		}
		else if (ObjectData.Type == EGridLevelObjectType::Trigger)
		{
			TriggerIndexesByCell.Add(FIntPoint(ObjectData.CellX, ObjectData.CellY), Index);
		}
	}

	const TArray<FGridObjectLink>& Links = RuntimeActor->LevelAsset->Links;

	for (int32 Index = 0; Index < Links.Num(); ++Index)
	{
		if (Links[Index].SourceObjectId.IsValid())
		{
			LinkIndexesBySource.Add(Links[Index].SourceObjectId, Index);
		}
	}
}

const FGridLevelObjectData* UGridActivationComponent::GetObjectByIndex(int32 ObjectIndex) const
{
	if (!RuntimeActor || !RuntimeActor->LevelAsset)
	{
		return nullptr;
	}

	return RuntimeActor->LevelAsset->Objects.IsValidIndex(ObjectIndex) ? &RuntimeActor->LevelAsset->Objects[ObjectIndex] : nullptr;
}

bool UGridActivationComponent::ActivateReadableObject(const FGridLevelObjectData& ObjectData)
{
	if (!RuntimeActor)
	{
		return false;
	}
	AGridGenericObjectActor* GenericActor = RuntimeActor->FindRuntimeObjectActor<AGridGenericObjectActor>(ObjectData.ObjectId);
	if (!GenericActor || !GenericActor->HasReadableText())
	{
		return false;
	}
	if (GenericActor->bRuntimeReadableOnlyOnce && GenericActor->bRuntimeHasBeenRead)
	{
		return true;
	}
	if (GEngine)
	{
		const FText ReadableText = GenericActor->GetReadableText();
		UE_LOG(LogGridActivation, Log, TEXT("Readable object %s: %s"), *ObjectData.ObjectId.ToString(), *ReadableText.ToString());
		RuntimeActor->ShowReadableMessage(ReadableText);
		GenericActor->MarkAsRead();
		return true;
	}
	GenericActor->MarkAsRead();
	return true;
}

bool UGridActivationComponent::ActivateReceptacle(const FGridLevelObjectData& ObjectData, AGrimrockPartyPawn* PartyPawn)
{
	if (!RuntimeActor || !PartyPawn)
	{
		return false;
	}
	AGridReceptacleActor* ReceptacleActor = RuntimeActor->FindRuntimeObjectActor<AGridReceptacleActor>(ObjectData.ObjectId);
	if (!ReceptacleActor)
	{
		return false;
	}
	const int32 PreviousItemCount = ReceptacleActor->GetContainedItemCount();
	if (!ReceptacleActor->TryInteractWithParty(PartyPawn))
	{
		return false;
	}
	const int32 CurrentItemCount = ReceptacleActor->GetContainedItemCount();
	if (CurrentItemCount > 0)
	{
		ActiveObjectIds.Add(ObjectData.ObjectId);
	}
	else
	{
		ActiveObjectIds.Remove(ObjectData.ObjectId);
	}

	// The receptacle actor emits insertion and removal events when the transfer succeeds.
	return CurrentItemCount != PreviousItemCount;
}

FString UGridActivationComponent::GetDebugSummary() const
{
	return FString::Printf(TEXT("Activation | Objects=%d Links=%d Interactables=%d Plates=%d Triggers=%d Active=%d LuaScripts=%d"), ObjectIndexById.Num(),
		LinkIndexesBySource.Num(), InteractableObjectIndexByEdge.Num(), PressurePlateIndexesByCell.Num(), TriggerIndexesByCell.Num(), ActiveObjectIds.Num(),
		LuaVm.GetLoadedScriptCount());
}

void UGridActivationComponent::LogDebugSummary() const
{
	UE_LOG(LogGridActivation, Log, TEXT("%s"), *GetDebugSummary());
}
