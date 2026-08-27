#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Magic/GridSpellbookPersistence.h"
#include "RPG/RPGSkillPersistence.h"
#include "RPG/StatusEffects/GridStatusEffectTypes.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridInventoryTypes.h"
#include "GrimrockPartySaveGame.generated.h"

class UGridStatusEffectDefinitionAsset;

/**
 * Transitional TD07.3.3.5 B1 projection only. SelectedClassProgressionChoiceIds
 * on FGridCharacterInventoryState is the authority; this type is removed in B2.
 */
USTRUCT(BlueprintType)
struct FRPGCharacterProgressionSaveState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "RPG|Progression")
	FGuid CharacterId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "RPG|Progression")
	TArray<FName> SelectedChoiceIds;
};

USTRUCT(BlueprintType)
struct FRPGPendingLevelUpSaveState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "RPG|Level Up")
	FGuid CharacterId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "RPG|Level Up")
	int32 PreviousLevel = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "RPG|Level Up")
	int32 NewLevel = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "RPG|Level Up")
	int32 LevelsGained = 0;
};

/** MON16.7 status snapshots for one party member, keyed by stable CharacterId. */
USTRUCT(BlueprintType)
struct FGridCharacterStatusEffectSaveState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "RPG|Status Effects|Save")
	FGuid CharacterId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "RPG|Status Effects|Save")
	TArray<FGridStatusEffectSaveState> StatusEffects;
};

UCLASS()
class GRIMROCKPROTOTYPE_API UGrimrockPartySaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** TD07.3.3.5 B1: Level is transient and rebuilt from Experience. */
	static constexpr int32 CurrentSaveVersion = 14;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	int32 SaveVersion = CurrentSaveVersion;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
	FGridPartyInventoryState PartyInventoryState;

	/** Transitional B1 field kept compile-visible but no longer authoritative or consumed. Removed in B2. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save|RPG")
	TArray<FRPGCharacterProgressionSaveState> ClassProgressionStates;

	/** MON15.6 level-up notifications that still need to be presented. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save|RPG")
	TArray<FRPGPendingLevelUpSaveState> PendingLevelUpNotifications;

	/** MON16.7 party status snapshots. Runtime DefinitionAsset pointers are excluded. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save|RPG|Status Effects")
	TArray<FGridCharacterStatusEffectSaveState> CharacterStatusEffectStates;

	/** MON18.8 sparse Spellbook snapshots. CharacterId + SpellId identities only. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save|Magic|Spellbook")
	TArray<FGridCharacterSpellbookSaveState> CharacterSpellbookStates;

	/** MON20.9 sparse Skill rank snapshots for active and pooled characters. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save|RPG|Skills")
	TArray<FRPGCharacterSkillSaveState> CharacterSkillStates;

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

	virtual void Serialize(FArchive& Ar) override;

	bool CaptureStatusEffectState(FString& OutError);
	bool RestoreStatusEffectState(FString& OutError);
	bool RestoreStatusEffectState(TFunctionRef<UGridStatusEffectDefinitionAsset*(FName)> DefinitionResolver, FString& OutError);
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