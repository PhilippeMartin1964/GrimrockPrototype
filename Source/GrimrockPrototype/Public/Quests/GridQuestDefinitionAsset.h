#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Quests/GridQuestTypes.h"
#include "GridQuestDefinitionAsset.generated.h"

/** Data-driven, campaign-stable quest definition. Runtime state lives elsewhere. */
UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API UGridQuestDefinitionAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName QuestId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest", meta = (MultiLine = "true"))
	FText Description;

	/** Objective order is authoritative for sequential progression. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TArray<FGridQuestObjectiveDefinition> Objectives;

	UFUNCTION(BlueprintCallable, Category = "Quest|Validation")
	bool ValidateDefinition(UPARAM(ref) FString& OutError) const;

	const FGridQuestObjectiveDefinition* FindObjective(FName ObjectiveId) const;
};
