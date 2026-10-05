#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGWarriorAuthoring.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"

namespace RPG0391Authoring
{
	URPGClassAsset* BuildWarrior()
	{
		URPGClassAsset* Warrior = NewObject<URPGClassAsset>(GetTransientPackage());
		Warrior->ClassId = TEXT("Warrior");
		Warrior->DisplayName = FText::FromString(TEXT("Guerrier"));
		Warrior->HealthAtLevelOne = 20;
		FRPGWarriorAuthoring::ConfigureClass(*Warrior);
		return Warrior;
	}

	const FGridCombatActionDefinition* FindAction(const URPGClassAsset* Warrior, FName ActionId)
	{
		return Warrior ? Warrior->CombatActions.FindByPredicate(
			[ActionId](const FGridCombatActionDefinition& Action)
			{
				return Action.ActionId == ActionId;
			}) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0391WarriorClassAuthoringTest, "Grimrock.RPG.RPG03.9.1.Authoring.WarriorClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0391WarriorClassAuthoringTest::RunTest(const FString&)
{
	URPGClassAsset* Warrior = RPG0391Authoring::BuildWarrior();
	TestEqual(TEXT("Warrior authoring exposes ten active actions"), Warrior->CombatActions.Num(), 10);
	TestEqual(TEXT("Fifteen logical talents with three specialization variants produce seventeen choice records"),
		Warrior->ProgressionChoices.Num(), 17);
	TestTrue(TEXT("Transient authored Warrior is structurally valid"), Warrior->IsValidDefinition());

	const FName SpecializationGroup(TEXT("TalentGroup_Warrior_WeaponMaster_MartialSpecialization"));
	const FName SpecializationAlias(TEXT("Talent_Warrior_WeaponMaster_MartialSpecialization"));
	int32 VariantCount = 0;
	for (const FRPGClassProgressionChoiceDefinition& Choice : Warrior->ProgressionChoices)
	{
		if (Choice.ExclusiveChoiceGroupId == SpecializationGroup)
		{
			++VariantCount;
			TestTrue(TEXT("Every specialization variant grants the logical specialization requirement"),
				Choice.GrantedRequirementIds.Contains(SpecializationAlias));
		}
	}
	TestEqual(TEXT("Exactly three martial specialization variants are authored"), VariantCount, 3);

	const FRPGClassProgressionChoiceDefinition* Critical =
		Warrior->FindProgressionChoice(TEXT("Talent_Warrior_WeaponMaster_CriticalMastery"));
	TestTrue(TEXT("Critical Mastery remains one canonical talent"), Critical && Critical->CombatModifiers.Num() == 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0391GuardianAuthoringTest, "Grimrock.RPG.RPG03.9.1.Authoring.Guardian",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0391GuardianAuthoringTest::RunTest(const FString&)
{
	URPGClassAsset* Warrior = RPG0391Authoring::BuildWarrior();

	const FGridCombatActionDefinition* Bash = RPG0391Authoring::FindAction(Warrior, TEXT("Action_Warrior_ShieldBash"));
	TestTrue(TEXT("Shield Bash is 80 percent WD and requires a shield tag"), Bash &&
		Bash->WeaponAttackProfile.WeaponDamagePercent == 80 &&
		Bash->Requirements.Contains(TEXT("Equipment.Shield")));
	TestTrue(TEXT("Shield Bash is forced to physical Bludgeoning"), Bash &&
		Bash->WeaponAttackProfile.bOverrideDamageDescriptor &&
		Bash->WeaponAttackProfile.OverridePhysicalSubtype == EGridPhysicalDamageSubtype::Bludgeoning);
	TestTrue(TEXT("Shield Bash stun uses the post-hit physical ArmorGate"), Bash &&
		Bash->StatusApplications.Num() == 1 &&
		Bash->StatusApplications[0].ArmorGate == EGridCombatStatusArmorGate::PhysicalArmorDepleted);

	const FRPGClassProgressionChoiceDefinition* Interception =
		Warrior->FindProgressionChoice(TEXT("Talent_Warrior_Guardian_Interception"));
	TestTrue(TEXT("Interception is a once-per-round 50 percent front-row physical redirect"), Interception &&
		Interception->CombatReactions.Num() == 1 &&
		Interception->CombatReactions[0].Trigger == EGridCombatReactionTrigger::IncomingAttackHit &&
		Interception->CombatReactions[0].Limit == EGridCombatReactionLimit::OncePerRound &&
		Interception->CombatReactions[0].InterceptFinalDamagePercent == 50 &&
		Interception->CombatReactions[0].bRequireOwnerFrontRow &&
		Interception->CombatReactions[0].bRequireEventTargetFrontRow);

	const FRPGClassProgressionChoiceDefinition* Bulwark =
		Warrior->FindProgressionChoice(TEXT("Talent_Warrior_Guardian_Bulwark"));
	TestTrue(TEXT("Bulwark adds 25 percent physical reference armor"), Bulwark &&
		Bulwark->CombatModifiers.Num() == 1 &&
		Bulwark->CombatModifiers[0].PhysicalArmorReferencePercentModifier == 25);

	const FGridCombatActionDefinition* Fortress = RPG0391Authoring::FindAction(Warrior, TEXT("Action_Warrior_Fortress"));
	TestTrue(TEXT("Fortress targets the front row"), Fortress &&
		Fortress->TargetingPolicy == EGridCombatTargetingPolicy::FrontRowParty);
	TestTrue(TEXT("Fortress restores 40 percent reference physical armor"), Fortress &&
		Fortress->ArmorEffects.Num() == 1 &&
		Fortress->ArmorEffects[0].Magnitude == EGridCombatArmorEffectMagnitude::ReferencePercent &&
		Fortress->ArmorEffects[0].Amount == 40);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0391BreakerAuthoringTest, "Grimrock.RPG.RPG03.9.1.Authoring.Breaker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0391BreakerAuthoringTest::RunTest(const FString&)
{
	URPGClassAsset* Warrior = RPG0391Authoring::BuildWarrior();

	const FGridCombatActionDefinition* Power = RPG0391Authoring::FindAction(Warrior, TEXT("Action_Warrior_PowerStrike"));
	TestTrue(TEXT("Power Strike is heavy-weapon 150 percent WD"), Power &&
		Power->WeaponAttackProfile.RequiredItemTags.Contains(TEXT("Weapon.Heavy")) &&
		Power->WeaponAttackProfile.WeaponDamagePercent == 150);

	const FGridCombatActionDefinition* ArmorBreak = RPG0391Authoring::FindAction(Warrior, TEXT("Action_Warrior_ArmorBreak"));
	TestTrue(TEXT("Armor Break adds 50 percent RawDamage against physical armor"), ArmorBreak &&
		ArmorBreak->ArmorEffects.Num() == 1 &&
		ArmorBreak->ArmorEffects[0].Pool == EGridCombatArmorPool::Physical &&
		ArmorBreak->ArmorEffects[0].Magnitude == EGridCombatArmorEffectMagnitude::RawDamagePercent &&
		ArmorBreak->ArmorEffects[0].Amount == 50);

	const FGridCombatActionDefinition* Sweep = RPG0391Authoring::FindAction(Warrior, TEXT("Action_Warrior_Sweep"));
	TestTrue(TEXT("Sweep is Area1, range one, 85 percent WD and no friendly fire"), Sweep &&
		Sweep->TargetingPolicy == EGridCombatTargetingPolicy::Area &&
		Sweep->RangeCells == 1 && Sweep->AreaRadiusCells == 1 &&
		Sweep->WeaponAttackProfile.WeaponDamagePercent == 85 && !Sweep->bAffectsAlliesInArea);

	const FGridCombatActionDefinition* Execution = RPG0391Authoring::FindAction(Warrior, TEXT("Action_Warrior_Execution"));
	TestTrue(TEXT("Execution is 200 percent WD gated by depleted physical armor and <=35 percent HP"), Execution &&
		Execution->WeaponAttackProfile.WeaponDamagePercent == 200 &&
		Execution->TargetFilter.bRequirePhysicalArmorDepleted &&
		Execution->TargetFilter.MaximumHealthPercent == 35);

	const FGridCombatActionDefinition* Devastation = RPG0391Authoring::FindAction(Warrior, TEXT("Action_Warrior_Devastation"));
	TestTrue(TEXT("Devastation is 140 percent WD Area1"), Devastation &&
		Devastation->AreaRadiusCells == 1 && Devastation->WeaponAttackProfile.WeaponDamagePercent == 140);
	TestTrue(TEXT("Devastation knockdown is primary-only after physical ArmorGate"), Devastation &&
		Devastation->StatusApplications.Num() == 1 &&
		Devastation->StatusApplications[0].TargetScope == EGridCombatResolvedTargetScope::PrimaryTargetOnly &&
		Devastation->StatusApplications[0].ArmorGate == EGridCombatStatusArmorGate::PhysicalArmorDepleted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0391WeaponMasterAuthoringTest, "Grimrock.RPG.RPG03.9.1.Authoring.WeaponMaster",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0391WeaponMasterAuthoringTest::RunTest(const FString&)
{
	URPGClassAsset* Warrior = RPG0391Authoring::BuildWarrior();

	const FRPGClassProgressionChoiceDefinition* Riposte =
		Warrior->FindProgressionChoice(TEXT("Talent_Warrior_WeaponMaster_Riposte"));
	TestTrue(TEXT("Riposte depends on the logical specialization requirement"), Riposte &&
		Riposte->PrerequisiteRequirementIds.Contains(TEXT("Talent_Warrior_WeaponMaster_MartialSpecialization")));
	TestTrue(TEXT("Riposte is a non-recursive 75 percent WD once-per-round counterattack"), Riposte &&
		Riposte->CombatReactions.Num() == 1 &&
		Riposte->CombatReactions[0].Trigger == EGridCombatReactionTrigger::AttackMiss &&
		Riposte->CombatReactions[0].Limit == EGridCombatReactionLimit::OncePerRound &&
		Riposte->CombatReactions[0].CounterAttackWeaponProfile.WeaponDamagePercent == 75 &&
		!Riposte->CombatReactions[0].bAllowReactionGeneratedEvents);

	const FGridCombatActionDefinition* SecondWind = RPG0391Authoring::FindAction(Warrior, TEXT("Action_Warrior_SecondWind"));
	TestTrue(TEXT("Second Wind restores 20 percent MaxHP and has CD4"), SecondWind &&
		SecondWind->EffectProfile.RestoreHealthMaximumPercent == 20 &&
		SecondWind->CooldownRounds == 4);

	const FRPGClassProgressionChoiceDefinition* Critical =
		Warrior->FindProgressionChoice(TEXT("Talent_Warrior_WeaponMaster_CriticalMastery"));
	TestTrue(TEXT("Critical Mastery has one conditional profile per martial specialization"), Critical &&
		Critical->CombatModifiers.Num() == 3);
	if (Critical)
	{
		for (const FGridCombatModifierProfile& Modifier : Critical->CombatModifiers)
		{
			TestEqual(TEXT("Critical Mastery adds ten critical chance points"), Modifier.CriticalChancePercentModifier, 10);
			TestEqual(TEXT("Critical Mastery adds twenty-five critical multiplier points"), Modifier.CriticalDamagePercentModifier, 25);
			TestEqual(TEXT("Critical Mastery profile is conditioned on one specialization ChoiceId"),
				Modifier.RequiredOwnerRequirementIds.Num(), 1);
		}
	}

	const FGridCombatActionDefinition* Warlord = RPG0391Authoring::FindAction(Warrior, TEXT("Action_Warrior_Warlord"));
	TestTrue(TEXT("Warlord targets all active living party members and applies Status_Warlord"), Warlord &&
		Warlord->TargetingPolicy == EGridCombatTargetingPolicy::Party &&
		Warlord->StatusApplications.Num() == 1 &&
		Warlord->StatusApplications[0].StatusEffectId == TEXT("Status_Warlord"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0391ProductionAssetsTest, "Grimrock.RPG.RPG03.9.1.Authoring.ProductionAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0391ProductionAssetsTest::RunTest(const FString&)
{
	URPGClassAsset* Warrior = LoadObject<URPGClassAsset>(nullptr, FRPGWarriorAuthoring::WarriorAssetPath());
	if (!TestNotNull(TEXT("Production DA_Class_Warrior loads"), Warrior))
	{
		return false;
	}

	TestEqual(TEXT("Production Warrior keeps the canonical ClassId"), Warrior->ClassId, FName(TEXT("Warrior")));
	TestEqual(TEXT("Production Warrior contains ten RPG03.9.1 actions"), Warrior->CombatActions.Num(), 10);
	TestEqual(TEXT("Production Warrior contains seventeen authored choice records"), Warrior->ProgressionChoices.Num(), 17);
	TestTrue(TEXT("Production Warrior is structurally valid"), Warrior->IsValidDefinition());

	TArray<FName> StatusIds;
	FRPGWarriorAuthoring::GetRequiredStatusIds(StatusIds);
	for (const FName StatusId : StatusIds)
	{
		UGridStatusEffectDefinitionAsset* Status =
			LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, *FRPGWarriorAuthoring::GetStatusObjectPath(StatusId));
		TestNotNull(*FString::Printf(TEXT("Production %s loads"), *StatusId.ToString()), Status);
		if (Status)
		{
			TestEqual(*FString::Printf(TEXT("Production %s preserves EffectId"), *StatusId.ToString()), Status->EffectId, StatusId);
			TestTrue(*FString::Printf(TEXT("Production %s is structurally valid"), *StatusId.ToString()), Status->IsValidDefinition());
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0391StatusAuthoringTest, "Grimrock.RPG.RPG03.9.1.Authoring.Statuses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0391StatusAuthoringTest::RunTest(const FString&)
{
	TArray<FName> StatusIds;
	FRPGWarriorAuthoring::GetRequiredStatusIds(StatusIds);
	TestEqual(TEXT("Warrior authoring defines five production statuses"), StatusIds.Num(), 5);

	for (const FName StatusId : StatusIds)
	{
		UGridStatusEffectDefinitionAsset* Status = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
		TestTrue(*FString::Printf(TEXT("%s has an authoring definition"), *StatusId.ToString()),
			FRPGWarriorAuthoring::ConfigureStatus(*Status, StatusId));
		TestTrue(*FString::Printf(TEXT("%s is structurally valid"), *StatusId.ToString()), Status->IsValidDefinition());
	}

	UGridStatusEffectDefinitionAsset* Guarded = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	FRPGWarriorAuthoring::ConfigureStatus(*Guarded, TEXT("Status_Guarded"));
	TestEqual(TEXT("Guarded has incoming, evasion and outgoing weapon modifiers"), Guarded->CombatModifiers.Num(), 3);

	UGridStatusEffectDefinitionAsset* Warlord = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	FRPGWarriorAuthoring::ConfigureStatus(*Warlord, TEXT("Status_Warlord"));
	TestEqual(TEXT("Warlord initiative bonus is +4"), Warlord->InitiativeModifier, 4);
	TestTrue(TEXT("Warlord accuracy bonus is +2"), Warlord->CombatModifiers.Num() == 1 &&
		Warlord->CombatModifiers[0].AccuracyModifier == 2);
	return true;
}

#endif
