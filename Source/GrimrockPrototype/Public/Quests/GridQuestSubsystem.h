#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Quests/GridQuestTypes.h"
#include "GridQuestSubsystem.generated.h"

class UGridQuestDefinitionAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGridQuestStateChangedSignature, FName, QuestId);

/**
 * Unique campaign/session authority for quest runtime state.
 *
 * Definitions are data-driven and registered by stable QuestId. The state is
 * transient in MON21.2; SaveGame persistence is deliberately deferred to
 * MON21.4. No world Actor or permanent Tick is introduced.
 */
UCLASS()
class GRIMROCKPROTOTYPE_API UGridQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "Quest|Runtime")
	FGridCampaignQuestRuntimeState CampaignState;

	/** Presentation/read-model notification. NAME_None means a full reset. */
	UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
	FGridQuestStateChangedSignature OnQuestStateChanged;

	UFUNCTION(BlueprintCallable, Category = "Quest|Definitions")
	bool RegisterQuestDefinition(UGridQuestDefinitionAsset* Definition, UPARAM(ref) FString& OutError);

	UFUNCTION(BlueprintPure, Category = "Quest|Definitions")
	UGridQuestDefinitionAsset* FindQuestDefinition(FName QuestId) const;

	UFUNCTION(BlueprintPure, Category = "Quest|Runtime")
	bool HasQuestRuntimeState(FName QuestId) const;

	UFUNCTION(BlueprintPure, Category = "Quest|Runtime")
	EGridQuestStatus GetQuestStatus(FName QuestId) const;

	UFUNCTION(BlueprintCallable, Category = "Quest|Runtime")
	bool GetQuestRuntimeState(FName QuestId, FGridQuestRuntimeState& OutState) const;

	UFUNCTION(BlueprintCallable, Category = "Quest|Runtime")
	bool GetObjectiveStatus(FName QuestId, FName ObjectiveId, EGridQuestObjectiveStatus& OutStatus) const;

	UFUNCTION(BlueprintCallable, Category = "Quest|Runtime")
	EGridQuestMutationResult StartQuest(FName QuestId);

	UFUNCTION(BlueprintCallable, Category = "Quest|Runtime")
	EGridQuestMutationResult CompleteObjective(FName QuestId, FName ObjectiveId);

	UFUNCTION(BlueprintCallable, Category = "Quest|Runtime")
	EGridQuestMutationResult CompleteQuest(FName QuestId);

	UFUNCTION(BlueprintCallable, Category = "Quest|Runtime")
	EGridQuestMutationResult FailQuest(FName QuestId);

	UFUNCTION(BlueprintCallable, Category = "Quest|Runtime")
	void ResetCampaignQuestState();

	UFUNCTION(BlueprintCallable, Category = "Quest|Validation")
	bool ValidateRuntimeState(UPARAM(ref) FString& OutError) const;

private:
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UGridQuestDefinitionAsset>> QuestDefinitionsById;

	bool BuildInitialRuntimeState(const UGridQuestDefinitionAsset& Definition, FGridQuestRuntimeState& OutState) const;
};
