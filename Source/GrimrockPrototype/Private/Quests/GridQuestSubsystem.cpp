#include "Quests/GridQuestSubsystem.h"

#include "Quests/GridQuestDefinitionAsset.h"

namespace
{
	bool GridQuestSubsystemValidateOrderedObjectives(
		const FGridQuestRuntimeState& QuestState, EGridQuestObjectiveStatus ExpectedTerminalStatus, FString& OutError)
	{
		if (QuestState.Objectives.IsEmpty())
		{
			return true;
		}

		int32 TerminalIndex = INDEX_NONE;
		for (int32 ObjectiveIndex = 0; ObjectiveIndex < QuestState.Objectives.Num(); ++ObjectiveIndex)
		{
			const EGridQuestObjectiveStatus ObjectiveStatus = QuestState.Objectives[ObjectiveIndex].Status;
			if (ObjectiveStatus == ExpectedTerminalStatus)
			{
				if (TerminalIndex != INDEX_NONE)
				{
					OutError = FString::Printf(TEXT("Quest '%s' has more than one terminal objective."), *QuestState.QuestId.ToString());
					return false;
				}
				TerminalIndex = ObjectiveIndex;
			}
			else if (ObjectiveStatus == EGridQuestObjectiveStatus::Failed || ObjectiveStatus == EGridQuestObjectiveStatus::Active)
			{
				OutError = FString::Printf(TEXT("Quest '%s' contains an objective state inconsistent with its quest status."), *QuestState.QuestId.ToString());
				return false;
			}
		}

		if (TerminalIndex == INDEX_NONE)
		{
			OutError = FString::Printf(TEXT("Quest '%s' has no current terminal objective."), *QuestState.QuestId.ToString());
			return false;
		}

		for (int32 ObjectiveIndex = 0; ObjectiveIndex < QuestState.Objectives.Num(); ++ObjectiveIndex)
		{
			const EGridQuestObjectiveStatus ExpectedStatus = ObjectiveIndex < TerminalIndex ? EGridQuestObjectiveStatus::Completed
				: ObjectiveIndex == TerminalIndex                               ? ExpectedTerminalStatus
				                                                               : EGridQuestObjectiveStatus::Inactive;
			if (QuestState.Objectives[ObjectiveIndex].Status != ExpectedStatus)
			{
				OutError = FString::Printf(TEXT("Quest '%s' objective ordering is inconsistent at index %d."), *QuestState.QuestId.ToString(), ObjectiveIndex);
				return false;
			}
		}

		return true;
	}
}

bool UGridQuestSubsystem::RegisterQuestDefinition(UGridQuestDefinitionAsset* Definition, FString& OutError)
{
	OutError.Reset();
	if (!IsValid(Definition))
	{
		OutError = TEXT("Quest definition is null or invalid.");
		return false;
	}
	if (!Definition->ValidateDefinition(OutError))
	{
		return false;
	}

	if (const TObjectPtr<UGridQuestDefinitionAsset>* Existing = QuestDefinitionsById.Find(Definition->QuestId))
	{
		if (Existing->Get() == Definition)
		{
			return true;
		}

		OutError = FString::Printf(TEXT("QuestId '%s' is already registered by another definition."), *Definition->QuestId.ToString());
		return false;
	}

	QuestDefinitionsById.Add(Definition->QuestId, Definition);
	return true;
}

UGridQuestDefinitionAsset* UGridQuestSubsystem::FindQuestDefinition(FName QuestId) const
{
	const TObjectPtr<UGridQuestDefinitionAsset>* Definition = QuestDefinitionsById.Find(QuestId);
	return Definition && IsValid(Definition->Get()) ? Definition->Get() : nullptr;
}

bool UGridQuestSubsystem::HasQuestRuntimeState(FName QuestId) const
{
	return !QuestId.IsNone() && CampaignState.FindQuest(QuestId) != nullptr;
}

EGridQuestStatus UGridQuestSubsystem::GetQuestStatus(FName QuestId) const
{
	const FGridQuestRuntimeState* QuestState = CampaignState.FindQuest(QuestId);
	return QuestState ? QuestState->Status : EGridQuestStatus::Inactive;
}

