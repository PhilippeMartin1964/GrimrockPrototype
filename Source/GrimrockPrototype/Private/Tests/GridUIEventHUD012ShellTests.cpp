#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "UI/GridEventFeedWidget.h"
#include "UI/GridPersistentHudWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD012DimensionsTest, "Grimrock.UI.EventHUD01.Shell.Dimensions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD012DimensionsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridEventFeedWidget* Widget = NewObject<UGridEventFeedWidget>();
	if (!TestNotNull(TEXT("Native Event Feed shell exists"), Widget))
	{
		return false;
	}

	TestEqual(TEXT("V1 shell width is fixed to 560 logical units"), Widget->GetPanelWidth(), 560.0f);
	TestEqual(TEXT("Initial shell height is 320 logical units"), Widget->GetPanelHeight(), 320.0f);
	TestEqual(TEXT("Minimum shell height is 180 logical units"), Widget->GetMinPanelHeight(), 180.0f);
	TestEqual(TEXT("Maximum shell height is 600 logical units"), Widget->GetMaxPanelHeight(), 600.0f);

	Widget->SetPanelHeight(40.0f);
	TestEqual(TEXT("Height clamps to the minimum"), Widget->GetPanelHeight(), 180.0f);
	Widget->SetPanelHeight(900.0f);
	TestEqual(TEXT("Height clamps to the maximum"), Widget->GetPanelHeight(), 600.0f);
	Widget->SetPanelHeight(360.0f);
	TestEqual(TEXT("Height accepts a value inside the allowed range"), Widget->GetPanelHeight(), 360.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridUIEventHUD012PersistentPlacementTest, "Grimrock.UI.EventHUD01.Shell.PersistentPlacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridUIEventHUD012PersistentPlacementTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(TEXT("56-unit Hotbar plus 8-unit gap resolves to 64"),
		UGridPersistentHudWidget::CalculateEventFeedBottomOffset(56.0f, 0.0f, 8.0f), 64.0f);
	TestEqual(TEXT("Fallback clearance protects the shell before geometry is valid"),
		UGridPersistentHudWidget::CalculateEventFeedBottomOffset(0.0f, 64.0f, 8.0f), 72.0f);
	TestEqual(TEXT("Larger authored bottom bar wins over the fallback"),
		UGridPersistentHudWidget::CalculateEventFeedBottomOffset(72.0f, 64.0f, 8.0f), 80.0f);
	TestEqual(TEXT("Negative layout inputs are sanitized"),
		UGridPersistentHudWidget::CalculateEventFeedBottomOffset(-10.0f, -5.0f, -2.0f), 0.0f);

	TestTrue(TEXT("Event Feed shell is a UUserWidget"),
		UGridEventFeedWidget::StaticClass()->IsChildOf(UUserWidget::StaticClass()));
	return true;
}

#endif
