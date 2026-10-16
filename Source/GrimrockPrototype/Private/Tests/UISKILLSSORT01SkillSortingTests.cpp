#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "UI/GridSkillsWidget.h"
#include "UObject/UnrealType.h"

namespace UISKILLSSORT01Tests
{
	FGridSkillEntryView MakeEntry(
		const TCHAR* SkillId,
		const TCHAR* DisplayName,
		ERPGSkillGoverningAttribute Attribute,
		int32 Rank)
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
		{ TEXT("Skill_HeavyWeapons"), TEXT("Skill_Acrobatics"), TEXT("Skill_Shield"), TEXT("Skill_Alchemy") },
		TEXT("Rank descending reverses the complete presentation order"));
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
		TestNotNull(*FString::Printf(TEXT("%s optional Designer hook exists"), *PropertyName.ToString()), Property);
		if (Property)
		{
			TestTrue(*FString::Printf(TEXT("%s remains optional before WBP materialization"), *PropertyName.ToString()),
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
