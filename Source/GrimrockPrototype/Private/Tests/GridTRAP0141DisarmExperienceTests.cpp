#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GridDungeonTrapRewardRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTRAP0141DisarmExperienceScaling,
    "Grimrock.TRAP01.4.1.DisarmExperienceScaling",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTRAP0141DisarmExperienceScaling::RunTest(const FString&)
{
    TestEqual(TEXT("Invalid zero DC cannot award XP"),
        FGridDungeonTrapRewardRules::GetDisarmExperience(0), 0);
    TestEqual(TEXT("Invalid negative DC cannot award XP"),
        FGridDungeonTrapRewardRules::GetDisarmExperience(-1), 0);
    TestEqual(TEXT("Very easy DC5 awards group pool 10"),
        FGridDungeonTrapRewardRules::GetDisarmExperience(5), 10);
    TestEqual(TEXT("Easy DC10 awards group pool 20"),
        FGridDungeonTrapRewardRules::GetDisarmExperience(10), 20);
    TestEqual(TEXT("Moderate DC15 awards group pool 30"),
        FGridDungeonTrapRewardRules::GetDisarmExperience(15), 30);
    TestEqual(TEXT("Hard DC20 awards group pool 40"),
        FGridDungeonTrapRewardRules::GetDisarmExperience(20), 40);
    TestEqual(TEXT("Very hard DC30 awards group pool 60"),
        FGridDungeonTrapRewardRules::GetDisarmExperience(30), 60);
    TestEqual(TEXT("Maximum legal DC40 awards group pool 80"),
        FGridDungeonTrapRewardRules::GetDisarmExperience(40), 80);
    TestEqual(TEXT("Invalid difficulty above maximum cannot award XP"),
        FGridDungeonTrapRewardRules::GetDisarmExperience(41), 0);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
