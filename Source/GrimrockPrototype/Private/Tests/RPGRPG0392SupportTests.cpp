#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillCheckService.h"
#include "RPG/RPGSkillRequirementProjectionService.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatReactionResolver.h"
#include "Runtime/GridInventoryTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0392TargetConditionModifierTest, "Grimrock.RPG.RPG03.9.2.Support.TargetConditions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0392TargetConditionModifierTest::RunTest(const FString&)
{
	FGridCombatModifierProfile Sneak;
	Sneak.ActionIds = { TEXT("Action_Sneak") };
	Sneak.AnyTargetConditions = {
		EGridCombatTargetCondition::HasNotActedThisRound,
		EGridCombatTargetCondition::PhysicalControl,
		EGridCombatTargetCondition::RearArc
	};
	Sneak.WeaponDamagePercentModifier = 50;
	TestTrue(TEXT("Conditional WD modifier is structurally valid"), Sneak.IsValid());

	FGridCombatModifierContext Context;
	Context.ActionId = TEXT("Action_Sneak");
	Context.ActionType = EGridCombatActionType::MeleeAttack;
	Context.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
	Context.TargetConditions = { EGridCombatTargetCondition::HasNotActedThisRound };

	FGridResolvedCombatModifiers Resolved;
	FGridCombatModifierResolver::Resolve({ Sneak }, Context, Resolved);
	TestEqual(TEXT("Any matching tactical condition adds 50 WD points"), Resolved.WeaponDamagePercentModifier, 50);

	Context.TargetConditions = { EGridCombatTargetCondition::HasActedThisRound };
	FGridCombatModifierResolver::Resolve({ Sneak }, Context, Resolved);
	TestEqual(TEXT("No matching tactical condition gives no WD bonus"), Resolved.WeaponDamagePercentModifier, 0);

	FGridCombatModifierProfile Backstab;
	Backstab.RequiredSourceTags = { TEXT("Weapon.Light") };
	Backstab.RequiredTargetConditions = { EGridCombatTargetCondition::RearArc };
	Backstab.bExcludeAreaActions = true;
	Backstab.OutgoingDamagePercentModifier = 20;
	Context.SourceTags = { TEXT("Weapon.Light") };
	Context.TargetConditions = { EGridCombatTargetCondition::RearArc };
	Context.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
	TestTrue(TEXT("Rear-arc light attack matches Backstab"), FGridCombatModifierResolver::Matches(Backstab, Context));
	Context.TargetingPolicy = EGridCombatTargetingPolicy::Area;
	TestFalse(TEXT("Area attack is explicitly excluded from Backstab"), FGridCombatModifierResolver::Matches(Backstab, Context));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0392WeaponSubtypeTest, "Grimrock.RPG.RPG03.9.2.Support.WeaponSubtype",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0392WeaponSubtypeTest::RunTest(const FString&)
{
	FGridCombatWeaponAttackProfile Profile;
	Profile.bUseEquippedWeapon = true;
	Profile.WeaponDamagePercent = 100;
	Profile.RequiredItemTags = { TEXT("Weapon.Light") };
	Profile.AllowedPhysicalSubtypes = { EGridPhysicalDamageSubtype::Slashing, EGridPhysicalDamageSubtype::Piercing };
	TestTrue(TEXT("Subtype-filtered light weapon profile is valid"), Profile.IsValid());

	FGridOffensiveEquipmentProfile Slash;
	Slash.AttackId = TEXT("Attack_Dagger");
	Slash.AttackDefinition.DamageType = EGridDamageType::Physical;
	Slash.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::Slashing;
	Slash.AttackDefinition.MinDamage = 2;
	Slash.AttackDefinition.MaxDamage = 4;
	Slash.RangeCells = 1;
	TestTrue(TEXT("Slashing profile projects"), Profile.ApplyToOffensiveProfile(TEXT("Action_Hemorrhage"), 1, Slash));

	FGridOffensiveEquipmentProfile Blunt = Slash;
	Blunt.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::Bludgeoning;
	TestFalse(TEXT("Bludgeoning profile is rejected by Slashing/Piercing authoring"), Profile.ApplyToOffensiveProfile(TEXT("Action_Hemorrhage"), 1, Blunt));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0392SkillModifierTest, "Grimrock.RPG.RPG03.9.2.Support.SkillModifiers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0392SkillModifierTest::RunTest(const FString&)
{
	URPGClassAsset* ClassAsset = NewObject<URPGClassAsset>(GetTransientPackage());
	ClassAsset->ClassId = TEXT("Rogue_Test");
	ClassAsset->DisplayName = FText::FromString(TEXT("Rogue Test"));
	ClassAsset->HealthAtLevelOne = 10;

	FRPGClassProgressionChoiceDefinition Choice;
	Choice.ChoiceId = TEXT("Talent_Locksmith");
	Choice.DisplayName = FText::FromString(TEXT("Locksmith"));
	FRPGSkillProgressionModifier Modifier;
	Modifier.SkillId = TEXT("Skill_Lockpicking");
	Modifier.CheckModifier = 2;
	Modifier.RequirementGrantRankModifier = 1;
	Modifier.SafeFailureMargin = 2;
	Choice.SkillModifiers.Add(Modifier);
	ClassAsset->ProgressionChoices.Add(Choice);

	FGridCharacterInventoryState Character;
	Character.ClassId = ClassAsset->ClassId;
	Character.ClassDefinition = ClassAsset;
	Character.Level = 1;
	Character.Attributes.Dexterity = 10;
	Character.SelectedClassProgressionChoiceIds = { Choice.ChoiceId };
	FRPGSkillRank Rank;
	Rank.SkillId = TEXT("Skill_Lockpicking");
	Rank.Rank = 3;
	Character.SkillRanks.Add(Rank);

	URPGSkillAsset* Skill = NewObject<URPGSkillAsset>(GetTransientPackage());
	Skill->SkillId = TEXT("Skill_Lockpicking");
	Skill->DisplayName = FText::FromString(TEXT("Crochetage"));
	Skill->GoverningAttribute = ERPGSkillGoverningAttribute::Dexterity;
	Skill->MaxRank = 5;
	Skill->bAllowUntrainedChecks = false;
	FRPGSkillRequirementGrant Grant;
	Grant.MinimumRank = 4;
	Grant.GrantedRequirementIds = { TEXT("Requirement_Lockpick4") };
	Skill->RequirementGrants.Add(Grant);
	TestTrue(TEXT("Transient Lockpicking definition is valid"), Skill->IsValidDefinition());

	FRandomStream Stream(392);
	FRPGSkillCheckResult Result;
	TestTrue(TEXT("Skill check resolves with progression modifier"),
		FRPGSkillCheckService::TryResolveSkillCheck(Character, Skill, 30, Stream, Result));
	TestEqual(TEXT("Selected talent contributes +2 to the check"), Result.ProgressionModifier, 2);
	TestEqual(TEXT("Safe-failure margin is projected"), Result.SafeFailureMargin, 2);
	TestEqual(TEXT("Safe failure flag mirrors a failure margin <=2"),
		Result.bSafeFailure, !Result.bSuccess && Result.FailureMargin > 0 && Result.FailureMargin <= 2);

	TSet<FName> Requirements;
	FString Error;
	TestTrue(TEXT("Requirement projection succeeds with injected canonical skill"),
		FRPGSkillRequirementProjectionService::AppendSatisfiedRequirements(
			Character,
			[Skill](FName SkillId) -> const URPGSkillAsset*
			{
				return SkillId == Skill->SkillId ? Skill : nullptr;
			},
			Requirements, Error));
	TestTrue(TEXT("Rank 3 plus talent effective-rank bonus unlocks the rank-4 grant"),
		Requirements.Contains(TEXT("Requirement_Lockpick4")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0392ReactionSemanticTest, "Grimrock.RPG.RPG03.9.2.Support.ReactionSemantics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0392ReactionSemanticTest::RunTest(const FString&)
{
	FGridCombatReactionProfile Profile;
	Profile.ReactionId = TEXT("Reaction_ShadowReach");
	Profile.Trigger = EGridCombatReactionTrigger::ActionResolved;
	Profile.Limit = EGridCombatReactionLimit::OncePerAction;
	Profile.ActionTypes = { EGridCombatActionType::MeleeAttack };
	Profile.RequiredSourceTags = { TEXT("Weapon.Light") };
	Profile.bRequireOffensiveAction = true;
	Profile.bConsumeOwningStatus = true;
	TestTrue(TEXT("Light-offense consumption reaction is valid"), Profile.IsValid());

	FGridCombatReactionEvent Event;
	Event.EventId = FGuid::NewGuid();
	Event.ActionInstanceId = FGuid::NewGuid();
	Event.RoundNumber = 1;
	Event.Trigger = EGridCombatReactionTrigger::ActionResolved;
	Event.SourceCombatantId = FGuid::NewGuid();
	Event.TargetCombatantId = Event.SourceCombatantId;
	Event.ActionId = TEXT("Attack_Dagger");
	Event.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
	Event.ActionType = EGridCombatActionType::MeleeAttack;
	Event.DamageType = EGridDamageType::Physical;
	Event.SourceTags = { TEXT("Weapon.Light") };
	Event.bOffensiveAction = true;
	TestTrue(TEXT("Light melee offensive action matches"), FGridCombatReactionResolver::Matches(Profile, Event));

	Event.SourceTags.Reset();
	TestFalse(TEXT("Missing light source tag does not consume the reaction"), FGridCombatReactionResolver::Matches(Profile, Event));
	Event.SourceTags = { TEXT("Weapon.Light") };
	Event.bOffensiveAction = false;
	TestFalse(TEXT("Non-offensive action does not consume the reaction"), FGridCombatReactionResolver::Matches(Profile, Event));

	FGridCombatReactionProfile Elusive;
	Elusive.ReactionId = TEXT("Reaction_Elusive");
	Elusive.Trigger = EGridCombatReactionTrigger::ActionResolved;
	Elusive.Limit = EGridCombatReactionLimit::OncePerAction;
	Elusive.ApplyOwnerStatusEffectId = TEXT("Status_Elusive");
	Elusive.ApplyOwnerStatusDurationOverride = 1;
	FGridCombatReactionBinding Binding;
	Binding.Profile = Elusive;
	Event.bOffensiveAction = false;
	Event.SourceTags.Reset();
	Event.ActionType = EGridCombatActionType::Ability;

	FGridCombatReactionLedger Ledger;
	TArray<FGridCombatReactionMatch> Matches;
	FGridCombatReactionResolver::ResolveMatches({ Binding }, Event.SourceCombatantId, Event, Ledger, true, Matches);
	TestTrue(TEXT("Reaction response preserves owner status payload"), Matches.Num() == 1 &&
		Matches[0].ApplyOwnerStatusEffectId == TEXT("Status_Elusive") &&
		Matches[0].ApplyOwnerStatusDurationOverride == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0392TrapAndSkillGateTest, "Grimrock.RPG.RPG03.9.2.Support.TrapAndSkillGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0392TrapAndSkillGateTest::RunTest(const FString&)
{
	FGridCombatTrapEffectProfile Trap;
	Trap.bPlaceTrap = true;
	Trap.TrapId = TEXT("Trap_Quick");
	Trap.DurationRounds = 3;
	Trap.BaseDamage = 6;
	Trap.DamageType = EGridDamageType::Physical;
	Trap.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
	Trap.DamageScalingAttribute = EGridAttackScalingAttribute::Dexterity;
	Trap.bConsumeOnTrigger = true;
	TestTrue(TEXT("Quick-trap payload is structurally valid"), Trap.IsValid());

	FGridCombatTrapState State;
	State.TrapId = Trap.TrapId;
	State.RemainingRounds = Trap.DurationRounds;
	State.SourceCombatantId = FGuid::NewGuid();
	State.SourceActionId = TEXT("Action_QuickTrap");
	State.RawDamage = 8;
	State.DamageType = Trap.DamageType;
	State.PhysicalSubtype = Trap.PhysicalSubtype;
	State.bConsumeOnTrigger = true;
	TestTrue(TEXT("Persisted trap state is valid"), State.IsValid());

	FGridCombatSkillCheckProfile SkillGate;
	SkillGate.SkillId = TEXT("Skill_Mechanics");
	SkillGate.bUseTargetDifficulty = true;
	TestTrue(TEXT("Target-DC Mechanics gate is structurally valid"), SkillGate.IsValid());
	SkillGate.FixedDifficulty = 15;
	TestFalse(TEXT("Target-DC and fixed DC cannot be authored simultaneously"), SkillGate.IsValid());
	return true;
}

#endif
