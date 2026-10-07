#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPGMON155TestHelpers.h"
#include "UI/GridSkillsWidget.h"
#include "UI/GridTalentDetailWidget.h"

namespace UIRPG043ATests
{
	FGridTalentVariantView MakeVariant(const TCHAR* ChoiceId, const TCHAR* Name)
	{
		FGridTalentVariantView Variant;
		Variant.ChoiceId = ChoiceId;
		Variant.DisplayName = FText::FromString(Name);
		Variant.Description = FText::FromString(TEXT("Description"));
		Variant.State = EGridTalentNodeState::Available;
		Variant.bAvailable = true;
		return Variant;
	}

	FGridTalentNodeView MakeNode(FName NodeId, FName BranchId, FName ChoiceId)
	{
		FGridTalentNodeView Node;
		Node.TalentNodeId = NodeId;
		Node.TalentBranchId = BranchId;
		Node.Tier = 1;
		Node.MinimumLevel = 2;
		Node.PointCost = 1;
		Node.State = EGridTalentNodeState::Available;
		Node.Variants.Add(MakeVariant(*ChoiceId.ToString(), TEXT("Talent simple")));
		return Node;
	}

	FRPGTalentBranchPresentationDefinition MakeBranch(FName BranchId)
	{
		FRPGTalentBranchPresentationDefinition Branch;
		Branch.TalentBranchId = BranchId;
		Branch.DisplayName = FText::FromString(TEXT("Branche"));
		return Branch;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG043AConfirmationStateTest,
	"Grimrock.UI.RPG04.Acquisition.ConfirmationState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG043AConfirmationStateTest::RunTest(const FString&)
{
	using namespace UIRPG043ATests;
	const FName BranchId(TEXT("Guardian"));
	FGridTalentNodeView Node = MakeNode(TEXT("Talent_A"), BranchId, TEXT("Choice_A"));

	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
	TestTrue(TEXT("Detail initializes"), Detail->InitializeTalentDetail(Node, MakeBranch(BranchId)));
	TestTrue(TEXT("Available one-choice node can request acquisition"), Detail->CanRequestSimpleAcquisition());
	TestTrue(TEXT("Acquire starts confirmation"), Detail->BeginAcquireConfirmation());
	TestTrue(TEXT("Confirmation becomes pending"), Detail->bAcquireConfirmationPending);
	Detail->CancelAcquireConfirmation();
	TestFalse(TEXT("Cancel clears confirmation"), Detail->bAcquireConfirmationPending);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG043AVariantDeferredTest,
	"Grimrock.UI.RPG04.Acquisition.VariantDeferred",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG043AVariantDeferredTest::RunTest(const FString&)
{
	using namespace UIRPG043ATests;
	const FName BranchId(TEXT("WeaponMaster"));
	FGridTalentNodeView Node = MakeNode(TEXT("Talent_Variant"), BranchId, TEXT("Choice_A"));
	Node.Variants.Add(MakeVariant(TEXT("Choice_B"), TEXT("Variante B")));

	FRPGTalentBranchPresentationDefinition Branch = MakeBranch(BranchId);
	FRPGTalentNodePresentationDefinition Override;
	Override.TalentNodeId = Node.TalentNodeId;
	Override.DisplayName = FText::FromString(TEXT("Talent à variantes"));
	Override.Description = FText::FromString(TEXT("Choisissez une variante."));
	Branch.NodePresentationOverrides.Add(Override);

	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
	TestTrue(TEXT("Variant detail initializes"), Detail->InitializeTalentDetail(Node, Branch));
	TestFalse(TEXT("Multi-choice node is deferred to UI-RPG04.4"), Detail->CanRequestSimpleAcquisition());
	TestFalse(TEXT("Variant node cannot enter simple confirmation"), Detail->BeginAcquireConfirmation());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG043ASimpleCommitTest,
	"Grimrock.UI.RPG04.Acquisition.SimpleCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG043ASimpleCommitTest::RunTest(const FString&)
{
	using namespace UIRPG043ATests;
	FMON155RuntimeStateGuard RuntimeGuard;
	URPGClassAsset* ClassDefinition = nullptr;
	UGridPartyInventoryComponent* Component = MakeMON155Inventory(2, 1000, ClassDefinition);
	const FGridCharacterInventoryState& Character = Component->PartyInventoryState.ActiveCharacters[0];

	UGridSkillsWidget* Skills = NewObject<UGridSkillsWidget>();
	Skills->InventoryComponent = Component;
	Skills->View.CharacterIndex = 0;
	Skills->View.CharacterId = Character.CharacterId;
	Skills->View.ClassId = Character.ClassId;
	Skills->View.TalentTree.ClassId = Character.ClassId;

	FGridTalentBranchView Branch;
	Branch.TalentBranchId = TEXT("Guardian");
	FGridTalentNodeView Node = MakeNode(TEXT("Talent_A"), Branch.TalentBranchId, TEXT("Choice_A"));
	Branch.Nodes.Add(Node);
	Skills->View.TalentTree.Branches.Add(Branch);
	Skills->SelectedTalentNodeId = Node.TalentNodeId;

	FText Feedback;
	TestTrue(TEXT("Confirmed simple Talent commits through the existing transaction service"),
		Skills->CommitConfirmedTalentChoice(TEXT("Choice_A"), Feedback));

	TArray<FName> Selected;
	TestTrue(TEXT("Committed state remains readable"),
		FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds(Component, 0, Selected));
	TestTrue(TEXT("Choice A is now authoritative character progression state"), Selected.Contains(TEXT("Choice_A")));
	TestEqual(TEXT("Exactly one choice was committed"), Selected.Num(), 1);
	return true;
}

#endif
