#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridSkillsPageService.h"
#include "UI/GridTalentDetailLineWidget.h"
#include "UI/GridTalentDetailWidget.h"
#include "UI/GridTalentVariantBlockWidget.h"
#include "UI/RPGTalentDetailGlyphSet.h"
#include "UI/RPGTalentDetailTextSet.h"
#include "Engine/Texture2D.h"
#include "UObject/UnrealType.h"

namespace UIRPGDESC01Tests
{
	struct FRuntimeGuard
	{
		FRuntimeGuard()
		{
			FRPGClassProgressionTransactionService::ResetRuntimeState();
		}

		~FRuntimeGuard()
		{
			FRPGClassProgressionTransactionService::ResetRuntimeState();
		}
	};

	FGridCombatActionDefinition MakeAction(FName RequirementId)
	{
		FGridCombatActionDefinition Action;
		Action.ActionId = TEXT("Action_DESC01_Test");
		Action.DisplayName = FText::FromString(TEXT("Trait de test"));
		Action.Description = FText::FromString(TEXT("Inflige un effet autoritaire de test."));
		Action.ActionType = EGridCombatActionType::Ability;
		Action.SourcePolicy = EGridCombatActionSourcePolicy::Ability;
		Action.TargetingPolicy = EGridCombatTargetingPolicy::Self;
		Action.ResolutionProfile = EGridCombatActionResolutionProfile::Effect;
		Action.ActionPointCost = 2;
		Action.ResourceCosts.ManaCost = 3;
		Action.CooldownRounds = 2;
		Action.EffectProfile.RestoreHealth = 1;
		Action.Requirements = { RequirementId };
		return Action;
	}

	UGridPartyInventoryComponent* MakeSimpleInventory(URPGClassAsset*& OutClass)
	{
		UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent>();
		OutClass = NewObject<URPGClassAsset>(Component);
		OutClass->ClassId = TEXT("DESC01_Class");
		OutClass->DisplayName = FText::FromString(TEXT("Classe DESC01"));
		OutClass->BaseAttributes = FRPGAttributes{ 12, 12, 12, 10, 10, 10 };
		OutClass->HealthAtLevelOne = 20;

		FRPGClassProgressionLevelGrant Grant;
		Grant.Level = 2;
		Grant.ChoicePointsGranted = 1;
		OutClass->ProgressionLevelGrants.Add(Grant);

		FRPGClassProgressionChoiceDefinition Choice;
		Choice.ChoiceId = TEXT("Talent_DESC01_Simple");
		Choice.TalentBranchId = TEXT("Branch_DESC01");
		Choice.TalentNodeId = Choice.ChoiceId;
		Choice.DisplayName = FText::FromString(TEXT("Talent simple"));
		Choice.Description = FText::FromString(TEXT("Débloque Trait de test."));
		Choice.PresentationType = ERPGTalentPresentationType::Active;
		Choice.MinimumLevel = 2;
		Choice.PointCost = 1;
		FGridCombatModifierProfile Modifier;
		Modifier.AccuracyModifier = 2;
		Modifier.CriticalChancePercentModifier = 10;
		Modifier.CriticalDamagePercentModifier = 25;
		Choice.CombatModifiers.Add(Modifier);
		OutClass->ProgressionChoices.Add(Choice);
		OutClass->CombatActions.Add(MakeAction(Choice.ChoiceId));

		FGridCharacterInventoryState Character;
		Character.CharacterId = FGuid::NewGuid();
		Character.DisplayName = FText::FromString(TEXT("Elias"));
		Character.ClassId = OutClass->ClassId;
		Character.ClassDisplayName = OutClass->DisplayName;
		Character.ClassDefinition = OutClass;
		Character.Level = 2;
		Character.Experience = URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(2);
		Character.Attributes = OutClass->BaseAttributes;
		Character.DerivedStats = URPGCharacterRulesLibrary::CalculateDerivedStats(Character.Attributes, OutClass, 2);
		Character.Resources = URPGCharacterRulesLibrary::InitializeCharacterResources(Character.DerivedStats, OutClass);

		Component->PartyInventoryState.ActiveCharacters.Add(Character);
		Component->PartyInventoryState.ActiveEquipment.SetNum(1);
		Component->PartyInventoryState.SelectedCharacterIndex = 0;
		return Component;
	}

	FRPGTalentBranchPresentationDefinition MakeBranch(FName BranchId)
	{
		FRPGTalentBranchPresentationDefinition Branch;
		Branch.TalentBranchId = BranchId;
		return Branch;
	}

	FGridTalentVariantView MakeVariant(
		FName ChoiceId,
		const TCHAR* Name,
		const TCHAR* Principle,
		EGridTalentNodeState State = EGridTalentNodeState::Available)
	{
		FGridTalentVariantView Variant;
		Variant.ChoiceId = ChoiceId;
		Variant.DisplayName = FText::FromString(Name);
		Variant.State = State;
		Variant.Type = ERPGTalentPresentationType::Passive;
		Variant.Principle = FText::FromString(Principle);
		Variant.bAcquired = State == EGridTalentNodeState::Acquired;
		Variant.bCanChoose = State == EGridTalentNodeState::Available;
		return Variant;
	}

	FGridTalentNodeView MakeCanonicalVariantNode(EGridTalentNodeState State = EGridTalentNodeState::Available)
	{
		FGridTalentNodeView Node;
		Node.TalentNodeId = TEXT("Talent_DESC01_Affinity");
		Node.TalentBranchId = TEXT("Branch_DESC01");
		Node.Tier = 1;
		Node.MinimumLevel = 2;
		Node.PointCost = 1;
		Node.State = State;
		Node.DisplayName = FText::FromString(TEXT("Talent conceptuel"));
		Node.Type = ERPGTalentPresentationType::Passive;
		Node.Principle = FText::FromString(TEXT("Choisissez une variante."));
		Node.Acquisition.MinimumLevel = 2;
		Node.Acquisition.PointCost = 1;
		Node.bHasExclusiveVariants = true;
		Node.Variants = {
			MakeVariant(TEXT("Choice_Fire"), TEXT("Talent conceptuel — Feu"), TEXT("Bonus de feu."), State),
			MakeVariant(TEXT("Choice_Frost"), TEXT("Talent conceptuel — Glace"), TEXT("Bonus de glace."), State)
		};
		return Node;
	}

