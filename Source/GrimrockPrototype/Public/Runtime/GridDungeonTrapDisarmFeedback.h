#pragma once

#include "CoreMinimal.h"
#include "RPG/RPGSkillTypes.h"

/** TRAP01.7: one localized, result-only description for both disarm input paths.
 * Never call this on an unrevealed trap or before a resolved check.
 * ProgressionModifier already includes character and group modifiers.
 */
namespace GridDungeonTrapDisarmFeedback
{
    inline FText Signed(int32 Value)
    {
        return FText::FromString(FString::Printf(TEXT("%+d"), Value));
    }

    inline FText Format(FText Disarmer, const FRPGSkillCheckResult& Check)
    {
        if (!Check.bResolved)
        {
            return FText::GetEmpty();
        }
        const FText& Pattern = Check.bSuccess
            ? NSLOCTEXT("TRAP017", "DisarmSuccessBreakdown",
                "{0} désamorce le piège. Pièges : d20 {1} + rang {2} + DEX {3} + progression {4} = {5} (difficulté {6}).")
            : NSLOCTEXT("TRAP017", "DisarmFailureBreakdown",
                "{0} échoue à désamorcer le piège. Pièges : d20 {1} + rang {2} + DEX {3} + progression {4} = {5} (difficulté {6}).");
        return FText::Format(Pattern, Disarmer,
            FText::AsNumber(Check.Roll), FText::AsNumber(Check.Rank),
            Signed(Check.AttributeModifier), Signed(Check.ProgressionModifier),
            FText::AsNumber(Check.Total), FText::AsNumber(Check.Difficulty));
    }
}
