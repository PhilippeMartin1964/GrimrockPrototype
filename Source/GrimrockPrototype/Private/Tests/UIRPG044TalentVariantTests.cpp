#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPGMON155TestHelpers.h"
#include "UI/GridSkillsPageService.h"
#include "UI/GridSkillsWidget.h"
#include "UI/GridTalentDetailWidget.h"

namespace UIRPG044Tests
{
	UGridPartyInventoryComponent* MakeVariantTransactionInventory(URPGClassAsset*& OutClassDefinition)
	{
		UGridPartyInventoryComponent* Component = NewObject<UGridPartyInventoryComponent>();
		OutClassDefinition = NewObject<URPGClassAsset>(Component);
		OutClassDefinition->ClassId = TEXT("RPG04_VariantTransaction");
		OutClassDefinition->DisplayName = FText::FromString(TEXT("RPG04 Variant Transaction"));
		OutClassDefinition->BaseAttributes = FRPGAttributes{ 12, 12, 12, 10, 10, 10 };
		OutClassDefinition->HealthAtLevelOne = 20;

		FRPGClassProgressionLevelGrant Grant;
		Grant.Level = 2;
		Grant.ChoicePointsGranted = 1;
		OutClassDefinition->ProgressionLevelGrants.Add(Grant);

		const FName ConceptNodeId(TEXT("Talent_Test_Variant"));
		const FName ExclusiveGroup(TEXT("TalentGroup_Test_Variant"));

		FRPGClassProgressionChoiceDefinition ChoiceA;
		ChoiceA.ChoiceId = TEXT("Choice_A");
		ChoiceA.DisplayName = FText::FromString(TEXT("Talent test — Variante A"));
		ChoiceA.Description = FText::FromString(TEXT("Variante A de test pour la projection d'acquisition."));
		ChoiceA.MinimumLevel = 2;
		ChoiceA.PointCost = 1;
		ChoiceA.TalentBranchId = TEXT("Branch");
		ChoiceA.TalentNodeId = ConceptNodeId;
		ChoiceA.ExclusiveChoiceGroupId = ExclusiveGroup;
		ChoiceA.GrantedRequirementIds.Add(ConceptNodeId);
		ChoiceA.PresentationType = ERPGTalentPresentationType::Passive;
		OutClassDefinition->ProgressionChoices.Add(ChoiceA);

		FRPGClassProgressionChoiceDefinition ChoiceB = ChoiceA;
		ChoiceB.ChoiceId = TEXT("Choice_B");
		ChoiceB.DisplayName = FText::FromString(TEXT("Talent test — Variante B"));
		ChoiceB.Description = FText::FromString(TEXT("Variante B de test pour la projection d'acquisition."));
		OutClassDefinition->ProgressionChoices.Add(ChoiceB);

		Component->PartyInventoryState.ActiveCharacters.Add(
			MakeMON155Character(OutClassDefinition, 2, 1000));
		Component->PartyInventoryState.ActiveEquipment.SetNum(1);
		return Component;
	}

	FGridTalentVariantView MakeVariant(FName ChoiceId, const TCHAR* DisplayName)
	{
		FGridTalentVariantView Variant;
		Variant.ChoiceId = ChoiceId;
		Variant.DisplayName = FText::FromString(DisplayName);
		Variant.State = EGridTalentNodeState::Available;
		Variant.Type = ERPGTalentPresentationType::Passive;
		Variant.TypeText = FText::FromString(TEXT("PASSIF"));
		Variant.StatusText = FText::FromString(TEXT("DISPONIBLE"));
		Variant.Principle = FText::FromString(TEXT("Description"));
		Variant.bCanChoose = true;
		return Variant;
	}

