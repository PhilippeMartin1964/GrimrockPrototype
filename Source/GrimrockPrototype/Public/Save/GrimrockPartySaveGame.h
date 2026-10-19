#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Quests/GridQuestTypes.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridInventoryTypes.h"
#include "GrimrockPartySaveGame.generated.h"

UCLASS()
class GRIMROCKPROTOTYPE_API UGrimrockPartySaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** EVENTHUD01.11: durable gameplay clock added. Exact-match, no backward migration. */
	static constexpr int32 CurrentSaveVersion = 27;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	int32 SaveVersion = CurrentSaveVersion;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	FGridPartyInventoryState PartyInventoryState;

	/** UI-PARTYHUD01.5: presentation order keyed only by stable active-character identity. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save|UI")
	TArray<FGuid> PartyPortraitOrder;

	/** MON21.4.2: durable campaign Quest snapshot keyed only by QuestId / ObjectiveId. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save|Quests")
	FGridCampaignQuestRuntimeState CampaignQuestState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	FGridDungeonRuntimeState DungeonRuntimeState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	FName CurrentDungeonLevelId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	int32 PartyCellX = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	int32 PartyCellY = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	EGridEdge PartyFacing = EGridEdge::North;

	/** EVENTHUD01.11 accumulated gameplay time; Event Feed history itself remains transient. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save|Gameplay Time")
	double GameplayElapsedSeconds = 0.0;

	virtual void Serialize(FArchive& Ar) override;

	bool ValidateCurrentState(FText& OutError) const;

	bool IsCompatible() const
	{
		return bLoadValid && SaveVersion == CurrentSaveVersion;
	}

	bool IsLoadValid() const
	{
		return bLoadValid;
	}

	const FString& GetLoadError() const
	{
		return LoadError;
	}

private:
	bool bLoadValid = true;
	FString LoadError;
};
