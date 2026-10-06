#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGPriestAuthoring.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"

namespace RPG0395B2
{
	URPGClassAsset* BuildPriest()
	{
		URPGClassAsset* Priest = NewObject<URPGClassAsset>(GetTransientPackage());
		Priest->ClassId = TEXT("Priest");
		Priest->DisplayName = FText::FromString(TEXT("Prêtre"));
		Priest->HealthAtLevelOne = 12;
		FRPGPriestAuthoring::ConfigureClass(*Priest);
		return Priest;
	}

	const FGridCombatActionDefinition* FindAction(const URPGClassAsset* Priest, FName ActionId)
	{
		return Priest ? Priest->CombatActions.FindByPredicate(
			[ActionId](const FGridCombatActionDefinition& Action)
			{
				return Action.ActionId == ActionId;
			}) : nullptr;
	}

	bool HasChainNode(const URPGClassAsset* Priest, FName ChoiceId, int32 Level, FName Prerequisite)
	{
		const FRPGClassProgressionChoiceDefinition* Choice = Priest ? Priest->FindProgressionChoice(ChoiceId) : nullptr;
		return Choice && Choice->MinimumLevel == Level && Choice->PointCost == 1 &&
			(Prerequisite.IsNone() ? Choice->PrerequisiteChoiceIds.IsEmpty() : Choice->PrerequisiteChoiceIds.Contains(Prerequisite));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395B2StructureTest, "Grimrock.RPG.RPG03.9.5B2.Structure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395B2StructureTest::RunTest(const FString&)
{
	using namespace RPG0395B2;
	URPGClassAsset* Priest = BuildPriest();
	for (const FGridCombatActionDefinition& Action : Priest->CombatActions)
	{
		TestTrue(*FString::Printf(TEXT("Authored action %s is structurally valid"), *Action.ActionId.ToString()), Action.IsValid());
	}
	TestTrue(TEXT("Complete transient Priest is structurally valid"), Priest->IsValidDefinition());
	TestEqual(TEXT("Priest has exactly fifteen conceptual Choice records"), Priest->ProgressionChoices.Num(), 15);
	TestEqual(TEXT("Priest has exactly fourteen active actions"), Priest->CombatActions.Num(), 14);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395B2ProgressionTest, "Grimrock.RPG.RPG03.9.5B2.Progression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395B2ProgressionTest::RunTest(const FString&)
{
	using namespace RPG0395B2;
	URPGClassAsset* Priest = BuildPriest();
	const FName HolyLight(TEXT("Talent_Priest_Exorcism_HolyLight"));
	const FName TurnUndead(TEXT("Talent_Priest_Exorcism_TurnUndead"));
	const FName HolyDispel(TEXT("Talent_Priest_Exorcism_HolyDispel"));
	const FName Smite(TEXT("Talent_Priest_Exorcism_Smite"));
	const FName Major(TEXT("Talent_Priest_Exorcism_MajorExorcism"));
	TestTrue(TEXT("Exorcism level 2"), HasChainNode(Priest, HolyLight, 2, NAME_None));
	TestTrue(TEXT("Exorcism level 6"), HasChainNode(Priest, TurnUndead, 6, HolyLight));
	TestTrue(TEXT("Exorcism level 10"), HasChainNode(Priest, HolyDispel, 10, TurnUndead));
	TestTrue(TEXT("Exorcism level 14"), HasChainNode(Priest, Smite, 14, HolyDispel));
	TestTrue(TEXT("Exorcism level 18"), HasChainNode(Priest, Major, 18, Smite));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395B2HolyDamageTest, "Grimrock.RPG.RPG03.9.5B2.HolyDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395B2HolyDamageTest::RunTest(const FString&)
{
	using namespace RPG0395B2;
	URPGClassAsset* Priest = BuildPriest();
	const FGridCombatActionDefinition* Light = FindAction(Priest, TEXT("Action_Priest_HolyLight"));
	const FGridCombatActionDefinition* Smite = FindAction(Priest, TEXT("Action_Priest_Smite"));
	const FGridCombatActionDefinition* Major = FindAction(Priest, TEXT("Action_Priest_MajorExorcism"));

	TestTrue(TEXT("Holy Light is 2 AP / 4 mana R5, base 5, WIS + Religion, Holy, no crit"), Light &&
		Light->ActionPointCost == 2 && Light->ResourceCosts.ManaCost == 4 && Light->RangeCells == 5 && Light->CooldownRounds == 0 &&
		Light->OffensiveProfile.AttackDefinition.DamageType == EGridDamageType::Holy &&
		Light->OffensiveProfile.AttackDefinition.MinDamage == 5 &&
		Light->OffensiveProfile.DamageScalingAttribute == EGridAttackScalingAttribute::Wisdom &&
		Light->DirectDamageScaling.AttributeModifierScale == 0 &&
		Light->DirectDamageScaling.ScalingSkillId == TEXT("Skill_Religion") &&
		!Light->OffensiveProfile.AttackDefinition.bCanCriticalHit);

	TestTrue(TEXT("Smite is 3 AP / 8 mana R4 CD2, base 10, 2x WIS + Religion"), Smite &&
		Smite->ActionPointCost == 3 && Smite->ResourceCosts.ManaCost == 8 && Smite->RangeCells == 4 && Smite->CooldownRounds == 2 &&
		Smite->OffensiveProfile.AttackDefinition.MinDamage == 10 &&
		Smite->OffensiveProfile.DamageScalingAttribute == EGridAttackScalingAttribute::Wisdom &&
		Smite->DirectDamageScaling.ScalingAttribute == EGridAttackScalingAttribute::Wisdom &&
		Smite->DirectDamageScaling.AttributeModifierScale == 1 &&
		Smite->DirectDamageScaling.ScalingSkillId == TEXT("Skill_Religion"));

	TestTrue(TEXT("Major Exorcism is 4 AP / 14 mana Area2 R4 CD5 restricted to Undead/Demon/Summoned"), Major &&
		Major->ActionPointCost == 4 && Major->ResourceCosts.ManaCost == 14 && Major->RangeCells == 4 &&
		Major->AreaRadiusCells == 2 && Major->CooldownRounds == 5 &&
		Major->TargetFilter.AllowedMonsterCategoryIds.Contains(TEXT("Undead")) &&
		Major->TargetFilter.AllowedMonsterCategoryIds.Contains(TEXT("Demon")) &&
		Major->TargetFilter.AllowedMonsterCategoryIds.Contains(TEXT("Summoned")) &&
		Major->StatusApplications.Num() == 1 &&
		Major->StatusApplications[0].StatusEffectId == TEXT("Status_Banished") &&
		Major->StatusApplications[0].ArmorGate == EGridCombatStatusArmorGate::MagicalArmorDepleted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395B2FavoredHolyTargetsTest, "Grimrock.RPG.RPG03.9.5B2.FavoredHolyTargets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395B2FavoredHolyTargetsTest::RunTest(const FString&)
{
	using namespace RPG0395B2;
	URPGClassAsset* Priest = BuildPriest();
	for (const FName ChoiceId : { FName(TEXT("Talent_Priest_Exorcism_HolyLight")), FName(TEXT("Talent_Priest_Exorcism_Smite")) })
	{
		const FRPGClassProgressionChoiceDefinition* Choice = Priest->FindProgressionChoice(ChoiceId);
		if (!TestNotNull(*FString::Printf(TEXT("%s choice exists"), *ChoiceId.ToString()), Choice) || Choice->CombatModifiers.Num() != 1)
		{
			continue;
		}
		const FGridCombatModifierProfile& Profile = Choice->CombatModifiers[0];
		TestEqual(TEXT("Undead/Demon bonus is +50 percent"), Profile.OutgoingDamagePercentModifier, 50);
		TestTrue(TEXT("Bonus covers Undead"), Profile.AllowedTargetMonsterCategoryIds.Contains(TEXT("Undead")));
		TestTrue(TEXT("Bonus covers Demon"), Profile.AllowedTargetMonsterCategoryIds.Contains(TEXT("Demon")));

		FGridCombatModifierContext Context;
		Context.ActionId = Profile.ActionIds[0];
		Context.SourcePolicy = EGridCombatActionSourcePolicy::Spell;
		Context.ActionType = EGridCombatActionType::Ability;
		Context.TargetMonsterCategoryId = TEXT("Undead");
		FGridResolvedCombatModifiers Resolved;
		FGridCombatModifierResolver::Resolve({ Profile }, Context, Resolved);
		TestEqual(TEXT("Undead context resolves +50 percent"), Resolved.OutgoingDamagePercentModifier, 50);
		Context.TargetMonsterCategoryId = TEXT("Vermin");
		FGridCombatModifierResolver::Resolve({ Profile }, Context, Resolved);
		TestEqual(TEXT("Unrelated category resolves no bonus"), Resolved.OutgoingDamagePercentModifier, 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395B2TurnUndeadTest, "Grimrock.RPG.RPG03.9.5B2.TurnUndead",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395B2TurnUndeadTest::RunTest(const FString&)
{
	using namespace RPG0395B2;
	URPGClassAsset* Priest = BuildPriest();
	const FGridCombatActionDefinition* Action = FindAction(Priest, TEXT("Action_Priest_TurnUndead"));
	TestTrue(TEXT("Turn Undead is party-centered Area2, 3 AP / 7 mana, CD3, Undead only"), Action &&
		Action->TargetingPolicy == EGridCombatTargetingPolicy::Area && Action->bAreaCenteredOnParty &&
		Action->AreaRadiusCells == 2 && Action->ActionPointCost == 3 && Action->ResourceCosts.ManaCost == 7 &&
		Action->CooldownRounds == 3 && Action->TargetFilter.AllowedMonsterCategoryIds.Num() == 1 &&
		Action->TargetFilter.AllowedMonsterCategoryIds.Contains(TEXT("Undead")) &&
		Action->OffensiveProfile.AttackDefinition.MinDamage == 4 && Action->OffensiveProfile.AttackDefinition.MaxDamage == 4);

	TestTrue(TEXT("Turn Undead pushes one cell away only after magical armor depletion"), Action &&
		Action->MovementEffects.Num() == 1 &&
		Action->MovementEffects[0].Subject == EGridCombatMovementSubject::TargetCombatant &&
		Action->MovementEffects[0].Direction == EGridCombatMovementDirection::AwayFromSource &&
		Action->MovementEffects[0].DistanceCells == 1 && Action->MovementEffects[0].bForced &&
		Action->MovementEffects[0].ArmorGate == EGridCombatStatusArmorGate::MagicalArmorDepleted);

	TestTrue(TEXT("Turn Undead applies one-round initiative penalty behind same armor gate"), Action &&
		Action->StatusApplications.Num() == 1 &&
		Action->StatusApplications[0].StatusEffectId == TEXT("Status_TurnedUndead") &&
		Action->StatusApplications[0].DurationOverride == 1 &&
		Action->StatusApplications[0].ArmorGate == EGridCombatStatusArmorGate::MagicalArmorDepleted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395B2HolyDispelTest, "Grimrock.RPG.RPG03.9.5B2.HolyDispel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395B2HolyDispelTest::RunTest(const FString&)
{
	using namespace RPG0395B2;
	URPGClassAsset* Priest = BuildPriest();
	const FGridCombatActionDefinition* Action = FindAction(Priest, TEXT("Action_Priest_HolyDispel"));
	if (!TestNotNull(TEXT("Holy Dispel exists"), Action))
	{
		return false;
	}
	TestTrue(TEXT("Holy Dispel is AllyOrHostile R3, 2 AP / 6 mana, CD2, hostile side restricted to Undead"),
		Action->TargetingPolicy == EGridCombatTargetingPolicy::AllyOrHostile &&
		Action->RangeCells == 3 && Action->ActionPointCost == 2 && Action->ResourceCosts.ManaCost == 6 &&
		Action->CooldownRounds == 2 && Action->TargetFilter.AllowedMonsterCategoryIds.Contains(TEXT("Undead")));
	TestEqual(TEXT("Holy Dispel has ally and hostile removal profiles"), Action->StatusRemovals.Num(), 2);
	const FGridCombatStatusRemovalProfile* Ally = Action->StatusRemovals.FindByPredicate(
		[](const FGridCombatStatusRemovalProfile& Profile)
		{
			return Profile.TargetSide == EGridCombatStatusRemovalTargetSide::Party;
		});
	const FGridCombatStatusRemovalProfile* Hostile = Action->StatusRemovals.FindByPredicate(
		[](const FGridCombatStatusRemovalProfile& Profile)
		{
			return Profile.TargetSide == EGridCombatStatusRemovalTargetSide::Hostile;
		});
	TestTrue(TEXT("Ally side removes up to two Necrotic/Curse Debuffs"), Ally &&
		Ally->MaximumRemovals == 2 &&
		Ally->AllowedDispositions.Contains(EGridStatusEffectDisposition::Debuff) &&
		Ally->AnyStatusTags.Contains(TEXT("Necrotic")) && Ally->AnyStatusTags.Contains(TEXT("Curse")));
	TestTrue(TEXT("Undead hostile side removes one magical Buff"), Hostile &&
		Hostile->MaximumRemovals == 1 &&
		Hostile->AllowedDispositions.Contains(EGridStatusEffectDisposition::Buff) &&
		Hostile->AnyStatusTags.Contains(TEXT("Dispel.Magical")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395B2StatusTest, "Grimrock.RPG.RPG03.9.5B2.Statuses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395B2StatusTest::RunTest(const FString&)
{
	UGridStatusEffectDefinitionAsset* Turned = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	TestTrue(TEXT("Status_TurnedUndead configures"), FRPGPriestAuthoring::ConfigureStatus(*Turned, TEXT("Status_TurnedUndead")));
	TestTrue(TEXT("Turned Undead is a one-round -4 initiative Debuff"),
		Turned->IsValidDefinition() && Turned->Disposition == EGridStatusEffectDisposition::Debuff &&
		Turned->DurationUnit == EGridStatusEffectDurationUnit::Rounds && Turned->DefaultDuration == 1 &&
		Turned->InitiativeModifier == -4);

	UGridStatusEffectDefinitionAsset* Banished = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	TestTrue(TEXT("Status_Banished configures"), FRPGPriestAuthoring::ConfigureStatus(*Banished, TEXT("Status_Banished")));
	TestTrue(TEXT("Banished lasts one Turn and skips activation"),
		Banished->IsValidDefinition() && Banished->Disposition == EGridStatusEffectDisposition::Debuff &&
		Banished->DurationUnit == EGridStatusEffectDurationUnit::Turns && Banished->DefaultDuration == 1 &&
		Banished->Control.bSkipActivation);
	return true;
}

#endif