	FString JoinDetailLines(const TArray<FGridTalentDetailLineView>& Lines)
	{
		TArray<FString> Values;
		for (const FGridTalentDetailLineView& Line : Lines)
		{
			const FString Value = Line.bHasNumericValue
				? FString::FromInt(Line.NumericValue)
				: (!Line.ValueTextId.IsNone()
					? Line.ValueTextId.ToString()
					: Line.Value.ToString());
			Values.Add(Line.LabelTextId.ToString() + TEXT(" : ") + Value);
		}
		return FString::Join(Values, TEXT("\n"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC01ActionProjectionTest,
	"Grimrock.UI.RPG.DESC01.ReadModel.ActionProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC01ActionProjectionTest::RunTest(const FString&)
{
	using namespace UIRPGDESC01Tests;
	FRuntimeGuard Guard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeSimpleInventory(ClassDefinition);
	TestTrue(TEXT("DESC01 class is structurally valid"), ClassDefinition->IsValidDefinition());

	FGridSkillsPageView View;
	TestTrue(TEXT("Skills page builds canonical Talent view"),
		FGridSkillsPageService::TryBuildCharacterView(Component, 0, {}, View));
	TestEqual(TEXT("One branch is projected"), View.TalentTree.Branches.Num(), 1);
	if (View.TalentTree.Branches.Num() != 1 || View.TalentTree.Branches[0].Nodes.Num() != 1)
	{
		return false;
	}

	const FGridTalentNodeView& Node = View.TalentTree.Branches[0].Nodes[0];
	TestFalse(TEXT("Simple Talent is not a variant node"), Node.bHasExclusiveVariants);
	TestTrue(TEXT("Simple Talent exposes no fake one-entry Variants array"), Node.Variants.IsEmpty());
	TestEqual(TEXT("Simple ChoiceId is canonical"), Node.SimpleChoiceId, FName(TEXT("Talent_DESC01_Simple")));
	TestTrue(TEXT("Simple Talent can be acquired"), Node.bCanAcquireSimple);
	TestEqual(TEXT("Talent TYPE remains ACTIF"), Node.Type, ERPGTalentPresentationType::Active);

	const FString Effects = JoinDetailLines(Node.Effects);
	const auto UsageNumericValue = [&Node](EGridTalentDetailLineSemantic Semantic)
	{
		const FGridTalentDetailLineView* Found = Node.Usage.FindByPredicate(
			[Semantic](const FGridTalentDetailLineView& Line)
			{
				return Line.Semantic == Semantic;
			});
		return Found && Found->bHasNumericValue ? Found->NumericValue : INDEX_NONE;
	};
	TestFalse(TEXT("Generic EFFETS rows do not repeat the section name"),
		Node.Effects.ContainsByPredicate(
			[](const FGridTalentDetailLineView& Line)
			{
				return Line.LabelTextId == TEXT("Talent.Detail.Effect");
			}));
	const auto HasUsageSemantic = [&Node](EGridTalentDetailLineSemantic Semantic)
	{
		return Node.Usage.ContainsByPredicate(
			[Semantic](const FGridTalentDetailLineView& Line)
			{
				return Line.Semantic == Semantic;
			});
	};
	TestTrue(TEXT("Passive mechanics expose raw signed numeric semantics"),
		Node.Effects.ContainsByPredicate(
			[](const FGridTalentDetailLineView& Line)
			{
				return Line.LabelTextId == TEXT("Talent.Effect.Accuracy") &&
					Line.bHasNumericValue &&
					Line.NumericValue == 2 &&
					Line.ValueStyle == EGridTalentDetailValueStyle::SignedInteger &&
					Line.Value.IsEmpty();
			}));
	TestTrue(TEXT("Critical chance uses an explicit localized label and signed percent value"),
		Node.Effects.ContainsByPredicate(
			[](const FGridTalentDetailLineView& Line)
			{
				return Line.LabelTextId == TEXT("Talent.Effect.CriticalChance") &&
					Line.bHasNumericValue &&
					Line.NumericValue == 10 &&
					Line.ValueStyle == EGridTalentDetailValueStyle::SignedPercent;
			}));
	TestTrue(TEXT("Critical damage uses an explicit localized label and signed percent value"),
		Node.Effects.ContainsByPredicate(
			[](const FGridTalentDetailLineView& Line)
			{
				return Line.LabelTextId == TEXT("Talent.Effect.CriticalDamage") &&
					Line.bHasNumericValue &&
					Line.NumericValue == 25 &&
					Line.ValueStyle == EGridTalentDetailValueStyle::SignedPercent;
			}));
	TestEqual(TEXT("AP cost is projected as raw numeric content"), UsageNumericValue(EGridTalentDetailLineSemantic::ActionPointCost), 2);
	TestEqual(TEXT("Mana cost is projected into UTILISATION"), UsageNumericValue(EGridTalentDetailLineSemantic::ManaCost), 3);
	TestEqual(TEXT("Cooldown is projected as raw numeric content"), UsageNumericValue(EGridTalentDetailLineSemantic::Cooldown), 2);
	TestTrue(TEXT("AP cost carries stable semantic identity"), HasUsageSemantic(EGridTalentDetailLineSemantic::ActionPointCost));
	TestTrue(TEXT("Target carries stable semantic identity"), HasUsageSemantic(EGridTalentDetailLineSemantic::Target));
	TestTrue(TEXT("Mana carries stable semantic identity"), HasUsageSemantic(EGridTalentDetailLineSemantic::ManaCost));
	TestTrue(TEXT("Cooldown carries stable semantic identity"), HasUsageSemantic(EGridTalentDetailLineSemantic::Cooldown));
	const FGridTalentDetailLineView* TargetLine = Node.Usage.FindByPredicate(
		[](const FGridTalentDetailLineView& Line)
		{
			return Line.Semantic == EGridTalentDetailLineSemantic::Target;
		});
	TestNotNull(TEXT("Target row exists"), TargetLine);
	if (TargetLine)
	{
		TestEqual(TEXT("Target wording is resolved through text data"),
			TargetLine->ValueTextId, FName(TEXT("Talent.Target.Self")));
		TestTrue(TEXT("Target row carries no hard-coded player text"), TargetLine->Value.IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC01CanonicalSimpleDetailTest,
	"Grimrock.UI.RPG.DESC01.Detail.CanonicalSimple",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC01CanonicalSimpleDetailTest::RunTest(const FString&)
{
	FGridTalentNodeView Node;
	Node.TalentNodeId = TEXT("Talent_DESC01_Simple");
	Node.TalentBranchId = TEXT("Branch_DESC01");
	Node.Tier = 1;
	Node.MinimumLevel = 2;
	Node.PointCost = 1;
	Node.State = EGridTalentNodeState::Available;
	Node.DisplayName = FText::FromString(TEXT("Talent simple"));
	Node.Type = ERPGTalentPresentationType::Active;
	Node.Principle = FText::FromString(TEXT("Principe canonique."));
	Node.SimpleChoiceId = TEXT("Talent_DESC01_Simple");
	Node.bCanAcquireSimple = true;
	Node.Acquisition.MinimumLevel = 2;
	Node.Acquisition.PointCost = 1;

	FGridTalentDetailLineView Effect;
	Effect.LabelTextId = TEXT("Test.Effect.Accuracy");
	Effect.Value = FText::FromString(TEXT("+2"));
	Node.Effects.Add(Effect);
	FGridTalentDetailLineView Usage;
	Usage.LabelTextId = TEXT("Talent.Detail.Cooldown");
	Usage.Value = FText::FromString(TEXT("2 rounds"));
	Node.Usage.Add(Usage);

	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
	TestTrue(TEXT("Canonical simple detail initializes"),
		Detail->InitializeTalentDetail(Node, UIRPGDESC01Tests::MakeBranch(Node.TalentBranchId)));
	TestEqual(TEXT("Name is canonical"), Detail->ResolvedDisplayName.ToString(), FString(TEXT("Talent simple")));
	TestEqual(TEXT("TYPE semantic survives detail initialization"), Detail->NodeView.Type, ERPGTalentPresentationType::Active);
	TestEqual(TEXT("STATUS semantic survives detail initialization"), Detail->NodeView.State, EGridTalentNodeState::Available);
	TestEqual(TEXT("PRINCIPE is independent"), Detail->ResolvedPrincipleText.ToString(), FString(TEXT("Principe canonique.")));
	TestTrue(TEXT("EFFETS are canonical"), Detail->NodeView.Effects.ContainsByPredicate(
		[](const FGridTalentDetailLineView& Line)
		{
			return Line.LabelTextId == TEXT("Test.Effect.Accuracy") && Line.Value.ToString() == TEXT("+2");
		}));
	TestTrue(TEXT("UTILISATION is canonical"), Detail->NodeView.Usage.ContainsByPredicate(
		[](const FGridTalentDetailLineView& Line)
		{
			return Line.LabelTextId == TEXT("Talent.Detail.Cooldown") && Line.Value.ToString() == TEXT("2 rounds");
		}));
	TestTrue(TEXT("Simple acquisition uses SimpleChoiceId authority"), Detail->CanRequestSimpleAcquisition());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC01CanonicalVariantDetailTest,
	"Grimrock.UI.RPG.DESC01.Detail.CanonicalVariants",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC01CanonicalVariantDetailTest::RunTest(const FString&)
{
	using namespace UIRPGDESC01Tests;
	FGridTalentNodeView Node = MakeCanonicalVariantNode();

	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
	TestTrue(TEXT("Canonical variant detail initializes"),
		Detail->InitializeTalentDetail(Node, MakeBranch(Node.TalentBranchId)));
	TestTrue(TEXT("Node is recognized as true variant node"), Detail->CanRequestVariantAcquisition());
	TestFalse(TEXT("True variant node is never a simple acquisition"), Detail->CanRequestSimpleAcquisition());
	TestTrue(TEXT("Variant selection begins"), Detail->BeginVariantSelection());
	TestTrue(TEXT("Fire can become pending"), Detail->SelectVariantChoice(TEXT("Choice_Fire")));
	TestEqual(TEXT("Pending ChoiceId is explicit"), Detail->GetSelectedVariantChoiceId(), FName(TEXT("Choice_Fire")));
	TestTrue(TEXT("Reselecting the same Talent reinitializes detail"),
		Detail->InitializeTalentDetail(Node, MakeBranch(Node.TalentBranchId)));
	TestTrue(TEXT("Reselecting clears pending ChoiceId"), Detail->GetSelectedVariantChoiceId().IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC01AcquiredVariantContractTest,
	"Grimrock.UI.RPG.DESC01.Detail.AcquiredVariantContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC01AcquiredVariantContractTest::RunTest(const FString&)
{
	using namespace UIRPGDESC01Tests;
	FGridTalentNodeView Node = MakeCanonicalVariantNode(EGridTalentNodeState::LockedExclusive);
	Node.State = EGridTalentNodeState::Acquired;
	Node.SelectedChoiceId = TEXT("Choice_Frost");
	Node.Variants[0].State = EGridTalentNodeState::LockedExclusive;
	Node.Variants[0].bCanChoose = false;
	Node.Variants[1].State = EGridTalentNodeState::Acquired;
	Node.Variants[1].bAcquired = true;
	Node.Variants[1].bCanChoose = false;

	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
	TestTrue(TEXT("Acquired variant detail initializes"),
		Detail->InitializeTalentDetail(Node, MakeBranch(Node.TalentBranchId)));
	TestFalse(TEXT("Acquired variant node cannot restart acquisition"), Detail->CanRequestVariantAcquisition());
	TestFalse(TEXT("Acquired variant node cannot enter pending flow"), Detail->BeginVariantSelection());

	UGridTalentVariantBlockWidget* AcquiredBlock = NewObject<UGridTalentVariantBlockWidget>();
	AcquiredBlock->ConfigureDetailRows(nullptr, nullptr);
	TestTrue(TEXT("Acquired block initializes"),
		AcquiredBlock->InitializeVariant(Node.Variants[1], FText::FromString(TEXT("Glace")), false));
	TestEqual(TEXT("Acquired block says ACQUISE"), AcquiredBlock->ResolvedChooseLabel.ToString(), FString(TEXT("ACQUISE")));

	UGridTalentVariantBlockWidget* SiblingBlock = NewObject<UGridTalentVariantBlockWidget>();
	SiblingBlock->ConfigureDetailRows(nullptr, nullptr);
	TestTrue(TEXT("Sibling block initializes"),
		SiblingBlock->InitializeVariant(Node.Variants[0], FText::FromString(TEXT("Feu")), false));
	TestEqual(TEXT("Sibling block says INDISPONIBLE"), SiblingBlock->ResolvedChooseLabel.ToString(), FString(TEXT("INDISPONIBLE")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC01LegacyBindingsRemovedTest,
	"Grimrock.UI.RPG.DESC01.Detail.LegacyBindingsRemoved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC01LegacyBindingsRemovedTest::RunTest(const FString&)
{
	UClass* DetailClass = UGridTalentDetailWidget::StaticClass();

	for (const TCHAR* PropertyName : {
		TEXT("Text_DetailDescription"),
		TEXT("Text_DetailLevel"),
		TEXT("Text_DetailCost"),
		TEXT("Text_DetailState"),
		TEXT("Text_DetailVariants"),
		TEXT("Text_DetailVariantName"),
		TEXT("Text_DetailVariantDescription"),
		TEXT("Text_DetailActionSummary"),
		TEXT("Button_ChooseVariant"),
		TEXT("Combo_VariantChoice"),
		TEXT("Button_CancelAcquire"),
		TEXT("Text_CancelAcquire"),
		TEXT("Text_DetailEffects"),
		TEXT("Text_DetailUsage"),
		TEXT("Text_DetailAcquisition"),
		TEXT("ResolvedEffectsText"),
		TEXT("ResolvedUsageText"),
		TEXT("ResolvedAcquisitionText")
	})
	{
		TestTrue(
			*FString::Printf(TEXT("%s legacy binding is removed"), PropertyName),
			FindFProperty<FProperty>(DetailClass, FName(PropertyName)) == nullptr);
	}

	for (const TCHAR* PropertyName : {
		TEXT("Text_DetailType"),
		TEXT("Text_DetailStatus"),
		TEXT("Text_DetailPrinciple"),
		TEXT("VB_DetailEffectsEntries"),
		TEXT("VB_DetailUsageEntries"),
		TEXT("VB_DetailAcquisitionEntries"),
		TEXT("VB_DetailVariants"),
		TEXT("VB_VariantEntries")
	})
	{
		TestNotNull(
			*FString::Printf(TEXT("%s canonical binding remains"), PropertyName),
			FindFProperty<FProperty>(DetailClass, FName(PropertyName)));
	}

	UClass* VariantClass = UGridTalentVariantBlockWidget::StaticClass();
	for (const TCHAR* PropertyName : {
		TEXT("Text_VariantType"),
		TEXT("Text_VariantStatus"),
		TEXT("ResolvedTypeText"),
		TEXT("ResolvedStatusText"),
		TEXT("Text_VariantEffects"),
		TEXT("Text_VariantUsage"),
		TEXT("ResolvedEffectsText"),
		TEXT("ResolvedUsageText")
	})
	{
		TestTrue(
			*FString::Printf(TEXT("%s variant legacy property is removed"), PropertyName),
			FindFProperty<FProperty>(VariantClass, FName(PropertyName)) == nullptr);
	}
	for (const TCHAR* PropertyName : {
		TEXT("VB_VariantEffectsEntries"),
		TEXT("VB_VariantUsageEntries")
	})
	{
		TestNotNull(
			*FString::Printf(TEXT("%s structured variant binding remains"), PropertyName),
			FindFProperty<FProperty>(VariantClass, FName(PropertyName)));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC01LockedVariantFlowTest,
	"Grimrock.UI.RPG.DESC01.Detail.LockedVariantFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC01LockedVariantFlowTest::RunTest(const FString&)
{
	using namespace UIRPGDESC01Tests;
	FGridTalentNodeView Node = MakeCanonicalVariantNode(EGridTalentNodeState::LockedPoints);

	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
	TestTrue(TEXT("Locked node initializes"),
		Detail->InitializeTalentDetail(Node, MakeBranch(Node.TalentBranchId)));
	TestFalse(TEXT("Locked node cannot begin purchase"), Detail->BeginVariantSelection());
	TestFalse(TEXT("Locked node cannot choose a variant"), Detail->SelectVariantChoice(TEXT("Choice_Fire")));
	TestFalse(TEXT("Locked node cannot confirm purchase"), Detail->ConfirmAcquire());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC01ReactionMechanicsTest,
	"Grimrock.UI.RPG.DESC01.ReadModel.ReactionMechanics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC01ReactionMechanicsTest::RunTest(const FString&)
{
	using namespace UIRPGDESC01Tests;
	FRuntimeGuard Guard;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeSimpleInventory(ClassDefinition);
	FRPGClassProgressionChoiceDefinition& Choice = ClassDefinition->ProgressionChoices[0];
	Choice.CombatModifiers.Reset();
	Choice.PresentationType = ERPGTalentPresentationType::AutomaticReaction;
	ClassDefinition->CombatActions.Reset();
	FGridCombatReactionProfile Reaction;
	Reaction.ReactionId = TEXT("Reaction_DESC01");
	Reaction.Trigger = EGridCombatReactionTrigger::IncomingAttackHit;
	Reaction.Limit = EGridCombatReactionLimit::OncePerRound;
	Reaction.InterceptFinalDamagePercent = 50;
	Choice.CombatReactions.Add(Reaction);
	TestTrue(TEXT("Reaction class stays valid"), ClassDefinition->IsValidDefinition());

	FGridSkillsPageView View;
	TestTrue(TEXT("Reaction view builds"), FGridSkillsPageService::TryBuildCharacterView(Component, 0, {}, View));
	const FGridTalentNodeView& Node = View.TalentTree.Branches[0].Nodes[0];
	TestEqual(TEXT("Reaction Talent TYPE is explicit"), Node.Type, ERPGTalentPresentationType::AutomaticReaction);
	TestTrue(TEXT("Automatic reaction exposes no voluntary UTILISATION"), Node.Usage.IsEmpty());
	TestTrue(TEXT("Reaction trigger wording is data-driven"),
		Node.Effects.ContainsByPredicate(
			[](const FGridTalentDetailLineView& Line)
			{
				return Line.LabelTextId == TEXT("Talent.Reaction.Trigger") &&
					Line.ValueTextId == TEXT("Talent.Reaction.Trigger.IncomingAttackHit") &&
					Line.Value.IsEmpty();
			}));
	TestTrue(TEXT("Reaction limit wording is data-driven"),
		Node.Effects.ContainsByPredicate(
			[](const FGridTalentDetailLineView& Line)
			{
				return Line.LabelTextId == TEXT("Talent.Reaction.Limit") &&
					Line.ValueTextId == TEXT("Talent.Reaction.Limit.OncePerRound") &&
					Line.Value.IsEmpty();
			}));
	TestTrue(TEXT("Reaction response keeps raw percent semantics"),
		Node.Effects.ContainsByPredicate(
			[](const FGridTalentDetailLineView& Line)
			{
				return Line.LabelTextId == TEXT("Talent.Reaction.InterceptFinalDamagePercent") &&
					Line.bHasNumericValue &&
					Line.NumericValue == 50 &&
					Line.ValueStyle == EGridTalentDetailValueStyle::Percent &&
					Line.Value.IsEmpty();
			}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC01SkillPartyMechanicsTest,
	"Grimrock.UI.RPG.DESC01.ReadModel.SkillPartyMechanics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC01SkillPartyMechanicsTest::RunTest(const FString&)
{
	using namespace UIRPGDESC01Tests;
	FRuntimeGuard Guard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeSimpleInventory(ClassDefinition);
	FRPGClassProgressionChoiceDefinition& Choice = ClassDefinition->ProgressionChoices[0];
	Choice.PresentationType = ERPGTalentPresentationType::Passive;
	Choice.CombatModifiers.Reset();
	ClassDefinition->CombatActions.Reset();

	FRPGSkillProgressionModifier Skill;
	Skill.SkillId = TEXT("Skill_Perception");
	Skill.CheckModifier = 2;
	Skill.RequirementGrantRankModifier = 1;
	Skill.SafeFailureMargin = 2;
	Skill.bRequireRangedContext = true;
	Choice.SkillModifiers.Add(Skill);

	FRPGPartyProgressionModifier Party;
	Party.StackingGroupId = TEXT("Party.DESC01");
	Party.GroupSkillIds = { TEXT("Skill_Perception") };
	Party.GroupSkillCheckModifier = 2;
	Party.MaximumMobilityActionPointsModifier = 1;
	Choice.PartyModifiers.Add(Party);
	Choice.FirstRoundInitiativeModifier = 3;

	TestTrue(TEXT("Skill/party class stays valid"), ClassDefinition->IsValidDefinition());

	FGridSkillsPageView View;
	TestTrue(TEXT("Skill/party view builds"),
		FGridSkillsPageService::TryBuildCharacterView(Component, 0, {}, View));
	const FGridTalentNodeView& Node = View.TalentTree.Branches[0].Nodes[0];

	auto HasRawValue = [&Node](FName LabelTextId, int32 ExpectedValue, EGridTalentDetailValueStyle ExpectedStyle)
	{
		return Node.Effects.ContainsByPredicate(
			[LabelTextId, ExpectedValue, ExpectedStyle](const FGridTalentDetailLineView& Line)
			{
				return Line.LabelTextId == LabelTextId &&
					Line.ValueTextId.IsNone() &&
					Line.Value.IsEmpty() &&
					Line.bHasNumericValue &&
					Line.NumericValue == ExpectedValue &&
					Line.ValueStyle == ExpectedStyle;
			});
	};

	TestTrue(TEXT("Skill check modifier is raw numeric content"),
		HasRawValue(TEXT("Talent.Skill.CheckModifier"), 2, EGridTalentDetailValueStyle::SignedInteger));
	TestTrue(TEXT("Effective prerequisite rank modifier is raw numeric content"),
		HasRawValue(TEXT("Talent.Skill.RequirementGrantRankModifier"), 1, EGridTalentDetailValueStyle::SignedInteger));
	TestTrue(TEXT("Safe failure margin is raw numeric content"),
		HasRawValue(TEXT("Talent.Skill.SafeFailureMargin"), 2, EGridTalentDetailValueStyle::Integer));
	TestTrue(TEXT("Ranged context wording is data-driven"),
		Node.Effects.ContainsByPredicate(
			[](const FGridTalentDetailLineView& Line)
			{
				return Line.LabelTextId == TEXT("Talent.Skill.Context") &&
					Line.ValueTextId == TEXT("Talent.Skill.Context.Ranged") &&
					Line.Value.IsEmpty();
			}));
	TestTrue(TEXT("Party group Skill modifier is raw numeric content"),
		HasRawValue(TEXT("Talent.Party.GroupSkillCheckModifier"), 2, EGridTalentDetailValueStyle::SignedInteger));
	TestTrue(TEXT("Party mobility modifier is raw numeric content"),
		HasRawValue(TEXT("Talent.Party.MaximumMobilityActionPointsModifier"), 1, EGridTalentDetailValueStyle::SignedInteger));
	TestTrue(TEXT("First-round initiative is raw numeric content"),
		HasRawValue(TEXT("Talent.Effect.FirstRoundInitiative"), 3, EGridTalentDetailValueStyle::SignedInteger));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC01PresentationTextAuthorityTest,
	"Grimrock.UI.RPG.DESC01.ReadModel.PresentationTextAuthority",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC01PresentationTextAuthorityTest::RunTest(const FString&)
{
	const URPGTalentDetailTextSet* TextSet = LoadObject<URPGTalentDetailTextSet>(
		nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_RPGTalentDetailTextSet.DA_RPGTalentDetailTextSet"));
	if (!TestNotNull(TEXT("Production Talent detail text set loads"), TextSet))
	{
		return false;
	}

	for (const FName TextId : {
		FName(TEXT("Talent.Type.Active")),
		FName(TEXT("Talent.Type.ActiveSpell")),
		FName(TEXT("Talent.Type.Passive")),
		FName(TEXT("Talent.Type.AutomaticReaction")),
		FName(TEXT("Talent.Type.RecipeQuickItem")),
		FName(TEXT("Talent.Type.RecipeActive")),
		FName(TEXT("Talent.Status.Acquired")),
		FName(TEXT("Talent.Status.Available")),
		FName(TEXT("Talent.Status.LockedLevel")),
		FName(TEXT("Talent.Status.LockedPrerequisite")),
		FName(TEXT("Talent.Status.LockedPoints")),
		FName(TEXT("Talent.Status.LockedExclusive")),
		FName(TEXT("Talent.Variant.Acquired")),
		FName(TEXT("Talent.Variant.Pending")),
		FName(TEXT("Talent.Variant.Choose")),
		FName(TEXT("Talent.Variant.Unavailable")),
		FName(TEXT("Talent.Section.Acquisition")),
		FName(TEXT("Talent.Action.Acquire")),
		FName(TEXT("Talent.Action.Confirm")),
		FName(TEXT("Talent.Section.Effects")),
		FName(TEXT("Talent.Section.Principle")),
		FName(TEXT("Talent.Section.Status")),
		FName(TEXT("Talent.Section.Type")),
		FName(TEXT("Talent.Section.Usage")),
		FName(TEXT("Talent.Section.Variants")),
		FName(TEXT("Talent.Effect.CriticalChance")),
		FName(TEXT("Talent.Effect.CriticalDamage")),
		FName(TEXT("Talent.Effect.PhysicalArmorReference")),
		FName(TEXT("Talent.Acquisition.Exclusivity.SingleVariant")),
		FName(TEXT("Talent.Acquisition.Prompt.ConfirmVariant")),
		FName(TEXT("Talent.Acquisition.Prompt.ChooseVariant")),
		FName(TEXT("Talent.Recipe.Recipe_Bomb_Fire")),
		FName(TEXT("Talent.Recipe.Recipe_Bomb_Toxic")),
		FName(TEXT("Talent.Recipe.Recipe_Antidote")),
		FName(TEXT("Talent.Recipe.Recipe_DefensiveElixir_Fire")),
		FName(TEXT("Talent.Recipe.Recipe_DefensiveElixir_Ice")),
		FName(TEXT("Talent.Recipe.Recipe_DefensiveElixir_Lightning")),
		FName(TEXT("Talent.Recipe.Recipe_DefensiveElixir_Poison")),
		FName(TEXT("Talent.Recipe.Recipe_Panacea")),
		FName(TEXT("Talent.Recipe.Recipe_Flask_Oil")),
		FName(TEXT("Talent.Recipe.Recipe_Flask_Acid")),
		FName(TEXT("Talent.Recipe.Recipe_Flask_CorrosiveCloud")),
		FName(TEXT("Talent.Recipe.Recipe_MajorTransmutation_Fire")),
		FName(TEXT("Talent.Recipe.Recipe_MajorTransmutation_Ice")),
		FName(TEXT("Talent.Recipe.Recipe_MajorTransmutation_Poison")),
		FName(TEXT("Talent.Recipe.Recipe_MajorTransmutation_Oil")),
		FName(TEXT("Talent.Node.LevelFormat")),
		FName(TEXT("Talent.Node.State.Acquired")),
		FName(TEXT("Talent.Node.State.Available")),
		FName(TEXT("Talent.Node.State.LockedLevel")),
		FName(TEXT("Talent.Node.State.LockedPrerequisite")),
		FName(TEXT("Talent.Node.State.LockedPoints")),
		FName(TEXT("Talent.Node.State.LockedExclusive")),
		FName(TEXT("Talent.Screen.NoCharacterSelected")),
		FName(TEXT("Talent.Screen.SelectedCharacterFallback")),
		FName(TEXT("Talent.Screen.ClassLevelFormat")),
		FName(TEXT("Talent.Screen.TalentPointsFormat")),
		FName(TEXT("Talent.Screen.SkillPointsUnavailable")),
		FName(TEXT("Talent.Screen.SkillPointsFormat")),
		FName(TEXT("Talent.Feedback.Reject.InvalidCharacter")),
		FName(TEXT("Talent.Feedback.Reject.InvalidClassDefinition")),
		FName(TEXT("Talent.Feedback.Reject.InvalidCurrentSelection")),
		FName(TEXT("Talent.Feedback.Reject.EmptyRequest")),
		FName(TEXT("Talent.Feedback.Reject.DuplicateRequest")),
		FName(TEXT("Talent.Feedback.Reject.UnknownChoice")),
		FName(TEXT("Talent.Feedback.Reject.AlreadySelected")),
		FName(TEXT("Talent.Feedback.Reject.LevelTooLow")),
		FName(TEXT("Talent.Feedback.Reject.MissingPrerequisite")),
		FName(TEXT("Talent.Feedback.Reject.InsufficientChoicePoints")),
		FName(TEXT("Talent.Feedback.Reject.MutuallyExclusiveChoice")),
		FName(TEXT("Talent.Feedback.Success.Title")),
		FName(TEXT("Talent.Feedback.Success.Generic")),
		FName(TEXT("Talent.Feedback.Success.Named")),
		FName(TEXT("Talent.Feedback.Rejected.Title")),
		FName(TEXT("Talent.Feedback.Rejected.Generic"))
	})
	{
		TestFalse(
			*FString::Printf(TEXT("%s resolves from editable text data"), *TextId.ToString()),
			TextSet->ResolveText(TextId).IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC014StructuredReadModelTest,
	"Grimrock.UI.RPG.DESC01.ReadModel.StructuredContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC014StructuredReadModelTest::RunTest(const FString&)
{
	using namespace UIRPGDESC01Tests;
	FRuntimeGuard Guard;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeSimpleInventory(ClassDefinition);

	FGridSkillsPageView View;
	TestTrue(TEXT("Structured read model builds"),
		FGridSkillsPageService::TryBuildCharacterView(Component, 0, {}, View));
	const FGridTalentNodeView& Node = View.TalentTree.Branches[0].Nodes[0];

	TestEqual(TEXT("Explicit Talent TYPE is projected"), Node.Type, ERPGTalentPresentationType::Active);
	TestEqual(TEXT("Available Talent state remains semantic"), Node.State, EGridTalentNodeState::Available);
	TestEqual(TEXT("PRINCIPE comes from authored Description"),
		Node.Principle.ToString(), ClassDefinition->ProgressionChoices[0].Description.ToString());
	TestEqual(TEXT("Simple ChoiceId is explicit"), Node.SimpleChoiceId, ClassDefinition->ProgressionChoices[0].ChoiceId);
	TestTrue(TEXT("Simple Talent can be acquired"), Node.bCanAcquireSimple);
	TestTrue(TEXT("Simple Talent has no Variants payload"), Node.Variants.IsEmpty());
	TestTrue(TEXT("Structured EFFETS are present"), !Node.Effects.IsEmpty());
	TestTrue(TEXT("Structured UTILISATION is present"), !Node.Usage.IsEmpty());

	FRPGClassProgressionChoiceDefinition& Choice = ClassDefinition->ProgressionChoices[0];
	Choice.GrantedRequirementIds.Add(TEXT("Recipe_Bomb_Fire"));
	FGridSkillsPageView UpdatedView;
	TestTrue(TEXT("Acquisition text identities reproject"),
		FGridSkillsPageService::TryBuildCharacterView(Component, 0, {}, UpdatedView));
	const FGridTalentNodeView& UpdatedNode = UpdatedView.TalentTree.Branches[0].Nodes[0];
	TestTrue(TEXT("Simple Talent has no exclusivity presentation"),
		UpdatedNode.Acquisition.ExclusivityTextId.IsNone());
	TestTrue(TEXT("Recipe acquisition uses a stable text id"),
		UpdatedNode.Acquisition.GrantedRecipeTextIds.Contains(
			TEXT("Talent.Recipe.Recipe_Bomb_Fire")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC014VariantTypeContractTest,
	"Grimrock.UI.RPG.DESC01.ReadModel.VariantTypeContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC014VariantTypeContractTest::RunTest(const FString&)
{
	using namespace UIRPGDESC01Tests;
	FRuntimeGuard Guard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeSimpleInventory(ClassDefinition);
	FRPGClassProgressionChoiceDefinition Base = ClassDefinition->ProgressionChoices[0];
	Base.PresentationType = ERPGTalentPresentationType::Passive;
	Base.ChoiceId = TEXT("Talent_DESC01_Affinity_Fire");
	Base.TalentNodeId = TEXT("Talent_DESC01_Affinity");
	Base.ExclusiveChoiceGroupId = TEXT("TalentGroup_DESC01_Affinity");
	Base.GrantedRequirementIds = { Base.TalentNodeId };
	Base.DisplayName = FText::FromString(TEXT("Affinité — Feu"));

	FRPGClassProgressionChoiceDefinition Other = Base;
	Other.ChoiceId = TEXT("Talent_DESC01_Affinity_Ice");
	Other.DisplayName = FText::FromString(TEXT("Affinité — Glace"));
	ClassDefinition->ProgressionChoices = { Base, Other };
	ClassDefinition->CombatActions.Reset();
	TestTrue(TEXT("Variant class remains valid"), ClassDefinition->IsValidDefinition());

	FGridSkillsPageView View;
	const bool bBuilt = FGridSkillsPageService::TryBuildCharacterView(Component, 0, {}, View);
	TestTrue(TEXT("Variant read model builds"), bBuilt);
	if (!bBuilt || View.TalentTree.Branches.Num() != 1 || View.TalentTree.Branches[0].Nodes.Num() != 1)
	{
		return false;
	}
	const FGridTalentNodeView& Node = View.TalentTree.Branches[0].Nodes[0];
	TestTrue(TEXT("Node is explicitly a true exclusive-variant node"), Node.bHasExclusiveVariants);
	TestEqual(TEXT("Both variants remain projected"), Node.Variants.Num(), 2);
	TestEqual(TEXT("Node TYPE stays PASSIF"), Node.Type, ERPGTalentPresentationType::Passive);
	TestEqual(TEXT("True variant node exposes the exclusivity text identity"),
		Node.Acquisition.ExclusivityTextId,
		FName(TEXT("Talent.Acquisition.Exclusivity.SingleVariant")));
	for (const FGridTalentVariantView& Variant : Node.Variants)
	{
		TestEqual(TEXT("Variant TYPE matches node TYPE"), Variant.Type, Node.Type);
		TestTrue(TEXT("Available variant can be chosen"), Variant.bCanChoose);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC01152AStaticSectionsTest,
	"Grimrock.UI.RPG.DESC01.Detail.StaticSections",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC01152AStaticSectionsTest::RunTest(const FString&)
{
	FGridTalentNodeView Node;
	Node.TalentNodeId = TEXT("StaticSections");
	Node.TalentBranchId = TEXT("StaticBranch");
	Node.DisplayName = FText::FromString(TEXT("Talent statique"));
	Node.Type = ERPGTalentPresentationType::ActiveSpell;
	Node.State = EGridTalentNodeState::Available;
	Node.Principle = FText::FromString(TEXT("Principe déjà résolu."));
	Node.Acquisition.MinimumLevel = 3;
	Node.Acquisition.PointCost = 2;
	Node.Acquisition.PrerequisiteTalentNames.Add(FText::FromString(TEXT("Précurseur")));
	Node.Acquisition.ExclusivityTextId = TEXT("Test.Acquisition.Exclusivity");

	FGridTalentDetailLineView Effect;
	Effect.LabelTextId = TEXT("Test.Effect.Damage");
	Effect.Value = FText::FromString(TEXT("10"));
	Node.Effects.Add(Effect);
	FGridTalentDetailLineView Usage;
	Usage.Semantic = EGridTalentDetailLineSemantic::ManaCost;
	Usage.LabelTextId = TEXT("Talent.Detail.ManaCost");
	Usage.Value = FText::FromString(TEXT("5"));
	Node.Usage.Add(Usage);

	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
	const bool bInitialized =
		Detail->InitializeTalentDetail(Node, UIRPGDESC01Tests::MakeBranch(Node.TalentBranchId));
	TestTrue(TEXT("Static section detail initializes"), bInitialized);
	if (!bInitialized)
	{
		return false;
	}
	TestEqual(TEXT("TYPE semantic is exposed independently"), Detail->NodeView.Type, ERPGTalentPresentationType::ActiveSpell);
	TestEqual(TEXT("STATUS semantic is exposed independently"), Detail->NodeView.State, EGridTalentNodeState::Available);
	TestEqual(TEXT("PRINCIPE is exposed independently"), Detail->ResolvedPrincipleText.ToString(), FString(TEXT("Principe déjà résolu.")));
	TestEqual(TEXT("One EFFETS row remains structured"), Detail->NodeView.Effects.Num(), 1);
	TestEqual(TEXT("One UTILISATION row remains structured"), Detail->NodeView.Usage.Num(), 1);
	if (Detail->NodeView.Effects.Num() != 1 || Detail->NodeView.Usage.Num() != 1)
	{
		return false;
	}
	TestEqual(TEXT("EFFETS remain structured"), Detail->NodeView.Effects[0].Value.ToString(), FString(TEXT("10")));
	TestEqual(TEXT("UTILISATION remains structured"), Detail->NodeView.Usage[0].Value.ToString(), FString(TEXT("5")));
	TestEqual(TEXT("ACQUISITION keeps canonical level"), Detail->NodeView.Acquisition.MinimumLevel, 3);
	TestEqual(TEXT("ACQUISITION keeps canonical cost"), Detail->NodeView.Acquisition.PointCost, 2);
	TestTrue(TEXT("ACQUISITION keeps resolved prerequisite name"),
		Detail->NodeView.Acquisition.PrerequisiteTalentNames.ContainsByPredicate(
			[](const FText& Value) { return Value.ToString() == TEXT("Précurseur"); }));
	TestEqual(TEXT("ACQUISITION keeps exclusivity text identity"),
		Detail->NodeView.Acquisition.ExclusivityTextId, FName(TEXT("Test.Acquisition.Exclusivity")));

	FGridTalentDetailLineView TargetLine;
	TargetLine.Semantic = EGridTalentDetailLineSemantic::Target;
	TargetLine.LabelTextId = TEXT("Talent.Detail.Target");
	TargetLine.Value = FText::FromString(TEXT("première cible dans l'axe"));
	URPGTalentDetailTextSet* TextSet = NewObject<URPGTalentDetailTextSet>();
	FRPGTalentDetailTextEntry TargetText;
	TargetText.TextId = TEXT("Talent.Detail.Target");
	TargetText.Text = FText::FromString(TEXT("Cible"));
	TextSet->Entries.Add(TargetText);
	FRPGTalentDetailTextEntry EquipmentText;
	EquipmentText.TextId = TEXT("Talent.Context.Source.Equipment");
	EquipmentText.Text = FText::FromString(TEXT("Actions d'équipement"));
	TextSet->Entries.Add(EquipmentText);
	TestTrue(TEXT("Detail text set validates"), TextSet->IsValidDefinition());
	TestEqual(TEXT("Detail text set resolves editable label"), TextSet->ResolveText(TargetLine.LabelTextId).ToString(), FString(TEXT("Cible")));

	UGridTalentDetailLineWidget* LineWidget = NewObject<UGridTalentDetailLineWidget>();
	TestTrue(TEXT("Unglyphed detail row initializes without Designer bindings"),
		LineWidget->InitializeLine(TargetLine, nullptr, TextSet->ResolveText(TargetLine.LabelTextId), TargetLine.Value));
	TestEqual(TEXT("Unglyphed row receives asset-resolved label content"), LineWidget->ResolvedLabelText.ToString(), FString(TEXT("Cible")));
	TestEqual(TEXT("Unglyphed row exposes raw value content"),
		LineWidget->ResolvedValueText.ToString(), FString(TEXT("première cible dans l'axe")));

	UTexture2D* InlineTargetGlyph = NewObject<UTexture2D>(LineWidget);
	TestTrue(TEXT("Glyphed detail row reinitializes"),
		LineWidget->InitializeLine(TargetLine, InlineTargetGlyph, TextSet->ResolveText(TargetLine.LabelTextId), TargetLine.Value));
	TestEqual(TEXT("Glyphed row keeps asset-resolved label content"), LineWidget->ResolvedLabelText.ToString(), FString(TEXT("Cible")));
	TestEqual(TEXT("Glyphed row still exposes raw value content"), LineWidget->ResolvedValueText.ToString(), FString(TEXT("première cible dans l'axe")));

	FGridTalentDetailLineView ContextLine;
	ContextLine.Semantic = EGridTalentDetailLineSemantic::Context;
	ContextLine.ValueTextId = TEXT("Talent.Context.Source.Equipment");
	TestTrue(TEXT("Data-driven context row initializes"),
		LineWidget->InitializeLine(
			ContextLine,
			nullptr,
			FText::GetEmpty(),
			TextSet->ResolveText(ContextLine.ValueTextId)));
	TestTrue(TEXT("Context row has no synthetic label"), LineWidget->ResolvedLabelText.IsEmpty());
	TestEqual(TEXT("Context row wording comes from text data"),
		LineWidget->ResolvedValueText.ToString(), FString(TEXT("Actions d'équipement")));

	FGridTalentDetailLineView NumericLine;
	NumericLine.LabelTextId = TEXT("Test.Effect.Accuracy");
	NumericLine.NumericValue = 10;
	NumericLine.bHasNumericValue = true;
	NumericLine.ValueStyle = EGridTalentDetailValueStyle::SignedPercent;
	TestTrue(TEXT("Signed-percent row initializes from mechanic data"),
		LineWidget->InitializeLine(NumericLine, nullptr, FText::FromString(TEXT("Bonus")), FText::GetEmpty()));
	TestEqual(TEXT("Numeric row keeps raw numeric authority"), LineWidget->LineView.NumericValue, 10);
	TestEqual(TEXT("Numeric row keeps value style in the canonical line view"),
		LineWidget->LineView.ValueStyle, EGridTalentDetailValueStyle::SignedPercent);
	TestEqual(TEXT("Signed-percent row renders the complete universal token"),
		LineWidget->ResolvedValueText.ToString(), FString(TEXT("+10%")));

	NumericLine.NumericValue = -25;
	TestTrue(TEXT("Negative signed-percent row reinitializes"),
		LineWidget->InitializeLine(NumericLine, nullptr, FText::FromString(TEXT("Bonus")), FText::GetEmpty()));
	TestEqual(TEXT("Negative signed-percent token is complete"),
		LineWidget->ResolvedValueText.ToString(), FString(TEXT("-25%")));

	NumericLine.NumericValue = 2;
	NumericLine.ValueStyle = EGridTalentDetailValueStyle::SignedInteger;
	TestTrue(TEXT("Signed-integer row reinitializes"),
		LineWidget->InitializeLine(NumericLine, nullptr, FText::FromString(TEXT("Bonus")), FText::GetEmpty()));
	TestEqual(TEXT("Signed-integer token is complete"),
		LineWidget->ResolvedValueText.ToString(), FString(TEXT("+2")));

	NumericLine.NumericValue = 25;
	NumericLine.ValueStyle = EGridTalentDetailValueStyle::Percent;
	TestTrue(TEXT("Percent row reinitializes"),
		LineWidget->InitializeLine(NumericLine, nullptr, FText::FromString(TEXT("Bonus")), FText::GetEmpty()));
	TestEqual(TEXT("Percent token is complete"),
		LineWidget->ResolvedValueText.ToString(), FString(TEXT("25%")));

	NumericLine.NumericValue = 10;
	NumericLine.ValueStyle = EGridTalentDetailValueStyle::Integer;
	TestTrue(TEXT("Integer row reinitializes"),
		LineWidget->InitializeLine(NumericLine, nullptr, FText::FromString(TEXT("Bonus")), FText::GetEmpty()));
	TestEqual(TEXT("Integer token stays undecorated"),
		LineWidget->ResolvedValueText.ToString(), FString(TEXT("10")));

	URPGTalentDetailGlyphSet* GlyphSet = NewObject<URPGTalentDetailGlyphSet>();
	UTexture2D* CostGlyph = NewObject<UTexture2D>(GlyphSet);
	UTexture2D* ManaGlyph = NewObject<UTexture2D>(GlyphSet);
	UTexture2D* ItemGlyph = NewObject<UTexture2D>(GlyphSet);
	UTexture2D* ActionGlyph = NewObject<UTexture2D>(GlyphSet);
	UTexture2D* TargetGlyph = NewObject<UTexture2D>(GlyphSet);
	UTexture2D* AreaGlyph = NewObject<UTexture2D>(GlyphSet);
	UTexture2D* SightGlyph = NewObject<UTexture2D>(GlyphSet);
	UTexture2D* ResolutionGlyph = NewObject<UTexture2D>(GlyphSet);
	UTexture2D* ExclusivityGlyph = NewObject<UTexture2D>(GlyphSet);
	UTexture2D* RecipeGlyph = NewObject<UTexture2D>(GlyphSet);

	GlyphSet->Cost = CostGlyph;
	GlyphSet->Action = ActionGlyph;
	GlyphSet->Target = TargetGlyph;
	GlyphSet->Area = AreaGlyph;
	GlyphSet->LineOfSight = SightGlyph;
	GlyphSet->ResolutionCount = ResolutionGlyph;
	GlyphSet->Exclusivity = ExclusivityGlyph;
	GlyphSet->Recipe = RecipeGlyph;

	TestEqual(TEXT("Action cost resolves shared Cost glyph"),
		GlyphSet->ResolveGlyph(EGridTalentDetailLineSemantic::ActionPointCost), CostGlyph);
	TestEqual(TEXT("Talent point cost resolves shared Cost glyph"),
		GlyphSet->ResolveGlyph(EGridTalentDetailLineSemantic::TalentPointCost), CostGlyph);
	TestEqual(TEXT("Mana cost falls back to Cost glyph when unset"),
		GlyphSet->ResolveGlyph(EGridTalentDetailLineSemantic::ManaCost), CostGlyph);
	TestEqual(TEXT("Item cost falls back to Cost glyph when unset"),
		GlyphSet->ResolveGlyph(EGridTalentDetailLineSemantic::ItemCost), CostGlyph);

	GlyphSet->ManaCost = ManaGlyph;
	GlyphSet->ItemCost = ItemGlyph;
	TestEqual(TEXT("Mana cost resolves dedicated glyph when assigned"),
		GlyphSet->ResolveGlyph(EGridTalentDetailLineSemantic::ManaCost), ManaGlyph);
	TestEqual(TEXT("Item cost resolves dedicated glyph when assigned"),
		GlyphSet->ResolveGlyph(EGridTalentDetailLineSemantic::ItemCost), ItemGlyph);
	TestEqual(TEXT("Action resolves Action glyph"),
		GlyphSet->ResolveGlyph(EGridTalentDetailLineSemantic::Action), ActionGlyph);
	TestEqual(TEXT("Target resolves Target glyph"),
		GlyphSet->ResolveGlyph(EGridTalentDetailLineSemantic::Target), TargetGlyph);
	TestEqual(TEXT("Area resolves Area glyph"),
		GlyphSet->ResolveGlyph(EGridTalentDetailLineSemantic::Area), AreaGlyph);
	TestEqual(TEXT("Line of sight resolves LineOfSight glyph"),
		GlyphSet->ResolveGlyph(EGridTalentDetailLineSemantic::LineOfSight), SightGlyph);
	TestEqual(TEXT("Resolution count resolves ResolutionCount glyph"),
		GlyphSet->ResolveGlyph(EGridTalentDetailLineSemantic::ResolutionCount), ResolutionGlyph);
	TestEqual(TEXT("Exclusivity resolves Exclusivity glyph"),
		GlyphSet->ResolveGlyph(EGridTalentDetailLineSemantic::Exclusivity), ExclusivityGlyph);
	TestEqual(TEXT("Recipe resolves Recipe glyph"),
		GlyphSet->ResolveGlyph(EGridTalentDetailLineSemantic::Recipe), RecipeGlyph);
	TestNull(TEXT("None semantic renders safely without a glyph"),
		GlyphSet->ResolveGlyph(EGridTalentDetailLineSemantic::None));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC01152CStatusActionEffectsTest,
	"Grimrock.UI.RPG.DESC01.ReadModel.StatusActionEffects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC01152CStatusActionEffectsTest::RunTest(const FString&)
{
	using namespace UIRPGDESC01Tests;
	FRuntimeGuard Guard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeSimpleInventory(ClassDefinition);
	FRPGClassProgressionChoiceDefinition& Choice = ClassDefinition->ProgressionChoices[0];
	Choice.Description = FText::FromString(TEXT("Applique Status_Guarded pendant 2 rounds : protection défensive."));
	FGridCombatStatusApplicationProfile Status;
	Status.StatusEffectId = TEXT("Status_Guarded");
	Status.Trigger = EGridCombatStatusApplicationTrigger::AfterResolution;
	Status.DurationOverride = 2;
	ClassDefinition->CombatActions[0].StatusApplications.Add(Status);

	FGridSkillsPageView View;
	TestTrue(TEXT("Status-action read model builds"),
		FGridSkillsPageService::TryBuildCharacterView(Component, 0, {}, View));
	const FGridTalentNodeView& Node = View.TalentTree.Branches[0].Nodes[0];

	TestTrue(TEXT("PRINCIPE resolves canonical status DisplayName"), Node.Principle.ToString().Contains(TEXT("Garde")));
	TestFalse(TEXT("PRINCIPE never leaks raw status ids"), Node.Principle.ToString().Contains(TEXT("Status_Guarded")));

	for (const FGridTalentDetailLineView& Line : Node.Effects)
	{
		const FString Text =
			Line.LabelTextId.ToString() + TEXT(" ") +
			Line.ValueTextId.ToString() + TEXT(" ") +
			Line.Value.ToString();
		TestFalse(TEXT("EFFETS never leaks raw status ids"), Text.Contains(TEXT("Status_Guarded")));
	}
	TestTrue(TEXT("Status name is projected as authored content"),
		Node.Effects.ContainsByPredicate(
			[](const FGridTalentDetailLineView& Line)
			{
				return Line.LabelTextId == TEXT("Talent.Detail.Status") &&
					Line.Value.ToString().Contains(TEXT("Garde"));
			}));
	TestTrue(TEXT("Status duration is projected as raw numeric data"),
		Node.Effects.ContainsByPredicate(
			[](const FGridTalentDetailLineView& Line)
			{
				return Line.LabelTextId == TEXT("Talent.Detail.StatusDurationRounds") &&
					Line.bHasNumericValue &&
					Line.NumericValue == 2 &&
					Line.ValueStyle == EGridTalentDetailValueStyle::Integer &&
					Line.Value.IsEmpty();
			}));
	TestTrue(TEXT("Status description remains authored content"),
		Node.Effects.ContainsByPredicate(
			[](const FGridTalentDetailLineView& Line)
			{
				return Line.LabelTextId == TEXT("Talent.Detail.StatusDescription") &&
					!Line.Value.IsEmpty();
			}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC01153VariantBlockPresenterTest,
	"Grimrock.UI.RPG.DESC01.Detail.VariantBlockPresenter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC01153VariantBlockPresenterTest::RunTest(const FString&)
{
	FGridTalentVariantView Variant;
	Variant.ChoiceId = TEXT("Talent_DESC01_Variant_Slash");
	Variant.DisplayName = FText::FromString(TEXT("Spécialisation martiale — Tranchant"));
	Variant.Principle = FText::FromString(TEXT("Maîtrise les armes tranchantes."));
	Variant.State = EGridTalentNodeState::Available;
	Variant.bCanChoose = true;

	FGridTalentDetailLineView Effect;
	Effect.LabelTextId = TEXT("Test.Effect.Accuracy");
	Effect.Value = FText::FromString(TEXT("+1"));
	Variant.Effects.Add(Effect);

	UGridTalentVariantBlockWidget* Block = NewObject<UGridTalentVariantBlockWidget>();
	URPGTalentDetailGlyphSet* VariantGlyphSet = NewObject<URPGTalentDetailGlyphSet>(Block);
	Block->ConfigureDetailRows(UGridTalentDetailLineWidget::StaticClass(), VariantGlyphSet);
	TestTrue(TEXT("Variant block initializes"),
		Block->InitializeVariant(Variant, FText::FromString(TEXT("Tranchant")), false));
	TestEqual(TEXT("Short display name is preserved"), Block->ResolvedDisplayName.ToString(), FString(TEXT("Tranchant")));
	TestTrue(TEXT("Variant EFFETS are structured"), Block->VariantView.Effects.ContainsByPredicate(
		[](const FGridTalentDetailLineView& Line)
		{
			return Line.LabelTextId == TEXT("Test.Effect.Accuracy") && Line.Value.ToString() == TEXT("+1");
		}));
	TestEqual(TEXT("Available variant exposes CHOISIR"), Block->ResolvedChooseLabel.ToString(), FString(TEXT("CHOISIR")));
	TestTrue(TEXT("Available variant choice is enabled"), Block->bChooseEnabled);

	TestTrue(TEXT("Pending variant reinitializes"),
		Block->InitializeVariant(Variant, FText::FromString(TEXT("Tranchant")), true));
	TestEqual(TEXT("Pending variant exposes CHOIX EN COURS"), Block->ResolvedChooseLabel.ToString(), FString(TEXT("CHOIX EN COURS")));
	TestFalse(TEXT("Pending variant cannot be chosen twice"), Block->bChooseEnabled);

	Variant.bAcquired = true;
	Variant.bCanChoose = false;
	Variant.State = EGridTalentNodeState::Acquired;
	TestTrue(TEXT("Acquired variant reinitializes"),
		Block->InitializeVariant(Variant, FText::FromString(TEXT("Tranchant")), false));
	TestEqual(TEXT("Acquired variant exposes ACQUISE"), Block->ResolvedChooseLabel.ToString(), FString(TEXT("ACQUISE")));
	TestFalse(TEXT("Acquired variant choice is disabled"), Block->bChooseEnabled);

	Variant.bAcquired = false;
	Variant.State = EGridTalentNodeState::LockedExclusive;
	Variant.bCanChoose = false;
	TestTrue(TEXT("Unavailable variant reinitializes"),
		Block->InitializeVariant(Variant, FText::FromString(TEXT("Perforant")), false));
	TestEqual(TEXT("Unavailable variant exposes INDISPONIBLE"), Block->ResolvedChooseLabel.ToString(), FString(TEXT("INDISPONIBLE")));
	TestFalse(TEXT("Unavailable variant choice is disabled"), Block->bChooseEnabled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC01163EmptyInitialStateTest,
	"Grimrock.UI.RPG.DESC01.Detail.EmptyInitialState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC01163EmptyInitialStateTest::RunTest(const FString&)
{
	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();

	TestFalse(TEXT("Fresh detail has no Talent initialized"), Detail->bInitialized);
	TestTrue(TEXT("Fresh detail name is empty"), Detail->ResolvedDisplayName.IsEmpty());
	TestTrue(TEXT("Fresh TYPE is empty"), Detail->ResolvedTypeText.IsEmpty());
	TestTrue(TEXT("Fresh STATUS is empty"), Detail->ResolvedStatusText.IsEmpty());
	TestTrue(TEXT("Fresh PRINCIPE is empty"), Detail->ResolvedPrincipleText.IsEmpty());
	TestFalse(TEXT("Fresh detail cannot acquire a simple Talent"), Detail->CanRequestSimpleAcquisition());
	TestFalse(TEXT("Fresh detail cannot acquire a variant"), Detail->CanRequestVariantAcquisition());

	FGridTalentNodeView Node;
	Node.TalentNodeId = TEXT("Talent_DESC01_EmptyState");
	Node.TalentBranchId = TEXT("Branch_DESC01_EmptyState");
	Node.Tier = 1;
	Node.State = EGridTalentNodeState::Available;
	Node.DisplayName = FText::FromString(TEXT("Talent temporaire"));
	Node.Type = ERPGTalentPresentationType::Passive;
	Node.Principle = FText::FromString(TEXT("Principe temporaire."));
	Node.SimpleChoiceId = Node.TalentNodeId;
	Node.bCanAcquireSimple = true;
	Node.Acquisition.MinimumLevel = 2;
	Node.Acquisition.PointCost = 1;

	FRPGTalentBranchPresentationDefinition Branch;
	Branch.TalentBranchId = Node.TalentBranchId;
	TestTrue(TEXT("Temporary Talent initializes"),
		Detail->InitializeTalentDetail(Node, Branch));
	TestTrue(TEXT("Initialized detail carries a name"), !Detail->ResolvedDisplayName.IsEmpty());

	Detail->ClearTalentDetail();

	TestFalse(TEXT("Clear returns detail to no-Talent state"), Detail->bInitialized);
	TestTrue(TEXT("Clear removes name"), Detail->ResolvedDisplayName.IsEmpty());
	TestTrue(TEXT("Clear removes TYPE"), Detail->ResolvedTypeText.IsEmpty());
	TestTrue(TEXT("Clear removes STATUS"), Detail->ResolvedStatusText.IsEmpty());
	TestTrue(TEXT("Clear removes PRINCIPE"), Detail->ResolvedPrincipleText.IsEmpty());
	TestTrue(TEXT("Clear removes pending variant choice"), Detail->GetSelectedVariantChoiceId().IsNone());
	return true;
}

#endif
