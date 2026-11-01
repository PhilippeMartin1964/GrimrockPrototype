#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/GridWorldTooltipPlacement.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTRAP0193TooltipQuadrants,
    "Grimrock.TRAP01.9.3.WorldTooltipQuadrants",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTRAP0193TooltipQuadrants::RunTest(const FString&)
{
    using FPlacement = FGridWorldTooltipPlacement;
    const FVector2D Viewport(1127.0, 450.0);
    const FVector2D Size(400.0, 120.0);
    auto VerifyInBounds = [&](const TCHAR* Label, const FPlacement& Placement, const FVector2D& Tooltip)
    {
        TestTrue(FString::Printf(TEXT("%s: respects left/top margins"), Label),
            Placement.Position.X >= 8.0 && Placement.Position.Y >= 8.0);
        TestTrue(FString::Printf(TEXT("%s: respects right/bottom margins"), Label),
            Placement.Position.X + Tooltip.X <= Viewport.X - 8.0 &&
            Placement.Position.Y + Tooltip.Y <= Viewport.Y - 8.0);
    };

    const FPlacement Center = FPlacement::Resolve(FVector2D(650.0, 250.0), Size, Viewport);
    TestEqual(TEXT("Default position is upper-left of mouse"),
        Center.Quadrant, EGridWorldTooltipQuadrant::UpperLeft);
    TestEqual(TEXT("Default upper-left X"), Center.Position.X, 226.0);
    TestEqual(TEXT("Default upper-left Y"), Center.Position.Y, 106.0);
    VerifyInBounds(TEXT("Default"), Center, Size);

    // Screenshot-like: a wide trap hint at mid-screen misses the left edge by
    // only a few pixels. Clamp gently; do not jump right and obscure the view.
    const FVector2D WideSize(529.0, 150.0);
    const FPlacement Wide = FPlacement::Resolve(FVector2D(545.0, 245.0), WideSize, Viewport);
    TestEqual(TEXT("Wide tooltip retains upper-left preference"),
        Wide.Quadrant, EGridWorldTooltipQuadrant::UpperLeft);
    TestEqual(TEXT("Wide tooltip is gently clamped to left margin"), Wide.Position.X, 8.0);
    TestTrue(TEXT("Mouse remains to the right of the wide tooltip"),
        Wide.Position.X + WideSize.X < 545.0);
    TestTrue(TEXT("Mouse remains below the wide tooltip"),
        Wide.Position.Y + WideSize.Y < 245.0);
    VerifyInBounds(TEXT("Wide tooltip"), Wide, WideSize);

    const FPlacement Left = FPlacement::Resolve(FVector2D(60.0, 250.0), Size, Viewport);
    TestEqual(TEXT("Near left edge, flip to upper-right"),
        Left.Quadrant, EGridWorldTooltipQuadrant::UpperRight);
    VerifyInBounds(TEXT("Left edge"), Left, Size);

    const FPlacement Top = FPlacement::Resolve(FVector2D(650.0, 25.0), Size, Viewport);
    TestEqual(TEXT("Near top edge, flip below-left"),
        Top.Quadrant, EGridWorldTooltipQuadrant::LowerLeft);
    VerifyInBounds(TEXT("Top edge"), Top, Size);

    const FPlacement TopLeft = FPlacement::Resolve(FVector2D(60.0, 25.0), Size, Viewport);
    TestEqual(TEXT("Near top-left corner, flip below-right"),
        TopLeft.Quadrant, EGridWorldTooltipQuadrant::LowerRight);
    VerifyInBounds(TEXT("Top-left corner"), TopLeft, Size);

    const FPlacement BottomRight = FPlacement::Resolve(FVector2D(1070.0, 425.0), Size, Viewport);
    TestEqual(TEXT("Near bottom-right corner, upper-left still fits"),
        BottomRight.Quadrant, EGridWorldTooltipQuadrant::UpperLeft);
    VerifyInBounds(TEXT("Bottom-right corner"), BottomRight, Size);

    const FPlacement SmallViewport = FPlacement::Resolve(
        FVector2D(60.0, 25.0), FVector2D(600.0, 300.0), FVector2D(240.0, 140.0));
    TestTrue(TEXT("Oversized tooltip keeps non-negative placement"),
        SmallViewport.Position.X >= 8.0 && SmallViewport.Position.Y >= 8.0);

    const FPlacement First = FPlacement::Resolve(FVector2D(650.0, 250.0), Size, Viewport);
    const FPlacement Second = FPlacement::Resolve(FVector2D(650.0, 250.0), Size, Viewport);
    TestEqual(TEXT("Pure layout is deterministic"), First.Position, Second.Position);
    TestEqual(TEXT("Pure layout keeps the same quadrant"), First.Quadrant, Second.Quadrant);
    return true;
}

#endif
