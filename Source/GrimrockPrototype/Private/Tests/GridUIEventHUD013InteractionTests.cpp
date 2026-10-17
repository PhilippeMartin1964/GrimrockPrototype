#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/GridEventFeedTypes.h"
#include "UI/GridEventFeedWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD013FiltersTest, "Grimrock.UI.EventHUD01.Interaction.Filters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD013FiltersTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridEventFeedWidget* Widget = NewObject<UGridEventFeedWidget>();
	if (!TestNotNull(TEXT("Event Feed widget exists"), Widget))
	{
		return false;
	}

	TestTrue(TEXT("Events filter starts enabled"), Widget->IsCategoryEnabled(EGridHudEventCategory::Dungeon));
	TestTrue(TEXT("Combat filter starts enabled"), Widget->IsCategoryEnabled(EGridHudEventCategory::Combat));
	TestTrue(TEXT("Discussion filter starts enabled"), Widget->IsCategoryEnabled(EGridHudEventCategory::Dialogue));

	Widget->SetCategoryEnabled(EGridHudEventCategory::Combat, false);
	TestFalse(TEXT("Combat can be hidden independently"), Widget->IsCategoryEnabled(EGridHudEventCategory::Combat));
	TestTrue(TEXT("Events remain enabled"), Widget->IsCategoryEnabled(EGridHudEventCategory::Dungeon));
	TestTrue(TEXT("Discussion remains enabled"), Widget->IsCategoryEnabled(EGridHudEventCategory::Dialogue));

	TArray<FGridHudEventEntry> SourceEntries;
	for (EGridHudEventCategory Category :
		{ EGridHudEventCategory::Dungeon, EGridHudEventCategory::Combat, EGridHudEventCategory::Dialogue })
	{
		FGridHudEventEntry Entry;
		Entry.Category = Category;
		Entry.Message = FText::FromString(TEXT("Test"));
		SourceEntries.Add(Entry);
	}

	TArray<FGridHudEventEntry> Filtered;
	UGridEventFeedWidget::FilterEntries(SourceEntries, true, false, true, Filtered);
	TestEqual(TEXT("Filtering does not mutate authoritative history"), SourceEntries.Num(), 3);
	TestEqual(TEXT("Only two enabled categories are projected"), Filtered.Num(), 2);
	if (Filtered.Num() == 2)
	{
		TestEqual(TEXT("Dungeon remains first in chronological projection"), Filtered[0].Category, EGridHudEventCategory::Dungeon);
		TestEqual(TEXT("Dialogue remains after hidden combat"), Filtered[1].Category, EGridHudEventCategory::Dialogue);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD013ResizeTest, "Grimrock.UI.EventHUD01.Interaction.Resize",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD013ResizeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridEventFeedWidget* Widget = NewObject<UGridEventFeedWidget>();
	if (!TestNotNull(TEXT("Event Feed widget exists"), Widget))
	{
		return false;
	}

	Widget->SetPanelHeight(320.0f);
	Widget->BeginResizeAtScreenY(500.0f, 1.0f);
	Widget->UpdateResizeAtScreenY(450.0f);
	TestEqual(TEXT("At DPI scale 1, 50 pixels moves the border by 50 logical units"), Widget->GetPanelHeight(), 370.0f);
	Widget->EndResize();

	Widget->SetPanelHeight(320.0f);
	Widget->BeginResizeAtScreenY(500.0f, 2.0f);
	Widget->UpdateResizeAtScreenY(400.0f);
	TestEqual(TEXT("At DPI scale 2, 100 pixels converts to 50 logical units"), Widget->GetPanelHeight(), 370.0f);

	Widget->UpdateResizeAtScreenY(900.0f);
	TestEqual(TEXT("Dragging downward clamps to minimum height"), Widget->GetPanelHeight(), 180.0f);
	Widget->EndResize();

	Widget->BeginResizeAtScreenY(500.0f, 0.5f);
	Widget->UpdateResizeAtScreenY(250.0f);
	TestEqual(TEXT("At DPI scale 0.5, 250 pixels reaches the maximum"), Widget->GetPanelHeight(), 600.0f);
	Widget->EndResize();

	TestEqual(TEXT("Screen delta converts through viewport scale"),
		UGridEventFeedWidget::ScreenDeltaToViewportDelta(100.0f, 2.0f), 50.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD013AutoScrollTest, "Grimrock.UI.EventHUD01.Interaction.AutoScroll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD013AutoScrollTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestTrue(TEXT("Exact end follows newest entries"), UGridEventFeedWidget::IsScrollAtEnd(120.0f, 120.0f));
	TestTrue(TEXT("Small rounding gap still counts as the end"), UGridEventFeedWidget::IsScrollAtEnd(114.0f, 120.0f, 8.0f));
	TestFalse(TEXT("Reading older history disables forced autoscroll"), UGridEventFeedWidget::IsScrollAtEnd(80.0f, 120.0f, 8.0f));
	TestTrue(TEXT("Empty scroll content is treated as already at the end"), UGridEventFeedWidget::IsScrollAtEnd(0.0f, 0.0f));
	return true;
}

#endif
