#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGPriestAuthoring.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Combat/GridCombatArmorEffectResolver.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"

namespace RPG0395B1
{
	const FName EnhancedHealing(TEXT("Talent_Priest_Restoration_EnhancedHealing"));
	const FName Regeneration(TEXT("Talent_Priest_Restoration_Regeneration"));
	const FName GroupHeal(TEXT("Talent_Priest_Restoration_GroupHeal"));
	const FName Purification(TEXT("Talent_Priest_Restoration_Purification"));
	const FName Miracle(TEXT("Talent_Priest_Restoration_Miracle"));
	const FName Blessing(TEXT("Talent_Priest_Protection_Blessing"));
	const FName Aegis(TEXT("Talent_Priest_Protection_Aegis"));
	const FName HolyProtection(TEXT("Talent_Priest_Protection_HolyProtection"));
	const FName Sanctuary(TEXT("Talent_Priest_Protection_Sanctuary"));
	const FName DivineBastion(TEXT("Talent_Priest_Protection_DivineBastion"));

	URPGClassAsset* BuildPriest()
	{
		URPGClassAsset* Priest = NewObject<URPGClassAsset>(GetTransientPackage());
		Priest->ClassId = TEXT("Priest");
		Priest->DisplayName = FText::FromString(TEXT("Prêtre"));
		Priest->HealthAtLevelOne = 12;
		for (const int32 Level : { 2, 6, 10, 14, 18 })
		{
			FRPGClassProgressionLevelGrant Grant;
			Grant.Level = Level;
			Grant.ChoicePointsGranted = 1;
			Priest->ProgressionLevelGrants.Add(Grant);
		}
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395B1StructureTest, "Grimrock.RPG.RPG03.9.5B1.Structure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395B1StructureTest::RunTest(const FString&)
{
	using namespace RPG0395B1;
	URPGClassAsset* Priest = BuildPriest();
	for (const FGridCombatActionDefinition& Action : Priest->CombatActions)
	{
		TestTrue(*FString::Printf(TEXT("Authored action %s is structurally valid"), *Action.ActionId.ToString()), Action.IsValid());
	}
	TestTrue(TEXT("B1 Priest is structurally valid"), Priest->IsValidDefinition());
	TestTrue(TEXT("Complete Priest still contains at least the ten B1 Choice records"), Priest->ProgressionChoices.Num() >= 10);
	TestTrue(TEXT("Complete Priest still contains at least the nine B1 active spells"), Priest->CombatActions.Num() >= 9);

	TestTrue(TEXT("Restoration chain level 2"), HasChainNode(Priest, EnhancedHealing, 2, NAME_None));
	TestTrue(TEXT("Restoration chain level 6"), HasChainNode(Priest, Regeneration, 6, EnhancedHealing));
	TestTrue(TEXT("Restoration chain level 10"), HasChainNode(Priest, GroupHeal, 10, Regeneration));
	TestTrue(TEXT("Restoration chain level 14"), HasChainNode(Priest, Purification, 14, GroupHeal));
	TestTrue(TEXT("Restoration chain level 18"), HasChainNode(Priest, Miracle, 18, Purification));

	TestTrue(TEXT("Protection chain level 2"), HasChainNode(Priest, Blessing, 2, NAME_None));
	TestTrue(TEXT("Protection chain level 6"), HasChainNode(Priest, Aegis, 6, Blessing));
	TestTrue(TEXT("Protection chain level 10"), HasChainNode(Priest, HolyProtection, 10, Aegis));
	TestTrue(TEXT("Protection chain level 14"), HasChainNode(Priest, Sanctuary, 14, HolyProtection));
	TestTrue(TEXT("Protection chain level 18"), HasChainNode(Priest, DivineBastion, 18, Sanctuary));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395B1RestorationActionsTest, "Grimrock.RPG.RPG03.9.5B1.RestorationActions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395B1RestorationActionsTest::RunTest(const FString&)
{
	using namespace RPG0395B1;
	URPGClassAsset* Priest = BuildPriest();

	const FRPGClassProgressionChoiceDefinition* Enhanced = Priest->FindProgressionChoice(EnhancedHealing);
	TestTrue(TEXT("Enhanced Healing is +25 percent outgoing Spell healing"), Enhanced && Enhanced->CombatModifiers.Num() == 1 &&
		Enhanced->CombatModifiers[0].SourcePolicies.Contains(EGridCombatActionSourcePolicy::Spell) &&
		Enhanced->CombatModifiers[0].OutgoingHealingPercentModifier == 25);

	const FGridCombatActionDefinition* Regen = FindAction(Priest, TEXT("Action_Priest_Regeneration"));
	TestTrue(TEXT("Regeneration is 2 AP / 5 mana Ally R3 CD2"), Regen &&
		Regen->SourcePolicy == EGridCombatActionSourcePolicy::Spell &&
		Regen->TargetingPolicy == EGridCombatTargetingPolicy::Ally &&
		Regen->ActionPointCost == 2 && Regen->ResourceCosts.ManaCost == 5 &&
		Regen->RangeCells == 3 && Regen->CooldownRounds == 2 &&
		Regen->StatusApplications.Num() == 1 &&
		Regen->StatusApplications[0].StatusEffectId == TEXT("Status_Regeneration") &&
		Regen->StatusApplications[0].DurationOverride == 3);

	const FGridCombatActionDefinition* Group = FindAction(Priest, TEXT("Action_Priest_GroupHeal"));
	TestTrue(TEXT("Group Heal is 3 AP / 8 mana Party CD3 with 5 + WIS + Medicine"), Group &&
		Group->TargetingPolicy == EGridCombatTargetingPolicy::Party &&
		Group->ActionPointCost == 3 && Group->ResourceCosts.ManaCost == 8 && Group->CooldownRounds == 3 &&
		Group->EffectProfile.RestoreHealth == 5 &&
		Group->HealingScaling.ScalingAttribute == EGridAttackScalingAttribute::Wisdom &&
		Group->HealingScaling.AttributeModifierScale == 1 &&
		Group->HealingScaling.ScalingSkillId == TEXT("Skill_Medicine") &&
		Group->HealingScaling.SkillRankScale == 1);

	const FGridCombatActionDefinition* Purify = FindAction(Priest, TEXT("Action_Priest_Purification"));
	TestTrue(TEXT("Purification removes up to two party Debuffs using explicit ids or Purifiable tag"), Purify &&
		Purify->ActionPointCost == 2 && Purify->ResourceCosts.ManaCost == 6 && Purify->CooldownRounds == 2 &&
		Purify->StatusRemovals.Num() == 1 &&
		Purify->StatusRemovals[0].MaximumRemovals == 2 &&
		Purify->StatusRemovals[0].TargetSide == EGridCombatStatusRemovalTargetSide::Party &&
		Purify->StatusRemovals[0].AllowedDispositions.Contains(EGridStatusEffectDisposition::Debuff) &&
		Purify->StatusRemovals[0].AnyStatusTags.Contains(TEXT("Purifiable")) &&
		Purify->StatusRemovals[0].EffectIds.Contains(TEXT("Status_Poison")));

	const FGridCombatActionDefinition* MiracleAction = FindAction(Priest, TEXT("Action_Priest_Miracle"));
	TestTrue(TEXT("Miracle keeps canonical costs, floor, purge and 25 percent magical armor restore"), MiracleAction &&
		MiracleAction->ActionPointCost == 4 && MiracleAction->ResourceCosts.ManaCost == 15 &&
		MiracleAction->CooldownRounds == 5 && MiracleAction->EffectProfile.RestoreHealth == 12 &&
		MiracleAction->HealingScaling.AttributeModifierScale == 2 &&
		MiracleAction->HealingScaling.ScalingSkillId == TEXT("Skill_Religion") &&
		MiracleAction->HealingScaling.SkillRankScale == 1 &&
		MiracleAction->HealingScaling.MinimumTargetHealthPercent == 50 &&
		MiracleAction->StatusRemovals.Num() == 1 && MiracleAction->StatusRemovals[0].MaximumRemovals == 3 &&
		MiracleAction->ArmorEffects.Num() == 1 &&
		MiracleAction->ArmorEffects[0].Magnitude == EGridCombatArmorEffectMagnitude::ReferencePercent &&
		MiracleAction->ArmorEffects[0].Amount == 25);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395B1HealingFormulaTest, "Grimrock.RPG.RPG03.9.5B1.HealingFormula",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395B1HealingFormulaTest::RunTest(const FString&)
{
	using namespace RPG0395B1;
	URPGClassAsset* Priest = BuildPriest();
	const FGridCombatActionDefinition* Group = FindAction(Priest, TEXT("Action_Priest_GroupHeal"));
	const FGridCombatActionDefinition* MiracleAction = FindAction(Priest, TEXT("Action_Priest_Miracle"));
	if (!TestNotNull(TEXT("Group Heal exists"), Group) || !TestNotNull(TEXT("Miracle exists"), MiracleAction))
	{
		return false;
	}

	FRPGAttributes Attributes;
	Attributes.Wisdom = 16;
	FRPGSkillRank Medicine;
	Medicine.SkillId = TEXT("Skill_Medicine");
	Medicine.Rank = 4;
	FRPGSkillRank Religion;
	Religion.SkillId = TEXT("Skill_Religion");
	Religion.Rank = 4;

	FGridResolvedCombatModifiers Enhanced;
	Enhanced.OutgoingHealingPercentModifier = 25;
	TestEqual(TEXT("Group Heal: (5 + WIS mod 3 + Medicine 4) x 1.25 = 15"),
		FGridCombatModifierResolver::ResolveDirectHealthRestore(
			*Group, Group->EffectProfile, Attributes, { Medicine }, Enhanced, 20, 100), 15);
	TestEqual(TEXT("Miracle floor reaches 50 percent when formula is lower"),
		FGridCombatModifierResolver::ResolveDirectHealthRestore(
			*MiracleAction, MiracleAction->EffectProfile, Attributes, { Religion }, FGridResolvedCombatModifiers(), 20, 100), 30);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395B1ProtectionActionsTest, "Grimrock.RPG.RPG03.9.5B1.ProtectionActions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395B1ProtectionActionsTest::RunTest(const FString&)
{
	using namespace RPG0395B1;
	URPGClassAsset* Priest = BuildPriest();

	const FGridCombatActionDefinition* Bless = FindAction(Priest, TEXT("Action_Priest_Blessing"));
	const FGridCombatActionDefinition* AegisAction = FindAction(Priest, TEXT("Action_Priest_Aegis"));
	const FGridCombatActionDefinition* Holy = FindAction(Priest, TEXT("Action_Priest_HolyProtection"));
	const FGridCombatActionDefinition* SanctuaryAction = FindAction(Priest, TEXT("Action_Priest_Sanctuary"));
	const FGridCombatActionDefinition* Bastion = FindAction(Priest, TEXT("Action_Priest_DivineBastion"));

	TestTrue(TEXT("Blessing is 2 AP / 5 mana Ally R3 CD2"), Bless &&
		Bless->ActionPointCost == 2 && Bless->ResourceCosts.ManaCost == 5 && Bless->RangeCells == 3 && Bless->CooldownRounds == 2);
	TestTrue(TEXT("Aegis is 2 AP / 6 mana Ally R3 CD2 with WIS + Religion armor scaling"), AegisAction &&
		AegisAction->ActionPointCost == 2 && AegisAction->ResourceCosts.ManaCost == 6 &&
		AegisAction->CooldownRounds == 2 && AegisAction->ArmorEffects.Num() == 1 &&
		AegisAction->ArmorEffects[0].Amount == 8 &&
		AegisAction->ArmorEffects[0].ScalingAttribute == EGridAttackScalingAttribute::Wisdom &&
		AegisAction->ArmorEffects[0].ScalingSkillId == TEXT("Skill_Religion"));
	TestTrue(TEXT("Holy Protection is 2 AP / 7 mana Ally R3 CD3"), Holy &&
		Holy->ActionPointCost == 2 && Holy->ResourceCosts.ManaCost == 7 && Holy->CooldownRounds == 3);
	TestTrue(TEXT("Sanctuary is 3 AP / 10 mana Ally R3 CD4"), SanctuaryAction &&
		SanctuaryAction->ActionPointCost == 3 && SanctuaryAction->ResourceCosts.ManaCost == 10 &&
		SanctuaryAction->CooldownRounds == 4);
	TestTrue(TEXT("Divine Bastion is 4 AP / 12 mana Party CD5 and restores 35 percent magical armor"), Bastion &&
		Bastion->ActionPointCost == 4 && Bastion->ResourceCosts.ManaCost == 12 && Bastion->CooldownRounds == 5 &&
		Bastion->ArmorEffects.Num() == 1 &&
		Bastion->ArmorEffects[0].Magnitude == EGridCombatArmorEffectMagnitude::ReferencePercent &&
		Bastion->ArmorEffects[0].Amount == 35);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395B1PriestStatusesTest, "Grimrock.RPG.RPG03.9.5B1.PriestStatuses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395B1PriestStatusesTest::RunTest(const FString&)
{
	for (const FName EffectId : {
		FName(TEXT("Status_Regeneration")), FName(TEXT("Status_Blessed")), FName(TEXT("Status_HolyProtection")),
		FName(TEXT("Status_Sanctuary")), FName(TEXT("Status_DivineBastion")) })
	{
		UGridStatusEffectDefinitionAsset* Status = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
		TestTrue(*FString::Printf(TEXT("%s configures and is valid"), *EffectId.ToString()),
			FRPGPriestAuthoring::ConfigureStatus(*Status, EffectId) && Status->IsValidDefinition());
	}

	UGridStatusEffectDefinitionAsset* Regen = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	FRPGPriestAuthoring::ConfigureStatus(*Regen, TEXT("Status_Regeneration"));
	TestTrue(TEXT("Regeneration is 3 Turns and ticks 3 + WIS mod as Spell healing"),
		Regen->DurationUnit == EGridStatusEffectDurationUnit::Turns && Regen->DefaultDuration == 3 &&
		Regen->PeriodicHealing.HealingPerStack == 3 &&
		Regen->PeriodicHealing.SourcePolicy == EGridCombatActionSourcePolicy::Spell &&
		Regen->PeriodicHealing.ScalingAttribute == EGridAttackScalingAttribute::Wisdom &&
		Regen->PeriodicHealing.AttributeModifierScale == 1);

	UGridStatusEffectDefinitionAsset* Blessed = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	FRPGPriestAuthoring::ConfigureStatus(*Blessed, TEXT("Status_Blessed"));
	TestTrue(TEXT("Blessed is RefreshDuration, +4 Initiative and +2 Accuracy"), Blessed->StackPolicy == EGridStatusEffectStackPolicy::RefreshDuration &&
		Blessed->DefaultDuration == 2 && Blessed->InitiativeModifier == 4 &&
		Blessed->CombatModifiers.Num() == 1 && Blessed->CombatModifiers[0].AccuracyModifier == 2);

	UGridStatusEffectDefinitionAsset* Holy = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	FRPGPriestAuthoring::ConfigureStatus(*Holy, TEXT("Status_HolyProtection"));
	TestTrue(TEXT("Holy Protection is Spell-only, non-Area, +2 Evasion and +25 holy/necrotic/arcane resistance"), Holy->CombatModifiers.Num() == 1 &&
		Holy->CombatModifiers[0].SourcePolicies.Contains(EGridCombatActionSourcePolicy::Spell) &&
		Holy->CombatModifiers[0].bExcludeAreaActions &&
		Holy->CombatModifiers[0].EvasionModifier == 2 &&
		Holy->CombatModifiers[0].ResistanceModifiers.HolyResistance == 25 &&
		Holy->CombatModifiers[0].ResistanceModifiers.NecroticResistance == 25 &&
		Holy->CombatModifiers[0].ResistanceModifiers.ArcaneResistance == 25);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0395B1SanctuaryBastionTest, "Grimrock.RPG.RPG03.9.5B1.SanctuaryBastion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0395B1SanctuaryBastionTest::RunTest(const FString&)
{
	UGridStatusEffectDefinitionAsset* SanctuaryStatus = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	FRPGPriestAuthoring::ConfigureStatus(*SanctuaryStatus, TEXT("Status_Sanctuary"));
	TestTrue(TEXT("Sanctuary blocks direct hostile targeting and expires at owner activation"),
		SanctuaryStatus->Control.bBlockDirectHostileTargeting &&
		SanctuaryStatus->bExpireAtOwnerNextActivation &&
		SanctuaryStatus->DurationUnit == EGridStatusEffectDurationUnit::Rounds &&
		SanctuaryStatus->DefaultDuration == 2);
	TestTrue(TEXT("Sanctuary has one damage-dealt consumption reaction"), SanctuaryStatus->CombatReactions.Num() == 1 &&
		SanctuaryStatus->CombatReactions[0].Trigger == EGridCombatReactionTrigger::AttackHit &&
		SanctuaryStatus->CombatReactions[0].bRequireOwnerAsEventSource &&
		SanctuaryStatus->CombatReactions[0].bRequireAppliedDamage &&
		SanctuaryStatus->CombatReactions[0].bConsumeOwningStatus);

	UGridStatusEffectDefinitionAsset* Bastion = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	FRPGPriestAuthoring::ConfigureStatus(*Bastion, TEXT("Status_DivineBastion"));
	TestTrue(TEXT("Divine Bastion lasts two rounds and reduces non-Physical damage by 20 percent"),
		Bastion->DefaultDuration == 2 && Bastion->CombatModifiers.Num() == 1 &&
		Bastion->CombatModifiers[0].IncomingDamagePercentModifier == -20 &&
		!Bastion->CombatModifiers[0].DamageTypes.Contains(EGridDamageType::Physical) &&
		Bastion->CombatModifiers[0].DamageTypes.Contains(EGridDamageType::Holy) &&
		Bastion->CombatModifiers[0].DamageTypes.Contains(EGridDamageType::Arcane));
	return true;
}

#endif
