#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGMageAuthoring.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatReactionResolver.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/Combat/GridCombatTargetingResolver.h"
#include "Runtime/GridInventoryTypes.h"

namespace RPG0394MageAuthoring
{
	const FName AffinityAlias(TEXT("Talent_Mage_Evoker_ElementalAffinity"));
	const FName AffinityGroup(TEXT("TalentGroup_Mage_Evoker_ElementalAffinity"));
	const FName OverloadTalentId(TEXT("Talent_Mage_Evoker_ElementalOverload"));
	const FName ControlledExplosionTalentId(TEXT("Talent_Mage_Evoker_ControlledExplosion"));
	const FName ElementalChainTalentId(TEXT("Talent_Mage_Evoker_ElementalChain"));
	const FName CataclysmTalentId(TEXT("Talent_Mage_Evoker_Cataclysm"));

	const FName OverloadActionId(TEXT("Action_Mage_ElementalOverload"));
	const FName ElementalChainActionId(TEXT("Action_Mage_ElementalChain"));
	const FName CataclysmActionId(TEXT("Action_Mage_Cataclysm"));
	const FName OverloadStatusId(TEXT("Status_ElementalOverload"));

	struct FAffinityExpectation
	{
		const TCHAR* Suffix;
		const TCHAR* SchoolTag;
		EGridDamageType DamageType;
		const TCHAR* CataclysmStatusId;
		int32 StatusDuration;
	};

	const FAffinityExpectation Affinities[] = {
		{ TEXT("Fire"), TEXT("Spell.School.Fire"), EGridDamageType::Fire, TEXT("Status_Burning"), 2 },
		{ TEXT("Frost"), TEXT("Spell.School.Frost"), EGridDamageType::Ice, TEXT("Status_Slow"), 2 },
		{ TEXT("Air"), TEXT("Spell.School.Air"), EGridDamageType::Lightning, TEXT("Status_Stunned"), 1 },
		{ TEXT("Earth"), TEXT("Spell.School.Earth"), EGridDamageType::Physical, TEXT("Status_Immobilized"), 1 }
	};

	FName MakeAffinityChoiceId(const TCHAR* Suffix)
	{
		return FName(*FString::Printf(TEXT("Talent_Mage_Evoker_ElementalAffinity_%s"), Suffix));
	}

	URPGClassAsset* BuildMage()
	{
		URPGClassAsset* Mage = NewObject<URPGClassAsset>(GetTransientPackage());
		Mage->ClassId = TEXT("Mage");
		Mage->DisplayName = FText::FromString(TEXT("Mage"));
		Mage->HealthAtLevelOne = 8;

		for (const int32 Level : { 2, 6, 10, 14, 18 })
		{
			FRPGClassProgressionLevelGrant Grant;
			Grant.Level = Level;
			Grant.ChoicePointsGranted = 1;
			Mage->ProgressionLevelGrants.Add(Grant);
		}

		FRPGMageAuthoring::ConfigureClass(*Mage);
		return Mage;
	}

	const FGridCombatActionDefinition* FindAction(const URPGClassAsset* Mage, FName ActionId)
	{
		return Mage ? Mage->CombatActions.FindByPredicate(
			[ActionId](const FGridCombatActionDefinition& Action)
			{
				return Action.ActionId == ActionId;
			}) : nullptr;
	}

	bool ProjectAffinityAction(const FGridCombatActionDefinition& AuthoredAction, const TCHAR* Suffix, FGridCombatActionDefinition& OutAction)
	{
		OutAction = AuthoredAction;
		TSet<FName> Requirements;
		Requirements.Add(MakeAffinityChoiceId(Suffix));
		return FGridCombatActionCatalog::ApplyOwnerRequirementVariant(OutAction, Requirements);
	}
}

