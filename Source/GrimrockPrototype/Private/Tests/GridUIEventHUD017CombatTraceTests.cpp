#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/Combat/GridCombatResolver.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD017AttackTraceTest, "Grimrock.UI.EventHUD01.CombatTrace.AttackPipeline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD017AttackTraceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridAttackSourceStats Source;
	Source.Accuracy = 3;
	Source.DamageBonus = 2;
	Source.RawDamagePercent = 150;
	Source.DamageMultiplier = 1.25f;
	Source.CriticalChancePercent = 5;
	Source.CriticalDamagePercent = 200;

	FGridAttackTargetStats Target;
	Target.Evasion = 4;
	Target.CurrentHealth = 50;
	Target.PhysicalArmor = 5;
	Target.MagicalArmor = 7;
	Target.ResistancePercent = 20;
	Target.DamageMultiplier = 0.8f;

	FGridAttackDefinition Attack;
	Attack.DamageType = EGridDamageType::Physical;
	Attack.PhysicalSubtype = EGridPhysicalDamageSubtype::Slashing;
	Attack.MinDamage = 1;
	Attack.MaxDamage = 8;
	Attack.AccuracyBonus = 2;

	const FGridAttackResult Result = FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Attack, 15, 6);
	TestTrue(TEXT("The attack hits"), Result.bHit);
	TestFalse(TEXT("The attack is not critical"), Result.bCriticalHit);
	TestTrue(TEXT("Trace is valid"), Result.Trace.bValid);

	const FGridCombatAttackTrace& AttackTrace = Result.Trace.Attack;
	TestTrue(TEXT("Attack roll is used"), AttackTrace.Roll.bUsed);
	TestEqual(TEXT("Attack roll is a die"), AttackTrace.Roll.Kind, EGridCombatRollTraceKind::Die);
	TestEqual(TEXT("Attack roll uses one die"), AttackTrace.Roll.DiceCount, 1);
	TestEqual(TEXT("Attack roll uses d20"), AttackTrace.Roll.DiceSides, 20);
	TestEqual(TEXT("Natural attack roll is retained"), AttackTrace.Roll.FinalValue, 15);
	TestEqual(TEXT("Source accuracy is retained"), AttackTrace.SourceAccuracy, 3);
	TestEqual(TEXT("Attack accuracy bonus is retained"), AttackTrace.AttackAccuracyBonus, 2);
	TestEqual(TEXT("Attack total is retained"), AttackTrace.FinalValue, 20);
	TestTrue(TEXT("Attack can critically hit"), AttackTrace.bCanCriticalHit);
	TestEqual(TEXT("Critical chance is retained"), AttackTrace.CriticalChancePercent, 5);
	TestEqual(TEXT("Five percent crit maps to natural twenty"), AttackTrace.CriticalMinimumRoll, 20);

	const FGridCombatDefenseTrace& Defense = Result.Trace.Defense;
	TestEqual(TEXT("Defense base is retained"), Defense.BaseValue, 10);
	TestEqual(TEXT("Evasion is retained"), Defense.EvasionModifier, 4);
	TestEqual(TEXT("Defense total is retained"), Defense.FinalValue, 14);

	const FGridCombatDamageTrace& Damage = Result.Trace.Damage;
	TestTrue(TEXT("Damage pipeline resolved"), Damage.bResolved);
	TestFalse(TEXT("Ordinary attack is not direct damage"), Damage.bDirectDamage);
	TestEqual(TEXT("Damage randomizer is an authored range"), Damage.Roll.Kind, EGridCombatRollTraceKind::Range);
	TestEqual(TEXT("Damage range minimum is retained"), Damage.Roll.Minimum, 1);
	TestEqual(TEXT("Damage range maximum is retained"), Damage.Roll.Maximum, 8);
	TestEqual(TEXT("Damage roll is retained"), Damage.Roll.FinalValue, 6);
	TestEqual(TEXT("Source damage bonus is retained"), Damage.SourceDamageBonus, 2);
	TestEqual(TEXT("Base damage is retained"), Damage.BaseDamage, 8);
	TestEqual(TEXT("Raw damage coefficient is retained"), Damage.RawDamagePercent, 150);
	TestEqual(TEXT("Scaled base damage is retained"), Damage.ScaledBaseDamage, 12);
	TestEqual(TEXT("Non-critical damage uses 100 percent critical coefficient"), Damage.CriticalDamagePercent, 100);
	TestEqual(TEXT("Raw damage is retained"), Damage.RawDamage, 12);
	TestTrue(TEXT("Source multiplier is retained"), FMath::IsNearlyEqual(Damage.SourceDamageMultiplier, 1.25f));
	TestTrue(TEXT("Target multiplier is retained"), FMath::IsNearlyEqual(Damage.TargetDamageMultiplier, 0.8f));
	TestTrue(TEXT("Combined multiplier is retained"), FMath::IsNearlyEqual(Damage.CombinedDamageMultiplier, 1.0f));
	TestEqual(TEXT("Damage after multiplier is retained"), Damage.DamageAfterMultiplier, 12);
	TestEqual(TEXT("Resistance is retained"), Damage.ResistancePercent, 20);
	TestEqual(TEXT("Damage after resistance is retained"), Damage.DamageAfterResistance, 10);
	TestEqual(TEXT("Physical armor before is retained"), Damage.PhysicalArmorBefore, 5);
	TestEqual(TEXT("Physical armor damage is retained"), Damage.PhysicalArmorDamage, 5);
	TestEqual(TEXT("Health before is retained"), Damage.HealthBefore, 50);
	TestEqual(TEXT("Health damage is retained"), Damage.HealthDamage, 5);
	TestEqual(TEXT("Health after is retained"), Damage.HealthAfter, 45);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD017CriticalAndMissTraceTest, "Grimrock.UI.EventHUD01.CombatTrace.CriticalAndMiss",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD017CriticalAndMissTraceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridAttackSourceStats Source;
	Source.CriticalChancePercent = 5;
	Source.CriticalDamagePercent = 250;

	FGridAttackTargetStats Target;
	Target.CurrentHealth = 100;

	FGridAttackDefinition Attack;
	Attack.MinDamage = 1;
	Attack.MaxDamage = 6;

	const FGridAttackResult Miss = FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Attack, 1, 6);
	TestFalse(TEXT("Natural one misses"), Miss.bHit);
	TestTrue(TEXT("Natural-one auto-miss is traced"), Miss.Trace.Attack.bNaturalAutoMiss);
	TestFalse(TEXT("Miss does not claim a used damage roll"), Miss.Trace.Damage.Roll.bUsed);
	TestFalse(TEXT("Miss has no resolved damage pipeline"), Miss.Trace.Damage.bResolved);

	const FGridAttackResult Critical = FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Attack, 20, 4);
	TestTrue(TEXT("Natural twenty hits"), Critical.bHit);
	TestTrue(TEXT("Natural twenty is traced as auto-hit"), Critical.Trace.Attack.bNaturalAutoHit);
	TestTrue(TEXT("Critical is resolved"), Critical.bCriticalHit);
	TestEqual(TEXT("Critical threshold is traced"), Critical.Trace.Attack.CriticalMinimumRoll, 20);
	TestEqual(TEXT("Critical coefficient is traced"), Critical.Trace.Damage.CriticalDamagePercent, 250);
	TestEqual(TEXT("Pre-critical scaled damage is retained"), Critical.Trace.Damage.ScaledBaseDamage, 4);
	TestEqual(TEXT("Critical raw damage is retained"), Critical.Trace.Damage.RawDamage, 10);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD017DirectDamageTraceTest, "Grimrock.UI.EventHUD01.CombatTrace.DirectDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD017DirectDamageTraceTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FGridAttackTargetStats Target;
	Target.CurrentHealth = 30;
	Target.MagicalArmor = 3;
	Target.ResistancePercent = 25;
	Target.DamageMultiplier = 0.5f;

	const FGridAttackResult Result =
		FGridCombatResolver::ResolveDirectDamage(Target, EGridDamageType::Fire, 20);

	TestTrue(TEXT("Direct damage resolves"), Result.bHit);
	TestTrue(TEXT("Direct trace is valid"), Result.Trace.bValid);
	TestTrue(TEXT("Direct damage is identified"), Result.Trace.Damage.bDirectDamage);
	TestFalse(TEXT("Direct damage has no attack roll"), Result.Trace.Attack.Roll.bUsed);
	TestFalse(TEXT("Direct damage has no random damage roll"), Result.Trace.Damage.Roll.bUsed);
	TestEqual(TEXT("Direct raw damage is retained"), Result.Trace.Damage.RawDamage, 20);
	TestEqual(TEXT("Damage after target multiplier is retained"), Result.Trace.Damage.DamageAfterMultiplier, 10);
	TestEqual(TEXT("Damage after resistance is retained"), Result.Trace.Damage.DamageAfterResistance, 8);
	TestEqual(TEXT("Magical armor damage is retained"), Result.Trace.Damage.MagicalArmorDamage, 3);
	TestEqual(TEXT("Health damage is retained"), Result.Trace.Damage.HealthDamage, 5);
	TestEqual(TEXT("Health after is retained"), Result.Trace.Damage.HealthAfter, 25);
	return true;
}

#endif
