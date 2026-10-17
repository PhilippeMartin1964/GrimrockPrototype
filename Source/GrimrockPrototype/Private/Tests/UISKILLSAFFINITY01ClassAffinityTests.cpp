#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/GridSkillEntryWidget.h"
#include "UI/GridSkillsWidget.h"
#include "UI/RPGSkillPresentationAsset.h"
#include "UObject/UnrealType.h"

namespace UISKILLSAFFINITY01Tests
{
	URPGSkillPresentationAsset* MakeCatalog()
	{
		URPGSkillPresentationAsset* Catalog = NewObject<URPGSkillPresentationAsset>();
		FRPGSkillPresentationDefinition Skill;
		Skill.SkillId = TEXT("Skill_Test");
		Skill.RecommendedClassIds = { TEXT("Rogue") };
		Skill.UsefulClassIds = { TEXT("Warrior") };
		Skill.DiscouragedClassIds = { TEXT("Mage") };
		Catalog->Skills = { Skill };
		return Catalog;
	}

	FGridSkillEntryView MakeEntry(const TCHAR* SkillId, const TCHAR* Name, EGridSkillClassAffinity Affinity)
	{
		FGridSkillEntryView Entry;
		Entry.SkillId = FName(SkillId);
		Entry.DisplayName = FText::FromString(Name);
		Entry.MaxRank = 5;
		Entry.CurrentRankCap = 5;
		Entry.bAllowUntrainedChecks = true;
		Entry.ClassAffinity = Affinity;
		return Entry;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLSAFFINITY01ResolutionTest,
	"Grimrock.UI.Skills02.Affinity.Resolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLSAFFINITY01ResolutionTest::RunTest(const FString&)
{
	using namespace UISKILLSAFFINITY01Tests;
	URPGSkillPresentationAsset* Catalog = MakeCatalog();
	TestTrue(TEXT("Affinity catalog remains structurally valid"), Catalog->IsValidDefinition());
	TestEqual(TEXT("Rogue is recommended"),
		Catalog->ResolveClassAffinity(TEXT("Skill_Test"), TEXT("Rogue")), EGridSkillClassAffinity::Recommended);
	TestEqual(TEXT("Warrior is useful"),
		Catalog->ResolveClassAffinity(TEXT("Skill_Test"), TEXT("Warrior")), EGridSkillClassAffinity::Useful);
	TestEqual(TEXT("Mage is discouraged"),
		Catalog->ResolveClassAffinity(TEXT("Skill_Test"), TEXT("Mage")), EGridSkillClassAffinity::Discouraged);
	TestEqual(TEXT("Unlisted class is neutral"),
		Catalog->ResolveClassAffinity(TEXT("Skill_Test"), TEXT("Priest")), EGridSkillClassAffinity::Neutral);
	TestEqual(TEXT("Missing Skill metadata is neutral"),
		Catalog->ResolveClassAffinity(TEXT("Skill_Missing"), TEXT("Rogue")), EGridSkillClassAffinity::Neutral);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLSAFFINITY01OverlapGuardTest,
	"Grimrock.UI.Skills02.Affinity.OverlapGuard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLSAFFINITY01OverlapGuardTest::RunTest(const FString&)
{
	using namespace UISKILLSAFFINITY01Tests;
	URPGSkillPresentationAsset* Catalog = MakeCatalog();
	Catalog->Skills[0].DiscouragedClassIds = { TEXT("Rogue") };
	TestFalse(TEXT("Recommended and discouraged cannot overlap"), Catalog->IsValidDefinition());

	Catalog = MakeCatalog();
	Catalog->Skills[0].DiscouragedClassIds = { TEXT("Warrior") };
	TestFalse(TEXT("Useful and discouraged cannot overlap"), Catalog->IsValidDefinition());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLSAFFINITY01SortTest,
	"Grimrock.UI.Skills02.Affinity.Sort",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLSAFFINITY01SortTest::RunTest(const FString&)
{
	using namespace UISKILLSAFFINITY01Tests;
	TArray<FGridSkillEntryView> Entries = {
		MakeEntry(TEXT("Skill_Neutral"), TEXT("Neutre"), EGridSkillClassAffinity::Neutral),
		MakeEntry(TEXT("Skill_Discouraged"), TEXT("Déconseillée"), EGridSkillClassAffinity::Discouraged),
		MakeEntry(TEXT("Skill_Useful"), TEXT("Utile"), EGridSkillClassAffinity::Useful),
		MakeEntry(TEXT("Skill_Recommended"), TEXT("Recommandée"), EGridSkillClassAffinity::Recommended)
	};

	UGridSkillsWidget::SortSkillEntries(Entries, EGridSkillSortColumn::Affinity, EGridSkillSortDirection::Ascending);
	TestEqual(TEXT("Ascending starts with Recommended"), Entries[0].ClassAffinity, EGridSkillClassAffinity::Recommended);
	TestEqual(TEXT("Ascending then Useful"), Entries[1].ClassAffinity, EGridSkillClassAffinity::Useful);
	TestEqual(TEXT("Ascending then Neutral"), Entries[2].ClassAffinity, EGridSkillClassAffinity::Neutral);
	TestEqual(TEXT("Ascending ends with Discouraged"), Entries[3].ClassAffinity, EGridSkillClassAffinity::Discouraged);

	UGridSkillsWidget::SortSkillEntries(Entries, EGridSkillSortColumn::Affinity, EGridSkillSortDirection::Descending);
	TestEqual(TEXT("Descending starts with Discouraged"), Entries[0].ClassAffinity, EGridSkillClassAffinity::Discouraged);
	TestEqual(TEXT("Descending ends with Recommended"), Entries[3].ClassAffinity, EGridSkillClassAffinity::Recommended);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLSAFFINITY01GlyphAndWidgetContractTest,
	"Grimrock.UI.Skills02.Affinity.WidgetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLSAFFINITY01GlyphAndWidgetContractTest::RunTest(const FString&)
{
	TestTrue(TEXT("Recommended uses two stars"),
		UGridSkillEntryWidget::ResolveAffinityGlyph(EGridSkillClassAffinity::Recommended).EqualTo(FText::FromString(TEXT("★★"))));
	TestTrue(TEXT("Useful uses one star"),
		UGridSkillEntryWidget::ResolveAffinityGlyph(EGridSkillClassAffinity::Useful).EqualTo(FText::FromString(TEXT("★"))));
	TestTrue(TEXT("Neutral uses an empty star"),
		UGridSkillEntryWidget::ResolveAffinityGlyph(EGridSkillClassAffinity::Neutral).EqualTo(FText::FromString(TEXT("☆"))));
	TestFalse(TEXT("Discouraged glyph differs from Useful"),
		UGridSkillEntryWidget::ResolveAffinityGlyph(EGridSkillClassAffinity::Discouraged).EqualTo(
			UGridSkillEntryWidget::ResolveAffinityGlyph(EGridSkillClassAffinity::Useful)));

	UClass* SkillsClass = UGridSkillsWidget::StaticClass();
	for (const FName PropertyName : { FName(TEXT("Button_SortAffinity")), FName(TEXT("Text_SortAffinityIndicator")) })
	{
		FProperty* Property = FindFProperty<FProperty>(SkillsClass, PropertyName);
		TestNotNull(*FString::Printf(TEXT("%s affinity header hook exists"), *PropertyName.ToString()), Property);
		if (Property)
		{
			TestTrue(*FString::Printf(TEXT("%s remains optional before WBP materialization"), *PropertyName.ToString()),
				Property->HasMetaData(TEXT("BindWidgetOptional")));
		}
	}

	UClass* EntryClass = UGridSkillEntryWidget::StaticClass();
	for (const FName PropertyName : {
		FName(TEXT("Text_SkillAffinity")),
		FName(TEXT("Border_AffinityRecommended")),
		FName(TEXT("Border_AffinityUseful")),
		FName(TEXT("Border_AffinityNeutral")),
		FName(TEXT("Border_AffinityDiscouraged"))
	})
	{
		FProperty* Property = FindFProperty<FProperty>(EntryClass, PropertyName);
		TestNotNull(*FString::Printf(TEXT("%s affinity row hook exists"), *PropertyName.ToString()), Property);
		if (Property)
		{
			TestTrue(*FString::Printf(TEXT("%s remains optional before WBP materialization"), *PropertyName.ToString()),
				Property->HasMetaData(TEXT("BindWidgetOptional")));
		}
	}
	return true;
}

#endif