bool UGridQuestSubsystem::GetQuestRuntimeState(FName QuestId, FGridQuestRuntimeState& OutState) const
{
	const FGridQuestRuntimeState* QuestState = CampaignState.FindQuest(QuestId);
	if (!QuestState)
	{
		OutState = FGridQuestRuntimeState();
		return false;
	}

	OutState = *QuestState;
	return true;
}

bool UGridQuestSubsystem::GetObjectiveStatus(FName QuestId, FName ObjectiveId, EGridQuestObjectiveStatus& OutStatus) const
{
	OutStatus = EGridQuestObjectiveStatus::Inactive;
	const FGridQuestRuntimeState* QuestState = CampaignState.FindQuest(QuestId);
	const FGridQuestObjectiveRuntimeState* ObjectiveState = QuestState ? QuestState->FindObjective(ObjectiveId) : nullptr;
	if (!ObjectiveState)
	{
		return false;
	}

	OutStatus = ObjectiveState->Status;
	return true;
}

EGridQuestMutationResult UGridQuestSubsystem::StartQuest(FName QuestId)
{
	if (QuestId.IsNone())
	{
		return EGridQuestMutationResult::InvalidQuestId;
	}

	UGridQuestDefinitionAsset* Definition = FindQuestDefinition(QuestId);
	if (!Definition)
	{
		return EGridQuestMutationResult::DefinitionNotFound;
	}

	FString DefinitionError;
	if (!Definition->ValidateDefinition(DefinitionError))
	{
		return EGridQuestMutationResult::InvalidDefinition;
	}

	if (FGridQuestRuntimeState* ExistingState = CampaignState.FindMutableQuest(QuestId))
	{
		if (ExistingState->Status == EGridQuestStatus::Active)
		{
			return EGridQuestMutationResult::AlreadyInState;
		}
		if (ExistingState->Status != EGridQuestStatus::Inactive)
		{
			return EGridQuestMutationResult::InvalidTransition;
		}
		if (!BuildInitialRuntimeState(*Definition, *ExistingState))
		{
			return EGridQuestMutationResult::InvalidDefinition;
		}
	}
	else
	{
		FGridQuestRuntimeState NewState;
		if (!BuildInitialRuntimeState(*Definition, NewState))
		{
			return EGridQuestMutationResult::InvalidDefinition;
		}
		CampaignState.Quests.Add(MoveTemp(NewState));
	}

	OnQuestStateChanged.Broadcast(QuestId);
	return EGridQuestMutationResult::Success;
}

EGridQuestMutationResult UGridQuestSubsystem::CompleteObjective(FName QuestId, FName ObjectiveId)
{
	if (QuestId.IsNone())
	{
		return EGridQuestMutationResult::InvalidQuestId;
	}
	if (ObjectiveId.IsNone())
	{
		return EGridQuestMutationResult::InvalidObjectiveId;
	}

	UGridQuestDefinitionAsset* Definition = FindQuestDefinition(QuestId);
	if (!Definition)
	{
		return EGridQuestMutationResult::DefinitionNotFound;
	}
	if (!Definition->FindObjective(ObjectiveId))
	{
		return EGridQuestMutationResult::InvalidObjectiveId;
	}

	FGridQuestRuntimeState* QuestState = CampaignState.FindMutableQuest(QuestId);
	if (!QuestState || QuestState->Status != EGridQuestStatus::Active)
	{
		return EGridQuestMutationResult::InvalidTransition;
	}

	FGridQuestObjectiveRuntimeState* ObjectiveState = QuestState->FindMutableObjective(ObjectiveId);
	if (!ObjectiveState)
	{
		return EGridQuestMutationResult::InvalidObjectiveId;
	}
	if (ObjectiveState->Status == EGridQuestObjectiveStatus::Completed)
	{
		return EGridQuestMutationResult::AlreadyInState;
	}
	if (ObjectiveState->Status != EGridQuestObjectiveStatus::Active)
	{
		return EGridQuestMutationResult::InvalidTransition;
	}

	const int32 ObjectiveIndex = QuestState->Objectives.IndexOfByPredicate(
		[ObjectiveId](const FGridQuestObjectiveRuntimeState& Candidate)
		{
			return Candidate.ObjectiveId == ObjectiveId;
		});
	if (ObjectiveIndex == INDEX_NONE)
	{
		return EGridQuestMutationResult::InvalidObjectiveId;
	}

	ObjectiveState->Status = EGridQuestObjectiveStatus::Completed;
	if (QuestState->Objectives.IsValidIndex(ObjectiveIndex + 1))
	{
		QuestState->Objectives[ObjectiveIndex + 1].Status = EGridQuestObjectiveStatus::Active;
	}
	else
	{
		QuestState->Status = EGridQuestStatus::Completed;
	}

	OnQuestStateChanged.Broadcast(QuestId);
	return EGridQuestMutationResult::Success;
}

