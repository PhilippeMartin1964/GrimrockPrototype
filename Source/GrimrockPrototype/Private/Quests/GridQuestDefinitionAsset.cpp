#include "Quests/GridQuestDefinitionAsset.h"

bool UGridQuestDefinitionAsset::ValidateDefinition(FString& OutError) const
{
	OutError.Reset();

	if (QuestId.IsNone())
	{
		OutError = TEXT("QuestId is NAME_None.");
		return false;
	}
	if (DisplayName.IsEmpty())
	{
		OutError = FString::Printf(TEXT("Quest '%s' has an empty DisplayName."), *QuestId.ToString());
		return false;
	}

	TSet<FName> SeenObjectiveIds;
	for (int32 ObjectiveIndex = 0; ObjectiveIndex < Objectives.Num(); ++ObjectiveIndex)
	{
		const FGridQuestObjectiveDefinition& Objective = Objectives[ObjectiveIndex];
		if (Objective.ObjectiveId.IsNone())
		{
			OutError = FString::Printf(TEXT("Quest '%s' objective %d has ObjectiveId=NAME_None."), *QuestId.ToString(), ObjectiveIndex);
			return false;
		}
		if (SeenObjectiveIds.Contains(Objective.ObjectiveId))
		{
			OutError = FString::Printf(TEXT("Quest '%s' contains duplicate ObjectiveId '%s'."), *QuestId.ToString(), *Objective.ObjectiveId.ToString());
			return false;
		}
		if (Objective.DisplayName.IsEmpty())
		{
			OutError = FString::Printf(TEXT("Quest '%s' objective '%s' has an empty DisplayName."), *QuestId.ToString(), *Objective.ObjectiveId.ToString());
			return false;
		}
		SeenObjectiveIds.Add(Objective.ObjectiveId);
	}

	return true;
}

const FGridQuestObjectiveDefinition* UGridQuestDefinitionAsset::FindObjective(FName ObjectiveId) const
{
	if (ObjectiveId.IsNone())
	{
		return nullptr;
	}

	return Objectives.FindByPredicate(
		[ObjectiveId](const FGridQuestObjectiveDefinition& Objective)
		{
			return Objective.ObjectiveId == ObjectiveId;
		});
}
