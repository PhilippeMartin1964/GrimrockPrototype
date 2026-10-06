#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGAlchemistAuthoring.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridQuickItemResolver.h"

namespace RPG0396B1
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

	const FRPGClassProgressionChoiceDefinition* Choice(const URPGClassAsset* Asset, FName Id)
	{
		return Asset ? Asset->FindProgressionChoice(Id) : nullptr;
	}

	bool Chain(const URPGClassAsset* Asset, FName Id, int32 Level, FName Prereq)
	{
		const FRPGClassProgressionChoiceDefinition* C = Choice(Asset, Id);
		return C && C->MinimumLevel == Level && C->PointCost == 1 &&
			(Prereq.IsNone() ? C->PrerequisiteChoiceIds.IsEmpty() : C->PrerequisiteChoiceIds.Contains(Prereq));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396B1StructureTest, "Grimrock.RPG.RPG03.9.6B1.Structure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396B1StructureTest::RunTest(const FString&)
{
	using namespace RPG0396B1;
	URPGClassAsset* Asset = BuildClass();
	TestTrue(TEXT("B1 Alchemist class is structurally valid"), Asset->IsValidDefinition());
	TestTrue(TEXT("Complete Alchemist still contains at least the ten B1 choices"), Asset->ProgressionChoices.Num() >= 10);
	TestTrue(TEXT("Class CombatActions never duplicate QuickItem sources"),
		!Asset->CombatActions.ContainsByPredicate([](const FGridCombatActionDefinition& Action)
		{
			return Action.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem;
		}));

	TestTrue(TEXT("Grenadier L2"), Chain(Asset, TEXT("Talent_Alchemist_Grenadier_FireBomb"), 2, NAME_None));
	TestTrue(TEXT("Grenadier L6"), Chain(Asset, TEXT("Talent_Alchemist_Grenadier_ToxicBomb"), 6, TEXT("Talent_Alchemist_Grenadier_FireBomb")));
	TestTrue(TEXT("Grenadier L10"), Chain(Asset, TEXT("Talent_Alchemist_Grenadier_PreciseCharge"), 10, TEXT("Talent_Alchemist_Grenadier_ToxicBomb")));
	TestTrue(TEXT("Grenadier L14"), Chain(Asset, TEXT("Talent_Alchemist_Grenadier_ChainReaction"), 14, TEXT("Talent_Alchemist_Grenadier_PreciseCharge")));
	TestTrue(TEXT("Grenadier L18"), Chain(Asset, TEXT("Talent_Alchemist_Grenadier_MasterGrenadier"), 18, TEXT("Talent_Alchemist_Grenadier_ChainReaction")));
	TestTrue(TEXT("Apothecary L2"), Chain(Asset, TEXT("Talent_Alchemist_Apothecary_EnhancedPotion"), 2, NAME_None));
	TestTrue(TEXT("Apothecary L6"), Chain(Asset, TEXT("Talent_Alchemist_Apothecary_Antidote"), 6, TEXT("Talent_Alchemist_Apothecary_EnhancedPotion")));
	TestTrue(TEXT("Apothecary L10"), Chain(Asset, TEXT("Talent_Alchemist_Apothecary_DefensiveElixir"), 10, TEXT("Talent_Alchemist_Apothecary_Antidote")));
	TestTrue(TEXT("Apothecary L14"), Chain(Asset, TEXT("Talent_Alchemist_Apothecary_Diffusion"), 14, TEXT("Talent_Alchemist_Apothecary_DefensiveElixir")));
	TestTrue(TEXT("Apothecary L18"), Chain(Asset, TEXT("Talent_Alchemist_Apothecary_Panacea"), 18, TEXT("Talent_Alchemist_Apothecary_Diffusion")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396B1ItemIdentityTest, "Grimrock.RPG.RPG03.9.6B1.ItemIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396B1ItemIdentityTest::RunTest(const FString&)
{
	TArray<FName> Ids;
	FRPGAlchemistAuthoring::GetB1ItemIds(Ids);
	TestEqual(TEXT("B1 defines eight concrete QuickItems"), Ids.Num(), 8);
	for (const FName Id : Ids)
	{
		UGridItemDefinitionAsset* Item = RPG0396B1::BuildItem(Id);
		if (!TestNotNull(*FString::Printf(TEXT("%s configures"), *Id.ToString()), Item))
		{
			continue;
		}
		FGridCombatActionDefinition Built;
		TestTrue(*FString::Printf(TEXT("%s builds QuickItem action"), *Id.ToString()), Item->BuildQuickItemCombatActionDefinition(Built));
		TestEqual(TEXT("Built action preserves explicit canonical ActionId"), Built.ActionId, Item->QuickItemActionIdOverride);
		TestEqual(TEXT("Built action uses QuickItem source"), Built.SourcePolicy, EGridCombatActionSourcePolicy::QuickItem);
		TestEqual(TEXT("Built action consumes exactly one item"), Built.ResourceCosts.SourceItemQuantityCost, 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396B1BombsTest, "Grimrock.RPG.RPG03.9.6B1.Bombs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396B1BombsTest::RunTest(const FString&)
{
	for (const FName Id : { FName(TEXT("Item_Bomb_Fire")), FName(TEXT("Item_Bomb_Toxic")) })
	{
		UGridItemDefinitionAsset* Item = RPG0396B1::BuildItem(Id);
		FGridCombatActionDefinition A;
		if (!TestTrue(*FString::Printf(TEXT("%s builds"), *Id.ToString()), Item && Item->BuildQuickItemCombatActionDefinition(A)))
		{
			continue;
		}
		TestTrue(TEXT("Bomb is Area1 R4 and costs 2 AP"), A.TargetingPolicy == EGridCombatTargetingPolicy::Area &&
			A.AreaRadiusCells == 1 && A.RangeCells == 4 && A.ActionPointCost == 2);
		TestTrue(TEXT("Bomb has 6 base direct damage + Alchemy rank"), A.OffensiveProfile.AttackDefinition.MinDamage == 6 &&
			A.OffensiveProfile.AttackDefinition.MaxDamage == 6 &&
			A.QuickItemScaling.ScalingSkillId == TEXT("Skill_Alchemy") &&
			A.QuickItemScaling.DirectDamageSkillRankScale == 1);
		TestTrue(TEXT("Bomb applies gated status and creates surface"), A.StatusApplications.Num() == 1 &&
			A.StatusApplications[0].ArmorGate == EGridCombatStatusArmorGate::MagicalArmorDepleted &&
			A.SurfaceEffects.Num() == 1 && A.bAffectsAlliesInArea);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396B1GrenadierPassivesTest, "Grimrock.RPG.RPG03.9.6B1.GrenadierPassives",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396B1GrenadierPassivesTest::RunTest(const FString&)
{
	URPGClassAsset* Asset = RPG0396B1::BuildClass();
	const auto* Precise = RPG0396B1::Choice(Asset, TEXT("Talent_Alchemist_Grenadier_PreciseCharge"));
	const auto* Chain = RPG0396B1::Choice(Asset, TEXT("Talent_Alchemist_Grenadier_ChainReaction"));
	const auto* Master = RPG0396B1::Choice(Asset, TEXT("Talent_Alchemist_Grenadier_MasterGrenadier"));
	TestTrue(TEXT("Precise Charge is bomb-tagged +1 range / -50 friendly direct"), Precise && Precise->CombatModifiers.Num() == 1 &&
		Precise->CombatModifiers[0].RequiredSourceTags.Contains(TEXT("QuickItem.Bomb")) &&
		Precise->CombatModifiers[0].RangeCellsModifier == 1 &&
		Precise->CombatModifiers[0].FriendlyDirectDamagePercentModifier == -50);
	TestTrue(TEXT("Chain Reaction is once per bomb action, +25 reaction damage and +1 radius"), Chain && Chain->CombatReactions.Num() == 1 &&
		Chain->CombatReactions[0].Trigger == EGridCombatReactionTrigger::SurfaceReaction &&
		Chain->CombatReactions[0].Limit == EGridCombatReactionLimit::OncePerAction &&
		Chain->CombatReactions[0].RequiredSourceTags.Contains(TEXT("QuickItem.Bomb")) &&
		Chain->CombatReactions[0].SurfaceReactionDamagePercentModifier == 25 &&
		Chain->CombatReactions[0].SurfaceReactionAreaRadiusModifier == 1 &&
		!Chain->CombatReactions[0].bAllowReactionGeneratedEvents);
	TestTrue(TEXT("Master Grenadier is bomb-tagged -1 AP / +20 direct"), Master && Master->CombatModifiers.Num() == 1 &&
		Master->CombatModifiers[0].ActionPointCostModifier == -1 &&
		Master->CombatModifiers[0].OutgoingDamagePercentModifier == 20);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396B1ApothecaryPassivesTest, "Grimrock.RPG.RPG03.9.6B1.ApothecaryPassives",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396B1ApothecaryPassivesTest::RunTest(const FString&)
{
	URPGClassAsset* Asset = RPG0396B1::BuildClass();
	const auto* Enhanced = RPG0396B1::Choice(Asset, TEXT("Talent_Alchemist_Apothecary_EnhancedPotion"));
	const auto* Diffusion = RPG0396B1::Choice(Asset, TEXT("Talent_Alchemist_Apothecary_Diffusion"));
	TestTrue(TEXT("Enhanced Potion is positive-potion +25 percent"), Enhanced && Enhanced->CombatModifiers.Num() == 1 &&
		Enhanced->CombatModifiers[0].RequiredSourceTags.Contains(TEXT("QuickItem.Potion.Positive")) &&
		Enhanced->CombatModifiers[0].PositiveEffectPercentModifier == 25);
	TestTrue(TEXT("Diffusion is one secondary ally at 50 percent magnitude/duration"), Diffusion && Diffusion->CombatModifiers.Num() == 1 &&
		Diffusion->CombatModifiers[0].QuickItemSecondaryTargetCount == 1 &&
		Diffusion->CombatModifiers[0].QuickItemSecondaryMagnitudePercent == 50 &&
		Diffusion->CombatModifiers[0].QuickItemSecondaryDurationPercent == 50);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396B1ApothecaryItemsTest, "Grimrock.RPG.RPG03.9.6B1.ApothecaryItems",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396B1ApothecaryItemsTest::RunTest(const FString&)
{
	UGridItemDefinitionAsset* Antidote = RPG0396B1::BuildItem(TEXT("Item_Antidote"));
	UGridItemDefinitionAsset* Panacea = RPG0396B1::BuildItem(TEXT("Item_Panacea"));
	FGridCombatActionDefinition A, P;
	TestTrue(TEXT("Antidote builds"), Antidote && Antidote->BuildQuickItemCombatActionDefinition(A));
	TestTrue(TEXT("Panacea builds"), Panacea && Panacea->BuildQuickItemCombatActionDefinition(P));
	TestTrue(TEXT("Antidote is Ally R1 AP1 with Poison + Toxin removal"), A.TargetingPolicy == EGridCombatTargetingPolicy::Ally &&
		A.RangeCells == 1 && A.ActionPointCost == 1 && A.StatusRemovals.Num() == 2);
	TestTrue(TEXT("Panacea is Ally R1 AP2 CD3, 10 + 2xAlchemy, +8 MagicalArmor, 3 removals"), P.TargetingPolicy == EGridCombatTargetingPolicy::Ally &&
		P.RangeCells == 1 && P.ActionPointCost == 2 && P.CooldownRounds == 3 &&
		P.EffectProfile.RestoreHealth == 10 && P.QuickItemScaling.RestoreHealthSkillRankScale == 2 &&
		P.ArmorEffects.Num() == 1 && P.ArmorEffects[0].Amount == 8 &&
		P.StatusRemovals.Num() == 1 && P.StatusRemovals[0].MaximumRemovals == 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG0396B1StatusesTest, "Grimrock.RPG.RPG03.9.6B1.Statuses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG0396B1StatusesTest::RunTest(const FString&)
{
	TArray<FName> Ids;
	FRPGAlchemistAuthoring::GetB1StatusIds(Ids);
	TestEqual(TEXT("B1 requires Poison plus four defensive elixir statuses"), Ids.Num(), 5);
	for (const FName Id : Ids)
	{
		UGridStatusEffectDefinitionAsset* Status = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
		TestTrue(*FString::Printf(TEXT("%s configures valid"), *Id.ToString()),
			FRPGAlchemistAuthoring::ConfigureStatus(*Status, Id) && Status->IsValidDefinition());
	}
	UGridStatusEffectDefinitionAsset* Poison = NewObject<UGridStatusEffectDefinitionAsset>(GetTransientPackage());
	FRPGAlchemistAuthoring::ConfigureStatus(*Poison, TEXT("Status_Poison"));
	TestTrue(TEXT("Poison is 3 Turns at 2 Poison/tick and Toxin-tagged"),
		Poison->DefaultDuration == 3 && Poison->DurationUnit == EGridStatusEffectDurationUnit::Turns &&
		Poison->PeriodicDamage.DamageType == EGridDamageType::Poison && Poison->PeriodicDamage.DamagePerStack == 2 &&
		Poison->StatusTags.Contains(TEXT("Toxin")));
	return true;
}

#endif
