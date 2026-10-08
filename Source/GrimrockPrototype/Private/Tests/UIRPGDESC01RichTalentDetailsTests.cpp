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
	TestTrue(TEXT("Simple Talent exposes authoritative action summary"), Detail->ResolvedActionSummary.ToString().Contains(TEXT("1 PA")));
	TestTrue(TEXT("Action summary exposes Mana"), Detail->ResolvedActionSummary.ToString().Contains(TEXT("4 Mana")));
	TestTrue(TEXT("Action summary exposes cooldown"), Detail->ResolvedActionSummary.ToString().Contains(TEXT("Recharge 3 tours")));
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
	TestTrue(TEXT("Variant acquisition can start"), Detail->BeginVariantSelection());
	TestTrue(TEXT("Fire can be previewed"), Detail->SelectVariantChoice(TEXT("Choice_Fire")));
	TestEqual(TEXT("Concrete short variant name is exposed"), Detail->ResolvedVariantDisplayName.ToString(), FString(TEXT("Feu")));
	TestEqual(
		TEXT("Actual FGridTalentVariantView description is exposed"),
		Detail->ResolvedVariantDescription.ToString(),
		FString(TEXT("Les sorts de Feu infligent +15 % de dégâts.")));
	TestTrue(TEXT("Variant action summary is exposed"), Detail->ResolvedActionSummary.ToString().Contains(TEXT("Action variante")));
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
	TestEqual(TEXT("Acquired concrete variant is previewed automatically"), Detail->ResolvedVariantDisplayName.ToString(), FString(TEXT("Glace")));
	TestEqual(TEXT("Acquired variant keeps its concrete description"), Detail->ResolvedVariantDescription.ToString(), FString(TEXT("Description Glace.")));
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
 TestTrue(TEXT("Locked Fire can be inspected"), Detail->SelectVariantChoice(TEXT("Choice_Fire")));
 TestEqual(TEXT("Locked Fire readable"), Detail->ResolvedVariantDescription.ToString(), FString(TEXT("Effet Feu.")));
 TestFalse(TEXT("Locked Fire cannot begin purchase"), Detail->BeginVariantSelection());
 TestFalse(TEXT("Locked Fire cannot confirm purchase"), Detail->ConfirmAcquire());
 TestTrue(TEXT("Locked Frost can be inspected"), Detail->SelectVariantChoice(TEXT("Choice_Frost")));
 TestEqual(TEXT("Locked Frost readable"), Detail->ResolvedVariantDescription.ToString(), FString(TEXT("Effet Glace.")));
 return true;
}

#endif
