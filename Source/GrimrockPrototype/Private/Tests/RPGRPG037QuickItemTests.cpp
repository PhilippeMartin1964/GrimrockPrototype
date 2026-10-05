#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/Combat/GridCombatArmorEffectResolver.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"
#include "Runtime/Combat/GridQuickItemResolver.h"
#include "Runtime/GridItemDefinitionAsset.h"

namespace RPG037
{
	FGridCombatActionDefinition MakeQuickAttack()
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = TEXT("Action_RPG037_Bomb");
		Action.DisplayName = FText::FromString(TEXT("RPG03.7 Bomb"));
		Action.ActionType = EGridCombatActionType::RangedAttack;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
		Action.SourceTags = { TEXT("QuickItem.Alchemy"), TEXT("QuickItem.Bomb") };
		Action.TargetingPolicy = EGridCombatTargetingPolicy::Area;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
		Action.ActionPointCost = 2;
		Action.RangeCells = 4;
		Action.AreaRadiusCells = 1;
		Action.ResourceCosts.SourceItemQuantityCost = 1;
		Action.OffensiveProfile.AttackId = TEXT("Attack_RPG037_Bomb");
		Action.OffensiveProfile.AttackDefinition.DamageType = EGridDamageType::Fire;
		Action.OffensiveProfile.AttackDefinition.MinDamage = 6;
		Action.OffensiveProfile.AttackDefinition.MaxDamage = 6;
		Action.OffensiveProfile.DamageScalingAttribute = EGridAttackScalingAttribute::None;
		Action.OffensiveProfile.RangeCells = 4;
		Action.QuickItemScaling.ScalingSkillId = TEXT("Skill_Alchemy");
		Action.QuickItemScaling.DirectDamageSkillRankScale = 1;
		return Action;
	}

	FGridCombatActionDefinition MakePositivePotion()
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = TEXT("Action_RPG037_PositivePotion");
		Action.DisplayName = FText::FromString(TEXT("RPG03.7 Potion"));
		Action.ActionType = EGridCombatActionType::Ability;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
		Action.SourceTags = { TEXT("QuickItem.Alchemy"), TEXT("QuickItem.Potion.Positive") };
		Action.TargetingPolicy = EGridCombatTargetingPolicy::Self;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
		Action.ActionPointCost = 1;
		Action.ResourceCosts.SourceItemQuantityCost = 1;
		Action.EffectProfile.RestoreHealth = 10;
		Action.QuickItemScaling.ScalingSkillId = TEXT("Skill_Alchemy");
		Action.QuickItemScaling.RestoreHealthSkillRankScale = 2;
		return Action;
	}

	FRPGSkillRank AlchemyRank(int32 Rank)
	{
		FRPGSkillRank Skill;
		Skill.SkillId = TEXT("Skill_Alchemy");
		Skill.Rank = Rank;
		return Skill;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG037SourceTagFilteringTest, "Grimrock.RPG.RPG03.7.SourceTagFiltering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG037SourceTagFilteringTest::RunTest(const FString&)
{
	FGridCombatModifierProfile Profile;
	Profile.SourcePolicies = { EGridCombatActionSourcePolicy::QuickItem };
	Profile.RequiredSourceTags = { TEXT("QuickItem.Bomb"), TEXT("QuickItem.Alchemy") };
	Profile.RangeCellsModifier = 1;

	FGridCombatModifierContext Context = FGridCombatModifierResolver::MakeActionContext(RPG037::MakeQuickAttack(), TEXT("Item_Bomb_Fire"));
	TestTrue(TEXT("All required source tags match"), FGridCombatModifierResolver::Matches(Profile, Context));

	Context.SourceTags.Remove(TEXT("QuickItem.Bomb"));
	TestFalse(TEXT("Missing one required tag rejects modifier"), FGridCombatModifierResolver::Matches(Profile, Context));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG037QuickItemNormalizationTest, "Grimrock.RPG.RPG03.7.QuickItemNormalization",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG037QuickItemNormalizationTest::RunTest(const FString&)
{
	UGridItemDefinitionAsset* Item = NewObject<UGridItemDefinitionAsset>(GetTransientPackage());
	Item->ItemDefinitionId = TEXT("Item_RPG037_Bomb");
	Item->ItemType = EGridItemType::Component;
	Item->DisplayName = FText::FromString(TEXT("RPG03.7 Bomb"));
	Item->bProvidesQuickItemCombatAction = true;
	Item->ItemTags = { TEXT("QuickItem.Bomb"), TEXT("QuickItem.Alchemy"), TEXT("QuickItem.Bomb"), NAME_None };
	Item->QuickItemCombatAction = RPG037::MakeQuickAttack();
	Item->QuickItemCombatAction.ActionId = TEXT("AuthoredIdentityIsNormalized");
	Item->QuickItemCombatAction.SourcePolicy = EGridCombatActionSourcePolicy::Ability;

	FGridCombatActionDefinition Built;
	TestTrue(TEXT("Explicit QuickItem opt-in supports non-Potion/Scroll item types"), Item->BuildQuickItemCombatActionDefinition(Built));
	TestEqual(TEXT("Source policy is normalized"), Built.SourcePolicy, EGridCombatActionSourcePolicy::QuickItem);
	TestEqual(TEXT("Source item cost is exactly at least one"), Built.ResourceCosts.SourceItemQuantityCost, 1);
	TestTrue(TEXT("Bomb tag is projected"), Built.SourceTags.Contains(TEXT("QuickItem.Bomb")));
	TestTrue(TEXT("Alchemy tag is projected"), Built.SourceTags.Contains(TEXT("QuickItem.Alchemy")));
	TestEqual(TEXT("Source tags are normalized unique"), Built.SourceTags.Num(), 2);
	TestEqual(TEXT("Area targeting remains authorable for C8"), Built.TargetingPolicy, EGridCombatTargetingPolicy::Area);

	UGridItemDefinitionAsset* LegacyPotion = NewObject<UGridItemDefinitionAsset>(GetTransientPackage());
	LegacyPotion->ItemDefinitionId = TEXT("Item_RPG037_LegacyPotion");
	LegacyPotion->ItemType = EGridItemType::Potion;
	LegacyPotion->DisplayName = FText::FromString(TEXT("Legacy Potion"));
	LegacyPotion->bProvidesQuickItemCombatAction = true;
	LegacyPotion->QuickItemCombatAction = RPG037::MakePositivePotion();
	LegacyPotion->QuickItemCombatAction.QuickItemScaling = FGridCombatQuickItemScalingProfile();
	LegacyPotion->QuickItemCombatAction.SourceTags.Reset();
	FGridCombatActionDefinition LegacyBuilt;
	TestTrue(TEXT("Existing unscaled Potion QuickItems remain compatible"), LegacyPotion->BuildQuickItemCombatActionDefinition(LegacyBuilt));
	TestTrue(TEXT("Legacy potion keeps its authored positive self effect"), LegacyBuilt.EffectProfile.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG037AlchemyDamageScalingTest, "Grimrock.RPG.RPG03.7.AlchemyDamageScaling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG037AlchemyDamageScalingTest::RunTest(const FString&)
{
	const FGridCombatActionDefinition Action = RPG037::MakeQuickAttack();
	FGridAttackSourceStats Source;
	Source.DamageBonus = 0;
	FGridQuickItemResolver::ApplyDirectDamageSkillScaling(Action, { RPG037::AlchemyRank(3) }, Source);
	TestEqual(TEXT("6 + Alchemy Rank receives +3 source damage bonus"), Source.DamageBonus, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG037PositivePotionScalingTest, "Grimrock.RPG.RPG03.7.PositivePotionScaling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG037PositivePotionScalingTest::RunTest(const FString&)
{
	FGridCombatModifierProfile EnhancedPotion;
	EnhancedPotion.SourcePolicies = { EGridCombatActionSourcePolicy::QuickItem };
	EnhancedPotion.RequiredSourceTags = { TEXT("QuickItem.Potion.Positive") };
	EnhancedPotion.PositiveEffectPercentModifier = 25;

	const FGridCombatActionDefinition Action = RPG037::MakePositivePotion();
	FGridResolvedCombatModifiers Modifiers;
	FGridCombatModifierResolver::Resolve({ EnhancedPotion }, FGridCombatModifierResolver::MakeActionContext(Action, TEXT("Item_Potion")), Modifiers);

	FGridCombatActionEffectProfile Effect;
	TestTrue(TEXT("Skill-scaled positive potion resolves"), FGridQuickItemResolver::ResolveEffectProfile(Action, { RPG037::AlchemyRank(3) }, Modifiers, Effect));
	TestEqual(TEXT("10 + 2x3 = 16 then Enhanced Potion +25% = 20"), Effect.RestoreHealth, 20);

	FGridCombatArmorEffectProfile Armor;
	Armor.Pool = EGridCombatArmorPool::Magical;
	Armor.Operation = EGridCombatArmorEffectOperation::Restore;
	Armor.Magnitude = EGridCombatArmorEffectMagnitude::Flat;
	Armor.Trigger = EGridCombatArmorEffectTrigger::AfterResolution;
	Armor.Amount = 4;
	FGridCombatArmorPoolSnapshot Snapshot;
	Snapshot.ReferenceMagicalArmor = 20;
	FGridCombatArmorEffectResult ArmorResult;
	TestTrue(TEXT("Positive potion armor effect resolves"),
		FGridCombatArmorEffectResolver::ResolveOne(Armor, Snapshot, Modifiers, nullptr, nullptr, ArmorResult));
	TestEqual(TEXT("4 armor with +25% becomes 5"), ArmorResult.RequestedAmount, 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG037GrenadierProjectionTest, "Grimrock.RPG.RPG03.7.GrenadierProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG037GrenadierProjectionTest::RunTest(const FString&)
{
	FGridCombatModifierProfile Profile;
	Profile.SourcePolicies = { EGridCombatActionSourcePolicy::QuickItem };
	Profile.RequiredSourceTags = { TEXT("QuickItem.Bomb") };
	Profile.ActionPointCostModifier = -1;
	Profile.RangeCellsModifier = 1;
	Profile.OutgoingDamagePercentModifier = 20;
	Profile.FriendlyDirectDamagePercentModifier = -50;

	FGridCombatActionDefinition Action = RPG037::MakeQuickAttack();
	FGridResolvedCombatModifiers Modifiers;
	FGridCombatModifierResolver::Resolve({ Profile }, FGridCombatModifierResolver::MakeActionContext(Action, TEXT("Item_Bomb")), Modifiers);
	FGridCombatModifierResolver::ApplyToActionDefinitionProjection(Action, Modifiers);

	TestEqual(TEXT("Master Grenadier lowers bomb AP by one"), Action.ActionPointCost, 1);
	TestEqual(TEXT("Precise Charge adds one range cell"), Action.RangeCells, 5);
	TestEqual(TEXT("Offensive profile follows projected range"), Action.OffensiveProfile.RangeCells, 5);
	TestEqual(TEXT("Friendly direct-damage hook is preserved for C8"), Modifiers.FriendlyDirectDamagePercentModifier, -50);

	FGridAttackSourceStats Source;
	Source.DamageMultiplier = 1.0f;
	FGridCombatModifierResolver::ApplyOutgoingAttackModifiers(Source, Modifiers);
	TestTrue(TEXT("Master Grenadier direct damage is +20%"), FMath::IsNearlyEqual(Source.DamageMultiplier, 1.2f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG037DiffusionProjectionTest, "Grimrock.RPG.RPG03.7.DiffusionProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG037DiffusionProjectionTest::RunTest(const FString&)
{
	FGridCombatModifierProfile Diffusion;
	Diffusion.SourcePolicies = { EGridCombatActionSourcePolicy::QuickItem };
	Diffusion.RequiredSourceTags = { TEXT("QuickItem.Potion.Positive") };
	Diffusion.QuickItemSecondaryTargetCount = 1;
	Diffusion.QuickItemSecondaryMagnitudePercent = 50;
	Diffusion.QuickItemSecondaryDurationPercent = 50;

	FGridResolvedCombatModifiers Modifiers;
	FGridCombatModifierResolver::Resolve(
		{ Diffusion }, FGridCombatModifierResolver::MakeActionContext(RPG037::MakePositivePotion(), TEXT("Item_Potion")), Modifiers);
	const FGridQuickItemSecondaryEffectProjection Projection = FGridQuickItemResolver::ResolveSecondaryEffect(Modifiers);
	TestTrue(TEXT("Diffusion secondary projection is enabled"), Projection.IsEnabled());
	TestEqual(TEXT("Diffusion adds exactly one secondary target"), Projection.TargetCount, 1);
	TestEqual(TEXT("Diffusion magnitude is fifty percent"), Projection.MagnitudePercent, 50);
	TestEqual(TEXT("Diffusion duration is fifty percent"), Projection.DurationPercent, 50);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG037CatalogSkillScaledEffectTest, "Grimrock.RPG.RPG03.7.CatalogSkillScaledEffect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG037CatalogSkillScaledEffectTest::RunTest(const FString&)
{
	FGridCombatActionDefinition Definition = RPG037::MakePositivePotion();
	Definition.EffectProfile.RestoreHealth = 0;
	Definition.QuickItemScaling.RestoreHealthSkillRankScale = 2;

	FGridCombatActionContribution Contribution;
	Contribution.Definition = Definition;
	Contribution.SourceDefinitionId = TEXT("Item_RPG037_SkillPotion");
	Contribution.AvailableSourceQuantity = 1;

	FGridCombatActionCatalogContext Context;
	Context.CharacterIndex = 0;
	Context.CharacterId = FGuid::NewGuid();
	Context.bCombatActive = true;
	Context.bActiveCombatant = true;
	Context.bEnableQuickItemExecutors = true;
	Context.RemainingActionPoints = 4;
	Context.CurrentHealth = 5;
	Context.MaximumHealth = 10;
	Context.SkillRanks = { RPG037::AlchemyRank(2) };

	TArray<FGridAvailableCombatAction> Actions;
	FGridCombatActionCatalog::Build(Context, { Contribution }, Actions);
	TestTrue(TEXT("Skill-only healing QuickItem remains catalogued"), Actions.Num() == 1);
	TestTrue(TEXT("Skill-only healing QuickItem is enabled below max health"), Actions.Num() == 1 && Actions[0].bEnabled);

	Context.CurrentHealth = 10;
	FGridCombatActionCatalog::Build(Context, { Contribution }, Actions);
	TestEqual(TEXT("Full health reports no applicable effect"),
		Actions.Num() == 1 ? Actions[0].AvailabilityReason : EGridCombatActionAvailabilityReason::None,
		EGridCombatActionAvailabilityReason::NoApplicableEffect);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRPG037ModifierValidationTest, "Grimrock.RPG.RPG03.7.ModifierValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRPG037ModifierValidationTest::RunTest(const FString&)
{
	FGridCombatModifierProfile Valid;
	Valid.RequiredSourceTags = { TEXT("QuickItem.Potion.Positive") };
	Valid.PositiveEffectPercentModifier = 25;
	TestTrue(TEXT("Tagged positive-effect modifier is valid"), Valid.IsValid());

	Valid.RequiredSourceTags.Add(TEXT("QuickItem.Potion.Positive"));
	TestFalse(TEXT("Duplicate required source tags are rejected"), Valid.IsValid());

	FGridCombatQuickItemScalingProfile Scaling;
	Scaling.ScalingSkillId = TEXT("Skill_Alchemy");
	Scaling.RestoreHealthSkillRankScale = 2;
	TestTrue(TEXT("Alchemy scaling profile is valid"), Scaling.IsValid());
	Scaling.ScalingSkillId = NAME_None;
	TestFalse(TEXT("Skill scaling requires a stable SkillId"), Scaling.IsValid());
	return true;
}

#endif
