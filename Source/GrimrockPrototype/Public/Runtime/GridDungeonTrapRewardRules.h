#pragma once

#include "CoreMinimal.h"

/** TRAP01.4.1: one shared XP pool per successfully disarmed dungeon trap.
 * Invalid DC cannot create a reward; canonical trap DC is in [1,40].
 * The runtime's durable DisarmedTriggerTrapIds guard prevents re-awarding.
 */
struct GRIMROCKPROTOTYPE_API FGridDungeonTrapRewardRules
{
    static int32 GetDisarmExperience(int32 DisarmDifficulty)
    {
        return DisarmDifficulty >= 1 && DisarmDifficulty <= 40
            ? DisarmDifficulty * 2 : 0;
    }
};