	FGridTalentNodeView MakeVariantNode(int32 Count)
	{
		FGridTalentNodeView Node;
		Node.TalentNodeId = TEXT("Talent_Test_Concept");
		Node.TalentBranchId = TEXT("Branch");
		Node.Tier = 1;
		Node.MinimumLevel = 2;
		Node.PointCost = 1;
		Node.State = EGridTalentNodeState::Available;
		Node.DisplayName = FText::FromString(TEXT("Talent conceptuel"));
		Node.Type = ERPGTalentPresentationType::Passive;
		Node.TypeText = FText::FromString(TEXT("PASSIF"));
		Node.StatusText = FText::FromString(TEXT("DISPONIBLE"));
		Node.Principle = FText::FromString(TEXT("Choisissez une variante."));
		Node.Acquisition.MinimumLevel = 2;
		Node.Acquisition.PointCost = 1;
		Node.bHasExclusiveVariants = true;

		static const TCHAR* Names[] = {
			TEXT("Talent conceptuel — Feu"),
			TEXT("Talent conceptuel — Glace"),
			TEXT("Talent conceptuel — Air"),
			TEXT("Talent conceptuel — Terre")
		};
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Node.Variants.Add(MakeVariant(
				FName(*FString::Printf(TEXT("Choice_%d"), Index)),
				Names[Index]));
		}
		return Node;
	}

	FRPGTalentBranchPresentationDefinition MakePresentation()
	{
		FRPGTalentBranchPresentationDefinition Branch;
		Branch.TalentBranchId = TEXT("Branch");
		FRPGTalentNodePresentationDefinition Override;
		Override.TalentNodeId = TEXT("Talent_Test_Concept");
		Override.DisplayName = FText::FromString(TEXT("Talent conceptuel"));
		Override.Description = FText::FromString(TEXT("Choisissez une variante."));
		Branch.NodePresentationOverrides.Add(Override);
		return Branch;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG044GenericNVariantSelectionTest,
	"Grimrock.UI.RPG04.Variants.GenericNSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG044GenericNVariantSelectionTest::RunTest(const FString&)
{
	using namespace UIRPG044Tests;
	FGridTalentNodeView Node = MakeVariantNode(4);
	Node.Variants[1].State = EGridTalentNodeState::LockedPoints;
	Node.Variants[1].bCanChoose = false;

	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
	TestTrue(TEXT("Four-variant conceptual node initializes"),
		Detail->InitializeTalentDetail(Node, MakePresentation()));
	TestFalse(TEXT("Multi-variant node is not a simple acquisition"), Detail->CanRequestSimpleAcquisition());
	TestTrue(TEXT("Multi-variant node can open generic selector"), Detail->CanRequestVariantAcquisition());
	TestTrue(TEXT("Variant selector enters pending state"), Detail->BeginVariantSelection());
	TestTrue(TEXT("Variant pending flag is set"), Detail->bVariantSelectionPending);
	TestFalse(TEXT("Locked sibling cannot become the pending choice"), Detail->SelectVariantChoice(TEXT("Choice_1")));
	TestTrue(TEXT("Rejected sibling leaves no pending ChoiceId"), Detail->GetSelectedVariantChoiceId().IsNone());
	TestTrue(TEXT("A concrete available ChoiceId can be selected"), Detail->SelectVariantChoice(TEXT("Choice_3")));
	TestEqual(TEXT("Selected variant ChoiceId is stable"), Detail->GetSelectedVariantChoiceId(), FName(TEXT("Choice_3")));

	Detail->CancelAcquireConfirmation();
	TestFalse(TEXT("Cancel clears variant pending state"), Detail->bVariantSelectionPending);
	TestTrue(TEXT("Cancel clears the pending ChoiceId"), Detail->GetSelectedVariantChoiceId().IsNone());
	TestFalse(TEXT("Nothing can be confirmed after cancel"), Detail->ConfirmAcquire());

	TestTrue(TEXT("Variant acquisition can be started again after cancel"), Detail->BeginVariantSelection());
	TestTrue(TEXT("Available variant can be selected again"), Detail->SelectVariantChoice(TEXT("Choice_3")));
	TestTrue(TEXT("Selected variant can be confirmed"), Detail->ConfirmAcquire());
	TestFalse(TEXT("Variant pending state clears after confirmation"), Detail->bVariantSelectionPending);
	TestTrue(TEXT("Confirmed ChoiceId is no longer retained as transient UI state"), Detail->GetSelectedVariantChoiceId().IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG044ShortVariantLabelsTest,
	"Grimrock.UI.RPG04.Variants.ShortLabels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG044ShortVariantLabelsTest::RunTest(const FString&)
{
	using namespace UIRPG044Tests;
	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
	TestTrue(TEXT("Variant node initializes"),
		Detail->InitializeTalentDetail(MakeVariantNode(3), MakePresentation()));

	FText Label;
	TestTrue(TEXT("Variant label resolves"), Detail->GetVariantDisplayLabel(TEXT("Choice_0"), Label));
	TestEqual(TEXT("Conceptual prefix is removed"), Label.ToString(), FString(TEXT("Feu")));
	TestTrue(TEXT("Third variant label resolves"), Detail->GetVariantDisplayLabel(TEXT("Choice_2"), Label));
	TestEqual(TEXT("Third variant is concise"), Label.ToString(), FString(TEXT("Air")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG044VariantTransactionTest,
	"Grimrock.UI.RPG04.Variants.Transaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG044VariantTransactionTest::RunTest(const FString&)
{
	using namespace UIRPG044Tests;
	FMON155RuntimeStateGuard RuntimeGuard;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeVariantTransactionInventory(ClassDefinition);

	const FName ConceptNodeId(TEXT("Talent_Test_Variant"));
	TestNotNull(TEXT("Dedicated transaction fixture has a class"), ClassDefinition);
	TestNotNull(TEXT("Dedicated transaction fixture has an inventory"), Component);
	if (!ClassDefinition || !Component)
	{
		return false;
	}
	TestEqual(TEXT("Dedicated transaction fixture contains exactly two variants"),
		ClassDefinition->ProgressionChoices.Num(), 2);
	TestTrue(TEXT("Dedicated transaction fixture is a valid RPG class"),
		ClassDefinition->IsValidDefinition());

	FGridSkillsPageView BeforeCommit;
	TestTrue(TEXT("Pre-commit read-model rebuild succeeds"),
		FGridSkillsPageService::TryBuildCharacterView(Component, 0, {}, BeforeCommit));

	const FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];
	UGridSkillsWidget* Skills = NewObject<UGridSkillsWidget>();
	Skills->InventoryComponent = Component;
	Skills->View.CharacterIndex = 0;
	Skills->View.CharacterId = Character.CharacterId;
	Skills->View.ClassId = Character.ClassId;
	Skills->View.TalentTree.ClassId = Character.ClassId;

	FGridTalentBranchView Branch;
	Branch.TalentBranchId = TEXT("Branch");
	FGridTalentNodeView Node;
	Node.TalentNodeId = ConceptNodeId;
	Node.TalentBranchId = Branch.TalentBranchId;
	Node.Tier = 1;
	Node.MinimumLevel = 2;
	Node.PointCost = 1;
	Node.State = EGridTalentNodeState::Available;
	Node.DisplayName = FText::FromString(TEXT("Talent test à variantes"));
	Node.Type = ERPGTalentPresentationType::Passive;
	Node.TypeText = FText::FromString(TEXT("PASSIF"));
	Node.StatusText = FText::FromString(TEXT("DISPONIBLE"));
	Node.Principle = FText::FromString(TEXT("Choisissez une variante."));
	Node.bHasExclusiveVariants = true;

	FGridTalentVariantView VariantA;
	VariantA.ChoiceId = TEXT("Choice_A");
	VariantA.DisplayName = FText::FromString(TEXT("Variante A"));
	VariantA.State = EGridTalentNodeState::Available;
	VariantA.Type = ERPGTalentPresentationType::Passive;
	VariantA.TypeText = FText::FromString(TEXT("PASSIF"));
	VariantA.StatusText = FText::FromString(TEXT("DISPONIBLE"));
	VariantA.Principle = FText::FromString(TEXT("Variante A."));
	VariantA.bCanChoose = true;
	Node.Variants.Add(VariantA);

	FGridTalentVariantView VariantB = VariantA;
	VariantB.ChoiceId = TEXT("Choice_B");
	VariantB.DisplayName = FText::FromString(TEXT("Variante B"));
	Node.Variants.Add(VariantB);

	Branch.Nodes.Add(Node);
	Skills->View.TalentTree.Branches.Add(Branch);
	Skills->SelectedTalentNodeId = ConceptNodeId;

	FText Feedback;
	TestTrue(TEXT("Confirmed variant ChoiceId commits through the existing transaction authority"),
		Skills->CommitConfirmedTalentChoice(TEXT("Choice_B"), Feedback));

	TArray<FName> Selected;
	TestTrue(TEXT("Committed choices remain readable"),
		FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds(Component, 0, Selected));
	TestTrue(TEXT("Requested variant is persisted"), Selected.Contains(TEXT("Choice_B")));
	TestFalse(TEXT("Sibling variant is not persisted"), Selected.Contains(TEXT("Choice_A")));
	TestEqual(TEXT("Exactly one exclusive variant is committed"), Selected.Num(), 1);

	FGridSkillsPageView Projected;
	TestTrue(TEXT("Post-commit read-model rebuild succeeds"),
		FGridSkillsPageService::TryBuildCharacterView(Component, 0, {}, Projected));

	const FGridTalentNodeView* ProjectedNode = nullptr;
	for (const FGridTalentBranchView& ProjectedBranch : Projected.TalentTree.Branches)
	{
		ProjectedNode = ProjectedBranch.Nodes.FindByPredicate(
			[ConceptNodeId](const FGridTalentNodeView& Candidate)
			{
				return Candidate.TalentNodeId == ConceptNodeId;
			});
		if (ProjectedNode) break;
	}
	TestNotNull(TEXT("Committed conceptual node remains projected"), ProjectedNode);
	if (!ProjectedNode)
	{
		return false;
	}

	TestEqual(TEXT("Conceptual node becomes ACQUIS"), ProjectedNode->StatusText.ToString(), FString(TEXT("ACQUIS")));
	TestEqual(TEXT("Committed ChoiceId becomes the selected variant"),
		ProjectedNode->SelectedChoiceId, FName(TEXT("Choice_B")));

	const FGridTalentVariantView* ProjectedA = ProjectedNode->Variants.FindByPredicate(
		[](const FGridTalentVariantView& Variant) { return Variant.ChoiceId == TEXT("Choice_A"); });
	const FGridTalentVariantView* ProjectedB = ProjectedNode->Variants.FindByPredicate(
		[](const FGridTalentVariantView& Variant) { return Variant.ChoiceId == TEXT("Choice_B"); });
	TestNotNull(TEXT("Sibling variant remains visible"), ProjectedA);
	TestNotNull(TEXT("Committed variant remains visible"), ProjectedB);
	if (!ProjectedA || !ProjectedB)
	{
		return false;
	}

	TestTrue(TEXT("Committed variant is acquired"), ProjectedB->bAcquired);
	TestEqual(TEXT("Committed variant status is ACQUIS"), ProjectedB->StatusText.ToString(), FString(TEXT("ACQUIS")));
	TestFalse(TEXT("Committed variant no longer exposes CHOISIR"), ProjectedB->bCanChoose);

	TestFalse(TEXT("Sibling variant is not acquired"), ProjectedA->bAcquired);
	TestEqual(TEXT("Sibling variant becomes exclusive-unavailable"),
		ProjectedA->State, EGridTalentNodeState::LockedExclusive);
	TestEqual(TEXT("Sibling variant status is player-facing INDISPONIBLE"),
		ProjectedA->StatusText.ToString(), FString(TEXT("INDISPONIBLE — autre variante déjà choisie")));
	TestFalse(TEXT("Sibling variant no longer exposes CHOISIR"), ProjectedA->bCanChoose);
	return true;
}

#endif