// Historical C contract remains valid after D adds the rest of the Evoker branch.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394CMageClassAuthoringTest, "Grimrock.RPG.RPG03.9.4C.MageClassAuthoring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394CMageClassAuthoringTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	URPGClassAsset* Mage = BuildMage();

	TestTrue(TEXT("Transient Mage authoring remains structurally valid"), Mage->IsValidDefinition());
	TestTrue(TEXT("Evoker authoring still contains the Elemental Overload action"), FindAction(Mage, OverloadActionId) != nullptr);
	TestTrue(TEXT("Evoker authoring still contains at least the five C choice records"), Mage->ProgressionChoices.Num() >= 5);

	const FGridCombatActionDefinition* Overload = FindAction(Mage, OverloadActionId);
	TestTrue(TEXT("Elemental Overload remains a 1 AP / 4 mana self action with cooldown three"), Overload &&
		Overload->SourcePolicy == EGridCombatActionSourcePolicy::Ability &&
		Overload->TargetingPolicy == EGridCombatTargetingPolicy::Self &&
		Overload->ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
		Overload->ActionPointCost == 1 && Overload->ResourceCosts.ManaCost == 4 && Overload->CooldownRounds == 3);
	TestTrue(TEXT("Elemental Overload still applies Status_ElementalOverload for one turn"), Overload &&
		Overload->StatusApplications.Num() == 1 &&
		Overload->StatusApplications[0].StatusEffectId == OverloadStatusId &&
		Overload->StatusApplications[0].DurationOverride == 1);

	const FRPGClassProgressionChoiceDefinition* OverloadChoice = Mage->FindProgressionChoice(OverloadTalentId);
	TestTrue(TEXT("Elemental Overload still depends on the logical Elemental Affinity alias"), OverloadChoice &&
		OverloadChoice->PrerequisiteRequirementIds.Contains(AffinityAlias));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394CAffinityAuthoringTest, "Grimrock.RPG.RPG03.9.4C.ElementalAffinity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394CAffinityAuthoringTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	URPGClassAsset* Mage = BuildMage();

	for (const FAffinityExpectation& Affinity : Affinities)
	{
		const FName ChoiceId = MakeAffinityChoiceId(Affinity.Suffix);
		const FRPGClassProgressionChoiceDefinition* Choice = Mage->FindProgressionChoice(ChoiceId);
		TestTrue(*FString::Printf(TEXT("Affinity variant %s exists"), Affinity.Suffix), Choice != nullptr);
		if (!Choice)
		{
			continue;
		}

		TestEqual(TEXT("Affinity variants share one exclusive group"), Choice->ExclusiveChoiceGroupId, AffinityGroup);
		TestTrue(TEXT("Affinity variant grants the logical alias"), Choice->GrantedRequirementIds.Contains(AffinityAlias));
		TestTrue(TEXT("Affinity variant carries one +15 spell-school damage modifier"),
			Choice->CombatModifiers.Num() == 1 &&
			Choice->CombatModifiers[0].SourcePolicies.Contains(EGridCombatActionSourcePolicy::Spell) &&
			Choice->CombatModifiers[0].RequiredSourceTags.Contains(FName(Affinity.SchoolTag)) &&
			Choice->CombatModifiers[0].OutgoingDamagePercentModifier == 15);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394COverloadStatusAuthoringTest, "Grimrock.RPG.RPG03.9.4C.ElementalOverloadStatus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394COverloadStatusAuthoringTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	UGridStatusEffectDefinitionAsset* Status = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	TestTrue(TEXT("Elemental Overload status configures successfully"), FRPGMageAuthoring::ConfigureElementalOverloadStatus(*Status));
	TestTrue(TEXT("Elemental Overload status is structurally valid"), Status->IsValidDefinition());
	TestEqual(TEXT("Elemental Overload uses the canonical EffectId"), Status->EffectId, OverloadStatusId);
	TestEqual(TEXT("Elemental Overload lasts one Turn"), Status->DurationUnit, EGridStatusEffectDurationUnit::Turns);
	TestEqual(TEXT("Elemental Overload default duration is one"), Status->DefaultDuration, 1);
	TestEqual(TEXT("Elemental Overload has four affinity-conditioned modifiers"), Status->CombatModifiers.Num(), 4);
	TestEqual(TEXT("Elemental Overload has four affinity-conditioned consumption reactions"), Status->CombatReactions.Num(), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394CEvokerRuntimeCompositionTest, "Grimrock.RPG.RPG03.9.4C.EvokerRuntimeComposition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394CEvokerRuntimeCompositionTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	URPGClassAsset* Mage = BuildMage();
	UGridStatusEffectDefinitionAsset* Status = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	if (!FRPGMageAuthoring::ConfigureElementalOverloadStatus(*Status))
	{
		return false;
	}

	FGridCharacterInventoryState Character;
	Character.CharacterId = FGuid::NewGuid();
	Character.ClassId = Mage->ClassId;
	Character.ClassDefinition = Mage;
	Character.Level = 6;
	Character.SelectedClassProgressionChoiceIds = {
		MakeAffinityChoiceId(TEXT("Fire")),
		OverloadTalentId
	};

	FGridStatusEffectApplyResult ApplyResult;
	FString Error;
	TestTrue(TEXT("Elemental Overload applies to the Fire-affinity Mage"),
		Character.StatusEffects.TryApply(*Status, Character.CharacterId, ApplyResult, Error));

	TArray<FGridCombatModifierProfile> Profiles;
	TestTrue(TEXT("Fire-affinity Mage combat modifiers project"), FGridCombatModifierResolver::CollectCharacterModifiers(Character, Profiles));

	FGridCombatActionDefinition FireSpell;
	FireSpell.ActionId = TEXT("Spell_RPG0394C_Fire");
	FireSpell.DisplayName = FText::FromString(TEXT("Fire spell"));
	FireSpell.Description = FireSpell.DisplayName;
	FireSpell.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
	FireSpell.ActionType = EGridCombatActionType::Ability;
	FireSpell.TargetingPolicy = EGridCombatTargetingPolicy::Hostile;
	FireSpell.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
	FireSpell.SourceTags = { TEXT("Spell.School.Fire") };

	FGridResolvedCombatModifiers FireModifiers;
	FGridCombatModifierResolver::Resolve(
		Profiles, FGridCombatModifierResolver::MakeActionContext(FireSpell, FireSpell.ActionId), FireModifiers);
	TestEqual(TEXT("Affinity +15 and Overload +35 compose to +50 percent on Fire spells"),
		FireModifiers.OutgoingDamagePercentModifier, 50);

	FGridCombatActionDefinition FrostSpell = FireSpell;
	FrostSpell.ActionId = TEXT("Spell_RPG0394C_Frost");
	FrostSpell.SourceTags = { TEXT("Spell.School.Frost") };
	FGridResolvedCombatModifiers FrostModifiers;
	FGridCombatModifierResolver::Resolve(
		Profiles, FGridCombatModifierResolver::MakeActionContext(FrostSpell, FrostSpell.ActionId), FrostModifiers);
	TestEqual(TEXT("Fire affinity and Overload do not modify Frost spells"), FrostModifiers.OutgoingDamagePercentModifier, 0);

	TArray<FGridCombatReactionBinding> Bindings;
	TestTrue(TEXT("Fire-affinity Mage reaction bindings project"), FGridCombatReactionResolver::CollectCharacterBindings(Character, Bindings));
	TestEqual(TEXT("Only the Fire Overload consumption reaction is projected"), Bindings.Num(), 1);
	if (Bindings.Num() != 1)
	{
		return false;
	}

	FGridCombatReactionEvent FireEvent;
	FireEvent.EventId = FGuid::NewGuid();
	FireEvent.ActionInstanceId = FGuid::NewGuid();
	FireEvent.RoundNumber = 1;
	FireEvent.Trigger = EGridCombatReactionTrigger::ActionResolved;
	FireEvent.SourceCombatantId = Character.CharacterId;
	FireEvent.TargetCombatantId = Character.CharacterId;
	FireEvent.ActionId = FireSpell.ActionId;
	FireEvent.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
	FireEvent.ActionType = EGridCombatActionType::Ability;
	FireEvent.SourceTags = FireSpell.SourceTags;

	FGridCombatReactionLedger Ledger;
	TArray<FGridCombatReactionMatch> Matches;
	FGridCombatReactionResolver::ResolveMatches(Bindings, Character.CharacterId, FireEvent, Ledger, false, Matches);
	TestTrue(TEXT("A Fire spell requests consumption of Status_ElementalOverload"),
		Matches.Num() == 1 && Matches[0].OwningStatusEffectId == OverloadStatusId && Matches[0].bConsumeOwningStatus);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394CProductionAssetsTest, "Grimrock.RPG.RPG03.9.4C.ProductionAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394CProductionAssetsTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	URPGClassAsset* Mage = LoadObject<URPGClassAsset>(nullptr, FRPGMageAuthoring::MageAssetPath());
	if (!TestNotNull(TEXT("Production DA_Class_Mage loads"), Mage))
	{
		return false;
	}
	TestEqual(TEXT("Production Mage keeps canonical ClassId"), Mage->ClassId, FName(TEXT("Mage")));
	TestTrue(TEXT("Production Mage still contains Elemental Overload"), FindAction(Mage, OverloadActionId) != nullptr);
	TestTrue(TEXT("Production Mage is structurally valid"), Mage->IsValidDefinition());

	UGridStatusEffectDefinitionAsset* Status =
		LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, FRPGMageAuthoring::ElementalOverloadStatusPath());
	if (!TestNotNull(TEXT("Production Status_ElementalOverload loads"), Status))
	{
		return false;
	}
	TestTrue(TEXT("Production Status_ElementalOverload is structurally valid"), Status->IsValidDefinition());
	TestEqual(TEXT("Production overload keeps four modifiers"), Status->CombatModifiers.Num(), 4);
	TestEqual(TEXT("Production overload keeps four reactions"), Status->CombatReactions.Num(), 4);
	return true;
}

