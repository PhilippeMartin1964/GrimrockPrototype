#include "Runtime/GridActivationComponent.h"

#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridLeverActor.h"
#include "Runtime/GridPressurePlateActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridPartyInventoryComponent.h"
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
		case EGridObjectCommand::QuestStart: Result = QuestSubsystem->StartQuest(LinkData.QuestId); break;
		case EGridObjectCommand::QuestCompleteObjective: Result = QuestSubsystem->CompleteObjective(LinkData.QuestId, LinkData.QuestObjectiveId); break;
		case EGridObjectCommand::QuestComplete: Result = QuestSubsystem->CompleteQuest(LinkData.QuestId); break;
		case EGridObjectCommand::QuestFail: Result = QuestSubsystem->FailQuest(LinkData.QuestId); break;
		default: return false;
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
	if (!ContainsIndexedObject(SourceObjectId))
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

bool UGridActivationComponent::ApplyLinkCommand(const FGridObjectLink& LinkData)
{
	if (!RuntimeActor || !RuntimeActor->LevelAsset)
	{
		LogLinkResult(LinkData, LinkData.Command, false, TEXT("missing runtime actor or level asset"));
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
		return EvaluateGridObjectLinkCondition(LinkData, SourceActor, nullptr) && ExecuteLuaCallbackLink(LinkData);
	}
	if (IsQuestCommand(LinkData.Command))
	{
		if (!EvaluateGridObjectLinkCondition(LinkData, SourceActor, nullptr))
		{
			return false;
		}
		const bool bQuestSuccess = ApplyQuestLinkCommand(LinkData);
		LogLinkResult(LinkData, LinkData.Command, bQuestSuccess, bQuestSuccess ? nullptr : TEXT("quest command failed"));
		return bQuestSuccess;
	}

	const EGridLevelObjectType TargetType = RuntimeActor->LevelAsset->GetTypedPlacementType(LinkData.TargetObjectId);
	if (TargetType == EGridLevelObjectType::None)
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

	if (TargetType == EGridLevelObjectType::StoryCompanion)
	{
		if (ResolvedCommand != EGridObjectCommand::OfferRecruitment)
		{
			LogLinkResult(LinkData, ResolvedCommand, false, TEXT("story companion only supports OfferRecruitment"));
			return false;
		}

		const FGridLogicObjectInstance* TargetObject = RuntimeActor->LevelAsset->FindLogicObjectInstanceById(LinkData.TargetObjectId);
		URPGStoryCompanionAsset* CompanionDefinition = TargetObject ? TargetObject->StoryCompanionDefinition.Get() : nullptr;
		if (!IsValid(CompanionDefinition) || !CompanionDefinition->IsValidDefinition())
		{
			LogLinkResult(LinkData, ResolvedCommand, false, TEXT("missing or invalid story companion definition"));
			return false;
		}

		AGrimrockPartyPawn* PartyPawn = RuntimeActor->GetWorld() ? Cast<AGrimrockPartyPawn>(UGameplayStatics::GetPlayerPawn(RuntimeActor->GetWorld(), 0)) : nullptr;
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
					[WeakThis, OfferSourceObjectId, CharacterId, CompanionId](URPGStoryCompanionRecruitmentWidget*)
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

	if (TargetType == EGridLevelObjectType::CustomRecruiter)
	{
		if (ResolvedCommand != EGridObjectCommand::OpenCustomRecruit)
		{
			LogLinkResult(LinkData, ResolvedCommand, false, TEXT("custom recruiter only supports OpenCustomRecruit"));
			return false;
		}
		AGrimrockPartyPawn* PartyPawn = RuntimeActor->GetWorld() ? Cast<AGrimrockPartyPawn>(UGameplayStatics::GetPlayerPawn(RuntimeActor->GetWorld(), 0)) : nullptr;
		if (!IsValid(PartyPawn))
		{
			LogLinkResult(LinkData, ResolvedCommand, false, TEXT("missing player party pawn"));
			return false;
		}
		bSuccess = PartyPawn->ShowCustomRecruitCharacterCreationWidget();
		LogLinkResult(LinkData, ResolvedCommand, bSuccess, bSuccess ? nullptr : TEXT("custom recruit modal rejected"));
		return bSuccess;
	}

	if (TargetType == EGridLevelObjectType::Logic)
	{
		if (DispatchingSourceObjectIds.Contains(LinkData.TargetObjectId))
		{
			LogLinkResult(LinkData, ResolvedCommand, false, TEXT("cyclic logic target dispatch"));
			return false;
		}
		const FGridLogicObjectInstance* LogicObject = RuntimeActor->LevelAsset->FindLogicObjectInstanceById(LinkData.TargetObjectId);
		FGridLevelRuntimeState* RuntimeState = RuntimeActor->GetOrCreateRuntimeStateForCurrentLevel();
		if (!LogicObject || !RuntimeState)
		{
			LogLinkResult(LinkData, ResolvedCommand, false, TEXT("missing logic placement or current-level runtime state"));
			return false;
		}

		FGridLogicExecutionResult LogicResult;
		bSuccess = GridLogicRuntime::ExecuteNode(*RuntimeActor->LevelAsset, *LogicObject, *RuntimeState, ResolvedCommand, LogicResult);
		if (bSuccess && LogicResult.bEmitEvent)
		{
			ExecuteLinksFromObjectForEvent(LinkData.TargetObjectId, LogicResult.EmittedEvent);
		}
		const FString LogicFailureReason = LogicResult.Error.IsEmpty() ? TEXT("logic command failed") : LogicResult.Error;
		LogLinkResult(LinkData, ResolvedCommand, bSuccess, bSuccess ? nullptr : *LogicFailureReason);
		return bSuccess;
	}

	if (TargetType == EGridLevelObjectType::MonsterSpawn)
	{
		switch (ResolvedCommand)
		{
			case EGridObjectCommand::StartEncounter:
				bSuccess = RuntimeActor->StartMonsterEncounter(LinkData.TargetObjectId);
				break;
			case EGridObjectCommand::Spawn:
			case EGridObjectCommand::Despawn:
			case EGridObjectCommand::Teleport:
			case EGridObjectCommand::Activate:
			case EGridObjectCommand::Deactivate:
			case EGridObjectCommand::Enable:
			case EGridObjectCommand::Disable:
			case EGridObjectCommand::Toggle:
				bSuccess = RuntimeActor->ExecuteMonsterSpawnCommand(LinkData.TargetObjectId, ResolvedCommand);
				break;
			default:
				break;
		}
		if (bSuccess)
		{
			if (RuntimeActor->FindSpawnedMonsterActor(LinkData.TargetObjectId))
			{
				ActiveObjectIds.Add(LinkData.TargetObjectId);
			}
			else
			{
				ActiveObjectIds.Remove(LinkData.TargetObjectId);
			}
			GridAutomaticPerceptionEngagement::Request(RuntimeActor,
				ResolvedCommand == EGridObjectCommand::StartEncounter ? TEXT("EncounterStarted") : TEXT("MonsterLifecycleCommand"));
		}
		LogLinkResult(LinkData, ResolvedCommand, bSuccess, bSuccess ? nullptr : TEXT("monster lifecycle or encounter command failed"));
		return bSuccess;
	}

	const FGridWorldObjectInstance* WorldObject = RuntimeActor->LevelAsset->FindWorldObjectInstanceById(LinkData.TargetObjectId);
	if (IsReceptacleCommand(ResolvedCommand))
	{
		bSuccess = WorldObject && ApplyReceptacleLinkCommand(*WorldObject, ResolvedCommand);
		FailureReason = bSuccess ? nullptr : TEXT("receptacle command failed");
		LogLinkResult(LinkData, ResolvedCommand, bSuccess, FailureReason);
		return bSuccess;
	}

	switch (TargetType)
	{
		case EGridLevelObjectType::Door:
			bSuccess = WorldObject && ApplyDoorLinkCommand(*WorldObject, ResolvedCommand);
			FailureReason = bSuccess ? nullptr : TEXT("door command failed");
			break;
		case EGridLevelObjectType::PressurePlate:
		case EGridLevelObjectType::Lever:
			bSuccess = WorldObject && ApplyStatefulLinkCommand(*WorldObject, ResolvedCommand);
			FailureReason = bSuccess ? nullptr : TEXT("stateful gameplay command failed");
			break;
		case EGridLevelObjectType::Pit:
			bSuccess = WorldObject && ApplyPitLinkCommand(*WorldObject, ResolvedCommand);
			FailureReason = bSuccess ? nullptr : TEXT("pit command failed");
			break;
		case EGridLevelObjectType::Button:
		case EGridLevelObjectType::Decoration:
		case EGridLevelObjectType::ItemSpawn:
		case EGridLevelObjectType::Item:
		case EGridLevelObjectType::Light:
		case EGridLevelObjectType::Teleporter:
		case EGridLevelObjectType::Trigger:
		case EGridLevelObjectType::Receptacle:
			FailureReason = TEXT("target type has no gameplay command handler");
			break;
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

	if (LinkData.Condition == EGridObjectCondition::LevelVariableBoolEquals || LinkData.Condition == EGridObjectCondition::LevelVariableIntCompare)
	{
		UE_LOG(LogGridActivation, Warning,
			TEXT("Grid link condition rejected: Source=%s Target=%s Condition=%s Reason=legacy LevelVariable link conditions are disabled; move puzzle logic into Lua"),
			*LinkData.SourceObjectId.ToString(), *LinkData.TargetObjectId.ToString(), *GridObjectConditionToString(LinkData.Condition));
		return false;
	}

	bool bConditionResult = false;
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
		case EGridObjectCondition::ReceptacleIsEmpty: bConditionResult = ReceptacleActor->IsEmpty(); break;
		case EGridObjectCondition::ReceptacleHasAnyItem: bConditionResult = ReceptacleActor->HasAnyItem(); break;
		case EGridObjectCondition::ReceptacleContainsItemDefinition:
			if (LinkData.ConditionItemDefinitionId.IsNone()) return RejectMissingConditionParameter(TEXT("ConditionItemDefinitionId"));
			bConditionResult = ReceptacleActor->ContainsItemDefinition(LinkData.ConditionItemDefinitionId);
			break;
		case EGridObjectCondition::ReceptacleContainsItemTag:
			if (LinkData.ConditionItemTag.IsNone()) return RejectMissingConditionParameter(TEXT("ConditionItemTag"));
			bConditionResult = ReceptacleActor->ContainsItemTag(LinkData.ConditionItemTag);
			break;
		case EGridObjectCondition::ReceptacleContainsItemType:
			if (LinkData.ConditionItemType == EGridItemType::None) return RejectMissingConditionParameter(TEXT("ConditionItemType"));
			bConditionResult = ReceptacleActor->ContainsItemType(LinkData.ConditionItemType);
			break;
		case EGridObjectCondition::ReceptacleItemCountAtLeast:
			if (LinkData.ConditionCount <= 0) return RejectMissingConditionParameter(TEXT("ConditionCount"));
			bConditionResult = ReceptacleActor->GetContainedItemCount() >= LinkData.ConditionCount;
			break;
		case EGridObjectCondition::ReceptacleWeightAtLeast:
			if (LinkData.ConditionWeight <= 0.0f) return RejectMissingConditionParameter(TEXT("ConditionWeight"));
			bConditionResult = ReceptacleActor->GetContainedTotalWeight() >= LinkData.ConditionWeight;
			break;
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

bool UGridActivationComponent::ApplyDoorLinkCommand(const FGridWorldObjectInstance& TargetObject, EGridObjectCommand Command)
{
	if (!RuntimeActor)
	{
		return false;
	}
	switch (Command)
	{
		case EGridObjectCommand::Toggle: return RuntimeActor->ToggleDoorOnEdge(TargetObject.CellX, TargetObject.CellY, TargetObject.WallSide);
		case EGridObjectCommand::Open:
		case EGridObjectCommand::Activate: return RuntimeActor->OpenDoorOnEdge(TargetObject.CellX, TargetObject.CellY, TargetObject.WallSide);
		case EGridObjectCommand::Close:
		case EGridObjectCommand::Deactivate: return RuntimeActor->CloseDoorOnEdge(TargetObject.CellX, TargetObject.CellY, TargetObject.WallSide);
		default: return false;
	}
}

bool UGridActivationComponent::ApplyPitLinkCommand(const FGridWorldObjectInstance& TargetObject, EGridObjectCommand Command)
{
	if (!RuntimeActor || TargetObject.Type != EGridLevelObjectType::Pit)
	{
		return false;
	}
	switch (Command)
	{
		case EGridObjectCommand::Toggle: return RuntimeActor->TogglePit(TargetObject.InstanceId);
		case EGridObjectCommand::Open:
		case EGridObjectCommand::Activate: return RuntimeActor->SetPitOpen(TargetObject.InstanceId, true);
		case EGridObjectCommand::Close:
		case EGridObjectCommand::Deactivate: return RuntimeActor->SetPitOpen(TargetObject.InstanceId, false);
		default: return false;
	}
}

bool UGridActivationComponent::ApplyReceptacleLinkCommand(const FGridWorldObjectInstance& TargetObject, EGridObjectCommand Command)
{
	UE_LOG(LogGridActivation, Log, TEXT("Grid receptacle command received: Command=%s Target=%s TargetType=%s"), *GridObjectCommandToString(Command),
		*TargetObject.InstanceId.ToString(), *GridObjectTypeToString(TargetObject.Type));

	AGridReceptacleActor* ReceptacleActor = RuntimeActor ? RuntimeActor->FindRuntimeObjectActor<AGridReceptacleActor>(TargetObject.InstanceId) : nullptr;
	if (!ReceptacleActor)
	{
		UE_LOG(LogGridActivation, Warning, TEXT("Grid receptacle command failed: Command=%s Target=%s Reason=target is not a receptacle"),
			*GridObjectCommandToString(Command), *TargetObject.InstanceId.ToString());
		return false;
	}

	bool bSuccess = true;
	switch (Command)
	{
		case EGridObjectCommand::ReceptacleConsumeItem:
			if (!ReceptacleActor->HasItem()) return false;
			bSuccess = ReceptacleActor->ConsumeItemAtIndex(0);
			break;
		case EGridObjectCommand::ReceptacleConsumeAllItems:
			if (!ReceptacleActor->HasItem()) return false;
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
	return bSuccess;
}

bool UGridActivationComponent::ApplyStatefulLinkCommand(const FGridWorldObjectInstance& TargetObject, EGridObjectCommand Command)
{
	switch (Command)
	{
		case EGridObjectCommand::Open:
		case EGridObjectCommand::Activate: return SetTargetActiveState(TargetObject, true);
		case EGridObjectCommand::Close:
		case EGridObjectCommand::Deactivate: return SetTargetActiveState(TargetObject, false);
		case EGridObjectCommand::Toggle: return SetTargetActiveState(TargetObject, !IsTargetActive(TargetObject.InstanceId));
		default: return false;
	}
}

bool UGridActivationComponent::SetTargetActiveState(const FGridWorldObjectInstance& TargetObject, bool bActive)
{
	if (!TargetObject.InstanceId.IsValid() || !RuntimeActor)
	{
		return false;
	}

	AGridLeverActor* LeverActor = nullptr;
	AGridPressurePlateActor* PlateActor = nullptr;
	switch (TargetObject.Type)
	{
		case EGridLevelObjectType::Lever:
			LeverActor = RuntimeActor->FindRuntimeObjectActor<AGridLeverActor>(TargetObject.InstanceId);
			if (!LeverActor) return false;
			break;
		case EGridLevelObjectType::PressurePlate:
			PlateActor = RuntimeActor->FindRuntimeObjectActor<AGridPressurePlateActor>(TargetObject.InstanceId);
			if (!PlateActor) return false;
			break;
		default:
			return false;
	}

	const bool bWasActive = IsTargetActive(TargetObject.InstanceId);
	const bool bStateChanged = bWasActive != bActive;
	if (bActive) ActiveObjectIds.Add(TargetObject.InstanceId); else ActiveObjectIds.Remove(TargetObject.InstanceId);
	if (LeverActor) LeverActor->SetLeverState(bActive); else if (PlateActor) PlateActor->SetPressed(bActive);

	if (bStateChanged)
	{
		const EGridObjectEvent StateEvent = bActive ? EGridObjectEvent::Activated : EGridObjectEvent::Deactivated;
		UE_LOG(LogGridActivation, Log, TEXT("Grid mechanism state changed by link command: Target=%s Type=%s PreviousActive=%s NewActive=%s Event=%s"),
			*TargetObject.InstanceId.ToString(), *GridObjectTypeToString(TargetObject.Type), bWasActive ? TEXT("true") : TEXT("false"),
			bActive ? TEXT("true") : TEXT("false"), *GridObjectEventToString(StateEvent));
		ExecuteLinksFromObjectForEvent(TargetObject.InstanceId, StateEvent);
	}
	return true;
}

bool UGridActivationComponent::IsTargetActive(FGuid ObjectId) const
{
	return ObjectId.IsValid() && ActiveObjectIds.Contains(ObjectId);
}

void UGridActivationComponent::LogLinkResult(const FGridObjectLink& LinkData, EGridObjectCommand ResolvedCommand, bool bSuccess, const TCHAR* FailureReason) const
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