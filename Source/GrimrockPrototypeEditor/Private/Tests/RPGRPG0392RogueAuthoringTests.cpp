#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRogueAuthoring.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"

namespace RPG0392Authoring
{
	URPGClassAsset* BuildRogue()
	{
		URPGClassAsset* Rogue = NewObject<URPGClassAsset>(GetTransientPackage());
		Rogue->ClassId = TEXT("Rogue");
		Rogue->DisplayName = FText::FromString(TEXT("Voleur"));
		Rogue->HealthAtLevelOne = 14;
		FRPGRogueAuthoring::ConfigureClass(*Rogue);
		return Rogue;
	}

	const FGridCombatActionDefinition* FindAction(const URPGClassAsset* Rogue, FName ActionId)
	{
		return Rogue ? Rogue->CombatActions.FindByPredicate(
			[ActionId](const FGridCombatActionDefinition& Action)
			{
				return Action.ActionId == ActionId;
			}) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0392RogueClassAuthoringTest, "Grimrock.RPG.RPG03.9.2.Authoring.RogueClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0392RogueClassAuthoringTest::RunTest(const FString&)
{
	URPGClassAsset* Rogue = RPG0392Authoring::BuildRogue();
	TestEqual(TEXT("Rogue authoring exposes eleven active actions"), Rogue->CombatActions.Num(), 11);
	TestEqual(TEXT("Rogue authoring exposes exactly fifteen canonical talent choices"), Rogue->ProgressionChoices.Num(), 15);
	TestTrue(TEXT("Transient authored Rogue is structurally valid"), Rogue->IsValidDefinition());

	for (const FName ChoiceId : {
		FName(TEXT("Talent_Rogue_Assassin_SneakAttack")),
		FName(TEXT("Talent_Rogue_Shadow_Dodge")),
		FName(TEXT("Talent_Rogue_Saboteur_ExpertDisarm"))
	})
	{
		const FRPGClassProgressionChoiceDefinition* Choice = Rogue->FindProgressionChoice(ChoiceId);
		TestTrue(*FString::Printf(TEXT("%s exists"), *ChoiceId.ToString()), Choice != nullptr);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0392AssassinAuthoringTest, "Grimrock.RPG.RPG03.9.2.Authoring.Assassin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0392AssassinAuthoringTest::RunTest(const FString&)
{
	URPGClassAsset* Rogue = RPG0392Authoring::BuildRogue();

	const FGridCombatActionDefinition* Sneak = RPG0392Authoring::FindAction(Rogue, TEXT("Action_Rogue_SneakAttack"));
	TestTrue(TEXT("Sneak Attack is light-weapon 125 percent WD"), Sneak &&
		Sneak->WeaponAttackProfile.RequiredItemTags.Contains(TEXT("Weapon.Light")) &&
		Sneak->WeaponAttackProfile.WeaponDamagePercent == 125);

	const FRPGClassProgressionChoiceDefinition* SneakChoice =
		Rogue->FindProgressionChoice(TEXT("Talent_Rogue_Assassin_SneakAttack"));
	TestTrue(TEXT("Sneak Attack conditional bonus adds 50 WD points"), SneakChoice &&
		SneakChoice->CombatModifiers.Num() == 1 &&
		SneakChoice->CombatModifiers[0].WeaponDamagePercentModifier == 50 &&
		SneakChoice->CombatModifiers[0].AnyTargetConditions.Contains(EGridCombatTargetCondition::HasNotActedThisRound) &&
		SneakChoice->CombatModifiers[0].AnyTargetConditions.Contains(EGridCombatTargetCondition::PhysicalControl) &&
		SneakChoice->CombatModifiers[0].AnyTargetConditions.Contains(EGridCombatTargetCondition::RearArc));

	const FRPGClassProgressionChoiceDefinition* Backstab =
		Rogue->FindProgressionChoice(TEXT("Talent_Rogue_Assassin_Backstab"));
	TestTrue(TEXT("Backstab is rear-arc light melee, non-AoE, +20 damage and +20 critical chance"), Backstab &&
		Backstab->CombatModifiers.Num() == 1 &&
		Backstab->CombatModifiers[0].RequiredSourceTags.Contains(TEXT("Weapon.Light")) &&
		Backstab->CombatModifiers[0].RequiredTargetConditions.Contains(EGridCombatTargetCondition::RearArc) &&
		Backstab->CombatModifiers[0].bExcludeAreaActions &&
		Backstab->CombatModifiers[0].OutgoingDamagePercentModifier == 20 &&
		Backstab->CombatModifiers[0].CriticalChancePercentModifier == 20);

	const FGridCombatActionDefinition* Hemorrhage = RPG0392Authoring::FindAction(Rogue, TEXT("Action_Rogue_Hemorrhage"));
	TestTrue(TEXT("Hemorrhage accepts only Slashing/Piercing weapon descriptors"), Hemorrhage &&
		Hemorrhage->WeaponAttackProfile.AllowedPhysicalSubtypes.Num() == 2 &&
		Hemorrhage->WeaponAttackProfile.AllowedPhysicalSubtypes.Contains(EGridPhysicalDamageSubtype::Slashing) &&
		Hemorrhage->WeaponAttackProfile.AllowedPhysicalSubtypes.Contains(EGridPhysicalDamageSubtype::Piercing));
	TestTrue(TEXT("Hemorrhage applies Bleeding through post-hit PhysicalArmor gate"), Hemorrhage &&
		Hemorrhage->StatusApplications.Num() == 1 &&
		Hemorrhage->StatusApplications[0].StatusEffectId == TEXT("Status_Bleeding") &&
		Hemorrhage->StatusApplications[0].ArmorGate == EGridCombatStatusArmorGate::PhysicalArmorDepleted &&
		Hemorrhage->StatusApplications[0].DurationOverride == 3);

	const FGridCombatActionDefinition* Finisher = RPG0392Authoring::FindAction(Rogue, TEXT("Action_Rogue_Finisher"));
	TestTrue(TEXT("Finisher is 220 percent WD gated by depleted physical armor and <=30 percent HP"), Finisher &&
		Finisher->WeaponAttackProfile.WeaponDamagePercent == 220 &&
		Finisher->TargetFilter.bRequirePhysicalArmorDepleted &&
		Finisher->TargetFilter.MaximumHealthPercent == 30);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0392ShadowAuthoringTest, "Grimrock.RPG.RPG03.9.2.Authoring.Shadow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0392ShadowAuthoringTest::RunTest(const FString&)
{
	URPGClassAsset* Rogue = RPG0392Authoring::BuildRogue();

	const FGridCombatActionDefinition* Vanish = RPG0392Authoring::FindAction(Rogue, TEXT("Action_Rogue_ShortVanish"));
	TestTrue(TEXT("Short Vanish applies Hidden"), Vanish && Vanish->StatusApplications.Num() == 1 &&
		Vanish->StatusApplications[0].StatusEffectId == TEXT("Status_Hidden"));

	const FGridCombatActionDefinition* ShadowStep = RPG0392Authoring::FindAction(Rogue, TEXT("Action_Rogue_ShadowStep"));
	TestTrue(TEXT("Shadow Step applies ShadowReach"), ShadowStep && ShadowStep->StatusApplications.Num() == 1 &&
		ShadowStep->StatusApplications[0].StatusEffectId == TEXT("Status_ShadowReach"));

	const FRPGClassProgressionChoiceDefinition* Elusive =
		Rogue->FindProgressionChoice(TEXT("Talent_Rogue_Shadow_Elusive"));
	TestTrue(TEXT("Elusive reacts to the three authored evasive actions and applies Status_Elusive"), Elusive &&
		Elusive->CombatReactions.Num() == 1 &&
		Elusive->CombatReactions[0].Trigger == EGridCombatReactionTrigger::ActionResolved &&
		Elusive->CombatReactions[0].ActionIds.Num() == 3 &&
		Elusive->CombatReactions[0].ApplyOwnerStatusEffectId == TEXT("Status_Elusive") &&
		Elusive->CombatReactions[0].ApplyOwnerStatusDurationOverride == 1);

	const FGridCombatActionDefinition* PerfectShadow = RPG0392Authoring::FindAction(Rogue, TEXT("Action_Rogue_PerfectShadow"));
	TestTrue(TEXT("Perfect Shadow is self-targeted CD5 and applies HiddenPerfect for two rounds"), PerfectShadow &&
		PerfectShadow->TargetingPolicy == EGridCombatTargetingPolicy::Self &&
		PerfectShadow->CooldownRounds == 5 &&
		PerfectShadow->StatusApplications.Num() == 1 &&
		PerfectShadow->StatusApplications[0].DurationOverride == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0392SaboteurAuthoringTest, "Grimrock.RPG.RPG03.9.2.Authoring.Saboteur",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0392SaboteurAuthoringTest::RunTest(const FString&)
{
	URPGClassAsset* Rogue = RPG0392Authoring::BuildRogue();

	const FRPGClassProgressionChoiceDefinition* Disarm =
		Rogue->FindProgressionChoice(TEXT("Talent_Rogue_Saboteur_ExpertDisarm"));
	TestTrue(TEXT("Expert Disarm gives Traps +2 and safe-failure margin two"), Disarm &&
		Disarm->SkillModifiers.Num() == 1 &&
		Disarm->SkillModifiers[0].SkillId == TEXT("Skill_Traps") &&
		Disarm->SkillModifiers[0].CheckModifier == 2 &&
		Disarm->SkillModifiers[0].SafeFailureMargin == 2);

	const FGridCombatActionDefinition* QuickTrap = RPG0392Authoring::FindAction(Rogue, TEXT("Action_Rogue_QuickTrap"));
	TestTrue(TEXT("Quick Trap is a three-round consumed Piercing trap with DEX scaling"), QuickTrap &&
		QuickTrap->TargetingPolicy == EGridCombatTargetingPolicy::Cell &&
		QuickTrap->TrapEffect.bPlaceTrap && QuickTrap->TrapEffect.TrapId == TEXT("Trap_Quick") &&
		QuickTrap->TrapEffect.DurationRounds == 3 && QuickTrap->TrapEffect.BaseDamage == 6 &&
		QuickTrap->TrapEffect.PhysicalSubtype == EGridPhysicalDamageSubtype::Piercing &&
		QuickTrap->TrapEffect.DamageScalingAttribute == EGridAttackScalingAttribute::Dexterity &&
		QuickTrap->TrapEffect.bConsumeOnTrigger);
	TestTrue(TEXT("Quick Trap immobilization uses a post-hit physical ArmorGate"), QuickTrap &&
		QuickTrap->TrapEffect.StatusApplications.Num() == 1 &&
		QuickTrap->TrapEffect.StatusApplications[0].StatusEffectId == TEXT("Status_Immobilized") &&
		QuickTrap->TrapEffect.StatusApplications[0].ArmorGate == EGridCombatStatusArmorGate::PhysicalArmorDepleted);

	const FGridCombatActionDefinition* Smoke = RPG0392Authoring::FindAction(Rogue, TEXT("Action_Rogue_SmokeBomb"));
	TestTrue(TEXT("Smoke Bomb is R3 Area1 and creates Smoke for two rounds"), Smoke &&
		Smoke->RangeCells == 3 && Smoke->AreaRadiusCells == 1 &&
		Smoke->SurfaceEffects.Num() == 1 &&
		Smoke->SurfaceEffects[0].SurfaceType == EGridCombatSurfaceType::Smoke &&
		Smoke->SurfaceEffects[0].DurationRounds == 2);

	const FRPGClassProgressionChoiceDefinition* Locksmith =
		Rogue->FindProgressionChoice(TEXT("Talent_Rogue_Saboteur_MasterLocksmith"));
	TestTrue(TEXT("Master Locksmith gives +2 checks, +1 requirement rank and safe-failure margin two"), Locksmith &&
		Locksmith->SkillModifiers.Num() == 1 &&
		Locksmith->SkillModifiers[0].SkillId == TEXT("Skill_Lockpicking") &&
		Locksmith->SkillModifiers[0].CheckModifier == 2 &&
		Locksmith->SkillModifiers[0].RequirementGrantRankModifier == 1 &&
		Locksmith->SkillModifiers[0].SafeFailureMargin == 2);

	const FGridCombatActionDefinition* Sabotage = RPG0392Authoring::FindAction(Rogue, TEXT("Action_Rogue_Sabotage"));
	TestTrue(TEXT("Sabotage targets Mechanical/Construct at range one"), Sabotage &&
		Sabotage->TargetingPolicy == EGridCombatTargetingPolicy::Hostile &&
		Sabotage->RangeCells == 1 &&
		Sabotage->TargetFilter.AllowedMonsterCategoryIds.Contains(TEXT("Mechanical")) &&
		Sabotage->TargetFilter.AllowedMonsterCategoryIds.Contains(TEXT("Construct")));
	TestTrue(TEXT("Sabotage is gated by target-difficulty Mechanics and applies Status_Sabotaged"), Sabotage &&
		Sabotage->SkillCheck.SkillId == TEXT("Skill_Mechanics") &&
		Sabotage->SkillCheck.bUseTargetDifficulty &&
		Sabotage->StatusApplications.Num() == 1 &&
		Sabotage->StatusApplications[0].StatusEffectId == TEXT("Status_Sabotaged"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0392StatusAuthoringTest, "Grimrock.RPG.RPG03.9.2.Authoring.Statuses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0392StatusAuthoringTest::RunTest(const FString&)
{
	TArray<FName> StatusIds;
	FRPGRogueAuthoring::GetRequiredStatusIds(StatusIds);
	TestEqual(TEXT("Rogue authoring defines nine production statuses"), StatusIds.Num(), 9);

	for (const FName StatusId : StatusIds)
	{
		UGridStatusEffectDefinitionAsset* Status = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
		TestTrue(*FString::Printf(TEXT("%s has an authoring definition"), *StatusId.ToString()),
			FRPGRogueAuthoring::ConfigureStatus(*Status, StatusId));
		TestTrue(*FString::Printf(TEXT("%s is structurally valid"), *StatusId.ToString()), Status->IsValidDefinition());
	}

	UGridStatusEffectDefinitionAsset* Hidden = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	FRPGRogueAuthoring::ConfigureStatus(*Hidden, TEXT("Status_Hidden"));
	TestTrue(TEXT("Hidden blocks direct hostile targeting and expires at next activation"), Hidden->Control.bBlockDirectHostileTargeting &&
		Hidden->bExpireAtOwnerNextActivation);
	TestTrue(TEXT("Hidden has an offensive-action consumption reaction"), Hidden->CombatReactions.Num() == 1 &&
		Hidden->CombatReactions[0].bRequireOffensiveAction && Hidden->CombatReactions[0].bConsumeOwningStatus);

	UGridStatusEffectDefinitionAsset* Reach = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	FRPGRogueAuthoring::ConfigureStatus(*Reach, TEXT("Status_ShadowReach"));
	TestTrue(TEXT("ShadowReach adds one cell only to light melee source tags"), Reach->CombatModifiers.Num() == 1 &&
		Reach->CombatModifiers[0].RangeCellsModifier == 1 &&
		Reach->CombatModifiers[0].RequiredSourceTags.Contains(TEXT("Weapon.Light")) &&
		Reach->CombatModifiers[0].ActionTypes.Contains(EGridCombatActionType::MeleeAttack));

	UGridStatusEffectDefinitionAsset* Perfect = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	FRPGRogueAuthoring::ConfigureStatus(*Perfect, TEXT("Status_HiddenPerfect"));
	TestTrue(TEXT("HiddenPerfect carries +50 damage and +2 accuracy"), Perfect->CombatModifiers.Num() == 1 &&
		Perfect->CombatModifiers[0].OutgoingDamagePercentModifier == 50 &&
		Perfect->CombatModifiers[0].AccuracyModifier == 2);
	TestEqual(TEXT("HiddenPerfect has offensive and direct-damage break reactions"), Perfect->CombatReactions.Num(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0392ProductionAssetsTest, "Grimrock.RPG.RPG03.9.2.Authoring.ProductionAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0392ProductionAssetsTest::RunTest(const FString&)
{
	URPGClassAsset* Rogue = LoadObject<URPGClassAsset>(nullptr, FRPGRogueAuthoring::RogueAssetPath());
	if (!TestNotNull(TEXT("Production DA_Class_Rogue loads"), Rogue))
	{
		return false;
	}

	TestEqual(TEXT("Production Rogue keeps the canonical ClassId"), Rogue->ClassId, FName(TEXT("Rogue")));
	TestEqual(TEXT("Production Rogue contains eleven RPG03.9.2 actions"), Rogue->CombatActions.Num(), 11);
	TestEqual(TEXT("Production Rogue contains fifteen authored choice records"), Rogue->ProgressionChoices.Num(), 15);
	TestTrue(TEXT("Production Rogue is structurally valid"), Rogue->IsValidDefinition());

	TArray<FName> StatusIds;
	FRPGRogueAuthoring::GetRequiredStatusIds(StatusIds);
	for (const FName StatusId : StatusIds)
	{
		UGridStatusEffectDefinitionAsset* Status =
			LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, *FRPGRogueAuthoring::GetStatusObjectPath(StatusId));
		TestNotNull(*FString::Printf(TEXT("Production %s loads"), *StatusId.ToString()), Status);
		if (Status)
		{
			TestEqual(*FString::Printf(TEXT("Production %s preserves EffectId"), *StatusId.ToString()), Status->EffectId, StatusId);
			TestTrue(*FString::Printf(TEXT("Production %s is structurally valid"), *StatusId.ToString()), Status->IsValidDefinition());
		}
	}
	return true;
}

#endif
