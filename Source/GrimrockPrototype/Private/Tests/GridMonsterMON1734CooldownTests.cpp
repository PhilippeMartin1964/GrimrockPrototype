#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Monsters/GridMonsterCombatComponent.h"

namespace
{
    FGridMonsterAttackDefinition MakeCooldownAttack (
        FName AttackId,
        int32 CooldownTurns)
    {
        FGridMonsterAttackDefinition Attack;
        Attack.AttackId = AttackId;
        Attack.MinDamage = 1;
        Attack.MaxDamage = 1;
        Attack.MinRangeCells = 1;
        Attack.RangeCells = 1;
        Attack.ActionPointCost = 1;
        Attack.CooldownTurns = CooldownTurns;
        return Attack;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON1734CooldownLifecycleTest,
    "Grimrock.Monsters.MON17.3.4.CooldownLifecycle",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1734CooldownLifecycleTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    FGridMonsterAttackCooldownState State;
    const FGridMonsterAttackDefinition Attack =
        MakeCooldownAttack (TEXT ("Attack_TestCooldown"), 2);

    TestFalse (
        TEXT ("A fresh attack is available"),
        State.IsOnCooldown (Attack.AttackId));
    TestFalse (
        TEXT ("Cooldown cannot start before the monster has a combat turn"),
        State.CommitAttack (Attack));

    State.BeginTurn ();
    TestEqual (
        TEXT ("First combat activation has serial one"),
        State.GetCurrentTurnSerial (),
        1);
    TestTrue (
        TEXT ("Using a two-turn cooldown starts cooldown state"),
        State.CommitAttack (Attack));
    TestTrue (
        TEXT ("The attack is unavailable after use on turn N"),
        State.IsOnCooldown (Attack.AttackId));

    State.BeginTurn ();
    TestTrue (
        TEXT ("CooldownTurns=2 blocks turn N+1"),
        State.IsOnCooldown (Attack.AttackId));

    State.BeginTurn ();
    TestTrue (
        TEXT ("CooldownTurns=2 blocks turn N+2"),
        State.IsOnCooldown (Attack.AttackId));

    State.BeginTurn ();
    TestFalse (
        TEXT ("CooldownTurns=2 is available again on turn N+3"),
        State.IsOnCooldown (Attack.AttackId));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON1734CooldownIsolationTest,
    "Grimrock.Monsters.MON17.3.4.CooldownIsolation",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1734CooldownIsolationTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    FGridMonsterAttackCooldownState State;
    const FGridMonsterAttackDefinition CooldownAttack =
        MakeCooldownAttack (TEXT ("Attack_A"), 1);
    const FGridMonsterAttackDefinition OtherAttack =
        MakeCooldownAttack (TEXT ("Attack_B"), 3);
    const FGridMonsterAttackDefinition ZeroCooldownAttack =
        MakeCooldownAttack (TEXT ("Attack_Zero"), 0);

    State.BeginTurn ();
    TestTrue (
        TEXT ("Attack A starts its authored cooldown"),
        State.CommitAttack (CooldownAttack));
    TestTrue (
        TEXT ("Attack A is blocked"),
        State.IsOnCooldown (CooldownAttack.AttackId));
    TestFalse (
        TEXT ("Attack B remains independently available"),
        State.IsOnCooldown (OtherAttack.AttackId));

    TestFalse (
        TEXT ("CooldownTurns=0 never starts a cooldown"),
        State.CommitAttack (ZeroCooldownAttack));
    TestFalse (
        TEXT ("CooldownTurns=0 remains available on the same activation"),
        State.IsOnCooldown (ZeroCooldownAttack.AttackId));

    State.BeginTurn ();
    TestTrue (
        TEXT ("CooldownTurns=1 blocks exactly turn N+1"),
        State.IsOnCooldown (CooldownAttack.AttackId));

    State.BeginTurn ();
    TestFalse (
        TEXT ("CooldownTurns=1 is available on turn N+2"),
        State.IsOnCooldown (CooldownAttack.AttackId));

    State.Reset ();
    TestEqual (
        TEXT ("Reset clears the combat turn serial"),
        State.GetCurrentTurnSerial (),
        0);
    TestFalse (
        TEXT ("Reset clears all attack cooldowns"),
        State.IsOnCooldown (CooldownAttack.AttackId));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMonsterMON1734GoblinZeroCooldownContractTest,
    "Grimrock.Monsters.MON17.3.4.GoblinZeroCooldownContract",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMonsterMON1734GoblinZeroCooldownContractTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;

    FGridMonsterAttackCooldownState State;
    FGridMonsterAttackDefinition ThrowKnife;
    ThrowKnife.AttackId = TEXT ("Attack_ThrowKnife");
    ThrowKnife.MinDamage = 2;
    ThrowKnife.MaxDamage = 5;
    ThrowKnife.MinRangeCells = 2;
    ThrowKnife.RangeCells = 6;
    ThrowKnife.Delivery = EGridMonsterAttackDelivery::Projectile;
    ThrowKnife.bRequiresLineOfSight = true;
    ThrowKnife.ActionPointCost = 2;
    ThrowKnife.CooldownTurns = 0;
    ThrowKnife.Priority = 100;

    TestTrue (
        TEXT ("Current Goblin ThrowKnife definition remains valid with cooldown zero"),
        ThrowKnife.IsValidDefinition ());

    State.BeginTurn ();
    TestTrue (
        TEXT ("ThrowKnife is initially available"),
        State.IsAttackAvailable (ThrowKnife));
    TestFalse (
        TEXT ("ThrowKnife cooldown zero does not create runtime cooldown"),
        State.CommitAttack (ThrowKnife));
    TestTrue (
        TEXT ("ThrowKnife remains available after use when cooldown is zero"),
        State.IsAttackAvailable (ThrowKnife));

    State.BeginTurn ();
    TestTrue (
        TEXT ("ThrowKnife remains available on the next Goblin turn"),
        State.IsAttackAvailable (ThrowKnife));

    return true;
}

#endif
