#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Texture2D.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGClassVisualAsset.h"
#include "UI/GridTalentDetailLineWidget.h"
#include "UI/GridTalentDetailWidget.h"
#include "UI/RPGTalentDetailGlyphSet.h"

namespace UIRPGVISUAL014
{
	struct FClassSpec
	{
		FName ClassId = NAME_None;
		const TCHAR* ClassPath = nullptr;
		const TCHAR* VisualPath = nullptr;
	};

	const FClassSpec Specs[] = {
		{
			TEXT("Warrior"),
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Warrior.DA_Class_Warrior"),
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/Visuals/ClassVisuals/DA_ClassVisual_Warrior.DA_ClassVisual_Warrior")
		},
		{
			TEXT("Rogue"),
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Rogue.DA_Class_Rogue"),
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/Visuals/ClassVisuals/DA_ClassVisual_Rogue.DA_ClassVisual_Rogue")
		},
		{
			TEXT("Ranger"),
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Ranger.DA_Class_Ranger"),
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/Visuals/ClassVisuals/DA_ClassVisual_Ranger.DA_ClassVisual_Ranger")
		},
		{
			TEXT("Mage"),
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Mage.DA_Class_Mage"),
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/Visuals/ClassVisuals/DA_ClassVisual_Mage.DA_ClassVisual_Mage")
		},
		{
			TEXT("Priest"),
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Priest.DA_Class_Priest"),
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/Visuals/ClassVisuals/DA_ClassVisual_Priest.DA_ClassVisual_Priest")
		},
		{
			TEXT("Alchemist"),
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Alchemist.DA_Class_Alchemist"),
			TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/Visuals/ClassVisuals/DA_ClassVisual_Alchemist.DA_ClassVisual_Alchemist")
		}
	};

	void TestTexture(
		FAutomationTestBase& Test,
		const FString& Context,
		const TSoftObjectPtr<UTexture2D>& Texture)
	{
		Test.TestFalse(*FString::Printf(TEXT("%s reference is assigned"), *Context), Texture.IsNull());
		if (!Texture.IsNull())
		{
			Test.TestNotNull(
				*FString::Printf(TEXT("%s texture loads"), *Context),
				Texture.LoadSynchronous());
		}
	}

