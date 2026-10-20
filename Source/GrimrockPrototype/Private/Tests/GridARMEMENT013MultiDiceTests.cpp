#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridCombatEventFeedFormatter.h"
#include "Runtime/Combat/GridCombatResolver.h"

namespace ARMEMENT013
{
	FGridAttackDefinition MakeDiceAttack()
	{
		FGridAttackDefinition Attack;
		Attack.DamageType = EGridDamageType::Physical;
		Attack.PhysicalSubtype = EGridPhysicalDamageSubtype::Slashing;
		Attack.DamageDice.bEnabled = true;
		Attack.DamageDice.DiceCount = 2;
		Attack.DamageDice.DiceSides = 8;
		Attack.DamageDice.FlatBonus = 4;
		Attack.MinDamage = 6;
		Attack.MaxDamage = 20;
		return Attack;
	}

	FGridAttackTargetStats MakeTarget()
	{
		FGridAttackTargetStats Target;
		Target.CurrentHealth = 200;
		return Target;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT013DiceValidationTest,
	"Grimrock.RPG.ARMEMENT01.3.DiceValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT013DiceValidationTest::RunTest(const FString&)
{
	FGridAttackDefinition Attack = ARMEMENT013::MakeDiceAttack();
	TestTrue(TEXT("2d8+4 profile is valid"), Attack.IsValid());
	for (const int32 Allowed : { 4, 6, 8, 10, 12, 20 })
	{
		Attack.DamageDice.DiceSides = Allowed;
		Attack.MaxDamage = Attack.DamageDice.GetMaximumDamage();
		TestTrue(TEXT("Allowed RPG damage die"), Attack.IsValid());
	}
	for (const int32 Disallowed : { 3, 5, 7, 15, 100 })
	{
		Attack.DamageDice.DiceSides = Disallowed;
		TestFalse(TEXT("Nonstandard RPG damage die rejected"), Attack.IsValid());
	}
	Attack = ARMEMENT013::MakeDiceAttack();
	Attack.DamageDice.DiceCount = 0;
	TestFalse(TEXT("Zero dice rejected"), Attack.IsValid());
	Attack.DamageDice.DiceCount = 33;
	TestFalse(TEXT("More than 32 dice rejected"), Attack.IsValid());
	Attack = ARMEMENT013::MakeDiceAttack();
	Attack.DamageDice.FlatBonus = -1;
	TestFalse(TEXT("Negative constant rejected"), Attack.IsValid());
	Attack.DamageDice.FlatBonus = MAX_int32;
	TestFalse(TEXT("Overflow rejected"), Attack.IsValid());
	Attack = ARMEMENT013::MakeDiceAttack();
	Attack.MinDamage = 5;
	TestFalse(TEXT("Mismatched minimum rejected"), Attack.IsValid());
	Attack = ARMEMENT013::MakeDiceAttack();
	Attack.MaxDamage = 21;
	TestFalse(TEXT("Mismatched maximum rejected"), Attack.IsValid());
	Attack.DamageDice.bEnabled = false;
	TestTrue(TEXT("Legacy range ignores disabled dice"), Attack.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT013BoundsTest,
	"Grimrock.RPG.ARMEMENT01.3.BoundsAndMean",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT013BoundsTest::RunTest(const FString&)
{
	FGridDamageDiceProfile Dice = ARMEMENT013::MakeDiceAttack().DamageDice;
	TestEqual(TEXT("2d8+4 minimum"), Dice.GetMinimumDamage(), 6);
	TestEqual(TEXT("2d8+4 maximum"), Dice.GetMaximumDamage(), 20);
	TestTrue(TEXT("2d8+4 expected value"), FMath::IsNearlyEqual(Dice.GetExpectedDamage(), 13.0));
	Dice.DiceSides = 6;
	Dice.FlatBonus = 3;
	TestEqual(TEXT("2d6+3 minimum"), Dice.GetMinimumDamage(), 5);
	TestEqual(TEXT("2d6+3 maximum"), Dice.GetMaximumDamage(), 15);
	TestTrue(TEXT("2d6+3 expected value"), FMath::IsNearlyEqual(Dice.GetExpectedDamage(), 10.0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT013DeterministicTest,
	"Grimrock.RPG.ARMEMENT01.3.DeterministicIndividualRolls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT013DeterministicTest::RunTest(const FString&)
{
	const FGridAttackSourceStats Source;
	const FGridAttackTargetStats Target = ARMEMENT013::MakeTarget();
	const FGridAttackDefinition Attack = ARMEMENT013::MakeDiceAttack();
	const FGridAttackResult Result = FGridCombatResolver::ResolveAttackFromDiceRolls(
		Source, Target, Attack, 15, { 3, 7 });
	TestTrue(TEXT("Hit"), Result.bHit);
	TestTrue(TEXT("Damage resolved"), Result.Trace.Damage.bResolved);
	const FGridCombatRollTrace& Roll = Result.Trace.Damage.Roll;
	TestEqual(TEXT("Real dice kind"), Roll.Kind, EGridCombatRollTraceKind::Die);
	TestEqual(TEXT("Count"), Roll.DiceCount, 2);
	TestEqual(TEXT("Sides"), Roll.DiceSides, 8);
	TestEqual(TEXT("K"), Roll.DiceFlatBonus, 4);
	TestEqual(TEXT("Trace minimum"), Roll.Minimum, 6);
	TestEqual(TEXT("Trace maximum"), Roll.Maximum, 20);
	TestEqual(TEXT("Individual roll count"), Roll.IndividualRolls.Num(), 2);
	if (Roll.IndividualRolls.Num() == 2)
	{
		TestEqual(TEXT("First die"), Roll.IndividualRolls[0], 3);
		TestEqual(TEXT("Second die"), Roll.IndividualRolls[1], 7);
	}
	TestEqual(TEXT("Total 2d8+4"), Roll.FinalValue, 14);
	TestEqual(TEXT("Damage total"), Result.Trace.Damage.BaseDamage, 14);
	TestEqual(TEXT("Authoritative formatted roll"),
		FGridCombatEventFeedFormatter::FormatRollAsDice(Roll).ToString(),
		FString(TEXT("2d8 (3 + 7) + 4 = 14")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT013ExtremesTest,
	"Grimrock.RPG.ARMEMENT01.3.ExtremesAndRejects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT013ExtremesTest::RunTest(const FString&)
{
	const FGridAttackSourceStats Source;
	const FGridAttackTargetStats Target = ARMEMENT013::MakeTarget();
	const FGridAttackDefinition Attack = ARMEMENT013::MakeDiceAttack();
	const FGridAttackResult Low = FGridCombatResolver::ResolveAttackFromDiceRolls(Source, Target, Attack, 15, { 1, 1 });
	const FGridAttackResult High = FGridCombatResolver::ResolveAttackFromDiceRolls(Source, Target, Attack, 15, { 8, 8 });
	TestEqual(TEXT("Both minimum faces"), Low.Trace.Damage.Roll.FinalValue, 6);
	TestEqual(TEXT("Both maximum faces"), High.Trace.Damage.Roll.FinalValue, 20);
	TestFalse(TEXT("Wrong dice count"),
		FGridCombatResolver::ResolveAttackFromDiceRolls(Source, Target, Attack, 15, { 3 }).Trace.bValid);
	TestFalse(TEXT("Out-of-bounds face"),
		FGridCombatResolver::ResolveAttackFromDiceRolls(Source, Target, Attack, 15, { 3, 9 }).Trace.bValid);
	TestFalse(TEXT("Zero face"),
		FGridCombatResolver::ResolveAttackFromDiceRolls(Source, Target, Attack, 15, { 0, 7 }).Trace.bValid);
	TestFalse(TEXT("Scalar injection forbidden for real dice"),
		FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Attack, 15, 14).Trace.bValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT013PipelineTest,
	"Grimrock.RPG.ARMEMENT01.3.MissCriticalAndWD",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT013PipelineTest::RunTest(const FString&)
{
	FGridAttackSourceStats Source;
	Source.CriticalChancePercent = 5;
	Source.CriticalDamagePercent = 200;
	const FGridAttackTargetStats Target = ARMEMENT013::MakeTarget();
	const FGridAttackDefinition Attack = ARMEMENT013::MakeDiceAttack();
	const FGridAttackResult Miss = FGridCombatResolver::ResolveAttackFromDiceRolls(Source, Target, Attack, 1, { 3, 7 });
	TestFalse(TEXT("Natural one misses"), Miss.bHit);
	TestFalse(TEXT("Miss does not expose resolved damage"), Miss.Trace.Damage.Roll.bUsed);
	TestEqual(TEXT("Miss does not expose faces"), Miss.Trace.Damage.Roll.IndividualRolls.Num(), 0);
	const FGridAttackResult Crit = FGridCombatResolver::ResolveAttackFromDiceRolls(Source, Target, Attack, 20, { 3, 7 });
	TestTrue(TEXT("Natural 20 critical"), Crit.bCriticalHit);
	TestEqual(TEXT("Critical raw damage"), Crit.RawDamage, 28);
	Source.DamageBonus = 2;
	Source.RawDamagePercent = 150;
	Source.CriticalChancePercent = 0;
	const FGridAttackResult WD = FGridCombatResolver::ResolveAttackFromDiceRolls(Source, Target, Attack, 15, { 3, 7 });
	TestEqual(TEXT("K applied only once"), WD.Trace.Damage.Roll.FinalValue, 14);
	TestEqual(TEXT("Weapon or source flat bonus applied once"), WD.Trace.Damage.BaseDamage, 16);
	TestEqual(TEXT("Coefficient before critical and armor"), WD.Trace.Damage.ScaledBaseDamage, 24);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT013RngTest,
	"Grimrock.RPG.ARMEMENT01.3.RngOrderAndLegacy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT013RngTest::RunTest(const FString&)
{
	const FGridAttackSourceStats Source;
	const FGridAttackTargetStats Target = ARMEMENT013::MakeTarget();
	const FGridAttackDefinition Attack = ARMEMENT013::MakeDiceAttack();
	FRandomStream Expected(7124);
	FRandomStream Actual(7124);
	const int32 D20 = Expected.RandRange(1, 20);
	const int32 A = Expected.RandRange(1, 8);
	const int32 B = Expected.RandRange(1, 8);
	const FGridAttackResult Result = FGridCombatResolver::ResolveAttack(Source, Target, Attack, Actual);
	TestEqual(TEXT("Attack d20 first"), Result.NaturalAttackRoll, D20);
	TestEqual(TEXT("Exactly two dice consumed after d20"), Actual.RandRange(1, 10000), Expected.RandRange(1, 10000));
	if (Result.bHit)
	{
		TestEqual(TEXT("Total from seeded faces"), Result.Trace.Damage.Roll.FinalValue, A + B + 4);
		TestEqual(TEXT("Correct number of faces"), Result.Trace.Damage.Roll.IndividualRolls.Num(), 2);
		if (Result.Trace.Damage.Roll.IndividualRolls.Num() == 2)
		{
			TestEqual(TEXT("Seeded first face"), Result.Trace.Damage.Roll.IndividualRolls[0], A);
			TestEqual(TEXT("Seeded second face"), Result.Trace.Damage.Roll.IndividualRolls[1], B);
		}
	}

	FGridAttackDefinition Legacy;
	Legacy.MinDamage = 2;
	Legacy.MaxDamage = 8;
	FRandomStream LegacyExpected(907);
	FRandomStream LegacyActual(907);
	const int32 LegacyD20 = LegacyExpected.RandRange(1, 20);
	const int32 LegacyDamage = LegacyExpected.RandRange(2, 8);
	const FGridAttackResult LegacyResult = FGridCombatResolver::ResolveAttack(Source, Target, Legacy, LegacyActual);
	TestEqual(TEXT("Legacy attack d20"), LegacyResult.NaturalAttackRoll, LegacyD20);
	TestEqual(TEXT("Legacy consumes one range sample"), LegacyActual.RandRange(1, 10000), LegacyExpected.RandRange(1, 10000));
	if (LegacyResult.bHit)
	{
		TestEqual(TEXT("Legacy roll preserved"), LegacyResult.Trace.Damage.Roll.FinalValue, LegacyDamage);
		TestEqual(TEXT("Legacy kind remains Range"), LegacyResult.Trace.Damage.Roll.Kind, EGridCombatRollTraceKind::Range);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT013PassiveTest,
	"Grimrock.RPG.ARMEMENT01.3.NoPassiveDice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT013PassiveTest::RunTest(const FString&)
{
	FGridAttackDefinition Passive;
	Passive.MinDamage = 0;
	Passive.MaxDamage = 0;
	TestTrue(TEXT("Legacy zero-damage metadata valid"), Passive.IsValid());
	Passive.DamageDice.bEnabled = true;
	TestFalse(TEXT("No implicit dice for passive equipment"), Passive.IsValid());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
