#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatResolver.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RPG031
{
	FGridCombatActionDefinition MakeAttack(FName ActionId, EGridCombatActionSourcePolicy SourcePolicy = EGridCombatActionSourcePolicy::Ability)
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = ActionId;
		Action.DisplayName = FText::FromName(ActionId);
		Action.ActionType = EGridCombatActionType::MeleeAttack;
		Action.SourcePolicy = SourcePolicy;
		Action.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
		Action.ActionPointCost = 2;
		Action.ResourceCosts.ManaCost = 3;
		Action.RangeCells = 2;
		Action.OffensiveProfile.AttackId = ActionId;
		Action.OffensiveProfile.AttackDefinition.DamageType = EGridDamageType::Physical;
		Action.OffensiveProfile.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::Slashing;
		Action.OffensiveProfile.AttackDefinition.MinDamage = 10;
		Action.OffensiveProfile.AttackDefinition.MaxDamage = 10;
		Action.OffensiveProfile.RangeCells = 2;
		return Action;
	}

	FGridCombatActionCatalogContext MakeContext()
	{
		FGridCombatActionCatalogContext Context;
		Context.CharacterIndex = 0;
		Context.CharacterId = FGuid::NewGuid();
		Context.bCombatActive = true;
		Context.bActiveCombatant = true;
		Context.bEnableClassActionExecutors = true;
		Context.RemainingActionPoints = 6;
		Context.CurrentHealth = 10;
		Context.MaximumHealth = 10;
		Context.CurrentMana = 10;
		Context.MaximumMana = 10;
		return Context;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG031ProfileValidationTest, "Grimrock.RPG.RPG03.1.ProfileValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG031ProfileValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatModifierProfile Profile;
	TestFalse(TEXT("Empty modifier profile is invalid"), Profile.IsValid());

	Profile.AccuracyModifier = 2;
	TestTrue(TEXT("Non-empty profile is valid"), Profile.IsValid());

	Profile.SourcePolicies.Add(EGridCombatActionSourcePolicy::None);
	TestFalse(TEXT("None source filter is invalid"), Profile.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG031ChoiceProjectionTest, "Grimrock.RPG.RPG03.1.ChoiceProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG031ChoiceProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	URPGClassAsset* Class = NewObject<URPGClassAsset>();
	Class->ClassId = TEXT("RPG031_Class");
	Class->HealthAtLevelOne = 10;

	FRPGClassProgressionChoiceDefinition Choice;
	Choice.ChoiceId = TEXT("Talent_RPG031");
	Choice.DisplayName = FText::FromString(TEXT("Modifier talent"));
	Choice.MinimumLevel = 1;
	Choice.PointCost = 1;
	FGridCombatModifierProfile Profile;
	Profile.AccuracyModifier = 2;
	Choice.CombatModifiers.Add(Profile);
	Class->ProgressionChoices.Add(Choice);

	TestTrue(TEXT("Class with valid modifier profile remains valid"), Class->IsValidDefinition());

	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Character.ClassId = Class->ClassId;
	Character.ClassDefinition = Class;
	Character.SelectedClassProgressionChoiceIds.Add(Choice.ChoiceId);

	TArray<FGridCombatModifierProfile> Profiles;
	TestTrue(TEXT("Selected choice modifiers resolve"), FGridCombatModifierResolver::CollectCharacterChoiceModifiers(Character, Profiles));
	TestEqual(TEXT("Exactly one profile is projected"), Profiles.Num(), 1);
	TestEqual(TEXT("Projected accuracy modifier survives"), Profiles[0].AccuracyModifier, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG031ActionProjectionTest, "Grimrock.RPG.RPG03.1.ActionProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG031ActionProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPG031;

	FGridCombatActionDefinition Action = MakeAttack(TEXT("Action_RPG031"));
	FGridCombatActionContribution Contribution;
	Contribution.Definition = Action;
	Contribution.SourceDefinitionId = TEXT("RPG031_Class");
	Contribution.AvailableSourceQuantity = 1;

	FGridCombatModifierProfile Profile;
	Profile.SourcePolicies.Add(EGridCombatActionSourcePolicy::Ability);
	Profile.ActionPointCostModifier = -1;
	Profile.ManaCostModifier = -2;
	Profile.RangeCellsModifier = 1;

	FGridCombatActionCatalogContext Context = MakeContext();
	Context.CombatModifiers.Add(Profile);

	TArray<FGridAvailableCombatAction> Actions;
	FGridCombatActionCatalog::Build(Context, { Contribution }, Actions);
	TestEqual(TEXT("One action remains projected"), Actions.Num(), 1);
	if (Actions.Num() == 1)
	{
		TestEqual(TEXT("Current AP cost is modified"), Actions[0].CurrentActionPointCost, 1);
		TestEqual(TEXT("Current mana cost is modified"), Actions[0].CurrentManaCost, 1);
		TestEqual(TEXT("Projected range is modified"), Actions[0].Definition.RangeCells, 3);
		TestEqual(TEXT("Offensive profile range follows projection"), Actions[0].Definition.OffensiveProfile.RangeCells, 3);
		TestEqual(TEXT("Authored definition remains unchanged"), Action.RangeCells, 2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG031FilterIsolationTest, "Grimrock.RPG.RPG03.1.FilterIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG031FilterIsolationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RPG031;
	const FGridCombatActionDefinition Action = MakeAttack(TEXT("Action_RPG031_Filter"));

	FGridCombatModifierProfile Profile;
	Profile.SourcePolicies.Add(EGridCombatActionSourcePolicy::Spell);
	Profile.AccuracyModifier = 9;

	FGridResolvedCombatModifiers Resolved;
	FGridCombatModifierResolver::Resolve({ Profile }, FGridCombatModifierResolver::MakeActionContext(Action, TEXT("RPG031_Class")), Resolved);
	TestTrue(TEXT("Mismatched source policy contributes nothing"), Resolved.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG031AttackModifierTest, "Grimrock.RPG.RPG03.1.AttackModifiers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG031AttackModifierTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatModifierProfile Profile;
	Profile.DamageTypes.Add(EGridDamageType::Physical);
	Profile.AccuracyModifier = 2;
	Profile.OutgoingDamagePercentModifier = 20;
	Profile.CriticalChancePercentModifier = 10;
	Profile.CriticalDamagePercentModifier = 25;

	const FGridCombatModifierContext Context = FGridCombatModifierResolver::MakeAttackContext(TEXT("Attack_Test"), NAME_None,
		EGridCombatActionSourcePolicy::Ability, EGridCombatActionType::MeleeAttack, EGridDamageType::Physical, EGridPhysicalDamageSubtype::Slashing);
	FGridResolvedCombatModifiers Resolved;
	FGridCombatModifierResolver::Resolve({ Profile }, Context, Resolved);

	FGridAttackSourceStats Source;
	FGridCombatModifierResolver::ApplyOutgoingAttackModifiers(Source, Resolved);
	TestEqual(TEXT("Accuracy applies"), Source.Accuracy, 2);
	TestEqual(TEXT("Critical chance starts at five and gains ten"), Source.CriticalChancePercent, 15);
	TestEqual(TEXT("Critical damage starts at 200 and gains 25"), Source.CriticalDamagePercent, 225);
	TestTrue(TEXT("Outgoing damage multiplier is 1.2"), FMath::IsNearlyEqual(Source.DamageMultiplier, 1.2f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG031CriticalResolutionTest, "Grimrock.RPG.RPG03.1.CriticalResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG031CriticalResolutionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridAttackSourceStats Source;
	Source.CriticalChancePercent = 15;
	Source.CriticalDamagePercent = 225;
	Source.DamageMultiplier = 1.2f;

	FGridAttackTargetStats Target;
	Target.CurrentHealth = 100;

	FGridAttackDefinition Attack;
	Attack.DamageType = EGridDamageType::Physical;
	Attack.PhysicalSubtype = EGridPhysicalDamageSubtype::Slashing;
	Attack.MinDamage = 10;
	Attack.MaxDamage = 10;

	const FGridAttackResult Critical = FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Attack, 18, 10);
	TestTrue(TEXT("15 percent crit band includes natural 18"), Critical.bCriticalHit);
	TestEqual(TEXT("225 percent crit changes raw damage"), Critical.RawDamage, 22);
	TestEqual(TEXT("Outgoing multiplier applies after critical raw damage"), Critical.DamageAfterModifiers, 26);

	const FGridAttackResult NaturalOne = FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Attack, 1, 10);
	TestFalse(TEXT("Natural one still always misses"), NaturalOne.bHit);
	TestFalse(TEXT("Natural one cannot crit"), NaturalOne.bCriticalHit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG031IncomingModifierTest, "Grimrock.RPG.RPG03.1.IncomingModifiers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG031IncomingModifierTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FGridCombatModifierProfile Profile;
	Profile.DamageTypes.Add(EGridDamageType::Fire);
	Profile.EvasionModifier = 3;
	Profile.IncomingDamagePercentModifier = -20;
	Profile.ResistanceModifiers.FireResistance = 25;

	FGridResolvedCombatModifiers Resolved;
	FGridCombatModifierResolver::Resolve({ Profile }, FGridCombatModifierResolver::MakeAttackContext(TEXT("Fire"), NAME_None,
		EGridCombatActionSourcePolicy::Universal, EGridCombatActionType::Ability, EGridDamageType::Fire, EGridPhysicalDamageSubtype::None), Resolved);

	FGridAttackTargetStats Target;
	Target.Evasion = 1;
	Target.CurrentHealth = 100;
	Target.ResistancePercent = 10;
	FGridCombatModifierResolver::ApplyIncomingAttackModifiers(Target, EGridDamageType::Fire, Resolved);

	TestEqual(TEXT("Evasion modifier applies"), Target.Evasion, 4);
	TestEqual(TEXT("Resistance modifier stacks with existing resistance"), Target.ResistancePercent, 35);
	TestTrue(TEXT("Incoming damage modifier becomes target multiplier"), FMath::IsNearlyEqual(Target.DamageMultiplier, 0.8f));

	const FGridAttackResult Damage = FGridCombatResolver::ResolveDirectDamage(Target, EGridDamageType::Fire, 100);
	TestEqual(TEXT("Incoming multiplier then resistance produces deterministic damage"), Damage.DamageAfterModifiers, 52);
	return true;
}

#endif
