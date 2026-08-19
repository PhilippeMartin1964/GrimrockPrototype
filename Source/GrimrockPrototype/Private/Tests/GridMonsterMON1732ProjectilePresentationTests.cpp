#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridCombatProjectileActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON1732ProjectileTimingTest,
    "Grimrock.Monsters.MON17.3.2.ProjectileTiming",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1732ProjectileTimingTest::RunTest (const FString& Parameters)
{
    (void)Parameters;

    TestTrue (
        TEXT ("A 0.20s projectile for a 0.25s impact launches after 0.05s"),
        FMath::IsNearlyEqual (
            AGridCombatProjectileActor::CalculateLaunchDelay (0.25f, 0.20f),
            0.05f,
            KINDA_SMALL_NUMBER));

    TestEqual (
        TEXT ("Travel longer than impact time launches immediately"),
        AGridCombatProjectileActor::CalculateLaunchDelay (0.10f, 0.20f),
        0.0f);

    TestEqual (
        TEXT ("Negative authored times are clamped to immediate launch"),
        AGridCombatProjectileActor::CalculateLaunchDelay (-1.0f, -2.0f),
        0.0f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON1732ProjectileTrajectoryTest,
    "Grimrock.Monsters.MON17.3.2.ProjectileTrajectory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1732ProjectileTrajectoryTest::RunTest (const FString& Parameters)
{
    (void)Parameters;

    const FVector Source (10.0f, 20.0f, 30.0f);
    const FVector Target (110.0f, 60.0f, 50.0f);

    TestEqual (
        TEXT ("Alpha zero returns the source"),
        AGridCombatProjectileActor::EvaluateTrajectoryLocation (
            Source,
            Target,
            0.0f),
        Source);

    TestEqual (
        TEXT ("Alpha one returns the target"),
        AGridCombatProjectileActor::EvaluateTrajectoryLocation (
            Source,
            Target,
            1.0f),
        Target);

    TestEqual (
        TEXT ("Half travel is a linear midpoint"),
        AGridCombatProjectileActor::EvaluateTrajectoryLocation (
            Source,
            Target,
            0.5f),
        FVector (60.0f, 40.0f, 40.0f));

    TestEqual (
        TEXT ("Negative alpha clamps to source"),
        AGridCombatProjectileActor::EvaluateTrajectoryLocation (
            Source,
            Target,
            -2.0f),
        Source);

    TestEqual (
        TEXT ("Alpha above one clamps to target"),
        AGridCombatProjectileActor::EvaluateTrajectoryLocation (
            Source,
            Target,
            3.0f),
        Target);

    return true;
}

#endif