	void TestTexture(
		FAutomationTestBase& Test,
		const FString& Context,
		const UTexture2D* Texture)
	{
		Test.TestNotNull(*FString::Printf(TEXT("%s texture is assigned"), *Context), Texture);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGVISUAL014SerializedVisualAssetsTest,
	"Grimrock.UI.RPG.VISUAL01.FinalAudit.SerializedVisualAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGVISUAL014SerializedVisualAssetsTest::RunTest(const FString&)
{
	using namespace UIRPGVISUAL014;

	int32 TotalBranches = 0;
	int32 TotalNodes = 0;
	for (const FClassSpec& Spec : Specs)
	{
		URPGClassAsset* ClassAsset = LoadObject<URPGClassAsset>(nullptr, Spec.ClassPath);
		URPGClassVisualAsset* VisualAsset = LoadObject<URPGClassVisualAsset>(nullptr, Spec.VisualPath);
		if (!TestNotNull(*FString::Printf(TEXT("%s serialized DA_Class loads"), *Spec.ClassId.ToString()), ClassAsset) ||
			!TestNotNull(*FString::Printf(TEXT("%s serialized DA_ClassVisual loads"), *Spec.ClassId.ToString()), VisualAsset))
		{
			continue;
		}

		TestEqual(TEXT("Class visual ClassId matches gameplay ClassId"), VisualAsset->ClassId, ClassAsset->ClassId);
		TestEqual(TEXT("Class visual uses requested ClassId"), VisualAsset->ClassId, Spec.ClassId);

		TestTexture(*this, Spec.ClassId.ToString() + TEXT("/ClassIcon"), VisualAsset->ClassIcon);
		TestTexture(*this, Spec.ClassId.ToString() + TEXT("/Banner"), VisualAsset->Banner);
		TestTexture(*this, Spec.ClassId.ToString() + TEXT("/Flag"), VisualAsset->Flag);

		TSet<FName> GameplayBranchIds;
		TSet<FName> GameplayNodeIds;
		for (const FRPGClassProgressionChoiceDefinition& Choice : ClassAsset->ProgressionChoices)
		{
			GameplayBranchIds.Add(Choice.TalentBranchId);
			GameplayNodeIds.Add(Choice.TalentNodeId);
		}

		TestEqual(*FString::Printf(TEXT("%s gameplay has three branches"), *Spec.ClassId.ToString()), GameplayBranchIds.Num(), 3);
		TestEqual(*FString::Printf(TEXT("%s gameplay has fifteen conceptual nodes"), *Spec.ClassId.ToString()), GameplayNodeIds.Num(), 15);
		TestEqual(*FString::Printf(TEXT("%s visual has three branch backgrounds"), *Spec.ClassId.ToString()), VisualAsset->TalentBranchVisuals.Num(), 3);
		TestEqual(*FString::Printf(TEXT("%s visual has fifteen Talent icons"), *Spec.ClassId.ToString()), VisualAsset->TalentNodeVisuals.Num(), 15);

		TSet<FName> VisualBranchIds;
		for (const FRPGTalentBranchVisualDefinition& BranchVisual : VisualAsset->TalentBranchVisuals)
		{
			TestFalse(TEXT("Branch visual id is not empty"), BranchVisual.TalentBranchId.IsNone());
			TestFalse(TEXT("Branch visual id is unique"), VisualBranchIds.Contains(BranchVisual.TalentBranchId));
			VisualBranchIds.Add(BranchVisual.TalentBranchId);
			TestTrue(TEXT("Branch visual matches a gameplay branch"), GameplayBranchIds.Contains(BranchVisual.TalentBranchId));
			TestTexture(
				*this,
				Spec.ClassId.ToString() + TEXT("/") + BranchVisual.TalentBranchId.ToString() + TEXT("/Background"),
				BranchVisual.Background);
		}

		TSet<FName> VisualNodeIds;
		for (const FRPGTalentNodeVisualDefinition& NodeVisual : VisualAsset->TalentNodeVisuals)
		{
			TestFalse(TEXT("Talent visual id is not empty"), NodeVisual.TalentNodeId.IsNone());
			TestFalse(TEXT("Talent visual id is unique"), VisualNodeIds.Contains(NodeVisual.TalentNodeId));
			VisualNodeIds.Add(NodeVisual.TalentNodeId);
			TestTrue(TEXT("Talent visual matches a gameplay node"), GameplayNodeIds.Contains(NodeVisual.TalentNodeId));
			TestTexture(
				*this,
				Spec.ClassId.ToString() + TEXT("/") + NodeVisual.TalentNodeId.ToString() + TEXT("/Icon"),
				NodeVisual.Icon);
		}

		for (const FName BranchId : GameplayBranchIds)
		{
			TestNotNull(
				*FString::Printf(TEXT("%s/%s has a visual background"), *Spec.ClassId.ToString(), *BranchId.ToString()),
				VisualAsset->FindTalentBranchVisual(BranchId));
		}
		for (const FName NodeId : GameplayNodeIds)
		{
			TestNotNull(
				*FString::Printf(TEXT("%s/%s has a Talent icon"), *Spec.ClassId.ToString(), *NodeId.ToString()),
				VisualAsset->FindTalentNodeVisual(NodeId));
		}

		TotalBranches += GameplayBranchIds.Num();
		TotalNodes += GameplayNodeIds.Num();
	}

	TestEqual(TEXT("Serialized visual audit covers eighteen branches"), TotalBranches, 18);
	TestEqual(TEXT("Serialized visual audit covers ninety conceptual Talents"), TotalNodes, 90);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGVISUAL014VariantIconCoverageTest,
	"Grimrock.UI.RPG.VISUAL01.FinalAudit.VariantIconCoverage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGVISUAL014VariantIconCoverageTest::RunTest(const FString&)
{
	using namespace UIRPGVISUAL014;

	int32 ExclusiveNodeCount = 0;
	int32 VariantChoiceCount = 0;

	for (const FClassSpec& Spec : Specs)
	{
		URPGClassAsset* ClassAsset = LoadObject<URPGClassAsset>(nullptr, Spec.ClassPath);
		URPGClassVisualAsset* VisualAsset = LoadObject<URPGClassVisualAsset>(nullptr, Spec.VisualPath);
		if (!TestNotNull(*FString::Printf(TEXT("%s serialized DA_Class loads"), *Spec.ClassId.ToString()), ClassAsset) ||
			!TestNotNull(*FString::Printf(TEXT("%s serialized DA_ClassVisual loads"), *Spec.ClassId.ToString()), VisualAsset))
		{
			continue;
		}

		TMap<FName, TArray<const FRPGClassProgressionChoiceDefinition*>> ChoicesByNode;
		for (const FRPGClassProgressionChoiceDefinition& Choice : ClassAsset->ProgressionChoices)
		{
			ChoicesByNode.FindOrAdd(Choice.TalentNodeId).Add(&Choice);
		}

		TSet<FName> ExpectedVariantChoiceIds;
		for (const TPair<FName, TArray<const FRPGClassProgressionChoiceDefinition*>>& NodePair : ChoicesByNode)
		{
			const TArray<const FRPGClassProgressionChoiceDefinition*>& Choices = NodePair.Value;
			if (Choices.Num() < 2)
			{
				continue;
			}

			const FName ExclusiveGroupId =
				Choices[0] ? Choices[0]->ExclusiveChoiceGroupId : NAME_None;
			if (ExclusiveGroupId.IsNone())
			{
				continue;
			}

			bool bExclusiveVariantNode = true;
			for (const FRPGClassProgressionChoiceDefinition* Choice : Choices)
			{
				if (!Choice || Choice->ExclusiveChoiceGroupId != ExclusiveGroupId)
				{
					bExclusiveVariantNode = false;
					break;
				}
			}
			if (!bExclusiveVariantNode)
			{
				continue;
			}

			++ExclusiveNodeCount;
			for (const FRPGClassProgressionChoiceDefinition* Choice : Choices)
			{
				TestFalse(
					*FString::Printf(
						TEXT("%s/%s variant ChoiceId is not empty"),
						*Spec.ClassId.ToString(),
						*NodePair.Key.ToString()),
					Choice->ChoiceId.IsNone());

				if (!Choice->ChoiceId.IsNone())
				{
					ExpectedVariantChoiceIds.Add(Choice->ChoiceId);
					++VariantChoiceCount;
				}
			}
		}

		TSet<FName> SeenVisualChoiceIds;
		for (const FRPGTalentVariantVisualDefinition& VariantVisual : VisualAsset->TalentVariantVisuals)
		{
			const FString Context =
				Spec.ClassId.ToString() + TEXT("/") + VariantVisual.ChoiceId.ToString();

			TestFalse(
				*FString::Printf(TEXT("%s variant visual ChoiceId is not empty"), *Context),
				VariantVisual.ChoiceId.IsNone());
			TestFalse(
				*FString::Printf(TEXT("%s variant visual ChoiceId is unique"), *Context),
				SeenVisualChoiceIds.Contains(VariantVisual.ChoiceId));

			if (!VariantVisual.ChoiceId.IsNone())
			{
				SeenVisualChoiceIds.Add(VariantVisual.ChoiceId);
				TestTrue(
					*FString::Printf(TEXT("%s variant visual maps a production exclusive ChoiceId"), *Context),
					ExpectedVariantChoiceIds.Contains(VariantVisual.ChoiceId));
			}

			TestTexture(*this, Context + TEXT("/Icon"), VariantVisual.Icon);
		}

		TestEqual(
			*FString::Printf(TEXT("%s variant visual count exactly covers production exclusive choices"), *Spec.ClassId.ToString()),
			SeenVisualChoiceIds.Num(),
			ExpectedVariantChoiceIds.Num());

		for (const FName ChoiceId : ExpectedVariantChoiceIds)
		{
			const FRPGTalentVariantVisualDefinition* VariantVisual =
				VisualAsset->FindTalentVariantVisual(ChoiceId);
			if (!TestNotNull(
					*FString::Printf(
						TEXT("%s/%s has a concrete variant visual"),
						*Spec.ClassId.ToString(),
						*ChoiceId.ToString()),
					VariantVisual))
			{
				continue;
			}

			TestTexture(
				*this,
				Spec.ClassId.ToString() + TEXT("/") + ChoiceId.ToString() + TEXT("/Icon"),
				VariantVisual->Icon);
		}
	}

	TestTrue(TEXT("Production visual coverage audits at least one exclusive Talent node"), ExclusiveNodeCount > 0);
	TestTrue(TEXT("Production visual coverage audits at least one concrete variant ChoiceId"), VariantChoiceCount > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGVISUAL014SerializedClassTextReadinessTest,
	"Grimrock.UI.RPG.VISUAL01.FinalAudit.SerializedClassTextReadiness",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGVISUAL014SerializedClassTextReadinessTest::RunTest(const FString&)
{
	using namespace UIRPGVISUAL014;

	int32 ChoiceCount = 0;
	int32 ActionCount = 0;
	for (const FClassSpec& Spec : Specs)
	{
		URPGClassAsset* ClassAsset = LoadObject<URPGClassAsset>(nullptr, Spec.ClassPath);
		if (!TestNotNull(*FString::Printf(TEXT("%s serialized class loads"), *Spec.ClassId.ToString()), ClassAsset))
		{
			continue;
		}

		TestFalse(*FString::Printf(TEXT("%s class DisplayName is authored"), *Spec.ClassId.ToString()), ClassAsset->DisplayName.IsEmpty());

		for (const FRPGClassProgressionChoiceDefinition& Choice : ClassAsset->ProgressionChoices)
		{
			++ChoiceCount;
			const FString Context = Spec.ClassId.ToString() + TEXT("/") + Choice.ChoiceId.ToString();
			TestFalse(*FString::Printf(TEXT("%s DisplayName is serialized"), *Context), Choice.DisplayName.IsEmpty());
			TestFalse(*FString::Printf(TEXT("%s Description is serialized"), *Context), Choice.Description.IsEmpty());
		}

		for (const FGridCombatActionDefinition& Action : ClassAsset->CombatActions)
		{
			++ActionCount;
			const FString Context = Spec.ClassId.ToString() + TEXT("/") + Action.ActionId.ToString();
			TestFalse(*FString::Printf(TEXT("%s DisplayName is serialized"), *Context), Action.DisplayName.IsEmpty());
			TestFalse(*FString::Printf(TEXT("%s Description is serialized"), *Context), Action.Description.IsEmpty());
		}
	}

	TestTrue(TEXT("Serialized class audit covers production progression choices"), ChoiceCount >= 90);
	TestTrue(TEXT("Serialized class audit covers class combat actions"), ActionCount > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGVISUAL014GlyphSetTest,
	"Grimrock.UI.RPG.VISUAL01.FinalAudit.GlyphSet",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGVISUAL014GlyphSetTest::RunTest(const FString&)
{
	using namespace UIRPGVISUAL014;

	URPGTalentDetailGlyphSet* Glyphs = LoadObject<URPGTalentDetailGlyphSet>(
		nullptr,
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/Visuals/ClassVisuals/DA_RPGTalentDetailGlyphSet.DA_RPGTalentDetailGlyphSet"));
	if (!TestNotNull(TEXT("Serialized Talent glyph set loads"), Glyphs))
	{
		return false;
	}

	TestTexture(*this, TEXT("Glyph/Cost"), Glyphs->Cost.Get());
	TestTexture(*this, TEXT("Glyph/ManaCost"), Glyphs->ManaCost.Get());
	TestTexture(*this, TEXT("Glyph/ItemCost"), Glyphs->ItemCost.Get());
	TestTexture(*this, TEXT("Glyph/Action"), Glyphs->Action.Get());
	TestTexture(*this, TEXT("Glyph/Target"), Glyphs->Target.Get());
	TestTexture(*this, TEXT("Glyph/Range"), Glyphs->Range.Get());
	TestTexture(*this, TEXT("Glyph/Area"), Glyphs->Area.Get());
	TestTexture(*this, TEXT("Glyph/LineOfSight"), Glyphs->LineOfSight.Get());
	TestTexture(*this, TEXT("Glyph/ResolutionCount"), Glyphs->ResolutionCount.Get());
	TestTexture(*this, TEXT("Glyph/Cooldown"), Glyphs->Cooldown.Get());
	TestTexture(*this, TEXT("Glyph/LevelRequirement"), Glyphs->LevelRequirement.Get());
	TestTexture(*this, TEXT("Glyph/Prerequisite"), Glyphs->Prerequisite.Get());
	TestTexture(*this, TEXT("Glyph/Exclusivity"), Glyphs->Exclusivity.Get());
	TestTexture(*this, TEXT("Glyph/Recipe"), Glyphs->Recipe.Get());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUIRPGVISUAL014DeadPresentationFieldsTest,
	"Grimrock.UI.RPG.VISUAL01.FinalAudit.DeadPresentationFields",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRPGVISUAL014DeadPresentationFieldsTest::RunTest(const FString&)
{
	TestNull(
		TEXT("ResolvedNumericValue migration mirror is removed"),
		UGridTalentDetailLineWidget::StaticClass()->FindPropertyByName(TEXT("ResolvedNumericValue")));
	TestNull(
		TEXT("ResolvedValueStyle migration mirror is removed"),
		UGridTalentDetailLineWidget::StaticClass()->FindPropertyByName(TEXT("ResolvedValueStyle")));
	TestNull(
		TEXT("bShowPositiveSign migration flag is removed"),
		UGridTalentDetailLineWidget::StaticClass()->FindPropertyByName(TEXT("bShowPositiveSign")));
	TestNull(
		TEXT("bShowPercentUnit migration flag is removed"),
		UGridTalentDetailLineWidget::StaticClass()->FindPropertyByName(TEXT("bShowPercentUnit")));
	TestNull(
		TEXT("Detail widget no longer owns a configurable second text authority"),
		UGridTalentDetailWidget::StaticClass()->FindPropertyByName(TEXT("DetailTextSet")));
	return true;
}

#endif
