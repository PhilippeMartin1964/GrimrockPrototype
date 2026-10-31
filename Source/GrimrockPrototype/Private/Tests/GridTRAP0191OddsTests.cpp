#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GridDungeonTrapDisarmOdds.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTRAP0191ExactD20Odds,
    "Grimrock.TRAP01.9.1.ExactD20Probabilities",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTRAP0191ExactD20Odds::RunTest(const FString&)
{
    const FGridDungeonTrapDisarmOdds Start =
        FGridDungeonTrapDisarmOdds::Calculate(19, 5, 2, false);
    TestEqual(TEXT("Seven winning faces out of 20 = 35%"), Start.SuccessPercent, 35);
    TestEqual(TEXT("Two protected near-miss faces = 10%"), Start.SafeFailurePercent, 10);
    TestEqual(TEXT("Other eleven faces trigger = 55%"), Start.TriggerPercent, 55);

    const FGridDungeonTrapDisarmOdds Improved =
        FGridDungeonTrapDisarmOdds::Calculate(19, 9, 2, false);
    TestEqual(TEXT("Improved character earns 55% success"), Improved.SuccessPercent, 55);
    TestEqual(TEXT("Safe margin remains a separate risk bucket"), Improved.SafeFailurePercent, 10);
    TestEqual(TEXT("Improvement reduces trigger risk"), Improved.TriggerPercent, 35);

    const FGridDungeonTrapDisarmOdds Used =
        FGridDungeonTrapDisarmOdds::Calculate(19, 5, 2, true);
    TestEqual(TEXT("Spent talent does not affect success"), Used.SuccessPercent, 35);
    TestEqual(TEXT("Spent talent has zero safe outcomes"), Used.SafeFailurePercent, 0);
    TestEqual(TEXT("Spent talent now risks 65% activations"), Used.TriggerPercent, 65);

    const FGridDungeonTrapDisarmOdds Easy =
        FGridDungeonTrapDisarmOdds::Calculate(1, 40, 2, false);
    TestEqual(TEXT("No fabricated natural-1 failure"), Easy.SuccessPercent, 100);
    TestEqual(TEXT("Guaranteed success never triggers"), Easy.TriggerPercent, 0);
    const FGridDungeonTrapDisarmOdds Impossible =
        FGridDungeonTrapDisarmOdds::Calculate(40, -5, 2, false);
    TestEqual(TEXT("No fabricated natural-20 success"), Impossible.SuccessPercent, 0);
    TestEqual(TEXT("Impossible check always triggers"), Impossible.TriggerPercent, 100);

    for (int32 DC = 1; DC <= 40; ++DC)
    {
        for (int32 Fixed = -10; Fixed <= 20; ++Fixed)
        {
            for (const bool bSpent : { false, true })
            {
                const FGridDungeonTrapDisarmOdds Odds =
                    FGridDungeonTrapDisarmOdds::Calculate(DC, Fixed, 2, bSpent);
                if (!TestEqual(TEXT("Every valid odds distribution totals 100"),
                        Odds.SuccessPercent + Odds.SafeFailurePercent + Odds.TriggerPercent, 100))
                {
                    return false;
                }
            }
        }
    }
    return true;
}
#endif