EGridQuestMutationResult UGridQuestSubsystem::CompleteQuest(FName QuestId)
{
	if (QuestId.IsNone())
	{
		return EGridQuestMutationResult::InvalidQuestId;
	}
	if (!FindQuestDefinition(QuestId))
	{
		return EGridQuestMutationResult::DefinitionNotFound;
	}

	FGridQuestRuntimeState* QuestState = CampaignState.FindMutableQuest(QuestId);
	if (!QuestState)
	{
		return EGridQuestMutationResult::InvalidTransition;
	}
	if (QuestState->Status == EGridQuestStatus::Completed)
	{
		return EGridQuestMutationResult::AlreadyInState;
	}
	if (QuestState->Status != EGridQuestStatus::Active)
	{
		return EGridQuestMutationResult::InvalidTransition;
	}

	QuestState->Status = EGridQuestStatus::Completed;
	for (FGridQuestObjectiveRuntimeState& Objective : QuestState->Objectives)
	{
		Objective.Status = EGridQuestObjectiveStatus::Completed;
	}

	OnQuestStateChanged.Broadcast(QuestId);
	return EGridQuestMutationResult::Success;
}

EGridQuestMutationResult UGridQuestSubsystem::FailQuest(FName QuestId)
{
	if (QuestId.IsNone())
	{
		return EGridQuestMutationResult::InvalidQuestId;
	}
	if (!FindQuestDefinition(QuestId))
	{
		return EGridQuestMutationResult::DefinitionNotFound;
	}

	FGridQuestRuntimeState* QuestState = CampaignState.FindMutableQuest(QuestId);
	if (!QuestState)
	{
		return EGridQuestMutationResult::InvalidTransition;
	}
	if (QuestState->Status == EGridQuestStatus::Failed)
	{
		return EGridQuestMutationResult::AlreadyInState;
	}
	if (QuestState->Status != EGridQuestStatus::Active)
	{
		return EGridQuestMutationResult::InvalidTransition;
	}

	QuestState->Status = EGridQuestStatus::Failed;
	for (FGridQuestObjectiveRuntimeState& Objective : QuestState->Objectives)
	{
		if (Objective.Status == EGridQuestObjectiveStatus::Active)
		{
			Objective.Status = EGridQuestObjectiveStatus::Failed;
			break;
		}
	}

	OnQuestStateChanged.Broadcast(QuestId);
	return EGridQuestMutationResult::Success;
}

void UGridQuestSubsystem::ResetCampaignQuestState()
{
	const bool bHadRuntimeState = !CampaignState.Quests.IsEmpty();
	CampaignState.Reset();
	if (bHadRuntimeState)
	{
		OnQuestStateChanged.Broadcast(NAME_None);
	}
}

