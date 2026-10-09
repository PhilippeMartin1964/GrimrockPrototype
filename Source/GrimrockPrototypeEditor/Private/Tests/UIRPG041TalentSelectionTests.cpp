#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

#include "UI/GridSkillsWidget.h"
#include "UI/GridTalentBranchWidget.h"
#include "UI/GridTalentNodeWidget.h"

namespace UIRPG041Tests
{
	FGridSkillsPageView MakeView()
	{
		FGridSkillsPageView View;
		View.CharacterIndex = 0;
		View.CharacterId = FGuid::NewGuid();
		View.ClassId = TEXT("Warrior");
		View.TalentTree.ClassId = View.ClassId;

		FGridTalentBranchView Branch;
		Branch.TalentBranchId = TEXT("Guardian");

		FGridTalentNodeView Node;
		Node.TalentNodeId = TEXT("Talent_Warrior_Guardian_Test");
		Node.TalentBranchId = Branch.TalentBranchId;
		Node.Tier = 1;
		Node.MinimumLevel = 2;
		Node.PointCost = 1;
		Node.State = EGridTalentNodeState::Available;
		Node.DisplayName = FText::FromString(TEXT("Test"));
		Node.Type = ERPGTalentPresentationType::Passive;
		Node.TypeText = FText::FromString(TEXT("PASSIF"));
		Node.StatusText = FText::FromString(TEXT("DISPONIBLE"));
		Node.Principle = FText::FromString(TEXT("Description"));
		Node.SimpleChoiceId = Node.TalentNodeId;
		Node.bCanAcquireSimple = true;

		Branch.Nodes.Add(Node);
		View.TalentTree.Branches.Add(Branch);
		return View;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG041KnownNodeSelectionTest,
	"Grimrock.UI.RPG04.Selection.KnownNode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG041KnownNodeSelectionTest::RunTest(const FString&)
{
	using namespace UIRPG041Tests;
	UGridSkillsWidget* Widget = NewObject<UGridSkillsWidget>();
	Widget->View = MakeView();

	const FName NodeId = Widget->View.TalentTree.Branches[0].Nodes[0].TalentNodeId;
	TestTrue(TEXT("Known conceptual Talent node can be selected"), Widget->SelectTalentNode(NodeId));
	TestEqual(TEXT("Selection stores only the conceptual TalentNodeId"), Widget->SelectedTalentNodeId, NodeId);

	FGridTalentNodeView Selected;
	TestTrue(TEXT("Selected node resolves from the current read model"), Widget->GetSelectedTalentNode(Selected));
	TestEqual(TEXT("Resolved selected node keeps its TalentNodeId"), Selected.TalentNodeId, NodeId);

	Widget->ClearTalentSelection();
	TestTrue(TEXT("Clear resets presentation-only selection"), Widget->SelectedTalentNodeId.IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG041UnknownNodeSelectionTest,
	"Grimrock.UI.RPG04.Selection.UnknownNode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG041UnknownNodeSelectionTest::RunTest(const FString&)
{
	using namespace UIRPG041Tests;
	UGridSkillsWidget* Widget = NewObject<UGridSkillsWidget>();
	Widget->View = MakeView();

	TestFalse(TEXT("Unknown conceptual Talent node is rejected"), Widget->SelectTalentNode(TEXT("Talent_Unknown")));
	TestTrue(TEXT("Rejected selection creates no UI state"), Widget->SelectedTalentNodeId.IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG041RoutingContractTest,
	"Grimrock.UI.RPG04.Selection.RoutingContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG041RoutingContractTest::RunTest(const FString&)
{
	UClass* NodeClass = UGridTalentNodeWidget::StaticClass();
	UClass* BranchClass = UGridTalentBranchWidget::StaticClass();
	UClass* SkillsClass = UGridSkillsWidget::StaticClass();

	TestNotNull(TEXT("Node exposes click delegate"),
		FindFProperty<FMulticastDelegateProperty>(NodeClass, TEXT("OnTalentNodeClicked")));
	TestNotNull(TEXT("Branch exposes node-click rebroadcast delegate"),
		FindFProperty<FMulticastDelegateProperty>(BranchClass, TEXT("OnTalentNodeClicked")));
	TestNotNull(TEXT("Skills exposes presentation selection delegate"),
		FindFProperty<FMulticastDelegateProperty>(SkillsClass, TEXT("OnTalentSelectionChanged")));

	TestNotNull(TEXT("Branch owns native node-click handler"),
		BranchClass->FindFunctionByName(TEXT("HandleTalentNodeClicked")));
	TestNotNull(TEXT("Skills owns native node-click handler"),
		SkillsClass->FindFunctionByName(TEXT("HandleTalentNodeClicked")));

	TestNotNull(TEXT("Skills exposes SelectTalentNode"), SkillsClass->FindFunctionByName(TEXT("SelectTalentNode")));
	TestNotNull(TEXT("Skills exposes ClearTalentSelection"), SkillsClass->FindFunctionByName(TEXT("ClearTalentSelection")));
	TestNotNull(TEXT("Skills exposes GetSelectedTalentNode"), SkillsClass->FindFunctionByName(TEXT("GetSelectedTalentNode")));
	return true;
}

#endif
