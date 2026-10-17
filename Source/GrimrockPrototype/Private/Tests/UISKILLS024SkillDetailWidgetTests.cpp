#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/GridSkillDetailWidget.h"
#include "UI/GridSkillsWidget.h"
#include "UObject/UnrealType.h"

namespace UISKILLS024Tests
{
	FGridSkillDetailView MakeValidDetail()
	{
		FGridSkillDetailView Detail;
		Detail.SkillId = TEXT("Skill_Acrobatics");
		Detail.DisplayName = FText::FromString(TEXT("Acrobaties"));
		Detail.Description = FText::FromString(TEXT("Agilité, équilibre et mouvements physiques."));
		Detail.GoverningAttribute = ERPGSkillGoverningAttribute::Dexterity;
		Detail.GoverningAttributeValue = 14;
		Detail.GoverningAttributeModifier = 2;
		Detail.BaseCheckBonus = 3;
		Detail.Rank = 1;
		Detail.MaxRank = 5;
		Detail.CurrentRankCap = 2;
		Detail.RemainingSkillPoints = 4;
		Detail.NextRank = 2;
		Detail.NextRankCost = 1;
		Detail.bAllowUntrainedChecks = true;
		Detail.TrainingState = EGridSkillTrainingState::Trained;
		Detail.bCanIncreaseRank = true;
		Detail.bCanDecreaseRank = false;
		Detail.bAtCurrentRankCap = false;
		Detail.bAtSkillMaxRank = false;
		Detail.UsageDescription = FText::FromString(TEXT("Usage de test."));
		Detail.BuildAdvice = FText::FromString(TEXT("Conseil de build."));

		FGridSkillClassAffinityView Rogue;
		Rogue.ClassId = TEXT("Rogue");
		Rogue.DisplayName = FText::FromString(TEXT("Voleur"));
		Detail.RecommendedClasses.Add(Rogue);

		FGridSkillClassAffinityView Scout;
		Scout.ClassId = TEXT("Scout");
		Scout.DisplayName = FText::FromString(TEXT("Éclaireur"));
		Detail.RecommendedClasses.Add(Scout);

		FGridSkillClassAffinityView Warrior;
		Warrior.ClassId = TEXT("Warrior");
		Warrior.DisplayName = FText::FromString(TEXT("Guerrier"));
		Detail.UsefulClasses.Add(Warrior);

		FGridSkillClassAffinityView Priest;
		Priest.ClassId = TEXT("Priest");
		Priest.DisplayName = FText::FromString(TEXT("Prêtre"));
		Detail.DiscouragedClasses.Add(Priest);

		FGridSkillSynergyView Lockpicking;
		Lockpicking.SkillId = TEXT("Skill_Lockpicking");
		Lockpicking.DisplayName = FText::FromString(TEXT("Crochetage"));
		Detail.Synergies.Add(Lockpicking);

		FGridSkillSynergyView Athletics;
		Athletics.SkillId = TEXT("Skill_Athletics");
		Athletics.DisplayName = FText::FromString(TEXT("Athlétisme"));
		Detail.Synergies.Add(Athletics);
		return Detail;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLS024DetailFormattingTest,
	"Grimrock.UI.Skills02.DetailWidget.Formatting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLS024DetailFormattingTest::RunTest(const FString&)
{
	using namespace UISKILLS024Tests;

	const FGridSkillDetailView Detail = MakeValidDetail();
	TestTrue(TEXT("Test detail fixture is valid"), Detail.IsValid());

	UGridSkillDetailWidget* Widget = NewObject<UGridSkillDetailWidget>();
	TestTrue(TEXT("Valid Skill detail initializes"), Widget->InitializeSkillDetail(Detail));
	TestTrue(TEXT("Detail widget reports initialized state"), Widget->bInitialized);
	TestEqual(TEXT("Canonical SkillId is preserved"), Widget->DetailView.SkillId, Detail.SkillId);
	TestTrue(TEXT("Attribute uses localized UI vocabulary"),
		Widget->ResolvedAttributeText.EqualTo(FText::FromString(TEXT("Dextérité"))));
	TestTrue(TEXT("Rank label is presentation-ready"),
		Widget->ResolvedRankText.EqualTo(FText::FromString(TEXT("Rang 1 / 5"))));
	TestTrue(TEXT("Training state is presentation-ready"),
		Widget->ResolvedTrainingText.EqualTo(FText::FromString(TEXT("Entraînée"))));
	TestTrue(TEXT("Rank check contribution is signed"),
		Widget->ResolvedCheckRankText.EqualTo(FText::FromString(TEXT("+1"))));
	TestTrue(TEXT("Attribute modifier is signed"),
		Widget->ResolvedCheckAttributeModifierText.EqualTo(FText::FromString(TEXT("+2"))));
	TestTrue(TEXT("Base check bonus is signed"),
		Widget->ResolvedCheckBaseBonusText.EqualTo(FText::FromString(TEXT("+3"))));
	TestTrue(TEXT("Recommended classes preserve canonical display order"),
		Widget->ResolvedRecommendedClassesText.EqualTo(FText::FromString(TEXT("Voleur, Éclaireur"))));
	TestTrue(TEXT("Useful classes are presentation-ready"),
		Widget->ResolvedUsefulClassesText.EqualTo(FText::FromString(TEXT("Guerrier"))));
	TestTrue(TEXT("Discouraged classes are presentation-ready"),
		Widget->ResolvedDiscouragedClassesText.EqualTo(FText::FromString(TEXT("Prêtre"))));
	TestTrue(TEXT("Synergies preserve canonical display order"),
		Widget->ResolvedSynergiesText.EqualTo(FText::FromString(TEXT("Crochetage, Athlétisme"))));
	TestTrue(TEXT("Current rank cap is presentation-ready"),
		Widget->ResolvedCurrentRankCapText.EqualTo(FText::FromString(TEXT("2"))));
	TestTrue(TEXT("Remaining points are presentation-ready"),
		Widget->ResolvedRemainingSkillPointsText.EqualTo(FText::FromString(TEXT("4"))));
	TestTrue(TEXT("Next rank is presentation-ready"),
		Widget->ResolvedNextRankText.EqualTo(FText::FromString(TEXT("2"))));
	TestTrue(TEXT("Next rank cost is presentation-ready"),
		Widget->ResolvedNextRankCostText.EqualTo(FText::FromString(TEXT("1"))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLS024TrainingAndMaximumRankTest,
	"Grimrock.UI.Skills02.DetailWidget.TrainingAndMaximumRank",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLS024TrainingAndMaximumRankTest::RunTest(const FString&)
{
	using namespace UISKILLS024Tests;

	FGridSkillDetailView Detail = MakeValidDetail();
	Detail.Rank = 0;
	Detail.GoverningAttributeModifier = 2;
	Detail.BaseCheckBonus = 2;
	Detail.bAllowUntrainedChecks = false;
	Detail.TrainingState = EGridSkillTrainingState::TrainingRequired;
	Detail.NextRank = 1;
	Detail.bCanIncreaseRank = true;

	UGridSkillDetailWidget* Widget = NewObject<UGridSkillDetailWidget>();
	TestTrue(TEXT("Training-required detail initializes"), Widget->InitializeSkillDetail(Detail));
	TestTrue(TEXT("Training-required wording is explicit"),
		Widget->ResolvedTrainingText.EqualTo(FText::FromString(TEXT("Entraînement requis"))));

	Detail.Rank = 5;
	Detail.GoverningAttributeModifier = 2;
	Detail.BaseCheckBonus = 7;
	Detail.CurrentRankCap = 5;
	Detail.NextRank = 0;
	Detail.NextRankCost = 0;
	Detail.bAllowUntrainedChecks = true;
	Detail.TrainingState = EGridSkillTrainingState::Trained;
	Detail.bCanIncreaseRank = false;
	Detail.bAtCurrentRankCap = true;
	Detail.bAtSkillMaxRank = true;

	TestTrue(TEXT("Maximum-rank detail initializes"), Widget->InitializeSkillDetail(Detail));
	TestTrue(TEXT("Maximum rank exposes no next rank text"), Widget->ResolvedNextRankText.IsEmpty());
	TestTrue(TEXT("Maximum rank exposes no next rank cost text"), Widget->ResolvedNextRankCostText.IsEmpty());
	TestTrue(TEXT("Maximum-rank base bonus remains exact"),
		Widget->ResolvedCheckBaseBonusText.EqualTo(FText::FromString(TEXT("+7"))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLS024RejectAndClearTest,
	"Grimrock.UI.Skills02.DetailWidget.RejectAndClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLS024RejectAndClearTest::RunTest(const FString&)
{
	using namespace UISKILLS024Tests;

	UGridSkillDetailWidget* Widget = NewObject<UGridSkillDetailWidget>();
	TestTrue(TEXT("Baseline valid detail initializes"), Widget->InitializeSkillDetail(MakeValidDetail()));

	FGridSkillDetailView Invalid = MakeValidDetail();
	Invalid.BaseCheckBonus = 99;
	TestFalse(TEXT("Incoherent detail read model is rejected"), Widget->InitializeSkillDetail(Invalid));
	TestFalse(TEXT("Rejected detail leaves widget cleared"), Widget->bInitialized);
	TestTrue(TEXT("Rejected detail clears SkillId"), Widget->DetailView.SkillId.IsNone());
	TestTrue(TEXT("Rejected detail clears resolved rank"), Widget->ResolvedRankText.IsEmpty());
	TestTrue(TEXT("Rejected detail clears discouraged classes"), Widget->ResolvedDiscouragedClassesText.IsEmpty());

	TestTrue(TEXT("Widget can initialize again after rejection"), Widget->InitializeSkillDetail(MakeValidDetail()));
	Widget->ClearSkillDetail();
	TestFalse(TEXT("Explicit clear resets initialized state"), Widget->bInitialized);
	TestTrue(TEXT("Explicit clear resets all resolved content"), Widget->ResolvedTrainingText.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLS024WidgetContractTest,
	"Grimrock.UI.Skills02.DetailWidget.WidgetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLS024WidgetContractTest::RunTest(const FString&)
{
	UClass* DetailClass = UGridSkillDetailWidget::StaticClass();

	for (const FName FunctionName : {
		FName(TEXT("InitializeSkillDetail")),
		FName(TEXT("ClearSkillDetail")),
		FName(TEXT("RefreshDetailVisual")),
		FName(TEXT("ResolveAttributeLabel")),
		FName(TEXT("ResolveRankLabel")),
		FName(TEXT("ResolveTrainingLabel")),
		FName(TEXT("ResolveSignedInteger"))
	})
	{
		TestNotNull(
			*FString::Printf(TEXT("%s function exists"), *FunctionName.ToString()),
			DetailClass->FindFunctionByName(FunctionName));
	}

	const FName RequiredBindings[] =
	{
		TEXT("Image_SkillIcon"),
		TEXT("Text_DetailName"),
		TEXT("Text_DetailAttribute"),
		TEXT("Text_DetailRank"),
		TEXT("VB_DetailDescription"),
		TEXT("Text_DetailDescription"),
		TEXT("VB_DetailTraining"),
		TEXT("Text_DetailTraining"),
		TEXT("VB_DetailUsage"),
		TEXT("Text_DetailUsage"),
		TEXT("VB_DetailBuildAdvice"),
		TEXT("Text_DetailBuildAdvice"),
		TEXT("VB_DetailCheck"),
		TEXT("Text_DetailCheckRank"),
		TEXT("Text_DetailCheckAttributeModifier"),
		TEXT("Text_DetailCheckBaseBonus"),
		TEXT("VB_DetailAffinities"),
		TEXT("VB_DetailRecommendedClasses"),
		TEXT("Text_DetailRecommendedClasses"),
		TEXT("VB_DetailUsefulClasses"),
		TEXT("Text_DetailUsefulClasses"),
		TEXT("VB_DetailSynergies"),
		TEXT("Text_DetailSynergies"),
		TEXT("VB_DetailProgression"),
		TEXT("Text_DetailCurrentRankCap"),
		TEXT("Text_DetailRemainingSkillPoints"),
		TEXT("VB_DetailNextRank"),
		TEXT("Text_DetailNextRank"),
		TEXT("VB_DetailNextRankCost"),
		TEXT("Text_DetailNextRankCost")
	};

	for (const FName PropertyName : RequiredBindings)
	{
		FProperty* Property = FindFProperty<FProperty>(DetailClass, PropertyName);
		TestNotNull(
			*FString::Printf(TEXT("%s property exists"), *PropertyName.ToString()),
			Property);
		if (Property)
		{
			TestTrue(
				*FString::Printf(TEXT("%s is mandatory after Designer materialization"), *PropertyName.ToString()),
				Property->HasMetaData(TEXT("BindWidget")));
			TestFalse(
				*FString::Printf(TEXT("%s is no longer optional"), *PropertyName.ToString()),
				Property->HasMetaData(TEXT("BindWidgetOptional")));
		}
	}

	const FName PendingDesignerBindings[] =
	{
		TEXT("VB_DetailDiscouragedClasses"),
		TEXT("Text_DetailDiscouragedClasses")
	};
	for (const FName PropertyName : PendingDesignerBindings)
	{
		FProperty* Property = FindFProperty<FProperty>(DetailClass, PropertyName);
		TestNotNull(
			*FString::Printf(TEXT("%s pending Designer hook exists"), *PropertyName.ToString()),
			Property);
		if (Property)
		{
			TestTrue(
				*FString::Printf(TEXT("%s stays optional only until manual WBP materialization"), *PropertyName.ToString()),
				Property->HasMetaData(TEXT("BindWidgetOptional")));
			TestFalse(
				*FString::Printf(TEXT("%s is not hardened before manual WBP materialization"), *PropertyName.ToString()),
				Property->HasMetaData(TEXT("BindWidget")));
		}
	}

	const FName RetiredStaticLabelBindings[] =
	{
		TEXT("Text_LabelDescription"),
		TEXT("Text_LabelTraining"),
		TEXT("Text_LabelUsage"),
		TEXT("Text_LabelBuildAdvice"),
		TEXT("Text_LabelCheck"),
		TEXT("Text_LabelCheckRank"),
		TEXT("Text_LabelCheckAttributeModifier"),
		TEXT("Text_LabelCheckBaseBonus"),
		TEXT("Text_LabelAffinities"),
		TEXT("Text_LabelRecommendedClasses"),
		TEXT("Text_LabelUsefulClasses"),
		TEXT("Text_LabelDiscouragedClasses"),
		TEXT("Text_LabelSynergies"),
		TEXT("Text_LabelProgression"),
		TEXT("Text_LabelCurrentRankCap"),
		TEXT("Text_LabelRemainingSkillPoints"),
		TEXT("Text_LabelNextRank"),
		TEXT("Text_LabelNextRankCost")
	};

	for (const FName PropertyName : RetiredStaticLabelBindings)
	{
		TestNull(
			*FString::Printf(
				TEXT("%s static label no longer has native C++ ownership"),
				*PropertyName.ToString()),
			FindFProperty<FProperty>(DetailClass, PropertyName));
	}

	UClass* SkillsClass = UGridSkillsWidget::StaticClass();
	FProperty* ParentDetail =
		FindFProperty<FProperty>(SkillsClass, TEXT("Detail_Skill"));
	TestNotNull(TEXT("Parent Detail_Skill binding exists"), ParentDetail);
	if (ParentDetail)
	{
		TestTrue(TEXT("Parent Detail_Skill is mandatory after WBP materialization"),
			ParentDetail->HasMetaData(TEXT("BindWidget")));
		TestFalse(TEXT("Parent Detail_Skill is no longer optional"),
			ParentDetail->HasMetaData(TEXT("BindWidgetOptional")));
	}
	return true;
}

#endif
