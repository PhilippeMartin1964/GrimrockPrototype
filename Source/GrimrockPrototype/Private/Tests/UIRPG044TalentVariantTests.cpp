#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPGMON155TestHelpers.h"
#include "UI/GridSkillsWidget.h"
#include "UI/GridTalentDetailWidget.h"

namespace UIRPG044Tests
{
	FGridTalentVariantView MakeVariant(FName ChoiceId, const TCHAR* DisplayName)
	{
		FGridTalentVariantView Variant;
		Variant.ChoiceId = ChoiceId;
		Variant.DisplayName = FText::FromString(DisplayName);
		Variant.Description = FText::FromString(TEXT("Description"));
		Variant.State = EGridTalentNodeState::Available;
		Variant.bAvailable = true;
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
	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
	TestTrue(TEXT("Four-variant conceptual node initializes"),
		Detail->InitializeTalentDetail(MakeVariantNode(4), MakePresentation()));
	TestFalse(TEXT("Multi-variant node is not a simple acquisition"), Detail->CanRequestSimpleAcquisition());
	TestTrue(TEXT("Multi-variant node can open generic selector"), Detail->CanRequestVariantAcquisition());
	TestTrue(TEXT("Variant selector enters pending state"), Detail->BeginVariantSelection());
	TestTrue(TEXT("Variant pending flag is set"), Detail->bVariantSelectionPending);
	TestTrue(TEXT("A concrete available ChoiceId can be selected"), Detail->SelectVariantChoice(TEXT("Choice_3")));
	TestEqual(TEXT("Selected variant ChoiceId is stable"), Detail->GetSelectedVariantChoiceId(), FName(TEXT("Choice_3")));
	TestTrue(TEXT("Selected variant can be confirmed"), Detail->ConfirmAcquire());
	TestFalse(TEXT("Variant pending state clears after confirmation"), Detail->bVariantSelectionPending);
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
	FMON155RuntimeStateGuard RuntimeGuard;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(2, 1000, ClassDefinition);

	FRPGClassProgressionChoiceDefinition* ChoiceA = ClassDefinition->ProgressionChoices.FindByPredicate(
		[](const FRPGClassProgressionChoiceDefinition& Choice) { return Choice.ChoiceId == TEXT("Choice_A"); });
	FRPGClassProgressionChoiceDefinition* ChoiceB = ClassDefinition->ProgressionChoices.FindByPredicate(
		[](const FRPGClassProgressionChoiceDefinition& Choice) { return Choice.ChoiceId == TEXT("Choice_B"); });
	TestNotNull(TEXT("Choice A exists"), ChoiceA);
	TestNotNull(TEXT("Choice B exists"), ChoiceB);
	if (!ChoiceA || !ChoiceB)
	{
		return false;
	}

	const FName ConceptNodeId(TEXT("Talent_Test_Variant"));
	const FName ExclusiveGroup(TEXT("TalentGroup_Test_Variant"));
	ChoiceA->TalentNodeId = ConceptNodeId;
	ChoiceB->TalentNodeId = ConceptNodeId;
	ChoiceA->ExclusiveChoiceGroupId = ExclusiveGroup;
	ChoiceB->ExclusiveChoiceGroupId = ExclusiveGroup;
	ChoiceB->PrerequisiteChoiceIds.Reset();
	ChoiceB->MinimumLevel = 2;

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

	FGridTalentVariantView VariantA;
	VariantA.ChoiceId = TEXT("Choice_A");
	VariantA.DisplayName = FText::FromString(TEXT("Variante A"));
	VariantA.State = EGridTalentNodeState::Available;
	VariantA.bAvailable = true;
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
	return true;
}

#endif
