#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/GridInventoryTypes.h"
#include "GrimrockPartySaveGame.generated.h"

UCLASS ()
class GRIMROCKPROTOTYPE_API UGrimrockPartySaveGame : public USaveGame
{
    GENERATED_BODY ()

public:
    static constexpr int32 CurrentSaveVersion = 3;
    static constexpr int32 MinimumCompatibleSaveVersion = 1;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
    int32 SaveVersion = CurrentSaveVersion;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Save")
    FGridPartyInventoryState PartyInventoryState;

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

    bool IsCompatible () const
    {
        return SaveVersion >= MinimumCompatibleSaveVersion &&
            SaveVersion <= CurrentSaveVersion;
    }
};
