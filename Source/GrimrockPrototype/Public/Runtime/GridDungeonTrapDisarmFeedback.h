#pragma once

#include "CoreMinimal.h"
#include "RPG/RPGSkillTypes.h"

/** TRAP01.7.1: presentation only. The canonical resolver owns every component
 * (d20, rank, attribute, class and group). Never roll or derive new bonuses here.
 * Both disarm input paths pass the same resolved result to this function.
 */
namespace GridDungeonTrapDisarmFeedback
{
    inline FText Format(const FText& Disarmer, const FRPGSkillCheckResult& Check,
        bool bTriggeredOnFailure = false)
    {
        if (!Check.bResolved)
        {
            return FText::GetEmpty();
        }

        const FText Breakdown = FText::Format(
            NSLOCTEXT("TRAP0171", "DisarmBreakdown",
                "(1d20)={0} + (Piège / désamorçage)={1} + (DEX)={2} + (Classe)={3} + (Groupe)={4} = {5}"),
            FText::AsNumber(Check.Roll),
            FText::AsNumber(Check.Rank),
            FText::AsNumber(Check.AttributeModifier),
            FText::AsNumber(Check.ClassModifier),
            FText::AsNumber(Check.GroupModifier),
            FText::AsNumber(Check.Total));

        const FText Outcome = Check.bSuccess
            ? FText::Format(NSLOCTEXT("TRAP0171", "DisarmSuccessDifficulty",
                  "Difficulté : {0} — Réussite !"), FText::AsNumber(Check.Difficulty))
            : Check.bSafeFailure
                ? FText::Format(NSLOCTEXT("TRAP018", "DisarmSafeFailure",
                      "Difficulté : {0} — Échec sûr ! Le piège reste armé."),
                      FText::AsNumber(Check.Difficulty))
                : bTriggeredOnFailure
                    ? FText::Format(NSLOCTEXT("TRAP018", "DisarmTriggeredFailure",
                          "Difficulté : {0} — Échec ! Le piège se déclenche !"),
                          FText::AsNumber(Check.Difficulty))
                    : FText::Format(NSLOCTEXT("TRAP0171", "DisarmFailureDifficulty",
                          "Difficulté : {0} — Échec !"),
                          FText::AsNumber(Check.Difficulty));

        const FText& Pattern = Check.bSuccess
            ? NSLOCTEXT("TRAP0171", "DisarmSuccessThreeLines",
                "{0} désamorce le piège.\n{1}\n{2}")
            : NSLOCTEXT("TRAP0171", "DisarmFailureThreeLines",
                "{0} échoue à désamorcer le piège.\n{1}\n{2}");
        return FText::Format(Pattern, Disarmer, Breakdown, Outcome);
    }
}
