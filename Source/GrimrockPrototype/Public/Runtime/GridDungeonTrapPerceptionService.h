#pragma once

#include "CoreMinimal.h"
#include "Core/GridObjectBehavior.h"
#include "Core/GridTypes.h"
#include "Core/GridDungeonTrapTypes.h"

struct FGridCharacterInventoryState;
class URPGSkillAsset;

/** TRAP01.2 pure rules, no Tick, world mutation or player input. */
struct GRIMROCKPROTOTYPE_API FGridDungeonTrapPerceptionService
{
    /** Returns INDEX_NONE when no living observer has a structurally valid skill
     * state. Rank plus Wisdom modifier determines the winner; ties use rank
     * then stable party slot index. Final d20 is always RPGSkillCheckService.
     */
    static int32 SelectBestObserver(
        const TArray<FGridCharacterInventoryState>& Characters,
        const URPGSkillAsset* PerceptionSkill, int32& OutEffectiveScore);

    static int32 GetMaxRangeCells(int32 EffectiveScore, bool bPartyHasLight,
        EGridDungeonTrapDetectionSense Sense, int32 DefinitionRangeCap);

    /** Cardinal straight-line range only in TRAP01.2; diagonals and corners
     * cannot reveal a hidden trap, irrespective of grid Manhattan distance. */
    static bool GetCardinalDirectionAndDistance(FIntPoint Origin, FIntPoint Target,
        EGridEdge& OutDirection, int32& OutDistance);

    /** Player-facing location phrase, with distance and mounting surface. */
    static FText FormatRelativeLocation(EGridEdge Facing, EGridEdge AbsoluteDirection,
        int32 Distance, EGridObjectPlacementKind PlacementSurface);
};
