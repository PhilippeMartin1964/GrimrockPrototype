#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGClassAsset.h"
#include "UI/GridTalentNodeWidget.h"
#include "UI/RPGTalentPresentationAsset.h"

namespace UIRPG032Tests
{
	const TCHAR* CatalogPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/UI/RPG/DA_RPGTalentPresentation.DA_RPGTalentPresentation");

	struct FClassSpec
	{
		FName ClassId;
		const TCHAR* ObjectPath;
	};

	const FClassSpec ClassSpecs[] = {
		{ TEXT("Warrior"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Warrior.DA_Class_Warrior") },
		{ TEXT("Rogue"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Rogue.DA_Class_Rogue") },
		{ TEXT("Ranger"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Ranger.DA_Class_Ranger") },
		{ TEXT("Mage"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Mage.DA_Class_Mage") },
		{ TEXT("Priest"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Priest.DA_Class_Priest") },
		{ TEXT("Alchemist"), TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Alchemist.DA_Class_Alchemist") }
	};

	int32 TierFromLevel(int32 MinimumLevel)
	{
		switch (MinimumLevel)
		{
			case 2: return 1;
			case 6: return 2;
			case 10: return 3;
			case 14: return 4;
			case 18: return 5;
			default: return 0;
		}
	}

	FGridTalentVariantView MakeVariant(const TCHAR* ChoiceId, const TCHAR* DisplayName)
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG032SimpleNodeResolutionTest,
	"Grimrock.UI.RPG03.NodeBinding.SimpleNodeUsesChoiceText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG032SimpleNodeResolutionTest::RunTest(const FString&)
{
	using namespace UIRPG032Tests;
	FRPGTalentBranchPresentationDefinition Branch;
	Branch.TalentBranchId = TEXT("TestBranch");
	Branch.DisplayName = FText::FromString(TEXT("Test Branch"));

	FGridTalentNodeView Node;
	Node.TalentNodeId = TEXT("Talent_Test");
	Node.TalentBranchId = Branch.TalentBranchId;
	Node.Tier = 1;
	Node.MinimumLevel = 2;
	Node.PointCost = 1;
	Node.State = EGridTalentNodeState::Available;
	Node.DisplayName = FText::FromString(TEXT("Talent réel"));
	Node.Principle = FText::FromString(TEXT("Description"));
	Node.SimpleChoiceId = Node.TalentNodeId;

	UGridTalentNodeWidget* Widget = NewObject<UGridTalentNodeWidget>();
	TestTrue(TEXT("Simple node initializes without presentation override"), Widget->InitializeTalentNode(Node, Branch));
	TestEqual(TEXT("Simple node uses Choice display name"), Widget->ResolvedDisplayName.ToString(), FString(TEXT("Talent réel")));
	TestEqual(TEXT("Simple node has no variant payload"), Widget->GetVariantCount(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG032VariantOverrideTest,
	"Grimrock.UI.RPG03.NodeBinding.VariantNodeRequiresConceptualOverride",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG032VariantOverrideTest::RunTest(const FString&)
{
	using namespace UIRPG032Tests;
	FRPGTalentBranchPresentationDefinition Branch;
	Branch.TalentBranchId = TEXT("TestBranch");
	Branch.DisplayName = FText::FromString(TEXT("Test Branch"));

	FGridTalentNodeView Node;
	Node.TalentNodeId = TEXT("Talent_Test_Variant");
	Node.TalentBranchId = Branch.TalentBranchId;
	Node.Tier = 1;
	Node.MinimumLevel = 2;
	Node.PointCost = 1;
	Node.State = EGridTalentNodeState::Available;
	Node.bHasExclusiveVariants = true;
	Node.Variants.Add(MakeVariant(TEXT("Talent_Test_A"), TEXT("Variante A")));
	Node.Variants.Add(MakeVariant(TEXT("Talent_Test_B"), TEXT("Variante B")));

	UGridTalentNodeWidget* Widget = NewObject<UGridTalentNodeWidget>();
	TestFalse(TEXT("Variant node is rejected before canonical conceptual presentation is projected"),
		Widget->InitializeTalentNode(Node, Branch));

	FRPGTalentNodePresentationDefinition Override;
	Override.TalentNodeId = Node.TalentNodeId;
	Override.DisplayName = FText::FromString(TEXT("Talent conceptuel"));
	Override.Description = FText::FromString(TEXT("Choisissez une variante."));
	Branch.NodePresentationOverrides.Add(Override);
	Node.DisplayName = Override.DisplayName;
	Node.Principle = Override.Description;

	TestTrue(TEXT("Variant node initializes once canonical conceptual presentation is projected"),
		Widget->InitializeTalentNode(Node, Branch));
	TestEqual(TEXT("Canonical conceptual display name is consumed"), Widget->ResolvedDisplayName.ToString(), FString(TEXT("Talent conceptuel")));
	TestEqual(TEXT("Variant count is preserved"), Widget->GetVariantCount(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG032OverrideValidationTest,
	"Grimrock.UI.RPG03.NodeBinding.OverrideValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG032OverrideValidationTest::RunTest(const FString&)
{
	FRPGTalentBranchPresentationDefinition Branch;
	Branch.TalentBranchId = TEXT("Branch");
	Branch.DisplayName = FText::FromString(TEXT("Branch"));

	FRPGTalentNodePresentationDefinition Override;
	Override.TalentNodeId = TEXT("Node");
	Override.DisplayName = FText::FromString(TEXT("Node"));
	Branch.NodePresentationOverrides.Add(Override);
	TestTrue(TEXT("One valid sparse node override is accepted"), Branch.IsValidDefinition());

	Branch.NodePresentationOverrides.Add(Override);
	TestFalse(TEXT("Duplicate conceptual node override is rejected"), Branch.IsValidDefinition());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRPG032ProductionCoverageTest,
	"Grimrock.UI.RPG03.NodeBinding.ProductionNinetyNodeCoverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPG032ProductionCoverageTest::RunTest(const FString&)
{
	using namespace UIRPG032Tests;
	URPGTalentPresentationAsset* Catalog = LoadObject<URPGTalentPresentationAsset>(nullptr, CatalogPath);
	if (!TestNotNull(TEXT("Production presentation catalog loads"), Catalog))
	{
		return false;
	}

	int32 ConceptualNodeCount = 0;
	int32 VariantNodeCount = 0;

	for (const FClassSpec& Spec : ClassSpecs)
	{
		URPGClassAsset* ClassAsset = LoadObject<URPGClassAsset>(nullptr, Spec.ObjectPath);
		TestNotNull(*FString::Printf(TEXT("%s class asset loads"), *Spec.ClassId.ToString()), ClassAsset);
		const FRPGClassPresentationDefinition* ClassPresentation = Catalog->FindClass(Spec.ClassId);
		TestNotNull(*FString::Printf(TEXT("%s presentation exists"), *Spec.ClassId.ToString()), ClassPresentation);
		if (!ClassAsset || !ClassPresentation)
		{
			continue;
		}

		TMap<FName, TMap<FName, TArray<const FRPGClassProgressionChoiceDefinition*>>> GroupedChoices;
		for (const FRPGClassProgressionChoiceDefinition& Choice : ClassAsset->ProgressionChoices)
		{
			GroupedChoices.FindOrAdd(Choice.TalentBranchId).FindOrAdd(Choice.TalentNodeId).Add(&Choice);
		}

		for (const FRPGTalentBranchPresentationDefinition& BranchPresentation : ClassPresentation->Branches)
		{
			const TMap<FName, TArray<const FRPGClassProgressionChoiceDefinition*>>* Nodes =
				GroupedChoices.Find(BranchPresentation.TalentBranchId);
			TestNotNull(*FString::Printf(TEXT("%s/%s gameplay branch exists"),
				*Spec.ClassId.ToString(), *BranchPresentation.TalentBranchId.ToString()), Nodes);
			if (!Nodes) continue;

			TestEqual(*FString::Printf(TEXT("%s/%s has five conceptual nodes"),
				*Spec.ClassId.ToString(), *BranchPresentation.TalentBranchId.ToString()), Nodes->Num(), 5);

			for (const TPair<FName, TArray<const FRPGClassProgressionChoiceDefinition*>>& Pair : *Nodes)
			{
				const TArray<const FRPGClassProgressionChoiceDefinition*>& Choices = Pair.Value;
				if (Choices.IsEmpty() || !Choices[0])
				{
					AddError(TEXT("Production node has no concrete choices."));
					continue;
				}

				FGridTalentNodeView Node;
				Node.TalentNodeId = Pair.Key;
				Node.TalentBranchId = BranchPresentation.TalentBranchId;
				Node.MinimumLevel = Choices[0]->MinimumLevel;
				Node.Tier = TierFromLevel(Node.MinimumLevel);
				Node.PointCost = Choices[0]->PointCost;
				Node.State = EGridTalentNodeState::Available;

				if (Choices.Num() > 1)
				{
					Node.bHasExclusiveVariants = true;
					const FRPGTalentNodePresentationDefinition* Override =
						BranchPresentation.FindNodeOverride(Pair.Key);
					TestNotNull(*FString::Printf(TEXT("%s/%s variant node has sparse presentation override"),
						*Spec.ClassId.ToString(), *Pair.Key.ToString()), Override);
					if (!Override) continue;
					Node.DisplayName = Override->DisplayName;
					Node.Principle = Override->Description;
					for (const FRPGClassProgressionChoiceDefinition* Choice : Choices)
					{
						if (!Choice) continue;
						FGridTalentVariantView Variant;
						Variant.ChoiceId = Choice->ChoiceId;
						Variant.DisplayName = Choice->DisplayName;
						Variant.State = EGridTalentNodeState::Available;
						Variant.Principle = Choice->Description;
						Variant.bCanChoose = true;
						Node.Variants.Add(MoveTemp(Variant));
					}
				}
				else
				{
					Node.DisplayName = Choices[0]->DisplayName;
					Node.Principle = Choices[0]->Description;
					Node.SimpleChoiceId = Choices[0]->ChoiceId;
				}

				UGridTalentNodeWidget* Widget = NewObject<UGridTalentNodeWidget>();
				TestTrue(*FString::Printf(TEXT("%s/%s resolves a conceptual display name"),
					*Spec.ClassId.ToString(), *Pair.Key.ToString()),
					Widget->InitializeTalentNode(Node, BranchPresentation));
				TestFalse(*FString::Printf(TEXT("%s/%s display name is non-empty"),
					*Spec.ClassId.ToString(), *Pair.Key.ToString()),
					Widget->ResolvedDisplayName.IsEmpty());

				++ConceptualNodeCount;
				if (Choices.Num() > 1)
				{
					++VariantNodeCount;
				}
			}
		}
	}

	TestEqual(TEXT("Production exposes exactly ninety conceptual talent nodes"), ConceptualNodeCount, 90);
	TestEqual(TEXT("Known production variant-node families remain exactly four"), VariantNodeCount, 4);
	return true;
}

#endif
