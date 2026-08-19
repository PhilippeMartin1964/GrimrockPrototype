#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Monsters/GridMonsterTypes.h"

#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON1733ProjectileSourceContractTest,
    "Grimrock.Monsters.MON17.3.3.ProjectileSourceContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1733ProjectileSourceContractTest::RunTest (const FString& Parameters)
{
    (void)Parameters;

    FGridMonsterAttackDefinition Attack;
    Attack.AttackId = TEXT ("Attack_TestProjectile");
    Attack.Delivery = EGridMonsterAttackDelivery::Projectile;
    Attack.MinRangeCells = 2;
    Attack.RangeCells = 6;

    TestTrue (
        TEXT ("Legacy projectile definitions remain valid without a source socket"),
        Attack.IsValidDefinition ());
    TestTrue (
        TEXT ("Projectile source socket defaults to None"),
        Attack.ProjectileSourceSocketName.IsNone ());
    TestEqual (
        TEXT ("Projectile source offset defaults to zero"),
        Attack.ProjectileSourceOffset,
        FVector::ZeroVector);

    Attack.ProjectileSourceSocketName = TEXT ("ProjectileSource");
    Attack.ProjectileSourceOffset = FVector (5.0f, -2.0f, 8.0f);
    TestTrue (
        TEXT ("An authored source socket and finite local offset remain valid"),
        Attack.IsValidDefinition ());

    FString Error;
    Attack.ProjectileSourceOffset.X =
        std::numeric_limits<float>::quiet_NaN ();
    TestFalse (
        TEXT ("Non-finite projectile source offsets are rejected"),
        Attack.ValidateDefinition (Error));
    TestTrue (
        TEXT ("Validation identifies ProjectileSourceOffset"),
        Error.Contains (TEXT ("ProjectileSourceOffset")));

    return true;
}

#endif
