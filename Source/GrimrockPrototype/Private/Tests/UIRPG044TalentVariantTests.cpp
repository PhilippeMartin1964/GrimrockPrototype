#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPGMON155TestHelpers.h"
#include "UI/GridSkillsPageService.h"
#include "UI/GridSkillsWidget.h"

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

	UGridSkillsWidget* Skills = NewObject<UGridSkillsWidget>();
	Skills->View.CharacterIndex = 0;
	Skills->View.CharacterId = FGuid::NewGuid();
	Skills->View.ClassId = TEXT("Mage");
	Skills->View.TalentTree.ClassId = Skills->View.ClassId;

	FGridTalentBranchView Branch;
	Branch.TalentBranchId = TEXT("Branch");
	FGridTalentNodeView Node = MakeVariantNode(4);
	Branch.Nodes.Add(Node);
	Skills->View.TalentTree.Branches.Add(Branch);

	TestEqual(TEXT("Conceptual node exposes four flyout choices"), Node.Variants.Num(), 4);
	for (const FGridTalentVariantView& Variant : Node.Variants)
	{
		TestTrue(
			*FString::Printf(TEXT("%s can be selected for consultation"), *Variant.ChoiceId.ToString()),
			Skills->SelectTalentVariant(Node.TalentNodeId, Variant.ChoiceId));
		TestEqual(TEXT("Consultation keeps conceptual node selected"), Skills->SelectedTalentNodeId, Node.TalentNodeId);
		TestEqual(TEXT("Consultation stores concrete ChoiceId"), Skills->SelectedTalentChoiceId, Variant.ChoiceId);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG044AuthoredVariantLabelsTest,
	"Grimrock.UI.RPG04.Variants.AuthoredLabels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG044AuthoredVariantLabelsTest::RunTest(const FString&)
{
	using namespace UIRPG044Tests;
	const FGridTalentNodeView Node = MakeVariantNode(3);

	TestEqual(TEXT("Three authored variants remain in the read model"), Node.Variants.Num(), 3);
	TestEqual(TEXT("First authored variant label is preserved verbatim"),
		Node.Variants[0].DisplayName.ToString(), FString(TEXT("Talent conceptuel — Feu")));
	TestEqual(TEXT("Third authored variant label is preserved verbatim"),
		Node.Variants[2].DisplayName.ToString(), FString(TEXT("Talent conceptuel — Air")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG044FlyoutConsultationSelectionTest,
	"Grimrock.UI.RPG04.Variants.FlyoutConsultationSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG044FlyoutConsultationSelectionTest::RunTest(const FString&)
{
	using namespace UIRPG044Tests;

	UGridSkillsWidget* Skills = NewObject<UGridSkillsWidget>();
	Skills->View.CharacterIndex = 0;
	Skills->View.CharacterId = FGuid::NewGuid();
	Skills->View.ClassId = TEXT("Mage");
	Skills->View.TalentTree.ClassId = Skills->View.ClassId;

	FGridTalentBranchView Branch;
	Branch.TalentBranchId = TEXT("SurfaceWeaver");
	FGridTalentNodeView Node = MakeVariantNode(4);
	Node.TalentBranchId = Branch.TalentBranchId;
	for (FGridTalentVariantView& Variant : Node.Variants)
	{
		Variant.State = EGridTalentNodeState::LockedLevel;
		Variant.bCanChoose = false;
	}
	Branch.Nodes.Add(Node);
	Skills->View.TalentTree.Branches.Add(Branch);

	const FName ChoiceId(TEXT("Choice_2"));
	TestTrue(TEXT("Locked variant remains consultable through flyout selection"),
		Skills->SelectTalentVariant(Node.TalentNodeId, ChoiceId));
	TestEqual(TEXT("Flyout selection keeps conceptual node authority"),
		Skills->SelectedTalentNodeId, Node.TalentNodeId);
	TestEqual(TEXT("Flyout selection stores concrete consultation ChoiceId only in UI state"),
		Skills->SelectedTalentChoiceId, ChoiceId);

	TestTrue(TEXT("Selecting conceptual node clears transient variant consultation"),
		Skills->SelectTalentNode(Node.TalentNodeId));
	TestTrue(TEXT("Concrete consultation state is cleared"), Skills->SelectedTalentChoiceId.IsNone());

	Node.SelectedChoiceId = TEXT("Choice_0");
	Skills->View.TalentTree.Branches[0].Nodes[0] = Node;
	TestFalse(TEXT("Acquired variant node cannot reopen a different flyout choice"),
		Skills->SelectTalentVariant(Node.TalentNodeId, TEXT("Choice_1")));
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
	Node.Principle = FText::FromString(TEXT("Choisissez une variante."));
	Node.bHasExclusiveVariants = true;

	FGridTalentVariantView VariantA;
	VariantA.ChoiceId = TEXT("Choice_A");
	VariantA.DisplayName = FText::FromString(TEXT("Variante A"));
	VariantA.State = EGridTalentNodeState::Available;
	VariantA.Type = ERPGTalentPresentationType::Passive;
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

	TestEqual(TEXT("Conceptual node becomes acquired"), ProjectedNode->State, EGridTalentNodeState::Acquired);
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
	TestEqual(TEXT("Committed variant state is acquired"), ProjectedB->State, EGridTalentNodeState::Acquired);
	TestFalse(TEXT("Committed variant no longer exposes CHOISIR"), ProjectedB->bCanChoose);

	TestFalse(TEXT("Sibling variant is not acquired"), ProjectedA->bAcquired);
	TestEqual(TEXT("Sibling variant becomes exclusive-unavailable"),
		ProjectedA->State, EGridTalentNodeState::LockedExclusive);
	TestEqual(TEXT("Sibling variant stays exclusive-unavailable"),
		ProjectedA->State, EGridTalentNodeState::LockedExclusive);
	TestFalse(TEXT("Sibling variant no longer exposes CHOISIR"), ProjectedA->bCanChoose);
	return true;
}

#endif
