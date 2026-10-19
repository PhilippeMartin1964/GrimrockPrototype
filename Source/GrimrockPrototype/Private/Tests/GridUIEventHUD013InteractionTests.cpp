#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/CheckBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Runtime/GridEventFeedTypes.h"
#include "Runtime/GridEventFeedSubsystem.h"
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

	Widget->Text_FilterEvents = NewObject<UTextBlock>(Widget);
	Widget->Text_FilterCombat = NewObject<UTextBlock>(Widget);
	Widget->Text_FilterDiscussion = NewObject<UTextBlock>(Widget);
	Widget->CheckBox_FilterEvents = NewObject<UCheckBox>(Widget);
	Widget->CheckBox_FilterCombat = NewObject<UCheckBox>(Widget);
	Widget->CheckBox_FilterDiscussion = NewObject<UCheckBox>(Widget);
	Widget->SynchronizeProperties();

	TestEqual(TEXT("Events filter uses plain text"), Widget->Text_FilterEvents->GetText().ToString(), FString(TEXT("Événements")));
	TestEqual(TEXT("Combat filter uses plain text"), Widget->Text_FilterCombat->GetText().ToString(), FString(TEXT("Combat")));
	TestEqual(TEXT("Discussion filter uses plain text"), Widget->Text_FilterDiscussion->GetText().ToString(), FString(TEXT("Discussion")));
	TestTrue(TEXT("Native events checkbox starts checked"), Widget->CheckBox_FilterEvents->IsChecked());
	TestTrue(TEXT("Native combat checkbox starts checked"), Widget->CheckBox_FilterCombat->IsChecked());
	TestTrue(TEXT("Native discussion checkbox starts checked"), Widget->CheckBox_FilterDiscussion->IsChecked());

	Widget->SetCategoryEnabled(EGridHudEventCategory::Combat, false);
	TestFalse(TEXT("Disabling combat unchecks its native checkbox"), Widget->CheckBox_FilterCombat->IsChecked());
	TestTrue(TEXT("Other native checkboxes stay checked"), Widget->CheckBox_FilterEvents->IsChecked() &&
		Widget->CheckBox_FilterDiscussion->IsChecked());
	Widget->SetCategoryEnabled(EGridHudEventCategory::Combat, true);
	TestTrue(TEXT("Re-enabling combat checks its native checkbox"), Widget->CheckBox_FilterCombat->IsChecked());
	// Automation constructs the widget without NativeConstruct(), so invoke the
	// UFUNCTION target directly instead of broadcasting an unbound delegate.
	UFunction* CheckStateHandler = Widget->FindFunction(FName(TEXT("HandleFilterCombatCheckStateChanged")));
	if (TestNotNull(TEXT("Native combat checkbox has a state-change handler"), CheckStateHandler))
	{
		struct FCheckStateChangeParams { bool bIsChecked; } Params{false};
		Widget->ProcessEvent(CheckStateHandler, &Params);
		TestFalse(TEXT("Native combat state-change handler disables the filter"), Widget->IsCategoryEnabled(EGridHudEventCategory::Combat));
		Params.bIsChecked = true;
		Widget->ProcessEvent(CheckStateHandler, &Params);
		TestTrue(TEXT("Native combat state-change handler restores the filter"), Widget->IsCategoryEnabled(EGridHudEventCategory::Combat));
	}
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

	TestTrue(TEXT("Exact end is detected for filter rebuilds"), UGridEventFeedWidget::IsScrollAtEnd(120.0f, 120.0f));
	TestTrue(TEXT("Small rounding gap still counts as the end during filter rebuild"), UGridEventFeedWidget::IsScrollAtEnd(114.0f, 120.0f, 8.0f));
	TestFalse(TEXT("Older history is detected for non-publication rebuilds"), UGridEventFeedWidget::IsScrollAtEnd(80.0f, 120.0f, 8.0f));
	TestTrue(TEXT("Empty scroll content is treated as already at the end"), UGridEventFeedWidget::IsScrollAtEnd(0.0f, 0.0f));
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD016ScrollFollowLayoutTest,
	"Grimrock.UI.EventHUD01.Interaction.ScrollFollowLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD016ScrollFollowLayoutTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	// The previous AutoScroll test checked only IsScrollAtEnd(float,float).
	// Here a real Event Feed publication rebuilds real UMG ScrollBox children
	// and requests a bounded follow-up after Slate layout.
	UGridEventFeedWidget* Widget = NewObject<UGridEventFeedWidget>();
	UGridEventFeedSubsystem* Feed = NewObject<UGridEventFeedSubsystem>();
	if (!TestNotNull(TEXT("Event Feed widget exists"), Widget) ||
		!TestNotNull(TEXT("Event Feed subsystem exists"), Feed))
	{
		return false;
	}
	Widget->ScrollBox_Entries = NewObject<UScrollBox>(Widget);
	if (!TestNotNull(TEXT("Native ScrollBox component exists"), Widget->ScrollBox_Entries.Get()))
	{
		return false;
	}
	Widget->EventFeedSubsystem = Feed;
	Feed->OnEventPublished.AddUniqueDynamic(Widget, &UGridEventFeedWidget::HandleEventPublished);

	FGridHudEventEntry Entry;
	Entry.Category = EGridHudEventCategory::Dungeon;
	Entry.Visibility = EGridEventVisibility::Always;
	Entry.Message = FText::FromString(TEXT("Long first event, with content that can wrap when the panel is narrow."));
	TestTrue(TEXT("Publishing the first event is accepted"), Feed->PublishEvent(Entry, true));
	TestEqual(TEXT("One row is rebuilt inside the ScrollBox"), Widget->ScrollBox_Entries->GetChildrenCount(), 1);
	TestTrue(TEXT("First event queues layout retries"), Widget->PendingScrollToEndFrames > 0);

	while (Widget->PendingScrollToEndFrames > 0)
	{
		Widget->ApplyPendingScrollToEnd();
	}
	TestEqual(TEXT("Follow-newest retries terminate without an infinite scroll lock"),
		Widget->PendingScrollToEndFrames, 0);

	Entry.Message = FText::FromString(TEXT("A newly published event is the latest row."));
	TestTrue(TEXT("Publishing another visible event is accepted"), Feed->PublishEvent(Entry, true));
	TestEqual(TEXT("A second event rebuilds two chronological rows"),
		Widget->ScrollBox_Entries->GetChildrenCount(), 2);
	TestTrue(TEXT("Each visible event re-arms deferred bottom alignment"),
		Widget->PendingScrollToEndFrames > 0);

	Widget->SetCategoryEnabled(EGridHudEventCategory::Dungeon, false);
	TestEqual(TEXT("Hidden category removes the displayed rows"),
		Widget->ScrollBox_Entries->GetChildrenCount(), 0);
	// Unrelated hidden publications may not force newly visible content.
	while (Widget->PendingScrollToEndFrames > 0)
	{
		Widget->ApplyPendingScrollToEnd();
	}
	Entry.Message = FText::FromString(TEXT("Hidden event must not force autoscroll."));
	TestTrue(TEXT("Hidden-category event still enters authoritative history"),
		Feed->PublishEvent(Entry, true));
	TestEqual(TEXT("Hidden event leaves the presentation empty"),
		Widget->ScrollBox_Entries->GetChildrenCount(), 0);
	TestEqual(TEXT("Hidden event does not queue a new scroll request"),
		Widget->PendingScrollToEndFrames, 0);

	Feed->OnEventPublished.RemoveDynamic(Widget, &UGridEventFeedWidget::HandleEventPublished);
	return true;
}

#endif
