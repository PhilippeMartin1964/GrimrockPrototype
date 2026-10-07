#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/GridTalentDetailWidget.h"

namespace UIRPG042Tests
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

	FGridTalentNodeView MakeNode(FName NodeId, FName BranchId)
	{
		FGridTalentNodeView Node;
		Node.TalentNodeId = NodeId;
		Node.TalentBranchId = BranchId;
		Node.Tier = 1;
		Node.MinimumLevel = 2;
		Node.PointCost = 1;
		Node.State = EGridTalentNodeState::Available;
		return Node;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG042SimpleDetailTest,
	"Grimrock.UI.RPG04.Detail.SimpleNode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG042SimpleDetailTest::RunTest(const FString&)
{
	using namespace UIRPG042Tests;
	FRPGTalentBranchPresentationDefinition Branch;
	Branch.TalentBranchId = TEXT("Guardian");
	Branch.AccentColor = FLinearColor(0.3f, 0.2f, 0.1f, 1.0f);

	FGridTalentNodeView Node = MakeNode(TEXT("Talent_Test"), Branch.TalentBranchId);
	Node.Variants.Add(MakeVariant(TEXT("Talent_Test"), TEXT("Talent simple")));

	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
	TestTrue(TEXT("Simple detail initializes"), Detail->InitializeTalentDetail(Node, Branch));
	TestEqual(TEXT("Simple detail reuses Choice display name"), Detail->ResolvedDisplayName.ToString(), FString(TEXT("Talent simple")));
	TestEqual(TEXT("Detail keeps node cost"), Detail->NodeView.PointCost, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG042VariantDetailTest,
	"Grimrock.UI.RPG04.Detail.VariantNode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG042VariantDetailTest::RunTest(const FString&)
{
	using namespace UIRPG042Tests;
	FRPGTalentBranchPresentationDefinition Branch;
	Branch.TalentBranchId = TEXT("WeaponMaster");

	FGridTalentNodeView Node = MakeNode(TEXT("Talent_MartialSpecialization"), Branch.TalentBranchId);
	Node.Variants.Add(MakeVariant(TEXT("Choice_Slashing"), TEXT("Tranchant")));
	Node.Variants.Add(MakeVariant(TEXT("Choice_Piercing"), TEXT("Perforant")));
	Node.Variants.Add(MakeVariant(TEXT("Choice_Bludgeoning"), TEXT("Contondant")));

	FRPGTalentNodePresentationDefinition Override;
	Override.TalentNodeId = Node.TalentNodeId;
	Override.DisplayName = FText::FromString(TEXT("Spécialisation martiale"));
	Override.Description = FText::FromString(TEXT("Choisissez une spécialisation."));
	Branch.NodePresentationOverrides.Add(Override);

	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
	TestTrue(TEXT("Variant detail initializes through conceptual override"), Detail->InitializeTalentDetail(Node, Branch));
	TestEqual(TEXT("Variant detail uses conceptual name"), Detail->ResolvedDisplayName.ToString(), FString(TEXT("Spécialisation martiale")));
	TestEqual(TEXT("Variant detail preserves three choices"), Detail->NodeView.Variants.Num(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG042SharedResolverTest,
	"Grimrock.UI.RPG04.Detail.SharedResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG042SharedResolverTest::RunTest(const FString&)
{
	using namespace UIRPG042Tests;
	FRPGTalentBranchPresentationDefinition Branch;
	Branch.TalentBranchId = TEXT("Branch");

	FGridTalentNodeView Node = MakeNode(TEXT("Node"), Branch.TalentBranchId);
	Node.Variants.Add(MakeVariant(TEXT("Choice"), TEXT("Nom partagé")));

	FText Name;
	FText Description;
	TestTrue(TEXT("Shared resolver accepts simple node"),
		UGridTalentNodeWidget::ResolvePresentationText(Node, Branch, Name, Description));
	TestEqual(TEXT("Shared resolver returns canonical display name"), Name.ToString(), FString(TEXT("Nom partagé")));
	return true;
}

#endif
