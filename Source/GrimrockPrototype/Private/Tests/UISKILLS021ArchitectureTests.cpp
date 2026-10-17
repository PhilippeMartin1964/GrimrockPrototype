#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UI/GridSkillsUiTypes.h"
#include "UI/RPGSkillPresentationAsset.h"
#include "UI/GridSkillsWidget.h"
#include "UObject/UnrealType.h"

namespace UISKILLS021Tests
{
	FRPGSkillPresentationDefinition MakeSkillPresentation(FName SkillId)
	{
		FRPGSkillPresentationDefinition Presentation;
		Presentation.SkillId = SkillId;
		return Presentation;
	}

	URPGSkillPresentationAsset* MakeValidCatalog()
	{
		URPGSkillPresentationAsset* Catalog = NewObject<URPGSkillPresentationAsset>();

		FRPGSkillPresentationDefinition Acrobatics = MakeSkillPresentation(TEXT("Skill_Acrobatics"));
		Acrobatics.UsageDescription = FText::FromString(TEXT("Usage guidance."));
		Acrobatics.BuildAdvice = FText::FromString(TEXT("Build advice."));
		Acrobatics.RecommendedClassIds = { TEXT("Rogue") };
		Acrobatics.UsefulClassIds = { TEXT("Warrior") };
		Acrobatics.SynergySkillIds = { TEXT("Skill_Athletics") };

		FRPGSkillPresentationDefinition Athletics = MakeSkillPresentation(TEXT("Skill_Athletics"));
		Athletics.SynergySkillIds = { TEXT("Skill_Acrobatics") };

		Catalog->Skills = { Acrobatics, Athletics };
		return Catalog;
	}

	bool LoadProjectSource(const TCHAR* RelativePath, FString& OutSource)
	{
		return FFileHelper::LoadFileToString(
			OutSource,
			*FPaths::Combine(FPaths::ProjectDir(), RelativePath));
	}

	int32 CountOccurrences(const FString& Source, const TCHAR* Needle)
	{
		int32 Count = 0;
		int32 SearchFrom = 0;
		while (true)
		{
			const int32 FoundAt = Source.Find(
				Needle,
				ESearchCase::CaseSensitive,
				ESearchDir::FromStart,
				SearchFrom);
			if (FoundAt == INDEX_NONE)
			{
				return Count;
			}
			++Count;
			SearchFrom = FoundAt + FCString::Strlen(Needle);
		}
	}

