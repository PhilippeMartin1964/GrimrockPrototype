#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/Combat/GridCombatTypes.h"

namespace RPG0390
{
	FGridOffensiveEquipmentProfile MakeWeaponProfile()
	{
		FGridOffensiveEquipmentProfile Profile;
		Profile.AttackId = TEXT("Attack_TestSword");
		Profile.AttackDefinition.DamageType = EGridDamageType::Physical;
		Profile.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::Slashing;
		Profile.AttackDefinition.MinDamage = 3;
		Profile.AttackDefinition.MaxDamage = 7;
		Profile.AttackDefinition.AccuracyBonus = 1;
		Profile.FlatDamageBonus = 2;
		Profile.DamageScalingAttribute = EGridAttackScalingAttribute::Strength;
		Profile.RangeCells = 1;
		return Profile;
	}

	FGridCombatActionDefinition MakeWeaponAction()
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = TEXT("Action_RPG0390_WeaponTalent");
		Action.DisplayName = FText::FromString(TEXT("RPG03.9.0 Weapon Talent"));
		Action.ActionType = EGridCombatActionType::MeleeAttack;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
		Action.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
		Action.ActionPointCost = 2;
		Action.RangeCells = 1;
		Action.WeaponAttackProfile.bUseEquippedWeapon = true;
		Action.WeaponAttackProfile.WeaponDamagePercent = 150;
		return Action;
	}

	FGridCombatActionCatalogContext MakeCatalogContext()
	{
		FGridCombatActionCatalogContext Context;
		Context.CharacterIndex = 0;
		Context.CharacterId = FGuid::NewGuid();
		Context.bCombatActive = true;
		Context.bActiveCombatant = true;
		Context.bEnableClassActionExecutors = true;
		Context.RemainingActionPoints = 4;
		Context.CurrentHealth = 20;
		Context.MaximumHealth = 20;
		return Context;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0390ProfileValidationTest, "Grimrock.RPG.RPG03.9.0.ProfileValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0390ProfileValidationTest::RunTest(const FString&)
{
	FGridCombatWeaponAttackProfile Profile;
	Profile.bUseEquippedWeapon = true;
	Profile.WeaponDamagePercent = 150;
	Profile.RequiredItemTags = { TEXT("Weapon.Heavy") };
	TestTrue(TEXT("A tagged equipped-weapon profile is valid"), Profile.IsValid());

	Profile.RequiredItemTags.Add(TEXT("Weapon.Heavy"));
	TestFalse(TEXT("Duplicate required tags are rejected"), Profile.IsValid());

	Profile.RequiredItemTags = { TEXT("Weapon.Heavy") };
	Profile.bAllowUnarmed = true;
	TestFalse(TEXT("Unarmed fallback cannot satisfy item-tag requirements"), Profile.IsValid());

	Profile.bAllowUnarmed = false;
	Profile.WeaponDamagePercent = 0;
	TestFalse(TEXT("A zero WD coefficient is rejected"), Profile.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0390ItemTagMatchingTest, "Grimrock.RPG.RPG03.9.0.ItemTagMatching",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0390ItemTagMatchingTest::RunTest(const FString&)
{
	FGridCombatWeaponAttackProfile Profile;
	Profile.bUseEquippedWeapon = true;
	Profile.RequiredItemTags = { TEXT("Weapon.Heavy"), TEXT("Weapon.Melee") };
	TestTrue(TEXT("Every required item tag must be present"),
		Profile.MatchesItemTags({ TEXT("Weapon.Melee"), TEXT("Weapon.Heavy"), TEXT("Weapon.Sword") }));
	TestFalse(TEXT("A partial tag match is rejected"), Profile.MatchesItemTags({ TEXT("Weapon.Heavy") }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0390ActionValidationTest, "Grimrock.RPG.RPG03.9.0.ActionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0390ActionValidationTest::RunTest(const FString&)
{
	FGridCombatActionDefinition Action = RPG0390::MakeWeaponAction();
	TestTrue(TEXT("A class attack may source its offense exclusively from an equipped weapon"), Action.IsValid());

	Action.OffensiveProfile = RPG0390::MakeWeaponProfile();
	TestFalse(TEXT("An authored offensive profile and equipped-weapon projection cannot be double authorities"), Action.IsValid());

	Action = RPG0390::MakeWeaponAction();
	Action.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
	TestFalse(TEXT("QuickItems cannot redirect their attack authority to equipped weapons"), Action.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0390DescriptorProjectionTest, "Grimrock.RPG.RPG03.9.0.DescriptorProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0390DescriptorProjectionTest::RunTest(const FString&)
{
	FGridOffensiveEquipmentProfile Weapon = RPG0390::MakeWeaponProfile();
	FGridCombatWeaponAttackProfile Projection;
	Projection.bUseEquippedWeapon = true;
	Projection.WeaponDamagePercent = 80;
	Projection.bOverrideDamageDescriptor = true;
	Projection.OverrideDamageType = EGridDamageType::Physical;
	Projection.OverridePhysicalSubtype = EGridPhysicalDamageSubtype::Bludgeoning;

	TestTrue(TEXT("The action can project its identity/range/descriptor onto the weapon copy"),
		Projection.ApplyToOffensiveProfile(TEXT("Action_ShieldBash"), 2, Weapon));
	TestEqual(TEXT("The Talent ActionId becomes the resolved attack identity"), Weapon.AttackId, FName(TEXT("Action_ShieldBash")));
	TestEqual(TEXT("The Talent range becomes authoritative"), Weapon.RangeCells, 2);
	TestEqual(TEXT("Weapon MinDamage is preserved"), Weapon.AttackDefinition.MinDamage, 3);
	TestEqual(TEXT("Weapon MaxDamage is preserved"), Weapon.AttackDefinition.MaxDamage, 7);
	TestEqual(TEXT("Weapon flat bonus is preserved"), Weapon.FlatDamageBonus, 2);
	TestEqual(TEXT("The requested physical subtype overrides the weapon descriptor"),
		Weapon.AttackDefinition.PhysicalSubtype, EGridPhysicalDamageSubtype::Bludgeoning);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0390RawDamageFloorTest, "Grimrock.RPG.RPG03.9.0.RawDamageFloor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0390RawDamageFloorTest::RunTest(const FString&)
{
	FGridAttackDefinition Attack;
	Attack.DamageType = EGridDamageType::Physical;
	Attack.PhysicalSubtype = EGridPhysicalDamageSubtype::Slashing;
	Attack.MinDamage = 3;
	Attack.MaxDamage = 3;

	FGridAttackSourceStats Source;
	Source.Accuracy = 20;
	Source.DamageBonus = 2;
	Source.RawDamagePercent = 85;
	Source.CriticalChancePercent = 0;

	FGridAttackTargetStats Target;
	Target.CurrentHealth = 100;
	const FGridAttackResult Result = FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Attack, 10, 3);
	TestTrue(TEXT("The deterministic attack hits"), Result.bHit);
	TestEqual(TEXT("85 percent of WD 5 floors to 4"), Result.RawDamage, 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0390RawDamageMinimumTest, "Grimrock.RPG.RPG03.9.0.RawDamageMinimum",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0390RawDamageMinimumTest::RunTest(const FString&)
{
	FGridAttackDefinition Attack;
	Attack.DamageType = EGridDamageType::Physical;
	Attack.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
	Attack.MinDamage = 1;
	Attack.MaxDamage = 1;

	FGridAttackSourceStats Source;
	Source.Accuracy = 20;
	Source.RawDamagePercent = 1;
	Source.CriticalChancePercent = 0;

	FGridAttackTargetStats Target;
	Target.CurrentHealth = 100;
	const FGridAttackResult Result = FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Attack, 10, 1);
	TestEqual(TEXT("A positive WD Talent hit keeps RPG02 minimum raw damage one"), Result.RawDamage, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0390CriticalOrderingTest, "Grimrock.RPG.RPG03.9.0.CriticalOrdering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0390CriticalOrderingTest::RunTest(const FString&)
{
	FGridAttackDefinition Attack;
	Attack.DamageType = EGridDamageType::Physical;
	Attack.PhysicalSubtype = EGridPhysicalDamageSubtype::Slashing;
	Attack.MinDamage = 3;
	Attack.MaxDamage = 3;

	FGridAttackSourceStats Source;
	Source.DamageBonus = 2;
	Source.RawDamagePercent = 150;
	Source.CriticalChancePercent = 100;
	Source.CriticalDamagePercent = 200;

	FGridAttackTargetStats Target;
	Target.CurrentHealth = 100;
	const FGridAttackResult Result = FGridCombatResolver::ResolveAttackFromRolls(Source, Target, Attack, 20, 3);
	TestTrue(TEXT("The test attack crits"), Result.bCriticalHit);
	TestEqual(TEXT("RPG02 floors 150 percent WD before applying the critical multiplier"), Result.RawDamage, 14);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0390ResolvedModifierContextTest, "Grimrock.RPG.RPG03.9.0.ResolvedModifierContext",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0390ResolvedModifierContextTest::RunTest(const FString&)
{
	FGridCombatActionDefinition Action = RPG0390::MakeWeaponAction();
	Action.SourceTags = { TEXT("Talent.Attack") };
	FGridOffensiveEquipmentProfile Weapon = RPG0390::MakeWeaponProfile();
	TestTrue(TEXT("Weapon projection succeeds before modifier matching"),
		Action.WeaponAttackProfile.ApplyToOffensiveProfile(Action.ActionId, Action.RangeCells, Weapon));

	const FGridCombatModifierContext Context = FGridCombatModifierResolver::MakeResolvedActionAttackContext(
		Action, TEXT("Class_Warrior"), Weapon, { TEXT("Weapon.Heavy"), TEXT("Weapon.Sword") });
	TestTrue(TEXT("Resolved context contains authored action tags"), Context.SourceTags.Contains(TEXT("Talent.Attack")));
	TestTrue(TEXT("Resolved context contains selected item tags"), Context.SourceTags.Contains(TEXT("Weapon.Heavy")));
	TestTrue(TEXT("Resolved context exposes the weapon damage descriptor"), Context.bHasDamageDescriptor);
	TestEqual(TEXT("Resolved physical subtype comes from the selected weapon"), Context.PhysicalSubtype, EGridPhysicalDamageSubtype::Slashing);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0390CatalogAvailabilityTest, "Grimrock.RPG.RPG03.9.0.CatalogAvailability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0390CatalogAvailabilityTest::RunTest(const FString&)
{
	FGridCombatActionDefinition Action = RPG0390::MakeWeaponAction();
	Action.WeaponAttackProfile.RequiredItemTags = { TEXT("Weapon.Heavy") };

	FGridCombatActionContribution Contribution;
	Contribution.Definition = Action;
	Contribution.SourceDefinitionId = TEXT("Class_Warrior");
	Contribution.AvailableSourceQuantity = 1;
	TestTrue(TEXT("The WD action contribution is structurally valid"), Contribution.IsValid());

	FGridCombatActionCatalogContext Context = RPG0390::MakeCatalogContext();
	TArray<FGridAvailableCombatAction> Actions;
	FGridCombatActionCatalog::Build(Context, { Contribution }, Actions);
	TestEqual(TEXT("The action remains visible without a compatible weapon"), Actions.Num(), 1);
	if (!Actions.IsValidIndex(0))
	{
		return false;
	}
	TestFalse(TEXT("The action is disabled without a compatible weapon"), Actions[0].bEnabled);
	TestEqual(TEXT("The disabled reason is explicit"), Actions[0].AvailabilityReason,
		EGridCombatActionAvailabilityReason::RequiredOffensiveEquipmentUnavailable);

	Context.EquippedOffensiveSourceTagSets = { { TEXT("Weapon.Light") }, { TEXT("Weapon.Heavy"), TEXT("Weapon.Melee") } };
	FGridCombatActionCatalog::Build(Context, { Contribution }, Actions);
	TestTrue(TEXT("A compatible equipped weapon enables the action"), Actions.IsValidIndex(0) && Actions[0].bEnabled);
	return true;
}

#endif
