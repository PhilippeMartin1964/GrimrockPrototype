#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGMageAuthoring.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/Combat/GridCombatArmorEffectResolver.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatReactionResolver.h"
#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/Combat/GridCombatSurfaceResolver.h"
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

	const FName ArcaneShieldTalentId(TEXT("Talent_Mage_Arcanist_ArcaneShield"));
	const FName DispelTalentId(TEXT("Talent_Mage_Arcanist_Dispel"));
	const FName RunicManipulationTalentId(TEXT("Talent_Mage_Arcanist_RunicManipulation"));
	const FName ShortTeleportTalentId(TEXT("Talent_Mage_Arcanist_ShortTeleport"));
	const FName ArcaneMasteryTalentId(TEXT("Talent_Mage_Arcanist_ArcaneMastery"));

	const FName ImbuementTalentId(TEXT("Talent_Mage_SurfaceWeaver_Imbuement"));
	const FName ImbuementAffinityGroup(TEXT("TalentGroup_Mage_SurfaceWeaver_ImbuementAffinity"));
	const FName ElementalConversionTalentId(TEXT("Talent_Mage_SurfaceWeaver_ElementalConversion"));
	const FName ConductionTalentId(TEXT("Talent_Mage_SurfaceWeaver_Conduction"));
	const FName PersistentSurfaceTalentId(TEXT("Talent_Mage_SurfaceWeaver_PersistentSurface"));
	const FName TerrainArchitectTalentId(TEXT("Talent_Mage_SurfaceWeaver_TerrainArchitect"));

	const FName OverloadActionId(TEXT("Action_Mage_ElementalOverload"));
	const FName ElementalChainActionId(TEXT("Action_Mage_ElementalChain"));
	const FName CataclysmActionId(TEXT("Action_Mage_Cataclysm"));
	const FName ArcaneShieldActionId(TEXT("Action_Mage_ArcaneShield"));
	const FName DispelActionId(TEXT("Action_Mage_Dispel"));
	const FName ShortTeleportActionId(TEXT("Action_Mage_ShortTeleport"));
	const FName ImbuementActionId(TEXT("Action_Mage_Imbuement"));
	const FName ElementalConversionActionId(TEXT("Action_Mage_ElementalConversion"));
	const FName TerrainArchitectActionId(TEXT("Action_Mage_TerrainArchitect"));
	const FName OverloadStatusId(TEXT("Status_ElementalOverload"));
	const FName ImbuementStatusId(TEXT("Status_ElementalImbuement"));

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

	FName MakeSurfaceWeaverAffinityChoiceId(const TCHAR* Suffix)
	{
		return FName(*FString::Printf(TEXT("Talent_Mage_SurfaceWeaver_Imbuement_%s"), Suffix));
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

	bool ProjectSurfaceWeaverAction(
		const FGridCombatActionDefinition& AuthoredAction, const TCHAR* Suffix, FGridCombatActionDefinition& OutAction)
	{
		OutAction = AuthoredAction;
		TSet<FName> Requirements;
		Requirements.Add(MakeSurfaceWeaverAffinityChoiceId(Suffix));
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
	TestTrue(TEXT("Evoker actions remain present after later Mage branches"), Mage->CombatActions.Num() >= 3);
	TestTrue(TEXT("Evoker choices remain present after later Mage branches"), Mage->ProgressionChoices.Num() >= 8);

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
	TestTrue(TEXT("Production Evoker actions remain present after later Mage branches"), Mage->CombatActions.Num() >= 3);
	TestTrue(TEXT("Production Evoker choices remain present after later Mage branches"), Mage->ProgressionChoices.Num() >= 8);
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


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394E2ArcanistBranchAuthoringTest, "Grimrock.RPG.RPG03.9.4E2.ArcanistBranchAuthoring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394E2ArcanistBranchAuthoringTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	URPGClassAsset* Mage = BuildMage();
	TestTrue(TEXT("Mage Evoker plus Arcanist authoring is structurally valid"), Mage->IsValidDefinition());
	TestTrue(TEXT("Evoker plus Arcanist actions remain present after later Mage branches"), Mage->CombatActions.Num() >= 6);
	TestTrue(TEXT("Evoker plus Arcanist Choice records remain present after later Mage branches"), Mage->ProgressionChoices.Num() >= 13);

	const FGridCombatActionDefinition* Shield = FindAction(Mage, ArcaneShieldActionId);
	TestTrue(TEXT("Arcane Shield is 2 AP / 5 mana, Ally R3, CD2, Arcane Spell"), Shield &&
		Shield->SourcePolicy == EGridCombatActionSourcePolicy::Spell &&
		Shield->SourceTags.Contains(TEXT("Spell.School.Arcane")) &&
		Shield->TargetingPolicy == EGridCombatTargetingPolicy::Ally &&
		Shield->ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
		Shield->ActionPointCost == 2 && Shield->ResourceCosts.ManaCost == 5 &&
		Shield->RangeCells == 3 && Shield->CooldownRounds == 2 &&
		Shield->ArmorEffects.Num() == 1);

	const FGridCombatActionDefinition* Dispel = FindAction(Mage, DispelActionId);
	TestTrue(TEXT("Dispel is 2 AP / 6 mana, AllyOrHostile R4, CD2, Arcane Spell"), Dispel &&
		Dispel->SourcePolicy == EGridCombatActionSourcePolicy::Spell &&
		Dispel->SourceTags.Contains(TEXT("Spell.School.Arcane")) &&
		Dispel->TargetingPolicy == EGridCombatTargetingPolicy::AllyOrHostile &&
		Dispel->ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
		Dispel->ActionPointCost == 2 && Dispel->ResourceCosts.ManaCost == 6 &&
		Dispel->RangeCells == 4 && Dispel->bRequiresLineOfSight && Dispel->CooldownRounds == 2 &&
		Dispel->StatusRemovals.Num() == 2);

	const FGridCombatActionDefinition* Teleport = FindAction(Mage, ShortTeleportActionId);
	TestTrue(TEXT("Short Teleport is 3 AP / 8 mana, visible Cell R2, CD4, no step movement"), Teleport &&
		Teleport->SourcePolicy == EGridCombatActionSourcePolicy::Spell &&
		Teleport->SourceTags.Contains(TEXT("Spell.School.Arcane")) &&
		Teleport->TargetingPolicy == EGridCombatTargetingPolicy::Cell &&
		Teleport->ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
		Teleport->ActionPointCost == 3 && Teleport->ResourceCosts.ManaCost == 8 &&
		Teleport->RangeCells == 2 && Teleport->bRequiresLineOfSight && Teleport->CooldownRounds == 4 &&
		Teleport->bRelocatePartyToTargetCell && Teleport->MovementEffects.IsEmpty());

	const FRPGClassProgressionChoiceDefinition* DispelChoice = Mage->FindProgressionChoice(DispelTalentId);
	const FRPGClassProgressionChoiceDefinition* RunicChoice = Mage->FindProgressionChoice(RunicManipulationTalentId);
	const FRPGClassProgressionChoiceDefinition* TeleportChoice = Mage->FindProgressionChoice(ShortTeleportTalentId);
	const FRPGClassProgressionChoiceDefinition* MasteryChoice = Mage->FindProgressionChoice(ArcaneMasteryTalentId);
	TestTrue(TEXT("Arcanist progression is a single level 2/6/10/14/18 chain"),
		DispelChoice && DispelChoice->MinimumLevel == 6 && DispelChoice->PrerequisiteChoiceIds.Contains(ArcaneShieldTalentId) &&
		RunicChoice && RunicChoice->MinimumLevel == 10 && RunicChoice->PrerequisiteChoiceIds.Contains(DispelTalentId) &&
		TeleportChoice && TeleportChoice->MinimumLevel == 14 && TeleportChoice->PrerequisiteChoiceIds.Contains(RunicManipulationTalentId) &&
		MasteryChoice && MasteryChoice->MinimumLevel == 18 && MasteryChoice->PrerequisiteChoiceIds.Contains(ShortTeleportTalentId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394E2ArcaneShieldFormulaTest, "Grimrock.RPG.RPG03.9.4E2.ArcaneShieldFormula",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394E2ArcaneShieldFormulaTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	URPGClassAsset* Mage = BuildMage();
	const FGridCombatActionDefinition* Shield = FindAction(Mage, ArcaneShieldActionId);
	if (!TestNotNull(TEXT("Arcane Shield exists"), Shield) || Shield->ArmorEffects.Num() != 1)
	{
		return false;
	}

	FGridCombatArmorEffectSourceContext Source;
	Source.Attributes.Intelligence = 16;
	FRPGSkillRank Arcana;
	Arcana.SkillId = TEXT("Skill_Arcana");
	Arcana.Rank = 4;
	Source.SkillRanks = { Arcana };

	FGridCombatArmorPoolSnapshot Snapshot;
	Snapshot.CurrentMagicalArmor = 2;
	Snapshot.ReferenceMagicalArmor = 20;
	FGridResolvedCombatModifiers Modifiers;
	TArray<FGridCombatArmorEffectResult> Results;
	TestEqual(TEXT("Arcane Shield applies one restore effect"),
		FGridCombatArmorEffectResolver::ApplyRestoreEffects(Shield->ArmorEffects, Snapshot, Modifiers, &Source, &Results), 1);
	TestEqual(TEXT("6 + INT mod 3 + Arcana 4 restores 13 MagicalArmor"), Snapshot.CurrentMagicalArmor, 15);

	Snapshot.CurrentMagicalArmor = 18;
	Results.Reset();
	FGridCombatArmorEffectResolver::ApplyRestoreEffects(Shield->ArmorEffects, Snapshot, Modifiers, &Source, &Results);
	TestEqual(TEXT("Arcane Shield clamps to reference MagicalArmor"), Snapshot.CurrentMagicalArmor, 20);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394E2DispelAuthoringTest, "Grimrock.RPG.RPG03.9.4E2.DispelAuthoring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394E2DispelAuthoringTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	URPGClassAsset* Mage = BuildMage();
	const FGridCombatActionDefinition* Dispel = FindAction(Mage, DispelActionId);
	if (!TestNotNull(TEXT("Dispel exists"), Dispel) || Dispel->StatusRemovals.Num() != 2)
	{
		return false;
	}

	const FGridCombatStatusRemovalProfile* PartyProfile = Dispel->StatusRemovals.FindByPredicate(
		[](const FGridCombatStatusRemovalProfile& Profile)
		{
			return Profile.TargetSide == EGridCombatStatusRemovalTargetSide::Party;
		});
	const FGridCombatStatusRemovalProfile* HostileProfile = Dispel->StatusRemovals.FindByPredicate(
		[](const FGridCombatStatusRemovalProfile& Profile)
		{
			return Profile.TargetSide == EGridCombatStatusRemovalTargetSide::Hostile;
		});
	TestTrue(TEXT("Ally Dissipation removes one magical Debuff"), PartyProfile &&
		PartyProfile->MaximumRemovals == 1 &&
		PartyProfile->AllowedDispositions.Contains(EGridStatusEffectDisposition::Debuff) &&
		PartyProfile->AnyStatusTags.Contains(TEXT("Dispel.Magical")));
	TestTrue(TEXT("Hostile Dissipation removes one magical Buff"), HostileProfile &&
		HostileProfile->MaximumRemovals == 1 &&
		HostileProfile->AllowedDispositions.Contains(EGridStatusEffectDisposition::Buff) &&
		HostileProfile->AnyStatusTags.Contains(TEXT("Dispel.Magical")));

	for (const FName EffectId : { FName(TEXT("Status_ElementalOverload")), FName(TEXT("Status_Burning")), FName(TEXT("Status_Slow")) })
	{
		UGridStatusEffectDefinitionAsset* Status = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
		TestTrue(*FString::Printf(TEXT("%s configures for Mage authoring"), *EffectId.ToString()),
			FRPGMageAuthoring::ConfigureStatus(*Status, EffectId));
		TestTrue(*FString::Printf(TEXT("%s is explicitly classed as magically dispellable"), *EffectId.ToString()),
			Status->StatusTags.Contains(TEXT("Dispel.Magical")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394E2RunicManipulationTest, "Grimrock.RPG.RPG03.9.4E2.RunicManipulation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394E2RunicManipulationTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	URPGClassAsset* Mage = BuildMage();
	const FRPGClassProgressionChoiceDefinition* Choice = Mage->FindProgressionChoice(RunicManipulationTalentId);
	if (!TestNotNull(TEXT("Runic Manipulation exists"), Choice))
	{
		return false;
	}
	TestTrue(TEXT("Runic Manipulation grants Skill_Runes +2"),
		Choice->SkillModifiers.Num() == 1 &&
		Choice->SkillModifiers[0].SkillId == TEXT("Skill_Runes") &&
		Choice->SkillModifiers[0].CheckModifier == 2);
	TestEqual(TEXT("Runic Manipulation owns one combat modifier"), Choice->CombatModifiers.Num(), 1);

	FGridCombatActionDefinition ArcaneSpell;
	ArcaneSpell.ActionId = TEXT("Spell_RPG0394E2_Arcane");
	ArcaneSpell.ActionType = EGridCombatActionType::Ability;
	ArcaneSpell.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
	ArcaneSpell.SourceTags = { TEXT("Spell.School.Arcane") };

	FGridCombatModifierContext Context = FGridCombatModifierResolver::MakeActionContext(ArcaneSpell, ArcaneSpell.ActionId);
	FGridStatusEffectCollection NoStatuses;
	FGridCombatModifierResolver::AddTargetStatusContext(Context, NoStatuses, FGuid::NewGuid(), TEXT("Construct"));
	FGridResolvedCombatModifiers Modifiers;
	FGridCombatModifierResolver::Resolve(Choice->CombatModifiers, Context, Modifiers);
	TestEqual(TEXT("Arcane damage against Construct gains +20 percent"), Modifiers.OutgoingDamagePercentModifier, 20);

	Context = FGridCombatModifierResolver::MakeActionContext(ArcaneSpell, ArcaneSpell.ActionId);
	FGridCombatModifierResolver::AddTargetStatusContext(
		Context, NoStatuses, FGuid::NewGuid(), TEXT("Other"), { FName(TEXT("Rune")) });
	FGridCombatModifierResolver::Resolve(Choice->CombatModifiers, Context, Modifiers);
	TestEqual(TEXT("Arcane damage against Rune semantic tag gains +20 percent"), Modifiers.OutgoingDamagePercentModifier, 20);

	Context = FGridCombatModifierResolver::MakeActionContext(ArcaneSpell, ArcaneSpell.ActionId);
	FGridCombatModifierResolver::AddTargetStatusContext(Context, NoStatuses, FGuid::NewGuid(), TEXT("Vermin"));
	FGridCombatModifierResolver::Resolve(Choice->CombatModifiers, Context, Modifiers);
	TestEqual(TEXT("Unrelated targets gain no Runic damage bonus"), Modifiers.OutgoingDamagePercentModifier, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394E2ArcaneMasteryTest, "Grimrock.RPG.RPG03.9.4E2.ArcaneMastery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394E2ArcaneMasteryTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	URPGClassAsset* Mage = BuildMage();
	const FRPGClassProgressionChoiceDefinition* Choice = Mage->FindProgressionChoice(ArcaneMasteryTalentId);
	if (!TestNotNull(TEXT("Arcane Mastery exists"), Choice) || Choice->CombatModifiers.Num() != 1)
	{
		return false;
	}

	FGridCombatActionDefinition Spell;
	Spell.ActionId = TEXT("Spell_RPG0394E2_Mastery");
	Spell.ActionType = EGridCombatActionType::Ability;
	Spell.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
	Spell.SourceTags = { TEXT("Spell.School.Arcane") };
	Spell.ResourceCosts.ManaCost = 5;
	Spell.RangeCells = 3;

	FGridResolvedCombatModifiers Modifiers;
	FGridCombatModifierResolver::Resolve(
		Choice->CombatModifiers, FGridCombatModifierResolver::MakeActionContext(Spell, Spell.ActionId), Modifiers);
	TestEqual(TEXT("Arcane Mastery grants +15 percent Arcane damage"), Modifiers.OutgoingDamagePercentModifier, 15);
	FGridCombatModifierResolver::ApplyToActionDefinitionProjection(Spell, Modifiers);
	TestEqual(TEXT("Arcane Mastery reduces mana by one"), Spell.ResourceCosts.ManaCost, 4);
	TestEqual(TEXT("Arcane Mastery adds one range"), Spell.RangeCells, 4);

	FGridCombatActionDefinition FloorSpell = Spell;
	FloorSpell.ResourceCosts.ManaCost = 1;
	FloorSpell.RangeCells = 32;
	FGridCombatModifierResolver::ApplyToActionDefinitionProjection(FloorSpell, Modifiers);
	TestEqual(TEXT("Arcane Mastery preserves minimum positive mana cost one"), FloorSpell.ResourceCosts.ManaCost, 1);
	TestEqual(TEXT("Arcane Mastery clamps range at 32"), FloorSpell.RangeCells, 32);

	FGridCombatActionDefinition NonArcane = Spell;
	NonArcane.SourceTags = { TEXT("Spell.School.Fire") };
	FGridResolvedCombatModifiers NonArcaneModifiers;
	FGridCombatModifierResolver::Resolve(
		Choice->CombatModifiers, FGridCombatModifierResolver::MakeActionContext(NonArcane, NonArcane.ActionId), NonArcaneModifiers);
	TestTrue(TEXT("Arcane Mastery does not match non-Arcane spells"), NonArcaneModifiers.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394E2ProductionAssetsTest, "Grimrock.RPG.RPG03.9.4E2.ProductionAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394E2ProductionAssetsTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	URPGClassAsset* Mage = LoadObject<URPGClassAsset>(nullptr, FRPGMageAuthoring::MageAssetPath());
	if (!TestNotNull(TEXT("Production DA_Class_Mage loads"), Mage))
	{
		return false;
	}
	TestTrue(TEXT("Production Mage is structurally valid"), Mage->IsValidDefinition());
	TestTrue(TEXT("Production Evoker plus Arcanist actions remain present after later Mage branches"), Mage->CombatActions.Num() >= 6);
	TestTrue(TEXT("Production Evoker plus Arcanist Choice records remain present after later Mage branches"), Mage->ProgressionChoices.Num() >= 13);
	TestNotNull(TEXT("Production Arcane Shield exists"), FindAction(Mage, ArcaneShieldActionId));
	TestNotNull(TEXT("Production Dispel exists"), FindAction(Mage, DispelActionId));
	TestNotNull(TEXT("Production Short Teleport exists"), FindAction(Mage, ShortTeleportActionId));

	for (const FName EffectId : { FName(TEXT("Status_ElementalOverload")), FName(TEXT("Status_Burning")), FName(TEXT("Status_Slow")) })
	{
		const FString Path = FRPGMageAuthoring::GetStatusObjectPath(EffectId);
		UGridStatusEffectDefinitionAsset* Status = LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, *Path);
		TestTrue(*FString::Printf(TEXT("Production %s remains valid"), *EffectId.ToString()),
			IsValid(Status) && Status->IsValidDefinition());
		TestTrue(*FString::Printf(TEXT("Production %s is tagged Dispel.Magical"), *EffectId.ToString()),
			IsValid(Status) && Status->StatusTags.Contains(TEXT("Dispel.Magical")));
	}
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394F2SurfaceWeaverBranchAuthoringTest,
	"Grimrock.RPG.RPG03.9.4F2.SurfaceWeaverBranchAuthoring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394F2SurfaceWeaverBranchAuthoringTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	URPGClassAsset* Mage = BuildMage();
	TestTrue(TEXT("Complete Mage authoring is structurally valid"), Mage->IsValidDefinition());
	TestEqual(TEXT("Complete Mage authors nine active actions"), Mage->CombatActions.Num(), 9);
	TestEqual(TEXT("Complete Mage authors twenty-one Choice records"), Mage->ProgressionChoices.Num(), 21);

	const FGridCombatActionDefinition* Imbuement = FindAction(Mage, ImbuementActionId);
	TestTrue(TEXT("Imbuement is 1 AP / 4 mana, Ally R3, CD1"), Imbuement &&
		Imbuement->SourcePolicy == EGridCombatActionSourcePolicy::Spell &&
		Imbuement->TargetingPolicy == EGridCombatTargetingPolicy::Ally &&
		Imbuement->ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
		Imbuement->ActionPointCost == 1 && Imbuement->ResourceCosts.ManaCost == 4 &&
		Imbuement->RangeCells == 3 && Imbuement->CooldownRounds == 1 &&
		Imbuement->StatusApplications.Num() == 1 &&
		Imbuement->StatusApplications[0].StatusEffectId == ImbuementStatusId &&
		Imbuement->StatusApplications[0].DurationOverride == 2);

	const FGridCombatActionDefinition* Conversion = FindAction(Mage, ElementalConversionActionId);
	TestTrue(TEXT("Elemental Conversion is 2 AP / 5 mana, Area1 R4, CD2"), Conversion &&
		Conversion->TargetingPolicy == EGridCombatTargetingPolicy::Area &&
		Conversion->ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
		Conversion->ActionPointCost == 2 && Conversion->ResourceCosts.ManaCost == 5 &&
		Conversion->RangeCells == 4 && Conversion->AreaRadiusCells == 1 && Conversion->CooldownRounds == 2 &&
		Conversion->OwnerVariants.Num() == 4);

	const FGridCombatActionDefinition* Architect = FindAction(Mage, TerrainArchitectActionId);
	TestTrue(TEXT("Terrain Architect is 4 AP / 12 mana, Area2 R5, CD5"), Architect &&
		Architect->TargetingPolicy == EGridCombatTargetingPolicy::Area &&
		Architect->ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
		Architect->ActionPointCost == 4 && Architect->ResourceCosts.ManaCost == 12 &&
		Architect->RangeCells == 5 && Architect->AreaRadiusCells == 2 && Architect->CooldownRounds == 5 &&
		Architect->OwnerVariants.Num() == 4);

	for (const FAffinityExpectation& Affinity : Affinities)
	{
		const FRPGClassProgressionChoiceDefinition* Variant =
			Mage->FindProgressionChoice(MakeSurfaceWeaverAffinityChoiceId(Affinity.Suffix));
		TestTrue(*FString::Printf(TEXT("Surface Weaver Imbuement variant %s exists"), Affinity.Suffix),
			Variant && Variant->MinimumLevel == 2 &&
			Variant->ExclusiveChoiceGroupId == ImbuementAffinityGroup &&
			Variant->GrantedRequirementIds.Contains(ImbuementTalentId));
	}

	const FRPGClassProgressionChoiceDefinition* ConversionChoice = Mage->FindProgressionChoice(ElementalConversionTalentId);
	const FRPGClassProgressionChoiceDefinition* ConductionChoice = Mage->FindProgressionChoice(ConductionTalentId);
	const FRPGClassProgressionChoiceDefinition* PersistentChoice = Mage->FindProgressionChoice(PersistentSurfaceTalentId);
	const FRPGClassProgressionChoiceDefinition* ArchitectChoice = Mage->FindProgressionChoice(TerrainArchitectTalentId);
	TestTrue(TEXT("Surface Weaver progression is a single 2/6/10/14/18 chain"),
		ConversionChoice && ConversionChoice->MinimumLevel == 6 && ConversionChoice->PrerequisiteRequirementIds.Contains(ImbuementTalentId) &&
		ConductionChoice && ConductionChoice->MinimumLevel == 10 && ConductionChoice->PrerequisiteChoiceIds.Contains(ElementalConversionTalentId) &&
		PersistentChoice && PersistentChoice->MinimumLevel == 14 && PersistentChoice->PrerequisiteChoiceIds.Contains(ConductionTalentId) &&
		ArchitectChoice && ArchitectChoice->MinimumLevel == 18 && ArchitectChoice->PrerequisiteChoiceIds.Contains(PersistentSurfaceTalentId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394F2ImbuementStatusTest,
	"Grimrock.RPG.RPG03.9.4F2.ImbuementStatus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394F2ImbuementStatusTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	UGridStatusEffectDefinitionAsset* Status = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	TestTrue(TEXT("Elemental Imbuement configures"), FRPGMageAuthoring::ConfigureStatus(*Status, ImbuementStatusId));
	TestTrue(TEXT("Elemental Imbuement is structurally valid"), Status->IsValidDefinition());
	TestEqual(TEXT("Elemental Imbuement lasts two Rounds"), Status->DurationUnit, EGridStatusEffectDurationUnit::Rounds);
	TestEqual(TEXT("Elemental Imbuement default duration is two"), Status->DefaultDuration, 2);
	TestEqual(TEXT("Elemental Imbuement owns four source-affinity reactions"), Status->CombatReactions.Num(), 4);
	TestTrue(TEXT("Elemental Imbuement is magically dispellable"), Status->StatusTags.Contains(TEXT("Dispel.Magical")));

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Affinities); ++Index)
	{
		const FAffinityExpectation& Affinity = Affinities[Index];
		const FGridCombatReactionProfile* Reaction = Status->CombatReactions.FindByPredicate(
			[&Affinity](const FGridCombatReactionProfile& Candidate)
			{
				return Candidate.RequiredStatusSourceRequirementIds.Contains(MakeSurfaceWeaverAffinityChoiceId(Affinity.Suffix));
			});
		TestTrue(*FString::Printf(TEXT("%s Imbuement reaction exists"), Affinity.Suffix), Reaction != nullptr);
		if (!Reaction)
		{
			continue;
		}
		TestTrue(TEXT("Imbuement reacts only to a weapon hit and consumes itself"),
			Reaction->Trigger == EGridCombatReactionTrigger::AttackHit &&
			Reaction->bRequireWeaponAttack && Reaction->bConsumeOwningStatus);
		TestEqual(TEXT("Imbuement base secondary damage is three"), Reaction->SecondaryDirectDamage, 3);
		TestEqual(TEXT("Imbuement uses source Mage INT modifier scaling"),
			Reaction->SecondaryDirectDamageScalingAttribute, EGridAttackScalingAttribute::Intelligence);
		TestEqual(TEXT("Imbuement affinity selects the expected DamageType"),
			Reaction->SecondaryDirectDamageType, Affinity.DamageType);
	}

	URPGClassAsset* MageClass = BuildMage();
	FGridCharacterInventoryState Mage;
	Mage.CharacterId = FGuid::NewGuid();
	Mage.ClassId = MageClass->ClassId;
	Mage.ClassDefinition = MageClass;
	Mage.Level = 2;
	Mage.Attributes.Intelligence = 16;
	Mage.SelectedClassProgressionChoiceIds = { MakeSurfaceWeaverAffinityChoiceId(TEXT("Fire")) };

	FGridCharacterInventoryState Ally;
	Ally.CharacterId = FGuid::NewGuid();
	FGridStatusEffectApplyResult ApplyResult;
	FString ApplyError;
	TestTrue(TEXT("Fire Mage can source an ally-held Imbuement"),
		Ally.StatusEffects.TryApply(*Status, Mage.CharacterId, ApplyResult, ApplyError));

	FGridPartyInventoryState Party;
	Party.ActiveCharacters = { Mage, Ally };
	TArray<FGridCombatReactionBinding> Bindings;
	TestTrue(TEXT("Imbuement reactions project from source Mage progression"),
		FGridCombatReactionResolver::CollectCharacterBindings(Party.ActiveCharacters[1], Party, Bindings));
	TestEqual(TEXT("Only the Fire source-affinity reaction projects"), Bindings.Num(), 1);
	TestTrue(TEXT("Projected binding preserves the Mage source id"),
		Bindings.Num() == 1 && Bindings[0].OwningStatusSourceId == Mage.CharacterId);
	TestEqual(TEXT("Fire Imbuement resolves 3 + INT mod 3 = 6"),
		Bindings.Num() == 1
			? FGridCombatReactionResolver::ResolveSecondaryDirectDamageAmount(Bindings[0].Profile, Mage.Attributes)
			: 0,
		6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394F2ElementalConversionTest,
	"Grimrock.RPG.RPG03.9.4F2.ElementalConversion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394F2ElementalConversionTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	URPGClassAsset* Mage = BuildMage();
	const FGridCombatActionDefinition* Authored = FindAction(Mage, ElementalConversionActionId);
	if (!TestNotNull(TEXT("Elemental Conversion exists"), Authored))
	{
		return false;
	}

	FGridCombatActionDefinition Fire;
	TestTrue(TEXT("Fire Conversion projects"), ProjectSurfaceWeaverAction(*Authored, TEXT("Fire"), Fire));
	TestTrue(TEXT("Fire converts Oil/Poison to Fire"), Fire.SurfaceConversions.Num() == 1 &&
		Fire.SurfaceConversions[0].InputSurfaceTypes.Contains(EGridCombatSurfaceType::Oil) &&
		Fire.SurfaceConversions[0].InputSurfaceTypes.Contains(EGridCombatSurfaceType::Poison) &&
		Fire.SurfaceConversions[0].OutputSurfaceType == EGridCombatSurfaceType::Fire);

	FGridCombatActionDefinition Frost;
	TestTrue(TEXT("Frost Conversion projects"), ProjectSurfaceWeaverAction(*Authored, TEXT("Frost"), Frost));
	TestTrue(TEXT("Frost converts Water to Ice"), Frost.SurfaceConversions.Num() == 1 &&
		Frost.SurfaceConversions[0].InputSurfaceTypes.Contains(EGridCombatSurfaceType::Water) &&
		Frost.SurfaceConversions[0].OutputSurfaceType == EGridCombatSurfaceType::Ice);

	FGridCombatActionDefinition Air;
	TestTrue(TEXT("Air Conversion projects"), ProjectSurfaceWeaverAction(*Authored, TEXT("Air"), Air));
	TestTrue(TEXT("Air converts Water/Blood to ElectrifiedWater"), Air.SurfaceConversions.Num() == 1 &&
		Air.SurfaceConversions[0].InputSurfaceTypes.Contains(EGridCombatSurfaceType::Water) &&
		Air.SurfaceConversions[0].InputSurfaceTypes.Contains(EGridCombatSurfaceType::Blood) &&
		Air.SurfaceConversions[0].OutputSurfaceType == EGridCombatSurfaceType::ElectrifiedWater);

	FGridCombatActionDefinition Earth;
	TestTrue(TEXT("Earth Conversion projects"), ProjectSurfaceWeaverAction(*Authored, TEXT("Earth"), Earth));
	TestEqual(TEXT("Earth owns Water->Poison and neutral->Oil rules"), Earth.SurfaceConversions.Num(), 2);
	TestTrue(TEXT("Earth converts Water to Poison"), Earth.SurfaceConversions.Num() == 2 &&
		Earth.SurfaceConversions[0].InputSurfaceTypes.Contains(EGridCombatSurfaceType::Water) &&
		Earth.SurfaceConversions[0].OutputSurfaceType == EGridCombatSurfaceType::Poison);
	TestTrue(TEXT("Earth neutral rule creates Oil"), Earth.SurfaceConversions.Num() == 2 &&
		Earth.SurfaceConversions[1].bAllowEmptyCell &&
		Earth.SurfaceConversions[1].OutputSurfaceType == EGridCombatSurfaceType::Oil);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394F2ConductionAndPersistentSurfaceTest,
	"Grimrock.RPG.RPG03.9.4F2.ConductionAndPersistentSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394F2ConductionAndPersistentSurfaceTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	URPGClassAsset* Mage = BuildMage();
	const FRPGClassProgressionChoiceDefinition* Conduction = Mage->FindProgressionChoice(ConductionTalentId);
	const FRPGClassProgressionChoiceDefinition* Persistent = Mage->FindProgressionChoice(PersistentSurfaceTalentId);
	if (!TestNotNull(TEXT("Conduction exists"), Conduction) || !TestNotNull(TEXT("Persistent Surface exists"), Persistent))
	{
		return false;
	}
	TestEqual(TEXT("Conduction owns five compatibility profiles including Earth Physical/Poison"), Conduction->CombatModifiers.Num(), 5);

	FGridCombatActionDefinition FireAttack;
	FireAttack.ActionId = TEXT("Attack_RPG0394F2_Fire");
	FireAttack.ActionType = EGridCombatActionType::Ability;
	FireAttack.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
	FireAttack.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
	FireAttack.TargetingPolicy = EGridCombatTargetingPolicy::Hostile;
	FireAttack.OffensiveProfile.AttackId = FireAttack.ActionId;
	FireAttack.OffensiveProfile.AttackDefinition.DamageType = EGridDamageType::Fire;
	FireAttack.OffensiveProfile.AttackDefinition.MinDamage = 1;
	FireAttack.OffensiveProfile.AttackDefinition.MaxDamage = 1;

	FGridCombatModifierContext FireContext =
		FGridCombatModifierResolver::MakeAttackContext(
			FireAttack.ActionId, Mage->ClassId, FireAttack.SourcePolicy, FireAttack.ActionType,
			EGridDamageType::Fire, EGridPhysicalDamageSubtype::None);
	FGridCombatModifierResolver::AddTargetSurfaceContext(FireContext, EGridCombatSurfaceType::Oil);
	FGridResolvedCombatModifiers Resolved;
	FGridCombatModifierResolver::Resolve(Conduction->CombatModifiers, FireContext, Resolved);
	TestEqual(TEXT("Fire attack exploiting Oil gains +20 percent"), Resolved.OutgoingDamagePercentModifier, 20);

	FGridCombatModifierContext NeutralContext =
		FGridCombatModifierResolver::MakeAttackContext(
			FireAttack.ActionId, Mage->ClassId, FireAttack.SourcePolicy, FireAttack.ActionType,
			EGridDamageType::Fire, EGridPhysicalDamageSubtype::None);
	FGridCombatModifierResolver::Resolve(Conduction->CombatModifiers, NeutralContext, Resolved);
	TestEqual(TEXT("Fire attack without compatible state gains no Conduction bonus"), Resolved.OutgoingDamagePercentModifier, 0);

	TestTrue(TEXT("Persistent Surface owns one source modifier"), Persistent->CombatModifiers.Num() == 1);
	if (Persistent->CombatModifiers.Num() == 1)
	{
		TestEqual(TEXT("Persistent Surface adds two rounds"), Persistent->CombatModifiers[0].SurfaceDurationRoundsModifier, 2);
		TestEqual(TEXT("Persistent Surface adds fifteen percent periodic surface damage"),
			Persistent->CombatModifiers[0].SurfacePeriodicDamagePercentModifier, 15);
		TestTrue(TEXT("Persistent Surface is source-side Spell-only"),
			Persistent->CombatModifiers[0].SourcePolicies.Contains(EGridCombatActionSourcePolicy::Spell));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394F2TerrainArchitectTest,
	"Grimrock.RPG.RPG03.9.4F2.TerrainArchitect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394F2TerrainArchitectTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	URPGClassAsset* Mage = BuildMage();
	const FGridCombatActionDefinition* Authored = FindAction(Mage, TerrainArchitectActionId);
	if (!TestNotNull(TEXT("Terrain Architect exists"), Authored))
	{
		return false;
	}

	struct FExpectedSurface
	{
		const TCHAR* Suffix;
		EGridCombatSurfaceType SurfaceType;
	};
	const FExpectedSurface Expected[] = {
		{ TEXT("Fire"), EGridCombatSurfaceType::Fire },
		{ TEXT("Frost"), EGridCombatSurfaceType::Ice },
		{ TEXT("Air"), EGridCombatSurfaceType::ElectrifiedWater },
		{ TEXT("Earth"), EGridCombatSurfaceType::Oil }
	};

	for (const FExpectedSurface& Item : Expected)
	{
		FGridCombatActionDefinition Projected;
		TestTrue(*FString::Printf(TEXT("%s Terrain Architect projects"), Item.Suffix),
			ProjectAffinityAction(*Authored, Item.Suffix, Projected));
		TestEqual(TEXT("Projected Terrain Architect has one surface"), Projected.SurfaceEffects.Num(), 1);
		if (Projected.SurfaceEffects.Num() == 1)
		{
			TestEqual(TEXT("Terrain surface matches affinity"), Projected.SurfaceEffects[0].SurfaceType, Item.SurfaceType);
			TestEqual(TEXT("Terrain surface lasts three rounds before source modifiers"), Projected.SurfaceEffects[0].DurationRounds, 3);
		}
	}

	const FRPGClassProgressionChoiceDefinition* Persistent = Mage->FindProgressionChoice(PersistentSurfaceTalentId);
	if (!TestNotNull(TEXT("Persistent Surface exists for Terrain Architect"), Persistent))
	{
		return false;
	}
	FGridResolvedCombatModifiers PersistentModifiers;
	FGridCombatModifierContext Context =
		FGridCombatModifierResolver::MakeActionContext(*Authored, Mage->ClassId);
	FGridCombatModifierResolver::Resolve(Persistent->CombatModifiers, Context, PersistentModifiers);

	FGridCombatActionDefinition Fire;
	ProjectSurfaceWeaverAction(*Authored, TEXT("Fire"), Fire);
	FGridCombatSurfaceState State;
	TestTrue(TEXT("Terrain surface state builds with Persistent Surface source modifiers"),
		FGridCombatSurfaceResolver::BuildState(
			Fire.SurfaceEffects[0], FGuid::NewGuid(), Fire.ActionId, PersistentModifiers, State));
	TestEqual(TEXT("3-round Terrain Architect surface becomes 5 rounds with Persistent Surface"), State.RemainingRounds, 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394F2ProductionAssetsTest,
	"Grimrock.RPG.RPG03.9.4F2.ProductionAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394F2ProductionAssetsTest::RunTest(const FString&)
{
	using namespace RPG0394MageAuthoring;
	URPGClassAsset* Mage = LoadObject<URPGClassAsset>(nullptr, FRPGMageAuthoring::MageAssetPath());
	if (!TestNotNull(TEXT("Production DA_Class_Mage loads"), Mage))
	{
		return false;
	}
	TestTrue(TEXT("Production Mage is structurally valid"), Mage->IsValidDefinition());
	TestEqual(TEXT("Production complete Mage has nine actions"), Mage->CombatActions.Num(), 9);
	TestEqual(TEXT("Production complete Mage has twenty-one Choice records"), Mage->ProgressionChoices.Num(), 21);
	TestNotNull(TEXT("Production Imbuement exists"), FindAction(Mage, ImbuementActionId));
	TestNotNull(TEXT("Production Elemental Conversion exists"), FindAction(Mage, ElementalConversionActionId));
	TestNotNull(TEXT("Production Terrain Architect exists"), FindAction(Mage, TerrainArchitectActionId));

	const FString StatusPath = FRPGMageAuthoring::GetStatusObjectPath(ImbuementStatusId);
	UGridStatusEffectDefinitionAsset* Imbuement =
		LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, *StatusPath);
	if (!TestNotNull(TEXT("Production Status_ElementalImbuement loads"), Imbuement))
	{
		return false;
	}
	TestTrue(TEXT("Production Elemental Imbuement is valid"), Imbuement->IsValidDefinition());
	TestEqual(TEXT("Production Elemental Imbuement has four affinity reactions"), Imbuement->CombatReactions.Num(), 4);
	return true;
}

#endif
