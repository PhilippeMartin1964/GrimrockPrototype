#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridSkillsPageService.h"
#include "UI/GridTalentDetailWidget.h"
#include "UI/GridTalentVariantBlockWidget.h"
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
		Variant.TypeText = FText::FromString(TEXT("PASSIF"));
		Variant.StatusText = State == EGridTalentNodeState::Available
			? FText::FromString(TEXT("DISPONIBLE"))
			: FText::FromString(TEXT("VERROUILLÉ — nécessite 1 point de Talent"));
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
		Node.TypeText = FText::FromString(TEXT("PASSIF"));
		Node.StatusText = State == EGridTalentNodeState::Available
			? FText::FromString(TEXT("DISPONIBLE"))
			: FText::FromString(TEXT("VERROUILLÉ — nécessite 1 point de Talent"));
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
			Values.Add(Line.Label.ToString() + TEXT(" : ") + Line.Value.ToString());
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
	TestEqual(TEXT("Talent TYPE label is canonical"), Node.TypeText.ToString(), FString(TEXT("ACTIF")));

	const FString Effects = JoinDetailLines(Node.Effects);
	const FString Usage = JoinDetailLines(Node.Usage);
	TestTrue(TEXT("Passive mechanics are projected into EFFETS"), Effects.Contains(TEXT("Précision : +2")));
	TestTrue(TEXT("AP cost is projected into UTILISATION"), Usage.Contains(TEXT("Coût : 2 points d'action")));
	TestTrue(TEXT("Mana cost is projected into UTILISATION"), Usage.Contains(TEXT("Mana : 3")));
	TestTrue(TEXT("Cooldown is projected into UTILISATION"), Usage.Contains(TEXT("Recharge : 2 rounds")));
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
	Node.TypeText = FText::FromString(TEXT("ACTIF"));
	Node.StatusText = FText::FromString(TEXT("DISPONIBLE"));
	Node.Principle = FText::FromString(TEXT("Principe canonique."));
	Node.SimpleChoiceId = TEXT("Talent_DESC01_Simple");
	Node.bCanAcquireSimple = true;
	Node.Acquisition.MinimumLevel = 2;
	Node.Acquisition.PointCost = 1;

	FGridTalentDetailLineView Effect;
	Effect.Label = FText::FromString(TEXT("Précision"));
	Effect.Value = FText::FromString(TEXT("+2"));
	Node.Effects.Add(Effect);
	FGridTalentDetailLineView Usage;
	Usage.Label = FText::FromString(TEXT("Recharge"));
	Usage.Value = FText::FromString(TEXT("2 rounds"));
	Node.Usage.Add(Usage);

	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
	TestTrue(TEXT("Canonical simple detail initializes"),
		Detail->InitializeTalentDetail(Node, UIRPGDESC01Tests::MakeBranch(Node.TalentBranchId)));
	TestEqual(TEXT("Name is canonical"), Detail->ResolvedDisplayName.ToString(), FString(TEXT("Talent simple")));
	TestEqual(TEXT("TYPE is independent"), Detail->ResolvedTypeText.ToString(), FString(TEXT("ACTIF")));
	TestEqual(TEXT("STATUS is independent"), Detail->ResolvedStatusText.ToString(), FString(TEXT("DISPONIBLE")));
	TestEqual(TEXT("PRINCIPE is independent"), Detail->ResolvedPrincipleText.ToString(), FString(TEXT("Principe canonique.")));
	TestTrue(TEXT("EFFETS are canonical"), Detail->ResolvedEffectsText.ToString().Contains(TEXT("Précision : +2")));
	TestTrue(TEXT("UTILISATION is canonical"), Detail->ResolvedUsageText.ToString().Contains(TEXT("Recharge : 2 rounds")));
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
	Detail->CancelAcquireConfirmation();
	TestTrue(TEXT("Cancel clears pending ChoiceId"), Detail->GetSelectedVariantChoiceId().IsNone());
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
	Node.StatusText = FText::FromString(TEXT("ACQUIS"));
	Node.SelectedChoiceId = TEXT("Choice_Frost");
	Node.Variants[0].State = EGridTalentNodeState::LockedExclusive;
	Node.Variants[0].StatusText = FText::FromString(TEXT("INDISPONIBLE — autre variante déjà choisie"));
	Node.Variants[0].bCanChoose = false;
	Node.Variants[1].State = EGridTalentNodeState::Acquired;
	Node.Variants[1].StatusText = FText::FromString(TEXT("ACQUIS"));
	Node.Variants[1].bAcquired = true;
	Node.Variants[1].bCanChoose = false;

	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
	TestTrue(TEXT("Acquired variant detail initializes"),
		Detail->InitializeTalentDetail(Node, MakeBranch(Node.TalentBranchId)));
	TestFalse(TEXT("Acquired variant node cannot restart acquisition"), Detail->CanRequestVariantAcquisition());
	TestFalse(TEXT("Acquired variant node cannot enter pending flow"), Detail->BeginVariantSelection());

	UGridTalentVariantBlockWidget* AcquiredBlock = NewObject<UGridTalentVariantBlockWidget>();
	TestTrue(TEXT("Acquired block initializes"),
		AcquiredBlock->InitializeVariant(Node.Variants[1], FText::FromString(TEXT("Glace")), false));
	TestEqual(TEXT("Acquired block says ACQUISE"), AcquiredBlock->ResolvedChooseLabel.ToString(), FString(TEXT("ACQUISE")));

	UGridTalentVariantBlockWidget* SiblingBlock = NewObject<UGridTalentVariantBlockWidget>();
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
		TEXT("Combo_VariantChoice")
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
		TEXT("Text_DetailEffects"),
		TEXT("Text_DetailUsage"),
		TEXT("Text_DetailAcquisition"),
		TEXT("VB_DetailVariants"),
		TEXT("VB_VariantEntries")
	})
	{
		TestNotNull(
			*FString::Printf(TEXT("%s canonical binding remains"), PropertyName),
			FindFProperty<FProperty>(DetailClass, FName(PropertyName)));
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
	TestEqual(TEXT("Reaction TYPE label is canonical"), Node.TypeText.ToString(), FString(TEXT("RÉACTION AUTOMATIQUE")));
	TestTrue(TEXT("Automatic reaction exposes no voluntary UTILISATION"), Node.Usage.IsEmpty());
	const FString Effects = JoinDetailLines(Node.Effects);
	TestTrue(TEXT("Reaction trigger is readable"), Effects.Contains(TEXT("attaque entrante touche")));
	TestTrue(TEXT("Reaction limit is readable"), Effects.Contains(TEXT("une fois par round")));
	TestTrue(TEXT("Reaction response is readable"), Effects.Contains(TEXT("redirige 50 %")));
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
	TestEqual(TEXT("TYPE label is canonical"), Node.TypeText.ToString(), FString(TEXT("ACTIF")));
	TestEqual(TEXT("Available Talent STATUS is isolated"), Node.StatusText.ToString(), FString(TEXT("DISPONIBLE")));
	TestEqual(TEXT("PRINCIPE comes from authored Description"),
		Node.Principle.ToString(), ClassDefinition->ProgressionChoices[0].Description.ToString());
	TestEqual(TEXT("Simple ChoiceId is explicit"), Node.SimpleChoiceId, ClassDefinition->ProgressionChoices[0].ChoiceId);
	TestTrue(TEXT("Simple Talent can be acquired"), Node.bCanAcquireSimple);
	TestTrue(TEXT("Simple Talent has no Variants payload"), Node.Variants.IsEmpty());
	TestTrue(TEXT("Structured EFFETS are present"), !Node.Effects.IsEmpty());
	TestTrue(TEXT("Structured UTILISATION is present"), !Node.Usage.IsEmpty());
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
	TestTrue(TEXT("Variant read model builds"), FGridSkillsPageService::TryBuildCharacterView(Component, 0, {}, View));
	const FGridTalentNodeView& Node = View.TalentTree.Branches[0].Nodes[0];
	TestTrue(TEXT("Node is explicitly a true exclusive-variant node"), Node.bHasExclusiveVariants);
	TestEqual(TEXT("Both variants remain projected"), Node.Variants.Num(), 2);
	TestEqual(TEXT("Node TYPE stays PASSIF"), Node.Type, ERPGTalentPresentationType::Passive);
	for (const FGridTalentVariantView& Variant : Node.Variants)
	{
		TestEqual(TEXT("Variant TYPE matches node TYPE"), Variant.Type, Node.Type);
		TestEqual(TEXT("Variant TYPE label is canonical"), Variant.TypeText.ToString(), FString(TEXT("PASSIF")));
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
	Node.TypeText = FText::FromString(TEXT("SORT ACTIF"));
	Node.StatusText = FText::FromString(TEXT("DISPONIBLE"));
	Node.Principle = FText::FromString(TEXT("Principe déjà résolu."));
	Node.Acquisition.MinimumLevel = 3;
	Node.Acquisition.PointCost = 2;
	Node.Acquisition.PrerequisiteTalentNames.Add(FText::FromString(TEXT("Précurseur")));
	Node.Acquisition.ExclusivityText = FText::FromString(TEXT("Exclusif avec une autre voie."));

	FGridTalentDetailLineView Effect;
	Effect.Label = FText::FromString(TEXT("Dégâts"));
	Effect.Value = FText::FromString(TEXT("10"));
	Node.Effects.Add(Effect);
	FGridTalentDetailLineView Usage;
	Usage.Label = FText::FromString(TEXT("Mana"));
	Usage.Value = FText::FromString(TEXT("5"));
	Node.Usage.Add(Usage);

	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
	TestTrue(TEXT("Static section detail initializes"),
		Detail->InitializeTalentDetail(Node, UIRPGDESC01Tests::MakeBranch(Node.TalentBranchId)));
	TestEqual(TEXT("TYPE is exposed independently"), Detail->ResolvedTypeText.ToString(), FString(TEXT("SORT ACTIF")));
	TestEqual(TEXT("STATUT is exposed independently"), Detail->ResolvedStatusText.ToString(), FString(TEXT("DISPONIBLE")));
	TestEqual(TEXT("PRINCIPE is exposed independently"), Detail->ResolvedPrincipleText.ToString(), FString(TEXT("Principe déjà résolu.")));
	TestEqual(TEXT("EFFETS are presentation-formatted only"), Detail->ResolvedEffectsText.ToString(), FString(TEXT("Dégâts : 10")));
	TestEqual(TEXT("UTILISATION is presentation-formatted only"), Detail->ResolvedUsageText.ToString(), FString(TEXT("Mana : 5")));
	TestTrue(TEXT("ACQUISITION contains canonical level"), Detail->ResolvedAcquisitionText.ToString().Contains(TEXT("Niveau requis : 3")));
	TestTrue(TEXT("ACQUISITION contains canonical cost"), Detail->ResolvedAcquisitionText.ToString().Contains(TEXT("Coût : 2 points de Talent")));
	TestTrue(TEXT("ACQUISITION contains resolved prerequisite name"), Detail->ResolvedAcquisitionText.ToString().Contains(TEXT("Précurseur")));
	TestTrue(TEXT("ACQUISITION contains exclusivity text"), Detail->ResolvedAcquisitionText.ToString().Contains(TEXT("Exclusif avec une autre voie.")));
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

	bool bSawGuardEffect = false;
	for (const FGridTalentDetailLineView& Line : Node.Effects)
	{
		const FString Text = Line.Label.ToString() + TEXT(" ") + Line.Value.ToString();
		TestFalse(TEXT("EFFETS never leaks raw status ids"), Text.Contains(TEXT("Status_")));
		if (Text.Contains(TEXT("Garde")) && Text.Contains(TEXT("2 rounds")))
		{
			bSawGuardEffect = true;
		}
	}
	TestTrue(TEXT("Action StatusApplications are projected into EFFETS"), bSawGuardEffect);
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
	Variant.TypeText = FText::FromString(TEXT("PASSIF"));
	Variant.StatusText = FText::FromString(TEXT("DISPONIBLE"));
	Variant.Principle = FText::FromString(TEXT("Maîtrise les armes tranchantes."));
	Variant.State = EGridTalentNodeState::Available;
	Variant.bCanChoose = true;

	FGridTalentDetailLineView Effect;
	Effect.Label = FText::FromString(TEXT("Précision"));
	Effect.Value = FText::FromString(TEXT("+1"));
	Variant.Effects.Add(Effect);

	UGridTalentVariantBlockWidget* Block = NewObject<UGridTalentVariantBlockWidget>();
	TestTrue(TEXT("Variant block initializes"),
		Block->InitializeVariant(Variant, FText::FromString(TEXT("Tranchant")), false));
	TestEqual(TEXT("Short display name is preserved"), Block->ResolvedDisplayName.ToString(), FString(TEXT("Tranchant")));
	TestEqual(TEXT("Variant TYPE is canonical"), Block->ResolvedTypeText.ToString(), FString(TEXT("PASSIF")));
	TestEqual(TEXT("Variant STATUS is canonical"), Block->ResolvedStatusText.ToString(), FString(TEXT("DISPONIBLE")));
	TestTrue(TEXT("Variant EFFETS are structured"), Block->ResolvedEffectsText.ToString().Contains(TEXT("Précision : +1")));
	TestEqual(TEXT("Available variant exposes CHOISIR"), Block->ResolvedChooseLabel.ToString(), FString(TEXT("CHOISIR")));
	TestTrue(TEXT("Available variant choice is enabled"), Block->bChooseEnabled);

	TestTrue(TEXT("Pending variant reinitializes"),
		Block->InitializeVariant(Variant, FText::FromString(TEXT("Tranchant")), true));
	TestEqual(TEXT("Pending variant exposes CHOIX EN COURS"), Block->ResolvedChooseLabel.ToString(), FString(TEXT("CHOIX EN COURS")));
	TestFalse(TEXT("Pending variant cannot be chosen twice"), Block->bChooseEnabled);

	Variant.bAcquired = true;
	Variant.bCanChoose = false;
	Variant.State = EGridTalentNodeState::Acquired;
	Variant.StatusText = FText::FromString(TEXT("ACQUIS"));
	TestTrue(TEXT("Acquired variant reinitializes"),
		Block->InitializeVariant(Variant, FText::FromString(TEXT("Tranchant")), false));
	TestEqual(TEXT("Acquired variant exposes ACQUISE"), Block->ResolvedChooseLabel.ToString(), FString(TEXT("ACQUISE")));
	TestFalse(TEXT("Acquired variant choice is disabled"), Block->bChooseEnabled);

	Variant.bAcquired = false;
	Variant.State = EGridTalentNodeState::LockedExclusive;
	Variant.bCanChoose = false;
	Variant.StatusText = FText::FromString(TEXT("INDISPONIBLE — autre variante déjà choisie"));
	TestTrue(TEXT("Unavailable variant reinitializes"),
		Block->InitializeVariant(Variant, FText::FromString(TEXT("Perforant")), false));
	TestEqual(TEXT("Unavailable variant exposes INDISPONIBLE"), Block->ResolvedChooseLabel.ToString(), FString(TEXT("INDISPONIBLE")));
	TestFalse(TEXT("Unavailable variant choice is disabled"), Block->bChooseEnabled);
	return true;
}

#endif
