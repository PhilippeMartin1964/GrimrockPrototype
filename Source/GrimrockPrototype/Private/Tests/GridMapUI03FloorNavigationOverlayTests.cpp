#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "UI/GridMapWidget.h"
#include "UI/GridMapSurfaceWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapUI03OverlayContractTest, "Grimrock.UI.MapUI03.OverlayContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapUI03OverlayContractTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* MapClass = UGridMapWidget::StaticClass();
	if (!TestNotNull(TEXT("Map widget class exists"), MapClass))
	{
		return false;
	}

	const FObjectPropertyBase* SurfaceProperty =
		CastField<FObjectPropertyBase>(MapClass->FindPropertyByName(TEXT("MapSurface")));
	const FObjectPropertyBase* FloorNavigationBorderProperty =
		CastField<FObjectPropertyBase>(MapClass->FindPropertyByName(TEXT("Border_FloorNavigation")));
	const FObjectPropertyBase* UpProperty =
		CastField<FObjectPropertyBase>(MapClass->FindPropertyByName(TEXT("Button_LevelUp")));
	const FObjectPropertyBase* DownProperty =
		CastField<FObjectPropertyBase>(MapClass->FindPropertyByName(TEXT("Button_LevelDown")));
	const FObjectPropertyBase* LabelProperty =
		CastField<FObjectPropertyBase>(MapClass->FindPropertyByName(TEXT("Text_FloorLabel")));

	TestTrue(TEXT("Dedicated map surface binding is a UGridMapSurfaceWidget"),
		SurfaceProperty && SurfaceProperty->PropertyClass == UGridMapSurfaceWidget::StaticClass());
	TestTrue(TEXT("Canonical floor-navigation container binding is a UBorder"),
		FloorNavigationBorderProperty && FloorNavigationBorderProperty->PropertyClass == UBorder::StaticClass());
	TestTrue(TEXT("Existing Level Up binding remains a UButton"),
		UpProperty && UpProperty->PropertyClass == UButton::StaticClass());
	TestTrue(TEXT("Existing Level Down binding remains a UButton"),
		DownProperty && DownProperty->PropertyClass == UButton::StaticClass());
	TestTrue(TEXT("Existing floor label binding remains a UTextBlock"),
		LabelProperty && LabelProperty->PropertyClass == UTextBlock::StaticClass());

	TestNotNull(TEXT("Existing NavigateFloorUp authority remains"),
		MapClass->FindFunctionByName(TEXT("NavigateFloorUp")));
	TestNotNull(TEXT("Existing NavigateFloorDown authority remains"),
		MapClass->FindFunctionByName(TEXT("NavigateFloorDown")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridMapUI03NoFloorVisibilityTest, "Grimrock.UI.MapUI03.NoFloorSelectionCollapsesOverlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridMapUI03NoFloorVisibilityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UGridMapWidget* Widget = NewObject<UGridMapWidget>();
	UBorder* FloorNavigationBorder = NewObject<UBorder>(Widget);
	if (!TestNotNull(TEXT("Map widget exists"), Widget) ||
		!TestNotNull(TEXT("Floor navigation border exists"), FloorNavigationBorder))
	{
		return false;
	}

	Widget->Border_FloorNavigation = FloorNavigationBorder;
	FloorNavigationBorder->SetVisibility(ESlateVisibility::Visible);

	// No party/runtime source means there is no selected floor.
	Widget->InitializeMapWidget(nullptr);

	TestEqual(TEXT("Floor navigation is hidden when no logical floor can be selected"),
		FloorNavigationBorder->GetVisibility(), ESlateVisibility::Collapsed);

	return true;
}

#endif