bool UGridQuestSubsystem::ValidateRuntimeState(FString& OutError) const
{
	OutError.Reset();
	TSet<FName> SeenQuestIds;

	for (const FGridQuestRuntimeState& QuestState : CampaignState.Quests)
	{
		if (QuestState.QuestId.IsNone())
		{
			OutError = TEXT("Campaign quest state contains QuestId=NAME_None.");
			return false;
		}
		if (SeenQuestIds.Contains(QuestState.QuestId))
		{
			OutError = FString::Printf(TEXT("Campaign quest state contains duplicate QuestId '%s'."), *QuestState.QuestId.ToString());
			return false;
		}
		SeenQuestIds.Add(QuestState.QuestId);

		const UGridQuestDefinitionAsset* Definition = FindQuestDefinition(QuestState.QuestId);
		if (!Definition)
		{
			OutError = FString::Printf(TEXT("Quest '%s' has runtime state but no registered definition."), *QuestState.QuestId.ToString());
			return false;
		}

		FString DefinitionError;
		if (!Definition->ValidateDefinition(DefinitionError))
		{
			OutError = FString::Printf(TEXT("Quest '%s' definition is invalid: %s"), *QuestState.QuestId.ToString(), *DefinitionError);
			return false;
		}
		if (QuestState.Objectives.Num() != Definition->Objectives.Num())
		{
			OutError = FString::Printf(TEXT("Quest '%s' runtime objective count differs from its definition."), *QuestState.QuestId.ToString());
			return false;
		}

		for (int32 ObjectiveIndex = 0; ObjectiveIndex < Definition->Objectives.Num(); ++ObjectiveIndex)
		{
			if (QuestState.Objectives[ObjectiveIndex].ObjectiveId != Definition->Objectives[ObjectiveIndex].ObjectiveId)
			{
				OutError = FString::Printf(TEXT("Quest '%s' runtime objective order differs from its definition at index %d."),
					*QuestState.QuestId.ToString(), ObjectiveIndex);
				return false;
			}
		}

		switch (QuestState.Status)
		{
			case EGridQuestStatus::Inactive:
				for (const FGridQuestObjectiveRuntimeState& Objective : QuestState.Objectives)
				{
					if (Objective.Status != EGridQuestObjectiveStatus::Inactive)
					{
						OutError = FString::Printf(TEXT("Inactive quest '%s' has a non-inactive objective."), *QuestState.QuestId.ToString());
						return false;
					}
				}
				break;

			case EGridQuestStatus::Active:
				if (!GridQuestSubsystemValidateOrderedObjectives(QuestState, EGridQuestObjectiveStatus::Active, OutError))
				{
					return false;
				}
				break;

			case EGridQuestStatus::Completed:
				for (const FGridQuestObjectiveRuntimeState& Objective : QuestState.Objectives)
				{
					if (Objective.Status != EGridQuestObjectiveStatus::Completed)
					{
						OutError = FString::Printf(TEXT("Completed quest '%s' has a non-completed objective."), *QuestState.QuestId.ToString());
						return false;
					}
				}
				break;

			case EGridQuestStatus::Failed:
				if (!GridQuestSubsystemValidateOrderedObjectives(QuestState, EGridQuestObjectiveStatus::Failed, OutError))
				{
					return false;
				}
				break;

			default:
				OutError = FString::Printf(TEXT("Quest '%s' has an unsupported runtime status."), *QuestState.QuestId.ToString());
				return false;
		}
	}

	return true;
}

bool UGridQuestSubsystem::BuildInitialRuntimeState(const UGridQuestDefinitionAsset& Definition, FGridQuestRuntimeState& OutState) const
{
	FString DefinitionError;
	if (!Definition.ValidateDefinition(DefinitionError))
	{
		return false;
	}

	OutState = FGridQuestRuntimeState();
	OutState.QuestId = Definition.QuestId;
	OutState.Status = EGridQuestStatus::Active;
	OutState.Objectives.Reserve(Definition.Objectives.Num());

	for (int32 ObjectiveIndex = 0; ObjectiveIndex < Definition.Objectives.Num(); ++ObjectiveIndex)
	{
		FGridQuestObjectiveRuntimeState& ObjectiveState = OutState.Objectives.AddDefaulted_GetRef();
		ObjectiveState.ObjectiveId = Definition.Objectives[ObjectiveIndex].ObjectiveId;
		ObjectiveState.Status = ObjectiveIndex == 0 ? EGridQuestObjectiveStatus::Active : EGridQuestObjectiveStatus::Inactive;
	}
	return true;
}
