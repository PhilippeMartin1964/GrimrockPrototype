#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridInventoryTypes.h"
#include "GrimrockPartySaveGame.generated.h"

USTRUCT (BlueprintType)
struct FRPGCharacterProgressionSaveState
{
    GENERATED_BODY ()

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "RPG|Progression")
    FGuid CharacterId;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "RPG|Progression")
    TArray<FName> SelectedChoiceIds;
};

USTRUCT (BlueprintType)
struct FRPGPendingLevelUpSaveState
{
    GENERATED_BODY ()

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "RPG|Level Up")
    FGuid CharacterId;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "RPG|Level Up")
    int32 PreviousLevel = 1;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "RPG|Level Up")
    int32 NewLevel = 1;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "RPG|Level Up")
    int32 LevelsGained = 0;
};

UCLASS ()
class GRIMROCKPROTOTYPE_API UGrimrockPartySaveGame : public USaveGame
{
    GENERATED_BODY ()

public:
    static constexpr int32 CurrentSaveVersion = 4;
    static constexpr int32 MinimumCompatibleSaveVersion = 1;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
    int32 SaveVersion = CurrentSaveVersion;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
    FGridPartyInventoryState PartyInventoryState;

    /** MON15.6 authoritative persisted class choices, keyed by CharacterId. */
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save|RPG")
    TArray<FRPGCharacterProgressionSaveState> ClassProgressionStates;

    /** MON15.6 level-up notifications that still need to be presented. */
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save|RPG")
    TArray<FRPGPendingLevelUpSaveState> PendingLevelUpNotifications;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
    FGridDungeonRuntimeState DungeonRuntimeState;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
    FName CurrentDungeonLevelId = NAME_None;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
    int32 PartyCellX = 0;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
    int32 PartyCellY = 0;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
    EGridEdge PartyFacing = EGridEdge::North;

    virtual void Serialize (FArchive& Ar) override;

    bool IsCompatible () const
    {
        return bProgressionLoadValid &&
            SaveVersion >= MinimumCompatibleSaveVersion &&
            SaveVersion <= CurrentSaveVersion;
    }

    bool IsProgressionLoadValid () const
    {
        return bProgressionLoadValid;
    }

    const FString& GetProgressionLoadError () const
    {
        return ProgressionLoadError;
    }

private:
    bool bProgressionLoadValid = true;
    FString ProgressionLoadError;
};
