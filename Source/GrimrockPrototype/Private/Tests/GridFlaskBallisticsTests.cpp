#include "Misc/AutomationTest.h"
#include "Core/GridFlaskBallistics.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridFlaskBallisticsTest,
    "Grimrock.RPG.FLASK01.BallisticTrajectory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridFlaskBallisticsTest::RunTest(const FString&)
{
    const FVector Start(100.f, 100.f, 110.f);
    for (int32 Cases = 1; Cases <= 4; ++Cases)
    {
        const FVector Target(100.f + 200.f * Cases, 100.f, 14.f);
        FGridFlaskBallistics Shot;
        TestTrue(TEXT("Target reachable at configured launch speed"),
            FGridFlaskBallistics::Solve(Start, Target, 1300.f, 980.f, Shot));
        TestTrue(TEXT("Positive physical flight time"), Shot.DurationSeconds > 0.f);
        TestTrue(TEXT("Starts at launch position"), Shot.PositionAt(0.f).Equals(Start, .01f));
        TestTrue(TEXT("Lands at authored target"), Shot.PositionAt(Shot.DurationSeconds).Equals(Target, .1f));
        const float Quarter = Shot.DurationSeconds * .25f;
        const float ThreeQuarter = Shot.DurationSeconds * .75f;
        TestTrue(TEXT("Monotonic horizontal travel"),
            Shot.PositionAt(Quarter).X < Shot.PositionAt(ThreeQuarter).X);
    }
    FGridFlaskBallistics Impossible;
    TestFalse(TEXT("Reject impossible low velocity rather than fake a parabola"),
        FGridFlaskBallistics::Solve(Start, FVector(900.f, 100.f, 14.f), 100.f, 980.f, Impossible));
    TestFalse(TEXT("Reject zero gravity"),
        FGridFlaskBallistics::Solve(Start, FVector(300.f, 100.f, 14.f), 1300.f, 0.f, Impossible));
    return true;
}
#endif
