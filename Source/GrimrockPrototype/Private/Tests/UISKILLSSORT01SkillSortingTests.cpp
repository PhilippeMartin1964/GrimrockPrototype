#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include <initializer_list>

#include "Internationalization/Internationalization.h"
#include "UI/GridSkillsWidget.h"
#include "UObject/UnrealType.h"

namespace UISKILLSSORT01Tests
{
	FGridSkillEntryView MakeEntry(
		const TCHAR* SkillId,
		const TCHAR* DisplayName,
		ERPGSkillGoverningAttribute Attribute,
		int32 Rank,
		EGridSkillClassAffinity Affinity = EGridSkillClassAffinity::Neutral)
	{
		FGridSkillEntryView Entry;
		Entry.SkillId = FName(SkillId);
		Entry.DisplayName = FText::FromString(DisplayName);
		Entry.GoverningAttribute = Attribute;
		Entry.Rank = Rank;
		Entry.MaxRank = 5;
		Entry.CurrentRankCap = 5;
		Entry.bTrained = Rank > 0;
		Entry.bAllowUntrainedChecks = true;
		Entry.ClassAffinity = Affinity;
		return Entry;
	}

	TArray<FGridSkillEntryView> MakeEntries()
	{
		return {
			MakeEntry(TEXT("Skill_HeavyWeapons"), TEXT("Armes lourdes"), ERPGSkillGoverningAttribute::Strength, 2),
			MakeEntry(TEXT("Skill_Alchemy"), TEXT("Alchimie"), ERPGSkillGoverningAttribute::Intelligence, 0),
			MakeEntry(TEXT("Skill_Shield"), TEXT("Bouclier"), ERPGSkillGoverningAttribute::Constitution, 0),
			MakeEntry(TEXT("Skill_Acrobatics"), TEXT("Acrobatie"), ERPGSkillGoverningAttribute::Dexterity, 2)
		};
	}

	void ExpectOrder(
		FAutomationTestBase& Test,
		const TArray<FGridSkillEntryView>& Entries,
		std::initializer_list<const TCHAR*> Expected,
		const TCHAR* Label)
	{
		int32 Index = 0;
		for (const TCHAR* ExpectedId : Expected)
		{
			Test.TestEqual(
				*FString::Printf(TEXT("%s [%d]"), Label, Index),
				Entries.IsValidIndex(Index) ? Entries[Index].SkillId : NAME_None,
				FName(ExpectedId));
			++Index;
		}
	}

	struct FCultureStateGuard
	{
		FCultureStateGuard()
			: Internationalization(FInternationalization::Get())
		{
			Internationalization.BackupCultureState(Snapshot);
		}

		~FCultureStateGuard()
		{
			Internationalization.RestoreCultureState(Snapshot);
		}

