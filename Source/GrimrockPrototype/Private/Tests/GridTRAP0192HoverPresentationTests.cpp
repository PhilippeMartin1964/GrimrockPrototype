#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GridInteractableInterface.h"
#include "Runtime/GrimrockPlayerController.h"
#include "UI/GridNavigationTooltipWidget.h"
#include "UI/GridPersistentHudWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridTRAP0192HoverPresenterContract,
    "Grimrock.TRAP01.9.2.WorldHoverPresenterContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridTRAP0192HoverPresenterContract::RunTest(const FString&)
{
    const UClass* Controller = AGrimrockPlayerController::StaticClass();
    const UClass* Presenter = UGridNavigationTooltipWidget::StaticClass();
    const UClass* PersistentHud = UGridPersistentHudWidget::StaticClass();
    const UClass* Interface = UGridInteractableInterface::StaticClass();

    TestNotNull(TEXT("PlayerController exposes the authored world tooltip override"),
        Controller->FindPropertyByName(TEXT("WorldInteractionTooltipWidgetClass")));
    TestNotNull(TEXT("PlayerController keeps one transient tooltip instance"),
        Controller->FindPropertyByName(TEXT("WorldInteractionTooltipWidget")));
    TestNotNull(TEXT("Persistent HUD exposes the shared compact tooltip class"),
        PersistentHud->FindPropertyByName(TEXT("NavigationTooltipWidgetClass")));
    TestNotNull(TEXT("Tooltip presenter accepts runtime text"),
        Presenter->FindFunctionByName(TEXT("InitializeTooltip")));
    TestNotNull(TEXT("Tooltip presenter uses authored text block"),
        Presenter->FindPropertyByName(TEXT("Text_Tooltip")));
    TestNotNull(TEXT("World interaction API exposes hover text"),
        Interface->FindFunctionByName(TEXT("GetInteractionText")));
    return true;
}

#endif