// D — complete Evoker branch.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394DEvokerBranchAuthoringTest, "Grimrock.RPG.RPG03.9.4D.EvokerBranchAuthoring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394DEvokerBranchAuthoringTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	URPGClassAsset* Mage = BuildMage();
	TestTrue(TEXT("Complete Evoker authoring is structurally valid"), Mage->IsValidDefinition());
	TestEqual(TEXT("Evoker branch authors exactly three active actions"), Mage->CombatActions.Num(), 3);
	TestEqual(TEXT("Evoker branch authors four affinity variants plus four talents"), Mage->ProgressionChoices.Num(), 8);

	const FGridCombatActionDefinition* Chain = FindAction(Mage, ElementalChainActionId);
	TestTrue(TEXT("Elemental Chain is a 3 AP / 8 mana / CD3 Cell R5 spell attack"), Chain &&
		Chain->SourcePolicy == EGridCombatActionSourcePolicy::Spell &&
		Chain->ResolutionProfile == EGridCombatActionResolutionProfile::Attack &&
		Chain->TargetingPolicy == EGridCombatTargetingPolicy::Cell &&
		Chain->ActionPointCost == 3 && Chain->ResourceCosts.ManaCost == 8 &&
		Chain->RangeCells == 5 && Chain->CooldownRounds == 3);
	TestTrue(TEXT("Elemental Chain resolves one primary plus at most two jump targets"), Chain &&
		Chain->MaximumResolvedTargets == 3 && Chain->ChainJumpRangeCells == 1);
	TestTrue(TEXT("Elemental Chain formula is 7 + INT mod + Arcana rank"), Chain &&
		Chain->OffensiveProfile.AttackDefinition.MinDamage == 7 &&
		Chain->OffensiveProfile.AttackDefinition.MaxDamage == 7 &&
		Chain->OffensiveProfile.DamageScalingAttribute == EGridAttackScalingAttribute::Intelligence &&
		Chain->DirectDamageScaling.ScalingSkillId == TEXT("Skill_Arcana") &&
		Chain->DirectDamageScaling.SkillRankScale == 1 &&
		Chain->OffensiveProfile.AttackDefinition.bAlwaysHits &&
		!Chain->OffensiveProfile.AttackDefinition.bCanCriticalHit);

	const FGridCombatActionDefinition* Cataclysm = FindAction(Mage, CataclysmActionId);
	TestTrue(TEXT("Cataclysm is a 4 AP / 16 mana / CD5 Area2 R5 spell attack"), Cataclysm &&
		Cataclysm->SourcePolicy == EGridCombatActionSourcePolicy::Spell &&
		Cataclysm->TargetingPolicy == EGridCombatTargetingPolicy::Area &&
		Cataclysm->ActionPointCost == 4 && Cataclysm->ResourceCosts.ManaCost == 16 &&
		Cataclysm->RangeCells == 5 && Cataclysm->AreaRadiusCells == 2 && Cataclysm->CooldownRounds == 5 &&
		Cataclysm->bAffectsAlliesInArea);
	TestTrue(TEXT("Cataclysm formula is 12 + INT mod + Arcana rank"), Cataclysm &&
		Cataclysm->OffensiveProfile.AttackDefinition.MinDamage == 12 &&
		Cataclysm->OffensiveProfile.AttackDefinition.MaxDamage == 12 &&
		Cataclysm->DirectDamageScaling.ScalingSkillId == TEXT("Skill_Arcana") &&
		Cataclysm->DirectDamageScaling.SkillRankScale == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394DControlledExplosionTest, "Grimrock.RPG.RPG03.9.4D.ControlledExplosion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394DControlledExplosionTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	URPGClassAsset* Mage = BuildMage();
	const FRPGClassProgressionChoiceDefinition* Choice = Mage->FindProgressionChoice(ControlledExplosionTalentId);
	const FGridCombatActionDefinition* CataclysmAuthored = FindAction(Mage, CataclysmActionId);
	const FGridCombatActionDefinition* ChainAuthored = FindAction(Mage, ElementalChainActionId);
	if (!TestNotNull(TEXT("Controlled Explosion choice exists"), Choice) ||
		!TestNotNull(TEXT("Cataclysm action exists"), CataclysmAuthored) ||
		!TestNotNull(TEXT("Elemental Chain action exists"), ChainAuthored))
	{
		return false;
	}
	TestEqual(TEXT("Controlled Explosion owns one generic modifier"), Choice->CombatModifiers.Num(), 1);

	FGridCombatActionDefinition Cataclysm;
	TestTrue(TEXT("Fire Cataclysm owner variant projects"), ProjectAffinityAction(*CataclysmAuthored, TEXT("Fire"), Cataclysm));
	FGridResolvedCombatModifiers AreaModifiers;
	FGridCombatModifierResolver::Resolve(Choice->CombatModifiers,
		FGridCombatModifierResolver::MakeActionContext(Cataclysm, Mage->ClassId), AreaModifiers);
	TestEqual(TEXT("Allied direct Area damage is reduced by 50 percent"), AreaModifiers.FriendlyDirectDamagePercentModifier, -50);
	TestEqual(TEXT("Caster direct Area damage is reduced by an additional 100 percent"), AreaModifiers.SelfDirectDamagePercentModifier, -100);

	FGridAttackSourceStats AllySource;
	FGridCombatModifierResolver::ApplyFriendlyDirectDamageModifiers(AllySource, AreaModifiers, false);
	TestEqual(TEXT("Controlled Explosion leaves allies at 50 percent direct damage"), AllySource.DamageMultiplier, 0.5f);

	FGridAttackSourceStats SelfSource;
	FGridCombatModifierResolver::ApplyFriendlyDirectDamageModifiers(SelfSource, AreaModifiers, true);
	TestEqual(TEXT("Controlled Explosion leaves the caster at zero direct damage"), SelfSource.DamageMultiplier, 0.0f);

	FGridCombatActionDefinition Chain;
	TestTrue(TEXT("Fire Chain owner variant projects"), ProjectAffinityAction(*ChainAuthored, TEXT("Fire"), Chain));
	FGridResolvedCombatModifiers NonAreaModifiers;
	FGridCombatModifierResolver::Resolve(Choice->CombatModifiers,
		FGridCombatModifierResolver::MakeActionContext(Chain, Mage->ClassId), NonAreaModifiers);
	TestEqual(TEXT("Controlled Explosion does not modify non-Area spells"), NonAreaModifiers.FriendlyDirectDamagePercentModifier, 0);
	TestEqual(TEXT("Controlled Explosion self protection does not match non-Area spells"), NonAreaModifiers.SelfDirectDamagePercentModifier, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394DVariantAndDamageFormulaTest, "Grimrock.RPG.RPG03.9.4D.VariantAndDamageFormula",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394DVariantAndDamageFormulaTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	URPGClassAsset* Mage = BuildMage();
	const FGridCombatActionDefinition* ChainAuthored = FindAction(Mage, ElementalChainActionId);
	if (!TestNotNull(TEXT("Elemental Chain exists"), ChainAuthored))
	{
		return false;
	}

	FGridCombatActionDefinition FireChain;
	TestTrue(TEXT("Exactly one Fire owner variant projects"), ProjectAffinityAction(*ChainAuthored, TEXT("Fire"), FireChain));
	TestTrue(TEXT("Fire variant adds the Fire School tag"), FireChain.SourceTags.Contains(TEXT("Spell.School.Fire")));
	TestEqual(TEXT("Fire variant selects Fire damage"), FireChain.OffensiveProfile.AttackDefinition.DamageType, EGridDamageType::Fire);
	TestTrue(TEXT("Owner variants are consumed from the runtime action projection"), FireChain.OwnerVariants.IsEmpty());

	FGridCombatActionDefinition MissingAffinity = *ChainAuthored;
	TSet<FName> NoRequirements;
	TestFalse(TEXT("An affinity-driven action cannot project without an affinity"),
		FGridCombatActionCatalog::ApplyOwnerRequirementVariant(MissingAffinity, NoRequirements));

	FGridCombatActionDefinition AmbiguousAffinity = *ChainAuthored;
	TSet<FName> TwoAffinities;
	TwoAffinities.Add(MakeAffinityChoiceId(TEXT("Fire")));
	TwoAffinities.Add(MakeAffinityChoiceId(TEXT("Frost")));
	TestFalse(TEXT("Multiple matching affinities are rejected rather than guessed"),
		FGridCombatActionCatalog::ApplyOwnerRequirementVariant(AmbiguousAffinity, TwoAffinities));

	FGridAttackSourceStats Source;
	Source.DamageBonus = 3; // INT 16 => +3, already resolved by the ordinary attack source builder.
	FRPGSkillRank Arcana;
	Arcana.SkillId = TEXT("Skill_Arcana");
	Arcana.Rank = 4;
	const TArray<FRPGSkillRank> SkillRanks = { Arcana };
	FGridCombatModifierResolver::ApplyDirectDamageSkillScaling(FireChain, SkillRanks, Source);
	TestEqual(TEXT("Arcana rank 4 adds exactly four direct-damage points"), Source.DamageBonus, 7);

	FGridAttackTargetStats Target;
	Target.Evasion = 100;
	Target.CurrentHealth = 100;
	const FGridAttackResult Result = FGridCombatResolver::ResolveAttackFromRolls(
		Source, Target, FireChain.OffensiveProfile.AttackDefinition, 1, 7);
	TestTrue(TEXT("Deterministic spell damage ignores the ordinary d20 miss gate"), Result.bHit);
	TestFalse(TEXT("Deterministic spell damage cannot critical hit"), Result.bCriticalHit);
	TestEqual(TEXT("Elemental Chain resolves 7 + INT mod 3 + Arcana 4 = 14 raw damage"), Result.RawDamage, 14);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394DChainTopologyTest, "Grimrock.RPG.RPG03.9.4D.ChainTopology",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394DChainTopologyTest::RunTest(const FString&)
{
	const FGuid PrimaryId(1, 1, 1, 1);
	const FGuid NorthId(2, 2, 2, 2);
	const FGuid WestId(3, 3, 3, 3);
	const FGuid NextNorthId(4, 4, 4, 4);

	TArray<FGridCombatChainTargetCandidate> Candidates;
	auto AddCandidate = [&Candidates](const FGuid& Id, const FIntPoint& Cell)
	{
		FGridCombatChainTargetCandidate Candidate;
		Candidate.TargetId = Id;
		Candidate.Cell = Cell;
		Candidates.Add(Candidate);
	};
	AddCandidate(PrimaryId, FIntPoint(2, 2));
	AddCandidate(NorthId, FIntPoint(2, 1));
	AddCandidate(WestId, FIntPoint(1, 2));
	AddCandidate(NextNorthId, FIntPoint(2, 0));

	TArray<FGridCombatChainTargetCandidate> Chain;
	FGridCombatTargetingResolver::BuildDeterministicChain(
		PrimaryId, FIntPoint(2, 2), Candidates, 1, 3, Chain);
	TestEqual(TEXT("Chain resolves exactly primary plus two additional targets"), Chain.Num(), 3);
	if (Chain.Num() != 3)
	{
		return false;
	}
	TestEqual(TEXT("Primary target remains first"), Chain[0].TargetId, PrimaryId);
	TestEqual(TEXT("Equal-distance tie chooses lower Y first"), Chain[1].TargetId, NorthId);
	TestEqual(TEXT("Next jump is resolved from the previous target, not the primary"), Chain[2].TargetId, NextNorthId);
	TestFalse(TEXT("A target that is no longer adjacent is not selected"), Chain.ContainsByPredicate(
		[&WestId](const FGridCombatChainTargetCandidate& Candidate)
		{
			return Candidate.TargetId == WestId;
	}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394DCataclysmAffinityTest, "Grimrock.RPG.RPG03.9.4D.CataclysmAffinity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394DCataclysmAffinityTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	URPGClassAsset* Mage = BuildMage();
	const FGridCombatActionDefinition* CataclysmAuthored = FindAction(Mage, CataclysmActionId);
	if (!TestNotNull(TEXT("Cataclysm exists"), CataclysmAuthored))
	{
		return false;
	}

	for (const FAffinityExpectation& Affinity : Affinities)
	{
		FGridCombatActionDefinition Projected;
		TestTrue(*FString::Printf(TEXT("%s Cataclysm variant projects"), Affinity.Suffix),
			ProjectAffinityAction(*CataclysmAuthored, Affinity.Suffix, Projected));
		TestTrue(TEXT("Projected Cataclysm keeps its affinity School tag"),
			Projected.SourceTags.Contains(FName(Affinity.SchoolTag)));
		TestEqual(TEXT("Projected Cataclysm selects the affinity damage descriptor"),
			Projected.OffensiveProfile.AttackDefinition.DamageType, Affinity.DamageType);
		TestEqual(TEXT("Projected Cataclysm has exactly one affinity control application"), Projected.StatusApplications.Num(), 1);
		if (Projected.StatusApplications.Num() == 1)
		{
			const FGridCombatStatusApplicationProfile& Status = Projected.StatusApplications[0];
			TestEqual(TEXT("Cataclysm selects the expected affinity status"), Status.StatusEffectId, FName(Affinity.CataclysmStatusId));
			TestEqual(TEXT("Cataclysm status uses MagicalArmorDepleted gate"), Status.ArmorGate,
				EGridCombatStatusArmorGate::MagicalArmorDepleted);
			TestEqual(TEXT("Cataclysm status keeps the canonical duration override"), Status.DurationOverride, Affinity.StatusDuration);
		}
	}

	UGridStatusEffectDefinitionAsset* Burning = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	TestTrue(TEXT("Status_Burning configures"), FRPGMageAuthoring::ConfigureStatus(*Burning, TEXT("Status_Burning")));
	TestEqual(TEXT("Burning uses Fire periodic damage"), Burning->PeriodicDamage.DamageType, EGridDamageType::Fire);
	TestEqual(TEXT("Burning deals two damage per tick"), Burning->PeriodicDamage.DamagePerStack, 2);
	TestEqual(TEXT("Burning lasts two Turns"), Burning->DurationUnit, EGridStatusEffectDurationUnit::Turns);
	TestEqual(TEXT("Burning default duration is two"), Burning->DefaultDuration, 2);

	UGridStatusEffectDefinitionAsset* Slow = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	TestTrue(TEXT("Status_Slow configures"), FRPGMageAuthoring::ConfigureStatus(*Slow, TEXT("Status_Slow")));
	TestEqual(TEXT("Slow applies Initiative -6"), Slow->InitiativeModifier, -6);
	TestEqual(TEXT("Slow lasts two Rounds"), Slow->DurationUnit, EGridStatusEffectDurationUnit::Rounds);
	TestEqual(TEXT("Slow default duration is two"), Slow->DefaultDuration, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394DProductionAssetsTest, "Grimrock.RPG.RPG03.9.4D.ProductionAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394DProductionAssetsTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	URPGClassAsset* Mage = LoadObject<URPGClassAsset>(nullptr, FRPGMageAuthoring::MageAssetPath());
	if (!TestNotNull(TEXT("Production DA_Class_Mage loads"), Mage))
	{
		return false;
	}
	TestTrue(TEXT("Production Mage is structurally valid"), Mage->IsValidDefinition());
	TestEqual(TEXT("Production Evoker has three active actions"), Mage->CombatActions.Num(), 3);
	TestEqual(TEXT("Production Evoker has eight Choice records"), Mage->ProgressionChoices.Num(), 8);
	TestNotNull(TEXT("Production Elemental Chain exists"), FindAction(Mage, ElementalChainActionId));
	TestNotNull(TEXT("Production Cataclysm exists"), FindAction(Mage, CataclysmActionId));

	for (const FName EffectId : { FName(TEXT("Status_ElementalOverload")), FName(TEXT("Status_Burning")), FName(TEXT("Status_Slow")),
		FName(TEXT("Status_Stunned")), FName(TEXT("Status_Immobilized")) })
	{
		const FString Path = FRPGMageAuthoring::GetStatusObjectPath(EffectId);
		UGridStatusEffectDefinitionAsset* Status = LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, *Path);
		TestTrue(*FString::Printf(TEXT("Production status %s loads and is valid"), *EffectId.ToString()),
			IsValid(Status) && Status->IsValidDefinition());
	}
	return true;
}

#endif