		FInternationalization& Internationalization;
		FInternationalization::FCultureStateSnapshot Snapshot;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLSSORT01NameSortingTest,
	"Grimrock.UI.Skills02.Sort.Name",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLSSORT01NameSortingTest::RunTest(const FString&)
{
	using namespace UISKILLSSORT01Tests;

	TArray<FGridSkillEntryView> Entries = MakeEntries();
	UGridSkillsWidget::SortSkillEntries(Entries, EGridSkillSortColumn::Name, EGridSkillSortDirection::Ascending);
	ExpectOrder(*this, Entries,
		{ TEXT("Skill_Acrobatics"), TEXT("Skill_Alchemy"), TEXT("Skill_HeavyWeapons"), TEXT("Skill_Shield") },
		TEXT("Name ascending"));

	UGridSkillsWidget::SortSkillEntries(Entries, EGridSkillSortColumn::Name, EGridSkillSortDirection::Descending);
	ExpectOrder(*this, Entries,
		{ TEXT("Skill_Shield"), TEXT("Skill_HeavyWeapons"), TEXT("Skill_Alchemy"), TEXT("Skill_Acrobatics") },
		TEXT("Name descending"));

	FCultureStateGuard CultureGuard;
	const bool bFrenchCultureAvailable =
		CultureGuard.Internationalization.SetCurrentCulture(TEXT("fr"));
	TestTrue(TEXT("French culture is available for localized collation"), bFrenchCultureAvailable);
	if (!bFrenchCultureAvailable)
	{
		return false;
	}

	TArray<FGridSkillEntryView> AccentedEntries = {
		MakeEntry(TEXT("Skill_Zoology"), TEXT("Zoologie"), ERPGSkillGoverningAttribute::Intelligence, 0),
		MakeEntry(TEXT("Skill_Evasion"), TEXT("Évasion"), ERPGSkillGoverningAttribute::Dexterity, 0)
	};
	UGridSkillsWidget::SortSkillEntries(
		AccentedEntries,
		EGridSkillSortColumn::Name,
		EGridSkillSortDirection::Ascending);
	ExpectOrder(
		*this,
		AccentedEntries,
		{ TEXT("Skill_Evasion"), TEXT("Skill_Zoology") },
		TEXT("French collation keeps accented E with E before Z"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLSSORT01AttributeSortingTest,
	"Grimrock.UI.Skills02.Sort.Attribute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLSSORT01AttributeSortingTest::RunTest(const FString&)
{
	using namespace UISKILLSSORT01Tests;

	TArray<FGridSkillEntryView> Entries = MakeEntries();
	UGridSkillsWidget::SortSkillEntries(Entries, EGridSkillSortColumn::Attribute, EGridSkillSortDirection::Ascending);
	ExpectOrder(*this, Entries,
		{ TEXT("Skill_Shield"), TEXT("Skill_Acrobatics"), TEXT("Skill_HeavyWeapons"), TEXT("Skill_Alchemy") },
		TEXT("Attribute ascending uses localized labels"));

	UGridSkillsWidget::SortSkillEntries(Entries, EGridSkillSortColumn::Attribute, EGridSkillSortDirection::Descending);
	ExpectOrder(*this, Entries,
		{ TEXT("Skill_Alchemy"), TEXT("Skill_HeavyWeapons"), TEXT("Skill_Acrobatics"), TEXT("Skill_Shield") },
		TEXT("Attribute descending uses localized labels"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLSSORT01AffinitySortingTest,
	"Grimrock.UI.Skills02.Sort.Affinity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLSSORT01AffinitySortingTest::RunTest(const FString&)
{
	using namespace UISKILLSSORT01Tests;

	TArray<FGridSkillEntryView> Entries = {
		MakeEntry(TEXT("Skill_Neutral"), TEXT("Neutre"), ERPGSkillGoverningAttribute::Wisdom, 0, EGridSkillClassAffinity::Neutral),
		MakeEntry(TEXT("Skill_UsefulB"), TEXT("B utile"), ERPGSkillGoverningAttribute::Wisdom, 0, EGridSkillClassAffinity::Useful),
		MakeEntry(TEXT("Skill_Discouraged"), TEXT("Déconseillée"), ERPGSkillGoverningAttribute::Wisdom, 0, EGridSkillClassAffinity::Discouraged),
		MakeEntry(TEXT("Skill_Recommended"), TEXT("Recommandée"), ERPGSkillGoverningAttribute::Wisdom, 0, EGridSkillClassAffinity::Recommended),
		MakeEntry(TEXT("Skill_UsefulA"), TEXT("A utile"), ERPGSkillGoverningAttribute::Wisdom, 0, EGridSkillClassAffinity::Useful)
	};

	UGridSkillsWidget::SortSkillEntries(
		Entries,
		EGridSkillSortColumn::Affinity,
		EGridSkillSortDirection::Ascending);
	ExpectOrder(
		*this,
		Entries,
		{
			TEXT("Skill_Recommended"),
			TEXT("Skill_UsefulA"),
			TEXT("Skill_UsefulB"),
			TEXT("Skill_Neutral"),
			TEXT("Skill_Discouraged")
		},
		TEXT("Affinity ascending uses semantic priority and localized name tie-break"));

	UGridSkillsWidget::SortSkillEntries(
		Entries,
		EGridSkillSortColumn::Affinity,
		EGridSkillSortDirection::Descending);
	ExpectOrder(
		*this,
		Entries,
		{
			TEXT("Skill_Discouraged"),
			TEXT("Skill_Neutral"),
			TEXT("Skill_UsefulA"),
			TEXT("Skill_UsefulB"),
			TEXT("Skill_Recommended")
		},
		TEXT("Affinity descending reverses semantic priority but keeps name ties ascending"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLSSORT01RankSortingTest,
	"Grimrock.UI.Skills02.Sort.Rank",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLSSORT01RankSortingTest::RunTest(const FString&)
{
	using namespace UISKILLSSORT01Tests;

	TArray<FGridSkillEntryView> Entries = MakeEntries();
	UGridSkillsWidget::SortSkillEntries(Entries, EGridSkillSortColumn::Rank, EGridSkillSortDirection::Ascending);
	ExpectOrder(*this, Entries,
		{ TEXT("Skill_Alchemy"), TEXT("Skill_Shield"), TEXT("Skill_Acrobatics"), TEXT("Skill_HeavyWeapons") },
		TEXT("Rank ascending falls back to Skill name"));

	UGridSkillsWidget::SortSkillEntries(Entries, EGridSkillSortColumn::Rank, EGridSkillSortDirection::Descending);
	ExpectOrder(*this, Entries,
		{ TEXT("Skill_Acrobatics"), TEXT("Skill_HeavyWeapons"), TEXT("Skill_Alchemy"), TEXT("Skill_Shield") },
		TEXT("Rank descending keeps alphabetical tie order"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUISKILLSSORT01WidgetContractTest,
	"Grimrock.UI.Skills02.Sort.WidgetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUISKILLSSORT01WidgetContractTest::RunTest(const FString&)
{
	UClass* SkillsClass = UGridSkillsWidget::StaticClass();

	for (const FName FunctionName : {
		FName(TEXT("SetSkillSort")),
		FName(TEXT("HandleSortNameClicked")),
		FName(TEXT("HandleSortAttributeClicked")),
		FName(TEXT("HandleSortRankClicked"))
	})
	{
		TestNotNull(*FString::Printf(TEXT("%s function exists"), *FunctionName.ToString()),
			SkillsClass->FindFunctionByName(FunctionName));
	}

	for (const FName PropertyName : {
		FName(TEXT("Button_SortName")),
		FName(TEXT("Button_SortAttribute")),
		FName(TEXT("Button_SortRank")),
		FName(TEXT("Text_SortNameIndicator")),
		FName(TEXT("Text_SortAttributeIndicator")),
		FName(TEXT("Text_SortRankIndicator"))
	})
	{
		FProperty* Property = FindFProperty<FProperty>(SkillsClass, PropertyName);
		TestNotNull(*FString::Printf(TEXT("%s materialized Designer hook exists"), *PropertyName.ToString()), Property);
		if (Property)
		{
			TestTrue(*FString::Printf(TEXT("%s is now a mandatory BindWidget"), *PropertyName.ToString()),
				Property->HasMetaData(TEXT("BindWidget")));
			TestFalse(*FString::Printf(TEXT("%s is no longer optional"), *PropertyName.ToString()),
				Property->HasMetaData(TEXT("BindWidgetOptional")));
		}
	}

	UGridSkillsWidget* Widget = NewObject<UGridSkillsWidget>();
	TestEqual(TEXT("Default sort column is Name"), Widget->SkillSortColumn, EGridSkillSortColumn::Name);
	TestEqual(TEXT("Default sort direction is ascending"), Widget->SkillSortDirection, EGridSkillSortDirection::Ascending);

	Widget->SetSkillSort(EGridSkillSortColumn::Rank, EGridSkillSortDirection::Descending);
	TestEqual(TEXT("SetSkillSort updates column"), Widget->SkillSortColumn, EGridSkillSortColumn::Rank);
	TestEqual(TEXT("SetSkillSort updates direction"), Widget->SkillSortDirection, EGridSkillSortDirection::Descending);
	return true;
}

#endif
