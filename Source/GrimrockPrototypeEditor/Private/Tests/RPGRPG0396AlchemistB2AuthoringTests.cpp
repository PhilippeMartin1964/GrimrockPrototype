#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGAlchemistAuthoring.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Combat/GridCombatArmorEffectResolver.h"

namespace RPG0396B2
{
	URPGClassAsset* BuildClass()
	{
		URPGClassAsset* Asset = NewObject<URPGClassAsset>(GetTransientPackage());
		Asset->ClassId = TEXT("Alchemist");
		Asset->DisplayName = FText::FromString(TEXT("Alchimiste"));
		Asset->HealthAtLevelOne = 10;
		FRPGAlchemistAuthoring::ConfigureClass(*Asset);
		return Asset;
	}

	UGridItemDefinitionAsset* BuildItem(FName Id)
	{
		UGridItemDefinitionAsset* Item = NewObject<UGridItemDefinitionAsset>(GetTransientPackage());
		return FRPGAlchemistAuthoring::ConfigureItem(*Item, Id) ? Item : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396B2StructureTest, "Grimrock.RPG.RPG03.9.6B2.Structure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396B2StructureTest::RunTest(const FString&)
{
	URPGClassAsset* Asset = RPG0396B2::BuildClass();
	TestTrue(TEXT("Complete Alchemist is structurally valid"), Asset->IsValidDefinition());
	TestEqual(TEXT("Alchemist has fifteen conceptual Choice records"), Asset->ProgressionChoices.Num(), 15);
	TestEqual(TEXT("Only Catalyst is a direct class action"), Asset->CombatActions.Num(), 1);
	TestTrue(TEXT("Catalyst action exists"), Asset->CombatActions.Num() == 1 &&
		Asset->CombatActions[0].ActionId == TEXT("Action_Alchemist_Catalyst"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396B2OilTest, "Grimrock.RPG.RPG03.9.6B2.OilSlick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396B2OilTest::RunTest(const FString&)
{
	UGridItemDefinitionAsset* Item = RPG0396B2::BuildItem(TEXT("Item_Flask_Oil"));
	FGridCombatActionDefinition A;
	TestTrue(TEXT("Oil item builds"), Item && Item->BuildQuickItemCombatActionDefinition(A));
	TestTrue(TEXT("Oil is AP2 R4 Area1 and creates four-round +1 traversal Oil"), A.ActionPointCost == 2 &&
		A.RangeCells == 4 && A.AreaRadiusCells == 1 && A.SurfaceEffects.Num() == 1 &&
		A.SurfaceEffects[0].SurfaceType == EGridCombatSurfaceType::Oil &&
		A.SurfaceEffects[0].DurationRounds == 4 && A.SurfaceEffects[0].TraversalCostModifier == 1 &&
		A.OffensiveProfile.AttackDefinition.MinDamage == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396B2AcidTest, "Grimrock.RPG.RPG03.9.6B2.AcidFlask",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396B2AcidTest::RunTest(const FString&)
{
	UGridItemDefinitionAsset* Item = RPG0396B2::BuildItem(TEXT("Item_Flask_Acid"));
	FGridCombatActionDefinition A;
	TestTrue(TEXT("Acid Flask builds"), Item && Item->BuildQuickItemCombatActionDefinition(A));
	TestTrue(TEXT("Acid Flask is Hostile R4 AP2 CD1 Effect"), A.TargetingPolicy == EGridCombatTargetingPolicy::Hostile &&
		A.ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
		A.RangeCells == 4 && A.ActionPointCost == 2 && A.CooldownRounds == 1);
	TestTrue(TEXT("Acid damages PhysicalArmor by 6 + 2xAlchemy without HP spill"), A.ArmorEffects.Num() == 1 &&
		A.ArmorEffects[0].Pool == EGridCombatArmorPool::Physical &&
		A.ArmorEffects[0].Operation == EGridCombatArmorEffectOperation::Damage &&
		A.ArmorEffects[0].Magnitude == EGridCombatArmorEffectMagnitude::Flat &&
		A.ArmorEffects[0].Amount == 6 && A.ArmorEffects[0].ScalingSkillId == TEXT("Skill_Alchemy") &&
		A.ArmorEffects[0].SkillRankScale == 2);
	TestTrue(TEXT("Acid applies Corroded for two rounds"), A.StatusApplications.Num() == 1 &&
		A.StatusApplications[0].StatusEffectId == TEXT("Status_Corroded") &&
		A.StatusApplications[0].DurationOverride == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396B2CloudTest, "Grimrock.RPG.RPG03.9.6B2.CorrosiveCloud",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396B2CloudTest::RunTest(const FString&)
{
	UGridItemDefinitionAsset* Item = RPG0396B2::BuildItem(TEXT("Item_Flask_CorrosiveCloud"));
	FGridCombatActionDefinition A;
	TestTrue(TEXT("Corrosive Cloud builds"), Item && Item->BuildQuickItemCombatActionDefinition(A));
	TestTrue(TEXT("Cloud is AP3 R4 Area1 CD2 with 4 + Alchemy Poison impact"), A.ActionPointCost == 3 &&
		A.RangeCells == 4 && A.AreaRadiusCells == 1 && A.CooldownRounds == 2 &&
		A.OffensiveProfile.AttackDefinition.DamageType == EGridDamageType::Poison &&
		A.OffensiveProfile.AttackDefinition.MinDamage == 4 &&
		A.QuickItemScaling.DirectDamageSkillRankScale == 1);
	TestTrue(TEXT("Cloud surface is three rounds, two Poison per round, gated Poison status for two Turns"),
		A.SurfaceEffects.Num() == 1 &&
		A.SurfaceEffects[0].SurfaceType == EGridCombatSurfaceType::PoisonCloud &&
		A.SurfaceEffects[0].DurationRounds == 3 &&
		A.SurfaceEffects[0].PeriodicDamageType == EGridDamageType::Poison &&
		A.SurfaceEffects[0].PeriodicDamagePerRound == 2 &&
		A.SurfaceEffects[0].PeriodicStatusApplications.Num() == 1 &&
		A.SurfaceEffects[0].PeriodicStatusApplications[0].StatusEffectId == TEXT("Status_Poison") &&
		A.SurfaceEffects[0].PeriodicStatusApplications[0].ArmorGate == EGridCombatStatusArmorGate::MagicalArmorDepleted &&
		A.SurfaceEffects[0].PeriodicStatusApplications[0].DurationOverride == 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396B2CatalystTest, "Grimrock.RPG.RPG03.9.6B2.Catalyst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396B2CatalystTest::RunTest(const FString&)
{
	URPGClassAsset* Asset = RPG0396B2::BuildClass();
	const FGridCombatActionDefinition* A = Asset->CombatActions.FindByPredicate(
		[](const FGridCombatActionDefinition& Candidate) { return Candidate.ActionId == TEXT("Action_Alchemist_Catalyst"); });
	TestTrue(TEXT("Catalyst is AP1 R4 CD2 Cell Effect with AnyCanonical interaction"), A &&
		A->ActionPointCost == 1 && A->RangeCells == 4 && A->CooldownRounds == 2 &&
		A->TargetingPolicy == EGridCombatTargetingPolicy::Cell &&
		A->ResolutionProfile == EGridCombatActionResolutionProfile::Effect &&
		A->SurfaceInteraction == EGridCombatSurfaceInteraction::AnyCanonical &&
		A->Requirements.Contains(TEXT("Talent_Alchemist_Transmuter_Catalyst")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396B2CorrodedTest, "Grimrock.RPG.RPG03.9.6B2.Corroded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396B2CorrodedTest::RunTest(const FString&)
{
	UGridStatusEffectDefinitionAsset* Status = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	TestTrue(TEXT("Corroded configures valid"), FRPGAlchemistAuthoring::ConfigureStatus(*Status, TEXT("Status_Corroded")));
	TestTrue(TEXT("Corroded is 2 rounds and -20 percent PhysicalArmor restoration"), Status->IsValidDefinition() &&
		Status->DurationUnit == EGridStatusEffectDurationUnit::Rounds && Status->DefaultDuration == 2 &&
		Status->Disposition == EGridStatusEffectDisposition::Debuff &&
		Status->CombatModifiers.Num() == 1 &&
		Status->CombatModifiers[0].PhysicalArmorRestorationPercentModifier == -20);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396B2MajorTransmutationTest, "Grimrock.RPG.RPG03.9.6B2.MajorTransmutation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396B2MajorTransmutationTest::RunTest(const FString&)
{
	for (const EGridCombatSurfaceType Output : {
		EGridCombatSurfaceType::Fire, EGridCombatSurfaceType::Ice,
		EGridCombatSurfaceType::Poison, EGridCombatSurfaceType::Oil })
	{
		FGridCombatActionDefinition A;
		TestTrue(TEXT("Recipe output builds a valid Major Transmutation contribution"),
			FRPGAlchemistAuthoring::BuildMajorTransmutationRecipeAction(Output, A));
		TestTrue(TEXT("Major Transmutation keeps canonical AP/R4/Area2/CD5/item cost"), A.ActionId == TEXT("Action_Alchemist_MajorTransmutation") &&
			A.ActionPointCost == 4 && A.RangeCells == 4 && A.AreaRadiusCells == 2 && A.CooldownRounds == 5 &&
			A.ResourceCosts.SourceItemQuantityCost == 1 && A.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem &&
			A.Requirements.Contains(TEXT("Talent_Alchemist_Transmuter_MajorTransmutation")));
		TestTrue(TEXT("Recipe contributes exactly one fixed four-round selected conversion"), A.SurfaceConversions.Num() == 1 &&
			A.SurfaceConversions[0].OutputSurfaceType == Output &&
			A.SurfaceConversions[0].EmptyCellDurationRounds == 4 &&
			A.SurfaceConversions[0].bUseFixedFinalDuration &&
			A.SurfaceConversions[0].FixedFinalDurationRounds == 4 &&
			A.SurfaceConversions[0].bAllowEmptyCell);
		if (Output == EGridCombatSurfaceType::Oil)
		{
			TestEqual(TEXT("Major Oil output preserves +1 traversal"), A.SurfaceConversions[0].OutputTraversalCostModifier, 1);
		}
	}

	URPGClassAsset* Asset = RPG0396B2::BuildClass();
	const FRPGClassProgressionChoiceDefinition* Major =
		Asset->FindProgressionChoice(TEXT("Talent_Alchemist_Transmuter_MajorTransmutation"));
	TestTrue(TEXT("Major Transmutation advertises four future recipe outputs"), Major &&
		Major->GrantedRequirementIds.Contains(TEXT("Recipe_MajorTransmutation_Fire")) &&
		Major->GrantedRequirementIds.Contains(TEXT("Recipe_MajorTransmutation_Ice")) &&
		Major->GrantedRequirementIds.Contains(TEXT("Recipe_MajorTransmutation_Poison")) &&
		Major->GrantedRequirementIds.Contains(TEXT("Recipe_MajorTransmutation_Oil")));
	TestTrue(TEXT("Major Transmutation contributes +50 percent surface reaction damage by ActionId"), Major &&
		Major->CombatModifiers.Num() == 1 &&
		Major->CombatModifiers[0].ActionIds.Contains(TEXT("Action_Alchemist_MajorTransmutation")) &&
		Major->CombatModifiers[0].SurfaceReactionDamagePercentModifier == 50);
	return true;
}

#endif
