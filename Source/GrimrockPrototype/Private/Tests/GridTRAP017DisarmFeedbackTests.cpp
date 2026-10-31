#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GridDungeonTrapDisarmFeedback.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTRAP017DisarmFeedbackTest,
    "Grimrock.TRAP01.7.DisarmFeedbackBreakdown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTRAP017DisarmFeedbackTest::RunTest(const FString&)
{
    FRPGSkillCheckResult Check;
    TestTrue(TEXT("Rejected/unresolved check has no feedback"),
        GridDungeonTrapDisarmFeedback::Format(FText::FromString(TEXT("Elias")), Check).IsEmpty());

    Check.bResolved = true;
    Check.bSuccess = true;
    Check.GoverningAttribute = ERPGSkillGoverningAttribute::Dexterity;
    Check.SkillId = TEXT("Skill_Traps");
    Check.Roll = 12;
    Check.Rank = 3;
    Check.AttributeModifier = 2;
    Check.ProgressionModifier = -1;
    Check.Total = 16;
    Check.Difficulty = 14;

    const FString Successful = GridDungeonTrapDisarmFeedback::Format(
        FText::FromString(TEXT("Elias")), Check).ToString();
    TestTrue(TEXT("Success identifies actual operator"), Successful.Contains(TEXT("Elias")));
    TestTrue(TEXT("Success explains that the trap was disarmed"), Successful.Contains(TEXT("désamorce")));
    TestTrue(TEXT("Success reports d20"), Successful.Contains(TEXT("d20")));
    TestTrue(TEXT("Success reports rank"), Successful.Contains(TEXT("rang")));
    TestTrue(TEXT("Success reports governing Dexterity"), Successful.Contains(TEXT("DEX")));
    TestTrue(TEXT("Success reports signed progression adjustment"), Successful.Contains(TEXT("-1")));
    TestTrue(TEXT("Success reports difficulty"), Successful.Contains(TEXT("difficulté")));
    TestTrue(TEXT("Success reports total"), Successful.Contains(TEXT("16")));

    Check.bSuccess = false;
    const FString Failed = GridDungeonTrapDisarmFeedback::Format(
        FText::FromString(TEXT("Elias")), Check).ToString();
    TestTrue(TEXT("Failure reports failure rather than success"), Failed.Contains(TEXT("échoue")));
    TestTrue(TEXT("Failure keeps transparent breakdown"), Failed.Contains(TEXT("d20")));
    TestTrue(TEXT("Both outcomes use the actual same difficulty"), Failed.Contains(TEXT("14")));
    return true;
}
#endif
