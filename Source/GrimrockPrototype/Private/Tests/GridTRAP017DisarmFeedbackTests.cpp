#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GridDungeonTrapDisarmFeedback.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTRAP017DisarmFeedbackTest,
    "Grimrock.TRAP01.7.DisarmFeedbackBreakdown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTRAP017DisarmFeedbackTest::RunTest(const FString&)
{
    const FText Operator = FText::FromString(TEXT("Elias"));
    FRPGSkillCheckResult Check;
    TestTrue(TEXT("Rejected or unresolved check has no feedback"),
        GridDungeonTrapDisarmFeedback::Format(Operator, Check).IsEmpty());

    // Exactly the player-approved example: 12 + 3 + 2 + 0 + 0 = 17.
    Check.bResolved = true;
    Check.bSuccess = true;
    Check.GoverningAttribute = ERPGSkillGoverningAttribute::Dexterity;
    Check.SkillId = TEXT("Skill_Traps");
    Check.Roll = 12;
    Check.Rank = 3;
    Check.AttributeModifier = 2;
    Check.ClassModifier = 0;
    Check.GroupModifier = 0;
    Check.ProgressionModifier = 0;
    Check.Total = 17;
    Check.Difficulty = 14;

    const FString Success = GridDungeonTrapDisarmFeedback::Format(Operator, Check).ToString();
    TestTrue(TEXT("First line identifies the disarmer"),
        Success.StartsWith(TEXT("Elias désamorce le piège.")));
    TestTrue(TEXT("Five visible components include class and group zeros"),
        Success.Contains(TEXT("(1d20)=12 + (Piège / désamorçage)=3 + (DEX)=2 + (Classe)=0 + (Groupe)=0 = 17")));
    TestTrue(TEXT("Final line reports actual difficulty and success"),
        Success.Contains(TEXT("Difficulté : 14 — Réussite !")));
    TArray<FString> Lines;
    Success.ParseIntoArrayLines(Lines);
    TestEqual(TEXT("One event message contains exactly three lines"), Lines.Num(), 3);

    // Nonzero positive and negative modifiers are displayed, not recomputed.
    Check.bSuccess = false;
    Check.ClassModifier = 2;
    Check.GroupModifier = -1;
    Check.ProgressionModifier = 1;
    Check.Total = 18;
    Check.Difficulty = 20;
    const FString Failure = GridDungeonTrapDisarmFeedback::Format(Operator, Check).ToString();
    TestTrue(TEXT("Failure identifies actor and outcome"),
        Failure.StartsWith(TEXT("Elias échoue à désamorcer le piège.")));
    TestTrue(TEXT("Class and negative group contribution remain separate"),
        Failure.Contains(TEXT("(Classe)=2 + (Groupe)=-1 = 18")));
    TestTrue(TEXT("Failure verdict reports true difficulty"),
        Failure.Contains(TEXT("Difficulté : 20 — Échec !")));

    // Keep the formatting contract even for standalone zero-bonus checks.
    Check.ClassModifier = 0;
    Check.GroupModifier = 0;
    Check.ProgressionModifier = 0;
    Check.Total = 17;
    const FString ZeroBonus = GridDungeonTrapDisarmFeedback::Format(Operator, Check).ToString();
    TestTrue(TEXT("Both zero bonuses are always rendered"),
        ZeroBonus.Contains(TEXT("(Classe)=0 + (Groupe)=0 = 17")));
    return true;
}
#endif
