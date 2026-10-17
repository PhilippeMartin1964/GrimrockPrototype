#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

#include "UI/GridSkillsUiTypes.h"
#include "UI/GridTalentDetailWidget.h"

namespace UIRPGTalentCodeAudit024
{
	FGridTalentNodeView MakeSimpleNode()
	{
		FGridTalentNodeView Node;
		Node.TalentNodeId = TEXT("Talent_Audit024_Simple");
		Node.TalentBranchId = TEXT("Branch_Audit024");
		Node.Tier = 3;
		Node.MinimumLevel = 6;
		Node.PointCost = 1;
		Node.State = EGridTalentNodeState::Available;
		Node.PreviousNodeId = TEXT("Talent_Audit024_Previous");
		Node.PreviousNodeDisplayName = FText::FromString(TEXT("Précédent"));
		Node.DisplayName = FText::FromString(TEXT("Talent simple"));
		Node.Type = ERPGTalentPresentationType::Passive;
		Node.Principle = FText::FromString(TEXT("Principe simple."));
		Node.Acquisition.MinimumLevel = 6;
		Node.Acquisition.PointCost = 1;
		Node.SimpleChoiceId = TEXT("Choice_Audit024_Simple");
		Node.bCanAcquireSimple = true;
		return Node;
	}

	FGridTalentNodeView MakeVariantNode()
	{
		FGridTalentNodeView Node = MakeSimpleNode();
		Node.TalentNodeId = TEXT("Talent_Audit024_Variant");
		Node.DisplayName = FText::FromString(TEXT("Talent conceptuel"));
		Node.Principle = FText::FromString(TEXT("Choisissez une variante."));
		Node.SimpleChoiceId = NAME_None;
		Node.bCanAcquireSimple = false;
		Node.bHasExclusiveVariants = true;

		FGridTalentVariantView Fire;
		Fire.ChoiceId = TEXT("Choice_Audit024_Fire");
		Fire.DisplayName = FText::FromString(TEXT("Variante Feu"));
		Fire.State = EGridTalentNodeState::Available;
		Fire.Type = ERPGTalentPresentationType::Passive;
		Fire.Principle = FText::FromString(TEXT("Bonus de feu."));
		Fire.bCanChoose = true;

		FGridTalentVariantView Ice = Fire;
		Ice.ChoiceId = TEXT("Choice_Audit024_Ice");
		Ice.DisplayName = FText::FromString(TEXT("Variante Glace"));
		Ice.Principle = FText::FromString(TEXT("Bonus de glace."));
		Node.Variants = { Fire, Ice };
		return Node;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGTalentCodeAudit024ShapeTest,
	"Grimrock.UI.RPG.CODEAUDIT02.TalentDetailReadModel.Shape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGTalentCodeAudit024ShapeTest::RunTest(const FString&)
{
	UScriptStruct* DetailStruct = FGridTalentDetailView::StaticStruct();
	TestNotNull(TEXT("Dedicated Talent detail struct exists"), DetailStruct);
	if (!DetailStruct)
	{
		return false;
	}

	for (const TCHAR* PropertyName : {
		TEXT("TalentNodeId"),
		TEXT("TalentBranchId"),
		TEXT("ChoiceId"),
		TEXT("DisplayName"),
		TEXT("State"),
		TEXT("Type"),
		TEXT("Principle"),
		TEXT("Effects"),
		TEXT("Usage"),
		TEXT("Acquisition"),
		TEXT("bCanAcquire")
	})
	{
		TestNotNull(*FString::Printf(TEXT("%s belongs to the dedicated detail contract"), PropertyName),
			FindFProperty<FProperty>(DetailStruct, FName(PropertyName)));
	}

	for (const TCHAR* PropertyName : {
		TEXT("Tier"),
		TEXT("PreviousNodeId"),
		TEXT("PreviousNodeDisplayName"),
		TEXT("SelectedChoiceId"),
		TEXT("SimpleChoiceId"),
		TEXT("bCanAcquireSimple"),
		TEXT("bHasExclusiveVariants"),
		TEXT("Variants")
	})
	{
		TestNull(*FString::Printf(TEXT("%s tree-only state is absent from detail"), PropertyName),
			FindFProperty<FProperty>(DetailStruct, FName(PropertyName)));
	}

	FProperty* DetailViewProperty =
		FindFProperty<FProperty>(UGridTalentDetailWidget::StaticClass(), TEXT("DetailView"));
	TestNotNull(TEXT("Talent detail widget exposes the dedicated DetailView"), DetailViewProperty);
	const FStructProperty* DetailStructProperty = CastField<FStructProperty>(DetailViewProperty);
	TestTrue(TEXT("DetailView uses FGridTalentDetailView"),
		DetailStructProperty && DetailStructProperty->Struct == FGridTalentDetailView::StaticStruct());
	TestNull(TEXT("Talent detail widget no longer stores a fake NodeView"),
		FindFProperty<FProperty>(UGridTalentDetailWidget::StaticClass(), TEXT("NodeView")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGTalentCodeAudit024ProjectionTest,
	"Grimrock.UI.RPG.CODEAUDIT02.TalentDetailReadModel.Projection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGTalentCodeAudit024ProjectionTest::RunTest(const FString&)
{
	using namespace UIRPGTalentCodeAudit024;

	const FGridTalentNodeView SimpleNode = MakeSimpleNode();
	FGridTalentDetailView SimpleDetail;
	TestTrue(TEXT("Simple node projects to detail"),
		FGridTalentDetailView::TryBuildFromNode(SimpleNode, SimpleNode.SimpleChoiceId, SimpleDetail));
	TestEqual(TEXT("Simple detail keeps conceptual node identity"), SimpleDetail.TalentNodeId, SimpleNode.TalentNodeId);
	TestEqual(TEXT("Simple detail exposes the concrete ChoiceId"), SimpleDetail.ChoiceId, SimpleNode.SimpleChoiceId);
	TestTrue(TEXT("Available simple detail is acquirable"), SimpleDetail.bCanAcquire);
	TestEqual(TEXT("Acquisition is projected without tree topology"), SimpleDetail.Acquisition.MinimumLevel, 6);

	const FGridTalentNodeView VariantNode = MakeVariantNode();
	FGridTalentDetailView VariantDetail;
	TestTrue(TEXT("Requested concrete variant projects to detail"),
		FGridTalentDetailView::TryBuildFromNode(VariantNode, TEXT("Choice_Audit024_Ice"), VariantDetail));
	TestEqual(TEXT("Variant detail keeps conceptual TalentNodeId"), VariantDetail.TalentNodeId, VariantNode.TalentNodeId);
	TestEqual(TEXT("Variant detail owns the concrete ChoiceId"), VariantDetail.ChoiceId, FName(TEXT("Choice_Audit024_Ice")));
	TestEqual(TEXT("Variant detail uses concrete display name"), VariantDetail.DisplayName.ToString(), FString(TEXT("Variante Glace")));
	TestEqual(TEXT("Variant detail uses concrete principle"), VariantDetail.Principle.ToString(), FString(TEXT("Bonus de glace.")));
	TestTrue(TEXT("Available concrete variant is acquirable"), VariantDetail.bCanAcquire);

	FGridTalentDetailView UnknownDetail;
	TestFalse(TEXT("Unknown variant cannot fabricate a detail"),
		FGridTalentDetailView::TryBuildFromNode(VariantNode, TEXT("Choice_Audit024_Unknown"), UnknownDetail));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGTalentCodeAudit024ConceptualTest,
	"Grimrock.UI.RPG.CODEAUDIT02.TalentDetailReadModel.Conceptual",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGTalentCodeAudit024ConceptualTest::RunTest(const FString&)
{
	using namespace UIRPGTalentCodeAudit024;
	const FGridTalentNodeView Node = MakeVariantNode();

	FGridTalentDetailView Detail;
	TestTrue(TEXT("Conceptual diagnostic detail remains representable without tree payload"),
		FGridTalentDetailView::TryBuildFromNode(Node, NAME_None, Detail));
	TestTrue(TEXT("Conceptual detail has no concrete ChoiceId"), Detail.ChoiceId.IsNone());
	TestFalse(TEXT("Conceptual detail is never directly acquirable"), Detail.bCanAcquire);
	TestEqual(TEXT("Conceptual detail keeps conceptual display name"),
		Detail.DisplayName.ToString(), FString(TEXT("Talent conceptuel")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
