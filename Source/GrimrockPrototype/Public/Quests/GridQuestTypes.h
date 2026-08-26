#pragma once

#include "CoreMinimal.h"
#include "GridQuestTypes.generated.h"

UENUM(BlueprintType)
enum class EGridQuestStatus : uint8
{
	Inactive UMETA(DisplayName = "Inactive"),
	Active UMETA(DisplayName = "Active"),
	Completed UMETA(DisplayName = "Completed"),
	Failed UMETA(DisplayName = "Failed")
};

UENUM(BlueprintType)
enum class EGridQuestObjectiveStatus : uint8
{
	Inactive UMETA(DisplayName = "Inactive"),
	Active UMETA(DisplayName = "Active"),
	Completed UMETA(DisplayName = "Completed"),
	Failed UMETA(DisplayName = "Failed")
};

UENUM(BlueprintType)
enum class EGridQuestMutationResult : uint8
{
	Success UMETA(DisplayName = "Success"),
	InvalidQuestId UMETA(DisplayName = "Invalid Quest Id"),
	InvalidObjectiveId UMETA(DisplayName = "Invalid Objective Id"),
	DefinitionNotFound UMETA(DisplayName = "Definition Not Found"),
	InvalidDefinition UMETA(DisplayName = "Invalid Definition"),
	AlreadyInState UMETA(DisplayName = "Already In State"),
	InvalidTransition UMETA(DisplayName = "Invalid Transition")
};

/** Ordered, data-driven objective definition. ObjectiveId is stable identity. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridQuestObjectiveDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
	FName ObjectiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective", meta = (MultiLine = "true"))
	FText Description;
};

/** Runtime state for one objective. No definition pointer is authoritative here. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridQuestObjectiveRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest|Runtime")
	FName ObjectiveId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest|Runtime")
	EGridQuestObjectiveStatus Status = EGridQuestObjectiveStatus::Inactive;
};

/** Unique runtime state for one stable QuestId. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridQuestRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest|Runtime")
	FName QuestId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest|Runtime")
	EGridQuestStatus Status = EGridQuestStatus::Inactive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest|Runtime")
	TArray<FGridQuestObjectiveRuntimeState> Objectives;

	const FGridQuestObjectiveRuntimeState* FindObjective(FName ObjectiveId) const
	{
		return Objectives.FindByPredicate(
			[ObjectiveId](const FGridQuestObjectiveRuntimeState& Objective)
			{
				return Objective.ObjectiveId == ObjectiveId;
			});
	}

	FGridQuestObjectiveRuntimeState* FindMutableObjective(FName ObjectiveId)
	{
		return Objectives.FindByPredicate(
			[ObjectiveId](FGridQuestObjectiveRuntimeState& Objective)
			{
				return Objective.ObjectiveId == ObjectiveId;
			});
	}
};

/** Campaign-scoped quest state. One runtime record is allowed per QuestId. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FGridCampaignQuestRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest|Runtime")
	TArray<FGridQuestRuntimeState> Quests;

	const FGridQuestRuntimeState* FindQuest(FName QuestId) const
	{
		return Quests.FindByPredicate(
			[QuestId](const FGridQuestRuntimeState& Quest)
			{
				return Quest.QuestId == QuestId;
			});
	}

	FGridQuestRuntimeState* FindMutableQuest(FName QuestId)
	{
		return Quests.FindByPredicate(
			[QuestId](FGridQuestRuntimeState& Quest)
			{
				return Quest.QuestId == QuestId;
			});
	}

	void Reset()
	{
		Quests.Reset();
	}
};
