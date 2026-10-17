#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/GridTalentDetailWidget.h"
#include "UI/GridTalentNodeWidget.h"

namespace UIRPG042Tests
{
	FGridTalentVariantView MakeVariant(const TCHAR* ChoiceId, const TCHAR* Name)
	{
		FGridTalentVariantView Variant;
		Variant.ChoiceId = ChoiceId;
		Variant.DisplayName = FText::FromString(Name);
		Variant.State = EGridTalentNodeState::Available;
		Variant.Type = ERPGTalentPresentationType::Passive;
		Variant.Principle = FText::FromString(TEXT("Description"));
		Variant.bCanChoose = true;
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
		Node.Type = ERPGTalentPresentationType::Passive;
		Node.Acquisition.MinimumLevel = 2;
		Node.Acquisition.PointCost = 1;
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
	Node.DisplayName = FText::FromString(TEXT("Talent simple"));
	Node.Principle = FText::FromString(TEXT("Description"));
	Node.SimpleChoiceId = Node.TalentNodeId;
	Node.bCanAcquireSimple = true;

	FGridTalentDetailView TalentDetail;
	TestTrue(TEXT("Simple node projects to dedicated detail"),
		FGridTalentDetailView::TryBuildFromNode(Node, Node.SimpleChoiceId, TalentDetail));
	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
	TestTrue(TEXT("Simple detail initializes"), Detail->InitializeTalentDetail(TalentDetail, Branch));
	TestEqual(TEXT("Simple detail reuses Choice display name"), Detail->ResolvedDisplayName.ToString(), FString(TEXT("Talent simple")));
	TestEqual(TEXT("Detail keeps acquisition cost"), Detail->DetailView.Acquisition.PointCost, 1);
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
	Node.DisplayName = FText::FromString(TEXT("Spécialisation martiale"));
	Node.Principle = FText::FromString(TEXT("Choisissez une spécialisation."));
	Node.bHasExclusiveVariants = true;
	Node.Variants.Add(MakeVariant(TEXT("Choice_Slashing"), TEXT("Tranchant")));
	Node.Variants.Add(MakeVariant(TEXT("Choice_Piercing"), TEXT("Perforant")));
	Node.Variants.Add(MakeVariant(TEXT("Choice_Bludgeoning"), TEXT("Contondant")));

	FGridTalentDetailView TalentDetail;
	TestTrue(TEXT("Conceptual variant node projects to dedicated detail"),
		FGridTalentDetailView::TryBuildFromNode(Node, NAME_None, TalentDetail));
	UGridTalentDetailWidget* Detail = NewObject<UGridTalentDetailWidget>();
	TestTrue(TEXT("Variant detail initializes from canonical conceptual projection"), Detail->InitializeTalentDetail(TalentDetail, Branch));
	TestEqual(TEXT("Variant detail uses conceptual name"), Detail->ResolvedDisplayName.ToString(), FString(TEXT("Spécialisation martiale")));
	TestTrue(TEXT("Conceptual detail carries no concrete ChoiceId"), Detail->DetailView.ChoiceId.IsNone());
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
	Node.DisplayName = FText::FromString(TEXT("Nom canonique"));
	Node.Principle = FText::FromString(TEXT("Description canonique."));
	Node.SimpleChoiceId = TEXT("Choice");

	UGridTalentNodeWidget* Widget = NewObject<UGridTalentNodeWidget>();
	TestTrue(TEXT("Canonical node projection initializes without a secondary resolver"),
		Widget->InitializeTalentNode(Node, Branch));
	TestEqual(TEXT("Canonical display name is consumed directly"),
		Widget->ResolvedDisplayName.ToString(), FString(TEXT("Nom canonique")));
	TestEqual(TEXT("Canonical principle is consumed directly"),
		Widget->ResolvedDescription.ToString(), FString(TEXT("Description canonique.")));
	return true;
}

#endif
