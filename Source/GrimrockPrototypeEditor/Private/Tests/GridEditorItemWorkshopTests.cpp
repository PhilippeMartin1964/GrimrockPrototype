#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../EditorTools/Inspection/GridItemWorkshopSession.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorItemWorkshopCatalogueTest,
	"Grimrock.Editor.Items.Inspection.Catalogue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorItemWorkshopCatalogueTest::RunTest(const FString&)
{
	FGridItemWorkshopSession Session;
	if (!TestTrue(TEXT("Configured production registry"), Session.Rebuild()))
	{
		AddError(Session.GetError());
		return false;
	}
	TestEqual(TEXT("Materialized definitions"), Session.GetEntries().Num(), 48);
	TArray<TSharedPtr<FGridWorkshopEntry>> Filtered;
	Session.Filter(TEXT(""), EGridWorkshopFilter::Armaments, Filtered);
	TestEqual(TEXT("ARM filter"), Filtered.Num(), 22);
	Session.Filter(TEXT("ARM014"), EGridWorkshopFilter::Armaments, Filtered);
	if (TestEqual(TEXT("Archetype search"), Filtered.Num(), 1))
	{
		TestEqual(TEXT("ARM014 item identity"), Filtered[0]->Id, FName(TEXT("Weapon_StraightSword_01")));
	}
	Session.Filter(TEXT("Stone"), EGridWorkshopFilter::All, Filtered);
	TestTrue(TEXT("Non-ARM is searchable"), Filtered.ContainsByPredicate(
		[](const TSharedPtr<FGridWorkshopEntry>& Entry) { return Entry->Id == FName(TEXT("Stone")); }));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorItemWorkshopSelectionTest,
	"Grimrock.Editor.Items.Inspection.Selection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorItemWorkshopSelectionTest::RunTest(const FString&)
{
	FGridItemWorkshopSession Session;
	if (!TestTrue(TEXT("Configured registry"), Session.Rebuild()))
	{
		return false;
	}
	TestTrue(TEXT("Select ARM014"), Session.Select(TEXT("Weapon_StraightSword_01")));
	for (int32 N = 1; N <= 20; ++N)
	{
		TestTrue(TEXT("Select exact level"), Session.SelectLevel(N));
		TestEqual(TEXT("No adjacent-level fallback"), Session.GetView().Item.ItemLevel, N);
	}
	TestTrue(TEXT("Refresh retains valid selection"), Session.Rebuild());
	TestEqual(TEXT("Level retained after refresh"), Session.GetSelectedLevel(), 20);
	TestFalse(TEXT("N21 rejected"), Session.SelectLevel(21));
	TestTrue(TEXT("Select unlevelled stone"), Session.Select(TEXT("Stone")));
	TestEqual(TEXT("Stone internal level"), Session.GetSelectedLevel(), 0);
	TestFalse(TEXT("No level control for stone"), Session.GetView().HasLevel());
	TestFalse(TEXT("No simulated ARM level for stone"), Session.SelectLevel(2));
	TestFalse(TEXT("Unknown identity fails closed"), Session.Select(TEXT("Inspection_Unknown")));
	TestFalse(TEXT("Prior valid view cleared"), Session.GetView().IsValid());
	return !HasAnyErrors();
}

#endif
