#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatReactionResolver.h"

namespace RPG0395
{
	FRPGAttributes MakeAttributes(int32 Wisdom)
	{
		FRPGAttributes Attributes;
		Attributes.Wisdom = Wisdom;
		return Attributes;
	}

	FRPGSkillRank MakeSkill(FName SkillId, int32 Rank)
	{
		FRPGSkillRank Skill;
		Skill.SkillId = SkillId;
		Skill.Rank = Rank;
		return Skill;
	}

	FGridCombatActionDefinition MakeHealingSpell(int32 BaseHealing)
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = TEXT("Action_RPG0395_Heal");
		Action.DisplayName = FText::FromString(TEXT("RPG0395 Heal"));
		Action.ActionType = EGridCombatActionType::Ability;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
		Action.TargetingPolicy = EGridCombatTargetingPolicy::Ally;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
		Action.ActionPointCost = 2;
		Action.RangeCells = 3;
		Action.EffectProfile.RestoreHealth = BaseHealing;
		return Action;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395DirectHealingScalingTest, "Grimrock.RPG.RPG03.9.5A.DirectHealingScaling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395DirectHealingScalingTest::RunTest(const FString& Parameters)
{
	using namespace RPG0395;
	FGridCombatActionDefinition Action = MakeHealingSpell(5);
	Action.HealingScaling.ScalingAttribute = EGridAttackScalingAttribute::Wisdom;
	Action.HealingScaling.AttributeModifierScale = 1;
	Action.HealingScaling.ScalingSkillId = TEXT("Skill_Medicine");
	Action.HealingScaling.SkillRankScale = 1;

	FGridResolvedCombatModifiers Modifiers;
	Modifiers.OutgoingHealingPercentModifier = 25;
	const int32 Healing = FGridCombatModifierResolver::ResolveDirectHealthRestore(
		Action, Action.EffectProfile, MakeAttributes(16), { MakeSkill(TEXT("Skill_Medicine"), 4) }, Modifiers, 10, 100);
	TestEqual(TEXT("5 + WIS mod 3 + Medicine 4, then +25 percent, heals 15"), Healing, 15);
	TestEqual(TEXT("A positive 25 percent modifier always grants at least +1"),
		FGridCombatModifierResolver::ApplyOutgoingHealingModifier(1, Modifiers), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395HealingFloorTest, "Grimrock.RPG.RPG03.9.5A.HealingFloor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395HealingFloorTest::RunTest(const FString& Parameters)
{
	using namespace RPG0395;
	FGridCombatActionDefinition Action = MakeHealingSpell(12);
	Action.HealingScaling.ScalingAttribute = EGridAttackScalingAttribute::Wisdom;
	Action.HealingScaling.AttributeModifierScale = 2;
	Action.HealingScaling.ScalingSkillId = TEXT("Skill_Religion");
	Action.HealingScaling.SkillRankScale = 1;
	Action.HealingScaling.MinimumTargetHealthPercent = 50;

	const int32 Healing = FGridCombatModifierResolver::ResolveDirectHealthRestore(
		Action, Action.EffectProfile, MakeAttributes(16), { MakeSkill(TEXT("Skill_Religion"), 4) },
		FGridResolvedCombatModifiers(), 20, 100);
	TestEqual(TEXT("Miracle-style floor heals enough to reach 50 percent when the formula is lower"), Healing, 30);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395DirectDamageAttributeSkillScalingTest,
	"Grimrock.RPG.RPG03.9.5A.DirectDamageAttributeSkillScaling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395DirectDamageAttributeSkillScalingTest::RunTest(const FString& Parameters)
{
	using namespace RPG0395;
	FGridCombatActionDefinition Action;
	Action.DirectDamageScaling.ScalingAttribute = EGridAttackScalingAttribute::Wisdom;
	Action.DirectDamageScaling.AttributeModifierScale = 1;
	Action.DirectDamageScaling.ScalingSkillId = TEXT("Skill_Religion");
	Action.DirectDamageScaling.SkillRankScale = 1;

	FGridAttackSourceStats Source;
	Source.DamageBonus = 3;
	const FRPGAttributes Attributes = MakeAttributes(16);
	FGridCombatModifierResolver::ApplyDirectDamageSkillScaling(
		Action, { MakeSkill(TEXT("Skill_Religion"), 4) }, Source, &Attributes);
	TestEqual(TEXT("Primary WIS mod 3 plus second WIS mod 3 plus Religion 4 totals 10"), Source.DamageBonus, 10);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395ReactionDamageSourceFilterTest, "Grimrock.RPG.RPG03.9.5A.ReactionDamageSourceFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395ReactionDamageSourceFilterTest::RunTest(const FString& Parameters)
{
	FGridCombatReactionProfile Profile;
	Profile.ReactionId = TEXT("Reaction_RPG0395_BreakOnOwnDamage");
	Profile.Trigger = EGridCombatReactionTrigger::AttackHit;
	Profile.bRequireOwnerAsEventSource = true;
	Profile.bRequireAppliedDamage = true;
	Profile.bConsumeOwningStatus = true;

	FGridCombatReactionBinding Binding;
	Binding.Profile = Profile;
	Binding.OwningStatusEffectId = TEXT("Status_RPG0395");
	const TArray<FGridCombatReactionBinding> Bindings = { Binding };
	const FGuid OwnerId = FGuid::NewGuid();

	FGridCombatReactionEvent Event;
	Event.EventId = FGuid::NewGuid();
	Event.ActionInstanceId = FGuid::NewGuid();
	Event.RoundNumber = 1;
	Event.Trigger = EGridCombatReactionTrigger::AttackHit;
	Event.SourceCombatantId = OwnerId;
	Event.TargetCombatantId = FGuid::NewGuid();
	Event.bAppliedDamage = true;

	FGridCombatReactionLedger Ledger;
	TArray<FGridCombatReactionMatch> Matches;
	FGridCombatReactionResolver::ResolveMatches(Bindings, OwnerId, Event, Ledger, false, Matches);
	TestEqual(TEXT("Owner dealing actual damage matches"), Matches.Num(), 1);

	Event.bAppliedDamage = false;
	FGridCombatReactionResolver::ResolveMatches(Bindings, OwnerId, Event, Ledger, false, Matches);
	TestEqual(TEXT("Zero applied damage does not match"), Matches.Num(), 0);

	Event.bAppliedDamage = true;
	Event.SourceCombatantId = FGuid::NewGuid();
	FGridCombatReactionResolver::ResolveMatches(Bindings, OwnerId, Event, Ledger, false, Matches);
	TestEqual(TEXT("Another source does not satisfy owner-as-source"), Matches.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395PeriodicHealingProfileTest, "Grimrock.RPG.RPG03.9.5A.PeriodicHealingProfile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395PeriodicHealingProfileTest::RunTest(const FString& Parameters)
{
	UGridStatusEffectDefinitionAsset* Status = NewObject<UGridStatusEffectDefinitionAsset>();
	Status->EffectId = TEXT("Status_RPG0395_Regeneration");
	Status->DisplayName = FText::FromString(TEXT("Regeneration"));
	Status->Disposition = EGridStatusEffectDisposition::Buff;
	Status->DurationUnit = EGridStatusEffectDurationUnit::Turns;
	Status->DefaultDuration = 3;
	Status->PeriodicHealing.HealingPerStack = 3;
	Status->PeriodicHealing.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
	Status->PeriodicHealing.ScalingAttribute = EGridAttackScalingAttribute::Wisdom;
	Status->PeriodicHealing.AttributeModifierScale = 1;

	TestTrue(TEXT("Periodic healing status is structurally valid"), Status->IsValidDefinition());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395PartyCenteredForcedMovementTest, "Grimrock.RPG.RPG03.9.5A.PartyCenteredForcedMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395PartyCenteredForcedMovementTest::RunTest(const FString& Parameters)
{
	FGridCombatActionDefinition Action;
	Action.ActionId = TEXT("Action_RPG0395_TurnUndead");
	Action.DisplayName = FText::FromString(TEXT("Turn Undead"));
	Action.ActionType = EGridCombatActionType::Ability;
	Action.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
	Action.TargetingPolicy = EGridCombatTargetingPolicy::Area;
	Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
	Action.ActionPointCost = 3;
	Action.ResourceCosts.ManaCost = 7;
	Action.RangeCells = 1;
	Action.AreaRadiusCells = 2;
	Action.bAreaCenteredOnParty = true;
	Action.OffensiveProfile.AttackId = Action.ActionId;
	Action.OffensiveProfile.AttackDefinition.DamageType = EGridDamageType::Holy;
	Action.OffensiveProfile.AttackDefinition.MinDamage = 4;
	Action.OffensiveProfile.AttackDefinition.MaxDamage = 4;
	Action.OffensiveProfile.AttackDefinition.bAlwaysHits = true;
	Action.OffensiveProfile.AttackDefinition.bCanCriticalHit = false;
	Action.OffensiveProfile.DamageScalingAttribute = EGridAttackScalingAttribute::None;
	Action.OffensiveProfile.RangeCells = 1;
	Action.TargetFilter.AllowedMonsterCategoryIds = { TEXT("Undead") };

	FGridCombatMovementEffectProfile Movement;
	Movement.Subject = EGridCombatMovementSubject::TargetCombatant;
	Movement.Direction = EGridCombatMovementDirection::AwayFromSource;
	Movement.DistanceCells = 1;
	Movement.bForced = true;
	Movement.ArmorGate = EGridCombatStatusArmorGate::MagicalArmorDepleted;
	Action.MovementEffects.Add(Movement);

	TestTrue(TEXT("Party-centered Area attack with gated forced target movement is structurally valid"), Action.IsValid());
	return true;
}

#endif
