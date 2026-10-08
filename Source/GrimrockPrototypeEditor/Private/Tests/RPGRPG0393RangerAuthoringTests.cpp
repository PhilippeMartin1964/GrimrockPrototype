#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRangerAuthoring.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"

namespace RPG0393Authoring
{
	const TArray<FRPGRangerFavoredEnemyCategoryDefinition> TestCategories = {
		{ TEXT("Goblin"), FText::FromString(TEXT("Gobelins")) },
		{ TEXT("Vermin"), FText::FromString(TEXT("Vermine")) }
	};

	URPGClassAsset* BuildRanger()
	{
		URPGClassAsset* Ranger = NewObject<URPGClassAsset>(GetTransientPackage());
		Ranger->ClassId = TEXT("Ranger");
		Ranger->DisplayName = FText::FromString(TEXT("Rôdeur"));
		Ranger->HealthAtLevelOne = 14;
		FRPGRangerAuthoring::ConfigureClass(*Ranger, TestCategories);
		return Ranger;
	}

	const FGridCombatActionDefinition* FindAction(const URPGClassAsset* Ranger, FName ActionId)
	{
		return Ranger ? Ranger->CombatActions.FindByPredicate(
			[ActionId](const FGridCombatActionDefinition& Action)
			{
				return Action.ActionId == ActionId;
			}) : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0393RangerClassAuthoringTest, "Grimrock.RPG.RPG03.9.3.Authoring.RangerClass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0393RangerClassAuthoringTest::RunTest(const FString&)
{
	URPGClassAsset* Ranger = RPG0393Authoring::BuildRanger();
	TestEqual(TEXT("Ranger authoring exposes nine active actions"), Ranger->CombatActions.Num(), 9);
	TestEqual(TEXT("Two favored categories produce sixteen choice records"), Ranger->ProgressionChoices.Num(), 16);
	TestTrue(TEXT("Transient authored Ranger is structurally valid"), Ranger->IsValidDefinition());

	const FRPGClassProgressionChoiceDefinition* Pinning =
		Ranger->FindProgressionChoice(TEXT("Talent_Ranger_Hunter_PinningShot"));
	TestTrue(TEXT("Pinning Shot depends on the logical Favored Enemy alias"), Pinning &&
		Pinning->PrerequisiteRequirementIds.Contains(TEXT("Talent_Ranger_Hunter_FavoredEnemy")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0393MarksmanAuthoringTest, "Grimrock.RPG.RPG03.9.3.Authoring.Marksman",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0393MarksmanAuthoringTest::RunTest(const FString&)
{
	URPGClassAsset* Ranger = RPG0393Authoring::BuildRanger();

	const FGridCombatActionDefinition* Precise = RPG0393Authoring::FindAction(Ranger, TEXT("Action_Ranger_PreciseShot"));
	TestTrue(TEXT("Precise Shot is 150 percent ranged WD using weapon range plus two"), Precise &&
		Precise->ActionType == EGridCombatActionType::RangedAttack &&
		Precise->WeaponAttackProfile.bRequireRangedWeapon &&
		Precise->WeaponAttackProfile.bUseWeaponRange &&
		Precise->WeaponAttackProfile.WeaponRangeModifier == 2 &&
		Precise->WeaponAttackProfile.WeaponDamagePercent == 150);
	const FRPGClassProgressionChoiceDefinition* PreciseChoice =
		Ranger->FindProgressionChoice(TEXT("Talent_Ranger_Marksman_PreciseShot"));
	TestTrue(TEXT("Precise Shot carries Accuracy +2 through C2"), PreciseChoice &&
		PreciseChoice->CombatModifiers.Num() == 1 && PreciseChoice->CombatModifiers[0].AccuracyModifier == 2);

	const FGridCombatActionDefinition* Piercing = RPG0393Authoring::FindAction(Ranger, TEXT("Action_Ranger_PiercingShot"));
	TestTrue(TEXT("Piercing Shot is 110 percent WD with Piercing descriptor and armor-only +50 percent RawDamage"), Piercing &&
		Piercing->WeaponAttackProfile.WeaponDamagePercent == 110 &&
		Piercing->WeaponAttackProfile.bOverrideDamageDescriptor &&
		Piercing->WeaponAttackProfile.OverridePhysicalSubtype == EGridPhysicalDamageSubtype::Piercing &&
		Piercing->ArmorEffects.Num() == 1 &&
		Piercing->ArmorEffects[0].Magnitude == EGridCombatArmorEffectMagnitude::RawDamagePercent &&
		Piercing->ArmorEffects[0].Amount == 50);

	const FGridCombatActionDefinition* Rapid = RPG0393Authoring::FindAction(Ranger, TEXT("Action_Ranger_RapidShot"));
	TestTrue(TEXT("Rapid Shot is two 65 percent independent resolutions with second Accuracy -1"), Rapid &&
		Rapid->WeaponAttackProfile.WeaponDamagePercent == 65 &&
		Rapid->ResolutionCount == 2 && Rapid->SubsequentResolutionAccuracyModifier == -1);

	const FGridCombatActionDefinition* Volley = RPG0393Authoring::FindAction(Ranger, TEXT("Action_Ranger_Volley"));
	TestTrue(TEXT("Volley is R5 Area1, 80 percent WD, hostile-only and requires grid LOS"), Volley &&
		Volley->TargetingPolicy == EGridCombatTargetingPolicy::Area && Volley->RangeCells == 5 &&
		Volley->AreaRadiusCells == 1 && Volley->WeaponAttackProfile.WeaponDamagePercent == 80 &&
		!Volley->bAffectsAlliesInArea && Volley->bRequiresLineOfSight);

	const FRPGClassProgressionChoiceDefinition* Eagle =
		Ranger->FindProgressionChoice(TEXT("Talent_Ranger_Marksman_EagleEye"));
	TestTrue(TEXT("Eagle Eye gives Ranged Accuracy +1 and Range +1"), Eagle &&
		Eagle->CombatModifiers.Num() == 1 &&
		Eagle->CombatModifiers[0].ActionTypes.Contains(EGridCombatActionType::RangedAttack) &&
		Eagle->CombatModifiers[0].AccuracyModifier == 1 &&
		Eagle->CombatModifiers[0].RangeCellsModifier == 1);
	TestTrue(TEXT("Eagle Eye gives contextual ranged Perception +2"), Eagle &&
		Eagle->SkillModifiers.Num() == 1 &&
		Eagle->SkillModifiers[0].SkillId == TEXT("Skill_Perception") &&
		Eagle->SkillModifiers[0].CheckModifier == 2 &&
		Eagle->SkillModifiers[0].bRequireRangedContext);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0393HunterAuthoringTest, "Grimrock.RPG.RPG03.9.3.Authoring.Hunter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0393HunterAuthoringTest::RunTest(const FString&)
{
	URPGClassAsset* Ranger = RPG0393Authoring::BuildRanger();

	const FGridCombatActionDefinition* Mark = RPG0393Authoring::FindAction(Ranger, TEXT("Action_Ranger_MarkPrey"));
	TestTrue(TEXT("Mark Prey applies Status_MarkedByRanger for three rounds at R5"), Mark &&
		Mark->RangeCells == 5 && Mark->StatusApplications.Num() == 1 &&
		Mark->StatusApplications[0].StatusEffectId == TEXT("Status_MarkedByRanger") &&
		Mark->StatusApplications[0].DurationOverride == 3);

	const FRPGClassProgressionChoiceDefinition* MarkChoice =
		Ranger->FindProgressionChoice(TEXT("Talent_Ranger_Hunter_MarkPrey"));
	TestTrue(TEXT("Own mark grants Accuracy +2 and damage +15 percent"), MarkChoice &&
		MarkChoice->CombatModifiers.Num() == 1 &&
		MarkChoice->CombatModifiers[0].bRequiredTargetStatusesFromOwner &&
		MarkChoice->CombatModifiers[0].RequiredTargetStatusEffectIds.Contains(TEXT("Status_MarkedByRanger")) &&
		MarkChoice->CombatModifiers[0].AccuracyModifier == 2 &&
		MarkChoice->CombatModifiers[0].OutgoingDamagePercentModifier == 15);

	for (const FRPGRangerFavoredEnemyCategoryDefinition& Category : RPG0393Authoring::TestCategories)
	{
		const FName ChoiceId(*FString::Printf(TEXT("Talent_Ranger_Hunter_FavoredEnemy_%s"), *Category.CategoryId.ToString()));
		const FRPGClassProgressionChoiceDefinition* Choice = Ranger->FindProgressionChoice(ChoiceId);
		TestTrue(*FString::Printf(TEXT("Favored Enemy %s exists"), *Category.CategoryId.ToString()), Choice != nullptr);
		if (Choice)
		{
			TestEqual(TEXT("Favored Enemy uses common exclusive group"), Choice->ExclusiveChoiceGroupId,
				FName(TEXT("TalentGroup_Ranger_Hunter_FavoredEnemy")));
			TestTrue(TEXT("Favored Enemy grants the logical alias"),
				Choice->GrantedRequirementIds.Contains(TEXT("Talent_Ranger_Hunter_FavoredEnemy")));
			TestTrue(TEXT("Favored Enemy damage is category-scoped +15 percent"),
				Choice->CombatModifiers.Num() == 1 &&
				Choice->CombatModifiers[0].AllowedTargetMonsterCategoryIds.Contains(Category.CategoryId) &&
				Choice->CombatModifiers[0].OutgoingDamagePercentModifier == 15);
			TestEqual(TEXT("Favored Enemy projects four related contextual skills"), Choice->SkillModifiers.Num(), 4);
			TestEqual(TEXT("Favored Enemy uses bestiary player-facing category DisplayName"),
				Choice->DisplayName.ToString(),
				FString::Printf(TEXT("Ennemi juré — %s"), *Category.DisplayName.ToString()));
		}
	}

	const FGridCombatActionDefinition* Pinning = RPG0393Authoring::FindAction(Ranger, TEXT("Action_Ranger_PinningShot"));
	TestTrue(TEXT("Pinning Shot applies existing Immobilized after depleted physical armor"), Pinning &&
		Pinning->WeaponAttackProfile.WeaponDamagePercent == 100 &&
		Pinning->StatusApplications.Num() == 1 &&
		Pinning->StatusApplications[0].StatusEffectId == TEXT("Status_Immobilized") &&
		Pinning->StatusApplications[0].ArmorGate == EGridCombatStatusArmorGate::PhysicalArmorDepleted);

	const FGridCombatActionDefinition* Predator = RPG0393Authoring::FindAction(Ranger, TEXT("Action_Ranger_PredatorStrike"));
	TestTrue(TEXT("Predator Strike is 170 percent WD and requires own mark"), Predator &&
		Predator->WeaponAttackProfile.WeaponDamagePercent == 170 &&
		Predator->TargetFilter.RequiredStatusEffectIds.Contains(TEXT("Status_MarkedByRanger")) &&
		Predator->TargetFilter.bRequiredStatusesFromSource);
	const FRPGClassProgressionChoiceDefinition* PredatorChoice =
		Ranger->FindProgressionChoice(TEXT("Talent_Ranger_Hunter_PredatorStrike"));
	TestTrue(TEXT("Predator Strike adds Accuracy +1"), PredatorChoice &&
		PredatorChoice->CombatModifiers.Num() == 1 && PredatorChoice->CombatModifiers[0].AccuracyModifier == 1);

	const FRPGClassProgressionChoiceDefinition* Alpha =
		Ranger->FindProgressionChoice(TEXT("Talent_Ranger_Hunter_AlphaHunter"));
	TestTrue(TEXT("Alpha Hunter is once-per-round own-mark death transfer R3 for two rounds"), Alpha &&
		Alpha->CombatReactions.Num() == 1 &&
		Alpha->CombatReactions[0].Trigger == EGridCombatReactionTrigger::OwnedStatusTargetDefeated &&
		Alpha->CombatReactions[0].Limit == EGridCombatReactionLimit::OncePerRound &&
		Alpha->CombatReactions[0].RequiredTargetStatusEffectIdsFromOwner.Contains(TEXT("Status_MarkedByRanger")) &&
		Alpha->CombatReactions[0].TransferTargetRangeCells == 3 &&
		Alpha->CombatReactions[0].TransferStatusDurationOverride == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0393ScoutAuthoringTest, "Grimrock.RPG.RPG03.9.3.Authoring.Scout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0393ScoutAuthoringTest::RunTest(const FString&)
{
	URPGClassAsset* Ranger = RPG0393Authoring::BuildRanger();

	const FRPGClassProgressionChoiceDefinition* Vigilance =
		Ranger->FindProgressionChoice(TEXT("Talent_Ranger_Scout_Vigilance"));
	TestTrue(TEXT("Vigilance gives nonstacking Perception +2 and first-round Initiative +2"), Vigilance &&
		Vigilance->PartyModifiers.Num() == 1 &&
		Vigilance->PartyModifiers[0].StackingGroupId == TEXT("Party.Ranger.Vigilance") &&
		Vigilance->PartyModifiers[0].GroupSkillIds.Contains(TEXT("Skill_Perception")) &&
		Vigilance->PartyModifiers[0].GroupSkillCheckModifier == 2 &&
		Vigilance->FirstRoundInitiativeModifier == 2);

	const FGridCombatActionDefinition* Trap = RPG0393Authoring::FindAction(Ranger, TEXT("Action_Ranger_HuntingTrap"));
	TestTrue(TEXT("Hunting Trap lasts four rounds and scales 5 Piercing damage with Wisdom"), Trap &&
		Trap->TrapEffect.bPlaceTrap && Trap->TrapEffect.TrapId == TEXT("Trap_Hunting") &&
		Trap->TrapEffect.DurationRounds == 4 && Trap->TrapEffect.BaseDamage == 5 &&
		Trap->TrapEffect.PhysicalSubtype == EGridPhysicalDamageSubtype::Piercing &&
		Trap->TrapEffect.DamageScalingAttribute == EGridAttackScalingAttribute::Wisdom &&
		Trap->TrapEffect.StatusApplications.Num() == 1 &&
		Trap->TrapEffect.StatusApplications[0].StatusEffectId == TEXT("Status_Immobilized"));

	const FGridCombatActionDefinition* Retreat = RPG0393Authoring::FindAction(Ranger, TEXT("Action_Ranger_TacticalRetreat"));
	TestTrue(TEXT("Tactical Retreat costs one AP, one PAM and moves party backward one"), Retreat &&
		Retreat->ActionPointCost == 1 && Retreat->MovementEffects.Num() == 1 &&
		Retreat->MovementEffects[0].Subject == EGridCombatMovementSubject::PartyGroup &&
		Retreat->MovementEffects[0].Direction == EGridCombatMovementDirection::BackwardFromFacing &&
		Retreat->MovementEffects[0].DistanceCells == 1 &&
		Retreat->MovementEffects[0].MobilityActionPointCost == 1 && !Retreat->MovementEffects[0].bForced);

	const FRPGClassProgressionChoiceDefinition* Terrain =
		Ranger->FindProgressionChoice(TEXT("Talent_Ranger_Scout_TerrainMaster"));
	TestTrue(TEXT("Terrain Master is stationary-qualified Ranged +10 damage and +1 Accuracy"), Terrain &&
		Terrain->CombatModifiers.Num() == 1 &&
		Terrain->CombatModifiers[0].ActionTypes.Contains(EGridCombatActionType::RangedAttack) &&
		Terrain->CombatModifiers[0].bRequirePartyStationarySincePreviousActivation &&
		Terrain->CombatModifiers[0].OutgoingDamagePercentModifier == 10 &&
		Terrain->CombatModifiers[0].AccuracyModifier == 1);

	const FRPGClassProgressionChoiceDefinition* Guide =
		Ranger->FindProgressionChoice(TEXT("Talent_Ranger_Scout_GroupGuide"));
	TestTrue(TEXT("Group Guide is one nonstacking Perception/Survival +2 and PAM +1 contribution"), Guide &&
		Guide->PartyModifiers.Num() == 1 &&
		Guide->PartyModifiers[0].StackingGroupId == TEXT("Party.Ranger.GroupGuide") &&
		Guide->PartyModifiers[0].GroupSkillIds.Contains(TEXT("Skill_Perception")) &&
		Guide->PartyModifiers[0].GroupSkillIds.Contains(TEXT("Skill_Survival")) &&
		Guide->PartyModifiers[0].GroupSkillCheckModifier == 2 &&
		Guide->PartyModifiers[0].MaximumMobilityActionPointsModifier == 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0393MarkedStatusAuthoringTest, "Grimrock.RPG.RPG03.9.3.Authoring.MarkedStatus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0393MarkedStatusAuthoringTest::RunTest(const FString&)
{
	UGridStatusEffectDefinitionAsset* Status = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	TestTrue(TEXT("Marked status configures successfully"), FRPGRangerAuthoring::ConfigureMarkedStatus(*Status));
	TestEqual(TEXT("Marked status keeps canonical EffectId"), Status->EffectId, FName(TEXT("Status_MarkedByRanger")));
	TestEqual(TEXT("Marked status defaults to three rounds"), Status->DefaultDuration, 3);
	TestEqual(TEXT("Marked status refreshes duration"), Status->StackPolicy, EGridStatusEffectStackPolicy::RefreshDuration);
	TestTrue(TEXT("Marked status permits distinct Ranger sources"), Status->bDistinctPerSource);
	TestTrue(TEXT("Each Ranger has only one marked monster"), Status->bUniquePerSourceAcrossMonsters);
	TestTrue(TEXT("Marked status is structurally valid"), Status->IsValidDefinition());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0393ProductionAssetsTest, "Grimrock.RPG.RPG03.9.3.Authoring.ProductionAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0393ProductionAssetsTest::RunTest(const FString&)
{
	TArray<FRPGRangerFavoredEnemyCategoryDefinition> Categories;
	FString Error;
	if (!TestTrue(TEXT("Production bestiary exposes Favored Enemy categories"),
			FRPGRangerAuthoring::CollectProductionFavoredEnemyCategories(Categories, Error)))
	{
		AddError(Error);
		return false;
	}

	URPGClassAsset* Ranger = LoadObject<URPGClassAsset>(nullptr, FRPGRangerAuthoring::RangerAssetPath());
	if (!TestNotNull(TEXT("Production DA_Class_Ranger loads"), Ranger))
	{
		return false;
	}
	TestEqual(TEXT("Production Ranger keeps canonical ClassId"), Ranger->ClassId, FName(TEXT("Ranger")));
	TestEqual(TEXT("Production Ranger contains nine active actions"), Ranger->CombatActions.Num(), 9);
	TestEqual(TEXT("Production Ranger choice count follows current bestiary categories"), Ranger->ProgressionChoices.Num(), 14 + Categories.Num());
	TestTrue(TEXT("Production Ranger is structurally valid"), Ranger->IsValidDefinition());

	for (const FRPGRangerFavoredEnemyCategoryDefinition& Category : Categories)
	{
		const FName ChoiceId(*FString::Printf(TEXT("Talent_Ranger_Hunter_FavoredEnemy_%s"), *Category.CategoryId.ToString()));
		const FRPGClassProgressionChoiceDefinition* Choice = Ranger->FindProgressionChoice(ChoiceId);
		TestTrue(*FString::Printf(TEXT("Production Favored Enemy %s uses category presentation authority"),
			*Category.CategoryId.ToString()),
			Choice && Choice->DisplayName.ToString() ==
				FString::Printf(TEXT("Ennemi juré — %s"), *Category.DisplayName.ToString()));
	}

	UGridStatusEffectDefinitionAsset* Marked =
		LoadObject<UGridStatusEffectDefinitionAsset>(nullptr, FRPGRangerAuthoring::MarkedStatusPath());
	TestNotNull(TEXT("Production Status_MarkedByRanger loads"), Marked);
	if (Marked)
	{
		TestTrue(TEXT("Production mark is structurally valid"), Marked->IsValidDefinition());
		TestTrue(TEXT("Production mark keeps per-source identity"), Marked->bDistinctPerSource && Marked->bUniquePerSourceAcrossMonsters);
	}

	UGridStatusEffectDefinitionAsset* Immobilized = LoadObject<UGridStatusEffectDefinitionAsset>(
		nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/StatusEffects/DA_Status_Immobilized.DA_Status_Immobilized"));
	TestTrue(TEXT("Ranger reuses the existing production Immobilized status"), IsValid(Immobilized) && Immobilized->IsValidDefinition());
	return true;
}

#endif