	FGridSkillDetailView MakeValidDetailView()
	{
		FGridSkillDetailView Detail;
		Detail.SkillId = TEXT("Skill_Acrobatics");
		Detail.DisplayName = FText::FromString(TEXT("Acrobaties"));
		Detail.Description = FText::FromString(TEXT("Compétence de mobilité."));
		Detail.GoverningAttribute = ERPGSkillGoverningAttribute::Dexterity;
		Detail.GoverningAttributeValue = 14;
		Detail.GoverningAttributeModifier = 2;
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
		Detail.BaseCheckBonus = 3;
		Detail.UsageDescription = FText::FromString(TEXT("Usage guidance."));
		Detail.BuildAdvice = FText::FromString(TEXT("Build advice."));

		FGridSkillClassAffinityView Recommended;
		Recommended.ClassId = TEXT("Rogue");
		Recommended.DisplayName = FText::FromString(TEXT("Voleur"));
		Detail.RecommendedClasses.Add(Recommended);

		FGridSkillClassAffinityView Useful;
		Useful.ClassId = TEXT("Warrior");
		Useful.DisplayName = FText::FromString(TEXT("Guerrier"));
		Detail.UsefulClasses.Add(Useful);

		FGridSkillSynergyView Synergy;
		Synergy.SkillId = TEXT("Skill_Athletics");
		Synergy.DisplayName = FText::FromString(TEXT("Athlétisme"));
		Detail.Synergies.Add(Synergy);
		return Detail;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLS021PresentationCatalogTest,
	"Grimrock.UI.Skills02.Architecture.PresentationCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLS021PresentationCatalogTest::RunTest(const FString&)
{
	using namespace UISKILLS021Tests;
	URPGSkillPresentationAsset* Catalog = MakeValidCatalog();

	TestTrue(TEXT("Structurally valid Skill presentation catalog is accepted"), Catalog->IsValidDefinition());

	FRPGSkillPresentationDefinition Acrobatics;
	TestTrue(
		TEXT("Skill presentation resolves by canonical SkillId"),
		Catalog->GetSkillPresentation(TEXT("Skill_Acrobatics"), Acrobatics));
	TestEqual(TEXT("Resolved SkillId is preserved"), Acrobatics.SkillId, FName(TEXT("Skill_Acrobatics")));
	TestEqual(TEXT("Recommended class metadata is preserved"), Acrobatics.RecommendedClassIds.Num(), 1);
	TestEqual(TEXT("Synergy metadata is preserved"), Acrobatics.SynergySkillIds.Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLS021InvalidPresentationReferencesTest,
	"Grimrock.UI.Skills02.Architecture.RejectInvalidPresentationReferences",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLS021InvalidPresentationReferencesTest::RunTest(const FString&)
{
	using namespace UISKILLS021Tests;

	URPGSkillPresentationAsset* Catalog = MakeValidCatalog();
	const FRPGSkillPresentationDefinition DuplicateSkill = Catalog->Skills[0];
	Catalog->Skills.Add(DuplicateSkill);
	TestFalse(TEXT("Duplicate SkillId is rejected"), Catalog->IsValidDefinition());

	Catalog = MakeValidCatalog();
	Catalog->Skills[0].UsefulClassIds = { TEXT("Rogue") };
	TestFalse(TEXT("Same class cannot be both recommended and useful"), Catalog->IsValidDefinition());

	Catalog = MakeValidCatalog();
	Catalog->Skills[0].SynergySkillIds = { TEXT("Skill_Acrobatics") };
	TestFalse(TEXT("A Skill cannot declare itself as a synergy"), Catalog->IsValidDefinition());

	Catalog = MakeValidCatalog();
	Catalog->Skills[0].SynergySkillIds = { TEXT("Skill_Unknown") };
	TestFalse(TEXT("Unknown synergy SkillId is rejected by the catalog"), Catalog->IsValidDefinition());

	Catalog = MakeValidCatalog();
	Catalog->Skills[0].RecommendedClassIds = { NAME_None };
	TestFalse(TEXT("None class ids are rejected"), Catalog->IsValidDefinition());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLS021DetailReadModelTest,
	"Grimrock.UI.Skills02.Architecture.DetailReadModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLS021DetailReadModelTest::RunTest(const FString&)
{
	using namespace UISKILLS021Tests;
	FGridSkillDetailView Detail = MakeValidDetailView();

	TestTrue(TEXT("Coherent Skill detail read model is valid"), Detail.IsValid());
	TestEqual(TEXT("Base check bonus is presentation-ready"), Detail.BaseCheckBonus, 3);
	TestEqual(TEXT("Next rank is presentation-ready"), Detail.NextRank, 2);

	Detail.TrainingState = EGridSkillTrainingState::TrainingRequired;
	TestFalse(TEXT("Training semantic must match rank and definition policy"), Detail.IsValid());

	Detail = MakeValidDetailView();
	Detail.RecommendedClasses.Add(Detail.UsefulClasses[0]);
	TestFalse(TEXT("Class affinity groups cannot overlap"), Detail.IsValid());

	Detail = MakeValidDetailView();
	Detail.Synergies[0].SkillId = Detail.SkillId;
	TestFalse(TEXT("Detail projection rejects self-synergy"), Detail.IsValid());

	Detail = MakeValidDetailView();
	Detail.BaseCheckBonus = 99;
	TestFalse(TEXT("Detail projection rejects a fabricated base check bonus"), Detail.IsValid());

	Detail = MakeValidDetailView();
	Detail.Rank = 3;
	Detail.BaseCheckBonus = 5;
	Detail.NextRank = 4;
	Detail.bCanIncreaseRank = false;
	Detail.bAtCurrentRankCap = true;
	TestFalse(TEXT("Detail projection rejects rank above current level cap"), Detail.IsValid());

	FGridSkillEntryView Entry;
	Entry.SkillId = TEXT("Skill_Acrobatics");
	Entry.Rank = 2;
	Entry.MaxRank = 5;
	Entry.CurrentRankCap = 2;
	Entry.bTrained = true;
	TestTrue(TEXT("Coherent Skill entry read model is valid"), Entry.IsValid());
	Entry.Rank = 3;
	TestFalse(TEXT("Skill entry rejects rank above current level cap"), Entry.IsValid());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLS021CanonicalSkillCacheContractTest,
	"Grimrock.UI.Skills02.Architecture.CanonicalSkillCacheContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLS021CanonicalSkillCacheContractTest::RunTest(const FString&)
{
	using namespace UISKILLS021Tests;

	UClass* SkillsClass = UGridSkillsWidget::StaticClass();
	for (const FName PropertyName : {
		FName(TEXT("CachedCanonicalSkillDefinitions")),
		FName(TEXT("CachedCanonicalSkillDefinitionsById")),
		FName(TEXT("CachedSkillIconTextures")),
		FName(TEXT("CachedSkillIconPresentationAsset"))
	})
	{
		FProperty* Property = FindFProperty<FProperty>(SkillsClass, PropertyName);
		TestNotNull(
			*FString::Printf(TEXT("%s cache property exists"), *PropertyName.ToString()),
			Property);
		if (Property)
		{
			TestTrue(
				*FString::Printf(TEXT("%s is transient"), *PropertyName.ToString()),
				Property->HasAnyPropertyFlags(CPF_Transient));
		}
	}

	FString CoreSource;
	FString SkillsSource;
	FString DetailSource;
	TestTrue(
		TEXT("GridSkillsWidget core source loads"),
		LoadProjectSource(
			TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsWidget.cpp"),
			CoreSource));
	TestTrue(
		TEXT("GridSkillsWidget Skills source loads"),
		LoadProjectSource(
			TEXT("Source/GrimrockPrototype/Private/UI/GridSkillsWidgetSkills.cpp"),
			SkillsSource));
	TestTrue(
		TEXT("GridSkillDetailWidget source loads"),
		LoadProjectSource(
			TEXT("Source/GrimrockPrototype/Private/UI/GridSkillDetailWidget.cpp"),
			DetailSource));

	TestEqual(
		TEXT("Widget core scans canonical Skill assets in one cache initializer only"),
		CountOccurrences(
			CoreSource,
			TEXT("FGridSkillsPageService::ResolveCanonicalSkillDefinitions(")),
		1);
	TestFalse(
		TEXT("Skill selection/allocation path never rescans canonical assets"),
		SkillsSource.Contains(TEXT("ResolveCanonicalSkillDefinitions(")));
	TestTrue(
		TEXT("Skill row rebuild uses the widget icon cache"),
		SkillsSource.Contains(
			TEXT("ResolveCachedSkillIcon(PresentationCatalog, Entry.SkillId)")));
	TestEqual(
		TEXT("Skill row icon synchronous load exists only inside first-use cache resolution"),
		CountOccurrences(SkillsSource, TEXT("Presentation->Icon.LoadSynchronous()")),
		1);
	TestTrue(
		TEXT("Skill detail checks an already loaded icon before synchronous fallback"),
		DetailSource.Contains(TEXT("UTexture2D* IconTexture = DetailView.Icon.Get();")));

	return true;
}


#endif
