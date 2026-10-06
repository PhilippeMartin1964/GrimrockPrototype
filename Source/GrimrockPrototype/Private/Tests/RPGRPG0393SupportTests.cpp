#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGPartyProgressionResolver.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillCheckService.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "RPG/StatusEffects/GridStatusEffectTypes.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatReactionResolver.h"
#include "Runtime/GridInventoryTypes.h"

namespace RPG0393
{
	URPGClassAsset* MakeClass(FName ClassId)
	{
		URPGClassAsset* Class = NewObject<URPGClassAsset>(GetTransientPackage());
		Class->ClassId = ClassId;
		Class->DisplayName = FText::FromName(ClassId);
		Class->HealthAtLevelOne = 12;
		return Class;
	}

	FGridCharacterInventoryState MakeCharacter(URPGClassAsset* Class, FName ChoiceId)
	{
		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid::NewGuid();
		Character.DisplayName = FText::FromString(TEXT("Ranger"));
		Character.ClassId = Class ? Class->ClassId : NAME_None;
		Character.ClassDefinition = Class;
		Character.Level = 18;
		Character.Resources.CurrentHealth = 10;
		if (!ChoiceId.IsNone())
		{
			Character.SelectedClassProgressionChoiceIds = { ChoiceId };
		}
		return Character;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0393WeaponRangeProjectionTest, "Grimrock.RPG.RPG03.9.3.Support.WeaponRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0393WeaponRangeProjectionTest::RunTest(const FString&)
{
	FGridCombatWeaponAttackProfile Profile;
	Profile.bUseEquippedWeapon = true;
	Profile.bRequireRangedWeapon = true;
	Profile.bUseWeaponRange = true;
	Profile.WeaponRangeModifier = 2;
	Profile.WeaponDamagePercent = 150;
	TestTrue(TEXT("Dynamic ranged weapon profile is valid"), Profile.IsValid());

	FGridOffensiveEquipmentProfile Bow;
	Bow.AttackId = TEXT("Attack_Bow");
	Bow.AttackDefinition.MinDamage = 3;
	Bow.AttackDefinition.MaxDamage = 6;
	Bow.AttackDefinition.DamageType = EGridDamageType::Physical;
	Bow.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
	Bow.RangeCells = 5;
	TestTrue(TEXT("Ranged weapon projects through Talent profile"),
		Profile.ApplyToOffensiveProfile(TEXT("Action_Precise"), 1, Bow));
	TestEqual(TEXT("Weapon range plus two is authoritative"), Bow.RangeCells, 7);

	FGridOffensiveEquipmentProfile Knife = Bow;
	Knife.RangeCells = 1;
	TestFalse(TEXT("Adjacent-only weapon is not a ranged weapon"), Profile.ApplyToOffensiveProfile(TEXT("Action_Precise"), 1, Knife));

	Bow.RangeCells = 32;
	TestTrue(TEXT("Projected range remains valid at global cap"), Profile.ApplyToOffensiveProfile(TEXT("Action_Precise"), 1, Bow));
	TestEqual(TEXT("Weapon range plus modifier clamps to 32"), Bow.RangeCells, 32);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0393DynamicWeaponRangeModifierCompositionTest,
	"Grimrock.RPG.RPG03.9.3.Support.DynamicWeaponRangeModifierComposition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0393DynamicWeaponRangeModifierCompositionTest::RunTest(const FString&)
{
	FGridCombatActionDefinition Action;
	Action.ActionId = TEXT("Action_DynamicWeaponRange");
	Action.ActionType = EGridCombatActionType::RangedAttack;
	Action.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
	Action.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
	Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
	Action.ActionPointCost = 2;
	Action.RangeCells = 1;
	Action.WeaponAttackProfile.bUseEquippedWeapon = true;
	Action.WeaponAttackProfile.bRequireRangedWeapon = true;
	Action.WeaponAttackProfile.bUseWeaponRange = true;
	Action.WeaponAttackProfile.WeaponRangeModifier = 2;

	FGridResolvedCombatModifiers RuntimeModifiers;
	RuntimeModifiers.RangeCellsModifier = 1;
	FGridCombatModifierResolver::ApplyToActionDefinitionProjection(Action, RuntimeModifiers);

	TestEqual(TEXT("Static action projection still receives the runtime range delta"), Action.RangeCells, 2);
	TestEqual(TEXT("Dynamic weapon range composes authored and runtime modifiers"),
		Action.WeaponAttackProfile.WeaponRangeModifier, 3);

	FGridOffensiveEquipmentProfile Bow;
	Bow.AttackId = TEXT("Attack_Bow");
	Bow.AttackDefinition.MinDamage = 3;
	Bow.AttackDefinition.MaxDamage = 6;
	Bow.AttackDefinition.DamageType = EGridDamageType::Physical;
	Bow.AttackDefinition.PhysicalSubtype = EGridPhysicalDamageSubtype::Piercing;
	Bow.RangeCells = 5;
	TestTrue(TEXT("Composed dynamic weapon range projects successfully"),
		Action.WeaponAttackProfile.ApplyToOffensiveProfile(Action.ActionId, Action.RangeCells, Bow));
	TestEqual(TEXT("Weapon R5 plus authored +2 plus runtime +1 resolves to R8"), Bow.RangeCells, 8);

	Bow.RangeCells = 31;
	TestTrue(TEXT("Composed dynamic weapon range remains valid near the global cap"),
		Action.WeaponAttackProfile.ApplyToOffensiveProfile(Action.ActionId, Action.RangeCells, Bow));
	TestEqual(TEXT("Composed dynamic weapon range clamps globally to R32"), Bow.RangeCells, 32);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0393DistinctSourceStatusTest, "Grimrock.RPG.RPG03.9.3.Support.DistinctSourceStatus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0393DistinctSourceStatusTest::RunTest(const FString&)
{
	UGridStatusEffectDefinitionAsset* Mark = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	Mark->EffectId = TEXT("Status_Marked");
	Mark->DisplayName = FText::FromString(TEXT("Marked"));
	Mark->DurationUnit = EGridStatusEffectDurationUnit::Rounds;
	Mark->DefaultDuration = 3;
	Mark->StackPolicy = EGridStatusEffectStackPolicy::RefreshDuration;
	Mark->MaxStacks = 1;
	Mark->bDistinctPerSource = true;
	Mark->bUniquePerSourceAcrossMonsters = true;
	TestTrue(TEXT("Per-source mark definition is valid"), Mark->IsValidDefinition());

	const FGuid RangerA = FGuid::NewGuid();
	const FGuid RangerB = FGuid::NewGuid();
	FGridStatusEffectCollection Effects;
	FGridStatusEffectApplyResult ApplyResult;
	FString Error;
	TestTrue(TEXT("First Ranger mark applies"), Effects.TryApply(*Mark, RangerA, 1, INDEX_NONE, INDEX_NONE, ApplyResult, Error));
	TestTrue(TEXT("Second Ranger mark coexists on the same target"),
		Effects.TryApply(*Mark, RangerB, 1, INDEX_NONE, INDEX_NONE, ApplyResult, Error));
	TestEqual(TEXT("Two source identities are active"), Effects.ActiveEffects.Num(), 2);
	TestNotNull(TEXT("Ranger A identity can be resolved"), Effects.FindByEffectIdAndSource(Mark->EffectId, RangerA));
	TestNotNull(TEXT("Ranger B identity can be resolved"), Effects.FindByEffectIdAndSource(Mark->EffectId, RangerB));

	FGridStatusEffectRuntimeState Removed;
	TestTrue(TEXT("One source identity can be removed independently"),
		Effects.RemoveByEffectIdAndSource(Mark->EffectId, RangerA, Removed));
	TestEqual(TEXT("Other Ranger mark remains"), Effects.ActiveEffects.Num(), 1);
	TestNotNull(TEXT("Ranger B still owns its mark"), Effects.FindByEffectIdAndSource(Mark->EffectId, RangerB));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0393PartyProgressionTest, "Grimrock.RPG.RPG03.9.3.Support.PartyProgression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0393PartyProgressionTest::RunTest(const FString&)
{
	URPGClassAsset* Ranger = RPG0393::MakeClass(TEXT("Ranger_Test"));
	FRPGClassProgressionChoiceDefinition Vigilance;
	Vigilance.ChoiceId = TEXT("Choice_Vigilance");
	Vigilance.DisplayName = FText::FromString(TEXT("Vigilance"));
	Vigilance.MinimumLevel = 2;
	Vigilance.PointCost = 1;
	Vigilance.FirstRoundInitiativeModifier = 2;
	FRPGPartyProgressionModifier VigilanceParty;
	VigilanceParty.StackingGroupId = TEXT("Party.Ranger.Vigilance");
	VigilanceParty.GroupSkillIds = { TEXT("Skill_Perception") };
	VigilanceParty.GroupSkillCheckModifier = 2;
	Vigilance.PartyModifiers.Add(VigilanceParty);
	Ranger->ProgressionChoices.Add(Vigilance);

	FRPGClassProgressionChoiceDefinition Guide;
	Guide.ChoiceId = TEXT("Choice_Guide");
	Guide.DisplayName = FText::FromString(TEXT("Guide"));
	Guide.MinimumLevel = 2;
	Guide.PointCost = 1;
	FRPGPartyProgressionModifier GuideParty;
	GuideParty.StackingGroupId = TEXT("Party.Ranger.GroupGuide");
	GuideParty.GroupSkillIds = { TEXT("Skill_Perception"), TEXT("Skill_Survival") };
	GuideParty.GroupSkillCheckModifier = 2;
	GuideParty.MaximumMobilityActionPointsModifier = 1;
	Guide.PartyModifiers.Add(GuideParty);
	Ranger->ProgressionChoices.Add(Guide);
	TestTrue(TEXT("Party-progression class is structurally valid"), Ranger->IsValidDefinition());

	FGridPartyInventoryState Party;
	Party.ActiveCharacters.Add(RPG0393::MakeCharacter(Ranger, Vigilance.ChoiceId));
	Party.ActiveCharacters.Add(RPG0393::MakeCharacter(Ranger, Vigilance.ChoiceId));
	FGridCharacterInventoryState GuideOwner = RPG0393::MakeCharacter(Ranger, Guide.ChoiceId);
	Party.ActiveCharacters.Add(GuideOwner);

	TestEqual(TEXT("Two Vigilance owners do not stack"), FRPGPartyProgressionResolver::ResolveGroupSkillCheckModifier(Party, TEXT("Skill_Perception")), 4);
	TestEqual(TEXT("Group Guide gives Survival +2"), FRPGPartyProgressionResolver::ResolveGroupSkillCheckModifier(Party, TEXT("Skill_Survival")), 2);
	TestEqual(TEXT("Group Guide gives only +1 maximum PAM"), FRPGPartyProgressionResolver::ResolveMaximumMobilityActionPointsModifier(Party), 1);
	TestEqual(TEXT("Vigilance owner receives +2 first-round Initiative"),
		FRPGPartyProgressionResolver::ResolveFirstRoundInitiativeModifier(Party.ActiveCharacters[0]), 2);

	Party.ActiveCharacters[0].Resources.CurrentHealth = 0;
	Party.ActiveCharacters[1].Resources.CurrentHealth = 0;
	TestEqual(TEXT("Dead Vigilance owners stop contributing"),
		FRPGPartyProgressionResolver::ResolveGroupSkillCheckModifier(Party, TEXT("Skill_Perception")), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0393GroupSkillCheckTest, "Grimrock.RPG.RPG03.9.3.Support.GroupSkillCheck",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0393GroupSkillCheckTest::RunTest(const FString&)
{
	URPGClassAsset* Ranger = RPG0393::MakeClass(TEXT("Ranger_Group"));
	FRPGClassProgressionChoiceDefinition Vigilance;
	Vigilance.ChoiceId = TEXT("Choice_Vigilance");
	Vigilance.DisplayName = FText::FromString(TEXT("Vigilance"));
	Vigilance.MinimumLevel = 2;
	Vigilance.PointCost = 1;
	FRPGPartyProgressionModifier PartyBonus;
	PartyBonus.StackingGroupId = TEXT("Party.Ranger.Vigilance");
	PartyBonus.GroupSkillIds = { TEXT("Skill_Perception") };
	PartyBonus.GroupSkillCheckModifier = 2;
	Vigilance.PartyModifiers.Add(PartyBonus);
	Ranger->ProgressionChoices.Add(Vigilance);
	TestTrue(TEXT("Group check class is valid"), Ranger->IsValidDefinition());

	URPGSkillAsset* Perception = NewObject<URPGSkillAsset>(GetTransientPackage());
	Perception->SkillId = TEXT("Skill_Perception");
	Perception->DisplayName = FText::FromString(TEXT("Perception"));
	Perception->GoverningAttribute = ERPGSkillGoverningAttribute::Wisdom;
	Perception->MaxRank = 5;
	Perception->bAllowUntrainedChecks = true;
	TestTrue(TEXT("Transient Perception definition is valid"), Perception->IsValidDefinition());

	FGridPartyInventoryState Party;
	FGridCharacterInventoryState Weak = RPG0393::MakeCharacter(Ranger, Vigilance.ChoiceId);
	Weak.Attributes.Wisdom = 10;
	FGridCharacterInventoryState Strong = RPG0393::MakeCharacter(Ranger, NAME_None);
	Strong.Attributes.Wisdom = 16;
	FRPGSkillRank Rank;
	Rank.SkillId = TEXT("Skill_Perception");
	Rank.Rank = 3;
	Strong.SkillRanks.Add(Rank);
	Party.ActiveCharacters = { Weak, Strong };

	FRandomStream Stream(393);
	FRPGSkillCheckResult Result;
	int32 CharacterIndex = INDEX_NONE;
	TestTrue(TEXT("Best-party Perception check resolves"),
		FRPGSkillCheckService::TryResolveBestPartySkillCheck(Party, Perception, 15, Stream, Result, CharacterIndex));
	TestEqual(TEXT("Best static member is selected"), CharacterIndex, 1);
	TestEqual(TEXT("Party Vigilance modifier is added once"), Result.ProgressionModifier, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0393OwnedTargetReactionTest, "Grimrock.RPG.RPG03.9.3.Support.OwnedTargetReaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0393OwnedTargetReactionTest::RunTest(const FString&)
{
	const FGuid RangerId = FGuid::NewGuid();
	FGridCombatReactionProfile Alpha;
	Alpha.ReactionId = TEXT("Reaction_Alpha");
	Alpha.Trigger = EGridCombatReactionTrigger::OwnedStatusTargetDefeated;
	Alpha.Limit = EGridCombatReactionLimit::OncePerRound;
	Alpha.RequiredTargetStatusEffectIdsFromOwner = { TEXT("Status_MarkedByRanger") };
	Alpha.TransferOwnedTargetStatusEffectId = TEXT("Status_MarkedByRanger");
	Alpha.TransferTargetRangeCells = 3;
	Alpha.TransferStatusDurationOverride = 2;
	TestTrue(TEXT("Owned-status death transfer reaction is valid"), Alpha.IsValid());

	FGridCombatReactionEvent Event;
	Event.EventId = FGuid::NewGuid();
	Event.ActionInstanceId = FGuid::NewGuid();
	Event.RoundNumber = 1;
	Event.Trigger = EGridCombatReactionTrigger::OwnedStatusTargetDefeated;
	Event.SourceCombatantId = RangerId;
	Event.TargetCombatantId = FGuid::NewGuid();
	Event.TargetStatusEffectIdsFromOwner = { TEXT("Status_MarkedByRanger") };
	TestTrue(TEXT("Owner mark death matches Alpha Hunter"), FGridCombatReactionResolver::Matches(Alpha, Event));

	Event.TargetStatusEffectIdsFromOwner.Reset();
	TestFalse(TEXT("Another Ranger's mark does not match Alpha Hunter"), FGridCombatReactionResolver::Matches(Alpha, Event));

	FGridCombatModifierProfile OwnMark;
	OwnMark.RequiredTargetStatusEffectIds = { TEXT("Status_MarkedByRanger") };
	OwnMark.bRequiredTargetStatusesFromOwner = true;
	OwnMark.AccuracyModifier = 2;
	FGridCombatModifierContext Context;
	Context.TargetStatusEffectIds = { TEXT("Status_MarkedByRanger") };
	TestFalse(TEXT("Same EffectId from another source is insufficient"), FGridCombatModifierResolver::Matches(OwnMark, Context));
	Context.TargetStatusEffectIdsFromSource = { TEXT("Status_MarkedByRanger") };
	TestTrue(TEXT("Own mark satisfies owner-qualified modifier"), FGridCombatModifierResolver::Matches(OwnMark, Context));
	return true;
}

#endif
