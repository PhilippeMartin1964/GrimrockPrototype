#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Blueprint.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UObject/UnrealType.h"
#include "UI/GridInventoryUiTypes.h"
#include "UI/GridItemTooltipWidget.h"

/**
 * UI-TOOLTIP-CPP-CLEAN02.3: ensure the actual native tooltip UMG assets
 * recompile with the reduced C++ model. Never edit/save any .uasset here.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorUITooltipCppClean023BlueprintCompatibility,
    "Grimrock.Editor.UITooltipCppClean02.3.BlueprintCompatibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorUITooltipCppClean023BlueprintCompatibility::RunTest(const FString&)
{
    const UScriptStruct* Comparison = FGridItemTooltipEquipmentComparison::StaticStruct();
    const UScriptStruct* Stat = FGridItemTooltipStatLine::StaticStruct();
    TestNull(TEXT("Legacy comparison occupancy flag absent"),
        FindFProperty<FProperty>(Comparison, TEXT("bHasEquippedItem")));
    TestNull(TEXT("Redundant equipped-item name absent"),
        FindFProperty<FProperty>(Comparison, TEXT("EquippedItemName")));
    TestNotNull(TEXT("Canonical equipped-item display remains"),
        FindFProperty<FProperty>(Comparison, TEXT("EquippedItemDisplayText")));
    TestNull(TEXT("Redundant stat delta text absent"),
        FindFProperty<FProperty>(Stat, TEXT("DeltaText")));
    TestNotNull(TEXT("Canonical formatted delta display remains"),
        FindFProperty<FProperty>(Stat, TEXT("DeltaDisplayText")));

    const TCHAR* ActivePaths[] = {
        TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemTooltip.WBP_ItemTooltip"),
        TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemTooltipNativeLine.WBP_ItemTooltipNativeLine"),
        TEXT("/Game/GrimrockPrototype/Blueprints/UI/Inventory/WBP_ItemTooltipNativeComparison.WBP_ItemTooltipNativeComparison")
    };

    for (const TCHAR* Path : ActivePaths)
    {
        UBlueprint* Widget = LoadObject<UBlueprint>(nullptr, Path);
        if (!TestNotNull(*FString::Printf(TEXT("Native tooltip Blueprint loads: %s"), Path), Widget))
        {
            continue;
        }
        FKismetEditorUtilities::CompileBlueprint(Widget);
        TestTrue(*FString::Printf(TEXT("Native tooltip Blueprint compiles without removed fields: %s"), Path),
            Widget->Status != BS_Error);
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
