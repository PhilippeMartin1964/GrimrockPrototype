#pragma once

#include "CoreMinimal.h"

/** TRAP01.9.1: read-only risk estimate for one eligible party specialist.
 * Exactly 20 equally likely d20 outcomes; no critical success/failure rules.
 * SafeFailureMargin is the character's best applicable personal talent margin.
 * Safe protection is once per trap GUID (TRAP01.8).
 */
struct FGridDungeonTrapDisarmOdds
{
    int32 BestCharacterIndex = INDEX_NONE;
    int32 Difficulty = 0;
    int32 StaticBonus = 0;
    int32 SuccessPercent = 0;
    int32 SafeFailurePercent = 0;
    int32 TriggerPercent = 0;

    static FGridDungeonTrapDisarmOdds Calculate(
        int32 DifficultyDC, int32 TotalStaticBonus, int32 SafeFailureMargin,
        bool bSafeFailureAlreadyUsed)
    {
        FGridDungeonTrapDisarmOdds Result;
        Result.Difficulty = DifficultyDC;
        Result.StaticBonus = TotalStaticBonus;
        int32 Successes = 0;
        int32 Safes = 0;
        int32 Triggers = 0;
        for (int32 Face = 1; Face <= 20; ++Face)
        {
            const int32 Total = Face + TotalStaticBonus;
            if (Total >= DifficultyDC)
            {
                ++Successes;
            }
            else if (!bSafeFailureAlreadyUsed && SafeFailureMargin > 0 &&
                DifficultyDC - Total <= SafeFailureMargin)
            {
                ++Safes;
            }
            else
            {
                ++Triggers;
            }
        }
        Result.SuccessPercent = Successes * 5;
        Result.SafeFailurePercent = Safes * 5;
        Result.TriggerPercent = Triggers * 5;
        return Result;
    }
};
