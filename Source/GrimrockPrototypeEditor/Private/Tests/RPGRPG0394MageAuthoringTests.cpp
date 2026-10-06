#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGMageAuthoring.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridCombatReactionResolver.h"
#include "Runtime/GridInventoryTypes.h"

namespace RPG0394CMageAuthoring
{
	const FName AffinityAlias(TEXT("Talent_Mage_Evoker_ElementalAffinity"));
	const FName AffinityGroup(TEXT("TalentGroup_Mage_Evoker_ElementalAffinity"));
	const FName OverloadTalentId(TEXT("Talent_Mage_Evoker_ElementalOverload"));
	const FName OverloadActionId(TEXT("Action_Mage_ElementalOverload"));
	const FName OverloadStatusId(TEXT("Status_ElementalOverload"));

	struct FAffinityExpectation
	{
		const TCHAR* Suffix;
		const TCHAR* SchoolTag;
	};

	const FAffinityExpectation Affinities[] = {
		{ TEXT("Fire"), TEXT("Spell.School.Fire") },
		{ TEXT("Frost"), TEXT("Spell.School.Frost") },
		{ TEXT("Air"), TEXT("Spell.School.Air") },
		{ TEXT("Earth"), TEXT("Spell.School.Earth") }
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

		FRPGClassProgressionLevelGrant Level2;
		Level2.Level = 2;
		Level2.ChoicePointsGranted = 1;
		Mage->ProgressionLevelGrants.Add(Level2);

		FRPGClassProgressionLevelGrant Level6;
		Level6.Level = 6;
		Level6.ChoicePointsGranted = 1;
		Mage->ProgressionLevelGrants.Add(Level6);

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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394CMageClassAuthoringTest, "Grimrock.RPG.RPG03.9.4C.MageClassAuthoring",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394CMageClassAuthoringTest::RunTest(const FString&)
{
	using namespace RPG0394CMageAuthoring;
	URPGClassAsset* Mage = BuildMage();

	TestTrue(TEXT("Transient Mage Evoker authoring is structurally valid"), Mage->IsValidDefinition());
	TestEqual(TEXT("Evoker foundation authors one active action"), Mage->CombatActions.Num(), 1);
	TestEqual(TEXT("Four affinity variants plus Elemental Overload produce five choice records"), Mage->ProgressionChoices.Num(), 5);

	const FGridCombatActionDefinition* Overload = FindAction(Mage, OverloadActionId);
	TestTrue(TEXT("Elemental Overload is a 1 AP / 4 mana self action with cooldown three"), Overload &&
		Overload->SourcePolicy == EGridCombatActionSourcePolicy::Ability &&
		Overload->TargetingPolicy == EGridCombatTargetingPolicy::Self &&
		Overload->ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
		Overload->ActionPointCost == 1 && Overload->ResourceCosts.ManaCost == 4 && Overload->CooldownRounds == 3);
	TestTrue(TEXT("Elemental Overload applies Status_ElementalOverload for one turn"), Overload &&
		Overload->StatusApplications.Num() == 1 &&
		Overload->StatusApplications[0].StatusEffectId == OverloadStatusId &&
		Overload->StatusApplications[0].DurationOverride == 1);

	const FRPGClassProgressionChoiceDefinition* OverloadChoice = Mage->FindProgressionChoice(OverloadTalentId);
	TestTrue(TEXT("Elemental Overload depends on the logical Elemental Affinity alias"), OverloadChoice &&
		OverloadChoice->PrerequisiteRequirementIds.Contains(AffinityAlias));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394CAffinityAuthoringTest, "Grimrock.RPG.RPG03.9.4C.ElementalAffinity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394CAffinityAuthoringTest::RunTest(const FString&)
{
	using namespace RPG0394CMageAuthoring;
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
	using namespace RPG0394CMageAuthoring;
	UGridStatusEffectDefinitionAsset* Status = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	TestTrue(TEXT("Elemental Overload status configures successfully"), FRPGMageAuthoring::ConfigureElementalOverloadStatus(*Status));
	TestTrue(TEXT("Elemental Overload status is structurally valid"), Status->IsValidDefinition());
	TestEqual(TEXT("Elemental Overload uses the canonical EffectId"), Status->EffectId, OverloadStatusId);
	TestEqual(TEXT("Elemental Overload lasts one Turn"), Status->DurationUnit, EGridStatusEffectDurationUnit::Turns);
	TestEqual(TEXT("Elemental Overload default duration is one"), Status->DefaultDuration, 1);
	TestEqual(TEXT("Elemental Overload has four affinity-conditioned modifiers"), Status->CombatModifiers.Num(), 4);
	TestEqual(TEXT("Elemental Overload has four affinity-conditioned consumption reactions"), Status->CombatReactions.Num(), 4);

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Affinities); ++Index)
	{
		const FAffinityExpectation& Affinity = Affinities[Index];
		const FName ChoiceId = MakeAffinityChoiceId(Affinity.Suffix);
		const FName SchoolTag(Affinity.SchoolTag);
		const FGridCombatModifierProfile& Modifier = Status->CombatModifiers[Index];
		const FGridCombatReactionProfile& Reaction = Status->CombatReactions[Index];

		TestTrue(TEXT("Overload modifier is owner-affinity and spell-school conditioned"),
			Modifier.RequiredOwnerRequirementIds.Contains(ChoiceId) &&
			Modifier.RequiredSourceTags.Contains(SchoolTag) &&
			Modifier.SourcePolicies.Contains(EGridCombatActionSourcePolicy::Spell) &&
			Modifier.OutgoingDamagePercentModifier == 35);
		TestTrue(TEXT("Overload reaction matches the same affinity and consumes the status"),
			Reaction.RequiredOwnerRequirementIds.Contains(ChoiceId) &&
			Reaction.RequiredSourceTags.Contains(SchoolTag) &&
			Reaction.SourcePolicies.Contains(EGridCombatActionSourcePolicy::Spell) &&
			Reaction.Trigger == EGridCombatReactionTrigger::ActionResolved &&
			Reaction.Limit == EGridCombatReactionLimit::OncePerAction &&
			Reaction.bConsumeOwningStatus);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394CEvokerRuntimeCompositionTest, "Grimrock.RPG.RPG03.9.4C.EvokerRuntimeComposition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394CEvokerRuntimeCompositionTest::RunTest(const FString&)
{
	using namespace RPG0394CMageAuthoring;
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

	FGridCombatReactionEvent FrostEvent = FireEvent;
	FrostEvent.EventId = FGuid::NewGuid();
	FrostEvent.ActionInstanceId = FGuid::NewGuid();
	FrostEvent.ActionId = FrostSpell.ActionId;
	FrostEvent.SourceTags = FrostSpell.SourceTags;
	FGridCombatReactionResolver::ResolveMatches(Bindings, Character.CharacterId, FrostEvent, Ledger, false, Matches);
	TestEqual(TEXT("A Frost spell cannot consume a Fire-affinity Overload"), Matches.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0394CProductionAssetsTest, "Grimrock.RPG.RPG03.9.4C.ProductionAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0394CProductionAssetsTest::RunTest(const FString&)
{
	using namespace RPG0394CMageAuthoring;
	URPGClassAsset* Mage = LoadObject<URPGClassAsset>(nullptr, FRPGMageAuthoring::MageAssetPath());
	if (!TestNotNull(TEXT("Production DA_Class_Mage loads"), Mage))
	{
		return false;
	}
	TestEqual(TEXT("Production Mage keeps canonical ClassId"), Mage->ClassId, FName(TEXT("Mage")));
	TestEqual(TEXT("Production Mage Evoker foundation has one active action"), Mage->CombatActions.Num(), 1);
	TestEqual(TEXT("Production Mage Evoker foundation has five choice records"), Mage->ProgressionChoices.Num(), 5);
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

#endif
