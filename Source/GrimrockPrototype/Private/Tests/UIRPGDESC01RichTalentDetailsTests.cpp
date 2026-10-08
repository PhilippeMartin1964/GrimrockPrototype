#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridSkillsPageService.h"
#include "UI/GridTalentDetailWidget.h"
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

	FRPGTalentBranchPresentationDefinition MakeBranch(FName BranchId, FName NodeId, const TCHAR* ConceptName = TEXT("Talent conceptuel"))
	{
		FRPGTalentBranchPresentationDefinition Branch;
		Branch.TalentBranchId = BranchId;
		FRPGTalentNodePresentationDefinition Override;
		Override.TalentNodeId = NodeId;
		Override.DisplayName = FText::FromString(ConceptName);
		Override.Description = FText::FromString(TEXT("Choisissez une variante."));
		Branch.NodePresentationOverrides.Add(Override);
		return Branch;
	}

	FGridTalentVariantView MakeVariant(FName ChoiceId, const TCHAR* Name, const TCHAR* Description)
	{
		FGridTalentVariantView Variant;
		Variant.ChoiceId = ChoiceId;
		Variant.DisplayName = FText::FromString(Name);
		Variant.Description = FText::FromString(Description);
		Variant.State = EGridTalentNodeState::Available;
		Variant.bAvailable = true;

		FGridTalentUnlockedActionView Action;
		Action.ActionId = TEXT("Action_DESC01_Variant");
		Action.DisplayName = FText::FromString(TEXT("Action variante"));
		Action.Description = FText::FromString(TEXT("Effet concret de la variante."));
		Action.ActionPointCost = 1;
		Action.ManaCost = 4;
		Action.CooldownRounds = 3;
		Variant.UnlockedActions.Add(Action);
		return Variant;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC01ActionProjectionTest,
	"Grimrock.UI.RPG.DESC01.ReadModel.ActionProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC01ActionProjectionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace UIRPGDESC01Tests;
	FRuntimeGuard Guard;

	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeSimpleInventory(ClassDefinition);
	TestTrue(TEXT("DESC01 class is structurally valid"), ClassDefinition->IsValidDefinition());

	FGridSkillsPageView View;
	TestTrue(TEXT("Skills page builds rich Talent view"), FGridSkillsPageService::TryBuildCharacterView(Component, 0, {}, View));
	TestEqual(TEXT("One branch is projected"), View.TalentTree.Branches.Num(), 1);
	if (View.TalentTree.Branches.Num() != 1 || View.TalentTree.Branches[0].Nodes.Num() != 1)
	{
		return false;
	}

	const FGridTalentVariantView& Variant = View.TalentTree.Branches[0].Nodes[0].Variants[0];
	TestEqual(TEXT("One authoritative action is associated with the Talent"), Variant.UnlockedActions.Num(), 1);
	if (Variant.UnlockedActions.Num() != 1)
	{
		return false;
	}

	const FGridTalentUnlockedActionView& Action = Variant.UnlockedActions[0];
	TestEqual(TEXT("Action id is projected"), Action.ActionId, FName(TEXT("Action_DESC01_Test")));
	TestEqual(TEXT("Action description is projected"), Action.Description.ToString(), FString(TEXT("Inflige un effet autoritaire de test.")));
	TestEqual(TEXT("AP cost is projected"), Action.ActionPointCost, 2);
	TestEqual(TEXT("Mana cost is projected"), Action.ManaCost, 3);
	TestEqual(TEXT("Cooldown is projected"), Action.CooldownRounds, 2);
	TestTrue(TEXT("Passive mechanics are projected from canonical modifier data"), Variant.MechanicsSummary.ToString().Contains(TEXT("Précision : +2")));
	TestEqual(TEXT("Action plus passive category is explicit"), Variant.EffectCategory.ToString(), FString(TEXT("CAPACITÉ ACTIVE + BONUS PASSIF")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC01SimpleDetailTest,
	"Grimrock.UI.RPG.DESC01.Detail.SimpleActionSummary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC01SimpleDetailTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace UIRPGDESC01Tests;

	FGridTalentNodeView Node;
	Node.TalentNodeId = TEXT("Talent_DESC01_Simple");
	Node.TalentBranchId = TEXT("Branch_DESC01");
	Node.Tier = 1;
	Node.MinimumLevel = 2;
	Node.PointCost = 1;
	Node.State = EGridTalentNodeState::Available;
	Node.Variants.Add(MakeVariant(Node.TalentNodeId, TEXT("Talent simple"), TEXT("Description du talent simple.")));

	FRPGTalentBranchPresentationDefinition Branch;
	Branch.TalentBranchId = Node.TalentBranchId;

	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
	TestTrue(TEXT("Simple detail initializes"), Detail->InitializeTalentDetail(Node, Branch));
	TestTrue(TEXT("Simple Talent exposes authoritative action summary"), Detail->ResolvedActionSummary.ToString().Contains(TEXT("1 point d'action")));
	TestTrue(TEXT("Unacquired simple Talent labels its action as future"), Detail->ResolvedActionSummary.ToString().StartsWith(TEXT("ACTION ACCORDÉE APRÈS ACQUISITION")));
	TestFalse(TEXT("Unacquired simple Talent never claims its action is already unlocked"), Detail->ResolvedActionSummary.ToString().Contains(TEXT("ACTION DÉBLOQUÉE")));
	TestTrue(TEXT("Action summary exposes Mana"), Detail->ResolvedActionSummary.ToString().Contains(TEXT("4 mana")));
	TestTrue(TEXT("Action summary exposes cooldown"), Detail->ResolvedActionSummary.ToString().Contains(TEXT("recharge : 3 tours")));
	TestTrue(TEXT("Action summary exposes action description"), Detail->ResolvedActionSummary.ToString().Contains(TEXT("Effet concret")));
	TestTrue(TEXT("Simple Talent does not duplicate a variant heading"), Detail->ResolvedVariantDisplayName.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC01VariantPreviewTest,
	"Grimrock.UI.RPG.DESC01.Detail.VariantPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC01VariantPreviewTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace UIRPGDESC01Tests;

	FGridTalentNodeView Node;
	Node.TalentNodeId = TEXT("Talent_DESC01_Affinity");
	Node.TalentBranchId = TEXT("Branch_DESC01");
	Node.Tier = 1;
	Node.MinimumLevel = 2;
	Node.PointCost = 1;
	Node.State = EGridTalentNodeState::Available;
	Node.Variants.Add(MakeVariant(TEXT("Choice_Fire"), TEXT("Talent conceptuel — Feu"), TEXT("Les sorts de Feu infligent +15 % de dégâts.")));
	Node.Variants.Add(MakeVariant(TEXT("Choice_Frost"), TEXT("Talent conceptuel — Glace"), TEXT("Les sorts de Glace infligent +15 % de dégâts.")));

	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
	TestTrue(TEXT("Variant detail initializes"), Detail->InitializeTalentDetail(Node, MakeBranch(Node.TalentBranchId, Node.TalentNodeId)));
	TestEqual(TEXT("Multi-variant detail has one stable heading"), Detail->ResolvedVariantDisplayName.ToString(), FString(TEXT("VARIANTES")));
	TestTrue(TEXT("Fire is visible immediately"), Detail->ResolvedVariantDescription.ToString().Contains(TEXT("Feu")));
	TestTrue(TEXT("Frost is visible immediately"), Detail->ResolvedVariantDescription.ToString().Contains(TEXT("Glace")));
	TestTrue(TEXT("Fire description is visible immediately"), Detail->ResolvedVariantDescription.ToString().Contains(TEXT("Les sorts de Feu infligent +15 % de dégâts.")));
	TestTrue(TEXT("Frost description is visible immediately"), Detail->ResolvedVariantDescription.ToString().Contains(TEXT("Les sorts de Glace infligent +15 % de dégâts.")));
	TestTrue(TEXT("Variant actions are included in the overview"), Detail->ResolvedVariantDescription.ToString().Contains(TEXT("Action variante")));
	TestTrue(TEXT("Variant actions are future before acquisition"), Detail->ResolvedVariantDescription.ToString().Contains(TEXT("ACTION ACCORDÉE APRÈS ACQUISITION")));
	TestTrue(TEXT("Variant acquisition can start as a separate interaction"), Detail->BeginVariantSelection());
	TestTrue(TEXT("Fire can be chosen as the acquisition candidate"), Detail->SelectVariantChoice(TEXT("Choice_Fire")));
	TestTrue(TEXT("Choosing Fire never hides Frost"), Detail->ResolvedVariantDescription.ToString().Contains(TEXT("Glace")));
	TestTrue(TEXT("The acquisition candidate is identified"), Detail->ResolvedVariantDescription.ToString().Contains(TEXT("Feu — SÉLECTIONNÉE")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC01AcquiredVariantTest,
	"Grimrock.UI.RPG.DESC01.Detail.AcquiredVariantPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC01AcquiredVariantTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace UIRPGDESC01Tests;

	FGridTalentNodeView Node;
	Node.TalentNodeId = TEXT("Talent_DESC01_Affinity");
	Node.TalentBranchId = TEXT("Branch_DESC01");
	Node.Tier = 1;
	Node.MinimumLevel = 2;
	Node.PointCost = 1;
	Node.State = EGridTalentNodeState::Acquired;
	Node.SelectedChoiceId = TEXT("Choice_Frost");

	FGridTalentVariantView Fire = MakeVariant(TEXT("Choice_Fire"), TEXT("Talent conceptuel — Feu"), TEXT("Description Feu."));
	FGridTalentVariantView Frost = MakeVariant(TEXT("Choice_Frost"), TEXT("Talent conceptuel — Glace"), TEXT("Description Glace."));
	Frost.bSelected = true;
	Frost.bAvailable = false;
	Frost.State = EGridTalentNodeState::Acquired;
	Node.Variants = { Fire, Frost };

	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
	TestTrue(TEXT("Acquired variant detail initializes"), Detail->InitializeTalentDetail(Node, MakeBranch(Node.TalentBranchId, Node.TalentNodeId)));
	TestEqual(TEXT("Acquired multi-variant detail keeps the stable heading"), Detail->ResolvedVariantDisplayName.ToString(), FString(TEXT("VARIANTES")));
	TestTrue(TEXT("Acquired variant is explicitly identified"), Detail->ResolvedVariantDescription.ToString().Contains(TEXT("Glace — CHOISIE")));
	TestTrue(TEXT("Non-selected variants remain visible"), Detail->ResolvedVariantDescription.ToString().Contains(TEXT("Feu")));
	TestTrue(TEXT("Acquired variant action is labeled available"), Detail->ResolvedVariantDescription.ToString().Contains(TEXT("ACTION DISPONIBLE")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGDESC01WidgetContractTest,
	"Grimrock.UI.RPG.DESC01.Detail.WidgetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGDESC01WidgetContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* DetailClass = UGridTalentDetailWidget::StaticClass();
	for (const TCHAR* PropertyName : {
		TEXT("Text_DetailVariantName"),
		TEXT("Text_DetailVariantDescription"),
		TEXT("Text_DetailActionSummary")
	})
	{
		TestNotNull(
			*FString::Printf(TEXT("%s is exposed as an optional Designer binding"), PropertyName),
			FindFProperty<FProperty>(DetailClass, FName(PropertyName)));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
 FUIRPGDESC01LockedVariantPreviewTest,
 "Grimrock.UI.RPG.DESC01.Detail.LockedVariantPreview",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUIRPGDESC01LockedVariantPreviewTest::RunTest(const FString& Parameters)
{
 (void)Parameters;
 using namespace UIRPGDESC01Tests;
 FGridTalentNodeView Node;
 Node.TalentNodeId = TEXT("Talent_DESC01_Affinity");
 Node.TalentBranchId = TEXT("Branch_DESC01");
 Node.Tier = 1;
 Node.MinimumLevel = 2;
 Node.PointCost = 1;
 Node.State = EGridTalentNodeState::LockedPoints;
 FGridTalentVariantView Fire = MakeVariant(TEXT("Choice_Fire"), TEXT("Talent conceptuel — Feu"), TEXT("Effet Feu."));
 FGridTalentVariantView Frost = MakeVariant(TEXT("Choice_Frost"), TEXT("Talent conceptuel — Glace"), TEXT("Effet Glace."));
 Fire.State = EGridTalentNodeState::LockedPoints;
 Frost.State = EGridTalentNodeState::LockedPoints;
 Fire.bAvailable = false;
 Frost.bAvailable = false;
 Node.Variants = { Fire, Frost };
 UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
 TestTrue(TEXT("Locked node initializes"), Detail->InitializeTalentDetail(Node, MakeBranch(Node.TalentBranchId, Node.TalentNodeId)));
 TestTrue(TEXT("Locked Fire is visible without interaction"), Detail->ResolvedVariantDescription.ToString().Contains(TEXT("Effet Feu.")));
 TestTrue(TEXT("Locked Frost is visible without interaction"), Detail->ResolvedVariantDescription.ToString().Contains(TEXT("Effet Glace.")));
 TestFalse(TEXT("Locked node cannot begin purchase"), Detail->BeginVariantSelection());
 TestFalse(TEXT("Locked node cannot choose a variant outside acquisition"), Detail->SelectVariantChoice(TEXT("Choice_Fire")));
 TestFalse(TEXT("Locked node cannot confirm purchase"), Detail->ConfirmAcquire());
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
 FUIRPGDESC01EffectCategoryTest,
 "Grimrock.UI.RPG.DESC01.Detail.EffectCategory",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUIRPGDESC01EffectCategoryTest::RunTest(const FString& Parameters)
{
 (void)Parameters;
 using namespace UIRPGDESC01Tests;
 FGridTalentNodeView Node;
 Node.TalentNodeId = TEXT("Talent_DESC01_Affinity");
 Node.TalentBranchId = TEXT("Branch_DESC01");
 Node.Tier = 1;
 Node.MinimumLevel = 2;
 Node.PointCost = 1;
 Node.State = EGridTalentNodeState::LockedPoints;
 FGridTalentVariantView Fire = MakeVariant(TEXT("Choice_Fire"), TEXT("Talent conceptuel — Feu"), TEXT("Bonus de feu."));
 FGridTalentVariantView Frost = MakeVariant(TEXT("Choice_Frost"), TEXT("Talent conceptuel — Glace"), TEXT("Bonus de glace."));
 Fire.EffectCategory = FText::FromString(TEXT("BONUS PASSIF"));
 Frost.EffectCategory = FText::FromString(TEXT("BONUS PASSIF"));
 Fire.bAvailable = false;
 Frost.bAvailable = false;
 Node.Variants = { Fire, Frost };
 UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
 TestTrue(TEXT("Categorized node initializes"), Detail->InitializeTalentDetail(Node, MakeBranch(Node.TalentBranchId, Node.TalentNodeId)));
 TestTrue(TEXT("Category is displayed without variant interaction"), Detail->ResolvedVariantDescription.ToString().Contains(TEXT("BONUS PASSIF")));
 TestTrue(TEXT("Fire effect is displayed without variant interaction"), Detail->ResolvedVariantDescription.ToString().Contains(TEXT("Bonus de feu.")));
 TestTrue(TEXT("Frost effect is displayed at the same time"), Detail->ResolvedVariantDescription.ToString().Contains(TEXT("Bonus de glace.")));
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
 FUIRPGDESC01StructuredActionSummaryTest,
 "Grimrock.UI.RPG.DESC01.Detail.StructuredActionSummary",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUIRPGDESC01StructuredActionSummaryTest::RunTest(const FString& Parameters)
{
 (void)Parameters;
 using namespace UIRPGDESC01Tests;
 FGridTalentNodeView Node;
 Node.TalentNodeId = TEXT("Talent_DESC01_Structured");
 Node.TalentBranchId = TEXT("Branch_DESC01");
 Node.Tier = 1;
 Node.MinimumLevel = 2;
 Node.PointCost = 1;
 Node.State = EGridTalentNodeState::Available;
 FGridTalentVariantView Variant = MakeVariant(Node.TalentNodeId, TEXT("Talent structuré"), TEXT("Description structurée."));
 FGridTalentUnlockedActionView& Action = Variant.UnlockedActions[0];
 Action.TargetSummary = FText::FromString(TEXT("une zone"));
 Action.RangeCells = 4;
 Action.AreaRadiusCells = 1;
 Action.SourceItemQuantityCost = 1;
 Action.bRequiresLineOfSight = true;
 Action.ResolutionCount = 2;
 Action.SubsequentResolutionAccuracyModifier = -1;
 Node.Variants.Add(Variant);
 FRPGTalentBranchPresentationDefinition Branch;
 Branch.TalentBranchId = Node.TalentBranchId;
 UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
 TestTrue(TEXT("Structured detail initializes"), Detail->InitializeTalentDetail(Node, Branch));
 const FString Summary = Detail->ResolvedActionSummary.ToString();
 TestTrue(TEXT("Structured cost exposes item consumption"), Summary.Contains(TEXT("1 objet consommé")));
 TestTrue(TEXT("Structured target is readable"), Summary.Contains(TEXT("Cible : une zone")));
 TestTrue(TEXT("Structured range is readable"), Summary.Contains(TEXT("portée : 4 cases")));
 TestTrue(TEXT("Structured area is readable"), Summary.Contains(TEXT("zone : rayon 1 case")));
 TestTrue(TEXT("Structured LOS is readable"), Summary.Contains(TEXT("ligne de vue requise")));
 TestTrue(TEXT("Structured multi-resolution is readable"), Summary.Contains(TEXT("2 résolutions")));
 TestTrue(TEXT("Structured follow-up accuracy is readable"), Summary.Contains(TEXT("-1 précision")));
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
 FUIRPGDESC01ReactionMechanicsTest,
 "Grimrock.UI.RPG.DESC01.ReadModel.ReactionMechanics",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUIRPGDESC01ReactionMechanicsTest::RunTest(const FString& Parameters)
{
 (void)Parameters;
 using namespace UIRPGDESC01Tests;
 URPGClassAsset* ClassDefinition = nullptr;
 UGridPartyInventoryComponent* Component = MakeSimpleInventory(ClassDefinition);
 FRPGClassProgressionChoiceDefinition& Choice = ClassDefinition->ProgressionChoices[0];
 Choice.CombatModifiers.Reset();
 FGridCombatReactionProfile Reaction;
 Reaction.ReactionId = TEXT("Reaction_DESC01");
 Reaction.Trigger = EGridCombatReactionTrigger::IncomingAttackHit;
 Reaction.Limit = EGridCombatReactionLimit::OncePerRound;
 Reaction.InterceptFinalDamagePercent = 50;
 Choice.CombatReactions.Add(Reaction);
 TestTrue(TEXT("Reaction class stays valid"), ClassDefinition->IsValidDefinition());
 FGridSkillsPageView View;
 TestTrue(TEXT("Reaction view builds"), FGridSkillsPageService::TryBuildCharacterView(Component, 0, {}, View));
 const FGridTalentVariantView& Variant = View.TalentTree.Branches[0].Nodes[0].Variants[0];
 TestEqual(TEXT("Reaction category is explicit"), Variant.EffectCategory.ToString(), FString(TEXT("CAPACITÉ ACTIVE + RÉACTION AUTOMATIQUE")));
 TestTrue(TEXT("Reaction trigger is readable"), Variant.MechanicsSummary.ToString().Contains(TEXT("attaque entrante touche")));
 TestTrue(TEXT("Reaction limit is readable"), Variant.MechanicsSummary.ToString().Contains(TEXT("une fois par round")));
 TestTrue(TEXT("Reaction response is readable"), Variant.MechanicsSummary.ToString().Contains(TEXT("redirige 50 %")));
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
 FUIRPGDESC01UnifiedPlayerLanguageTest,
 "Grimrock.UI.RPG.DESC01.Detail.UnifiedPlayerLanguage",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FUIRPGDESC01UnifiedPlayerLanguageTest::RunTest(const FString& Parameters)
{
 (void)Parameters;
 using namespace UIRPGDESC01Tests;
 FGridTalentNodeView Node;
 Node.TalentNodeId = TEXT("Talent_DESC01_Language");
 Node.TalentBranchId = TEXT("Branch_DESC01");
 Node.Tier = 2;
 Node.MinimumLevel = 6;
 Node.PointCost = 1;
 Node.State = EGridTalentNodeState::LockedLevel;
 FGridTalentVariantView Variant = MakeVariant(
  Node.TalentNodeId,
  TEXT("Talent lisible"),
  TEXT("Accuracy +2 ; applique Status_Stunned si PhysicalArmor est épuisée."));
 Variant.EffectCategory = FText::FromString(TEXT("CAPACITÉ ACTIVE"));
 Variant.MechanicsSummary = FText::FromString(TEXT("Accuracy : +2"));
 Node.Variants.Add(Variant);
 FRPGTalentBranchPresentationDefinition Branch;
 Branch.TalentBranchId = Node.TalentBranchId;
 UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
 TestTrue(TEXT("Readable Talent initializes"), Detail->InitializeTalentDetail(Node, Branch));
 const FString Main = Detail->ResolvedMainDetailText.ToString();
 TestTrue(TEXT("Stable TYPE section exists"), Main.Contains(TEXT("TYPE")));
 TestTrue(TEXT("Stable FONCTIONNEMENT section exists"), Main.Contains(TEXT("FONCTIONNEMENT")));
 TestTrue(TEXT("Stable EFFETS section exists"), Main.Contains(TEXT("EFFETS")));
 TestTrue(TEXT("Accuracy becomes player-readable"), Main.Contains(TEXT("Précision")));
 TestTrue(TEXT("Stunned status becomes player-readable"), Main.Contains(TEXT("Étourdi")));
 TestTrue(TEXT("PhysicalArmor becomes player-readable"), Main.Contains(TEXT("armure physique")));
 TestFalse(TEXT("Raw Status identifier is hidden"), Main.Contains(TEXT("Status_")));
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
	FRPGClassProgressionChoiceDefinition& Choice = ClassDefinition->ProgressionChoices[0];
	Choice.PresentationType = ERPGTalentPresentationType::Active;
	TestTrue(TEXT("DESC01.14 transient class is valid"), ClassDefinition->IsValidDefinition());

	FGridSkillsPageView View;
	TestTrue(TEXT("DESC01.14 structured read model builds"),
		FGridSkillsPageService::TryBuildCharacterView(Component, 0, {}, View));
	const FGridTalentNodeView& Node = View.TalentTree.Branches[0].Nodes[0];

	TestEqual(TEXT("Explicit Talent TYPE is projected"), Node.Type, ERPGTalentPresentationType::Active);
	TestEqual(TEXT("TYPE label is canonical"), Node.TypeText.ToString(), FString(TEXT("ACTIF")));
	TestEqual(TEXT("Available Talent STATUS is isolated from action state"), Node.StatusText.ToString(), FString(TEXT("DISPONIBLE")));
	TestEqual(TEXT("PRINCIPE comes from authored Description"), Node.Principle.ToString(), Choice.Description.ToString());
	TestEqual(TEXT("Simple ChoiceId is explicit"), Node.SimpleChoiceId, Choice.ChoiceId);
	TestTrue(TEXT("Simple Talent can be acquired"), Node.bCanAcquireSimple);
	TestTrue(TEXT("Structured EFFETS are present"), !Node.Effects.IsEmpty());
	TestTrue(TEXT("Structured UTILISATION is present"), !Node.Usage.IsEmpty());
	TestEqual(TEXT("Acquisition level is projected"), Node.Acquisition.MinimumLevel, Choice.MinimumLevel);
	TestEqual(TEXT("Acquisition point cost is projected"), Node.Acquisition.PointCost, Choice.PointCost);

	bool bSawRoundCooldown = false;
	for (const FGridTalentDetailLineView& Line : Node.Usage)
	{
		const FString Joined = Line.Label.ToString() + TEXT(" ") + Line.Value.ToString();
		TestFalse(TEXT("Structured usage never claims ACTION DISPONIBLE"), Joined.Contains(TEXT("ACTION DISPONIBLE")));
		TestFalse(TEXT("Structured usage never claims future action unlock"), Joined.Contains(TEXT("ACTION ACCORDÉE")));
		if (Line.Label.ToString() == TEXT("Recharge") && Line.Value.ToString().Contains(TEXT("round")))
		{
			bSawRoundCooldown = true;
		}
	}
	TestTrue(TEXT("CooldownRounds is rendered in rounds"), bSawRoundCooldown);
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

#endif
