#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallFunction.h"
#include "K2Node_FunctionEntry.h"
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
        if (Widget->GetFName() == TEXT("WBP_ItemTooltip"))
        {
            const UEdGraph* Graph = nullptr;
            for (const UEdGraph* Candidate : Widget->FunctionGraphs)
            {
                if (Candidate && Candidate->GetFName() == TEXT("InitFromInventorySlot"))
                    Graph = Candidate;
            }
            if (!TestNotNull(TEXT("Native tooltip initialization graph remains"), Graph))
                return false;

            const UK2Node_FunctionEntry* Entry = nullptr;
            const UK2Node_CallFunction* Presenter = nullptr;
            for (const UEdGraphNode* Node : Graph->Nodes)
            {
                if (const auto* FunctionEntry = Cast<UK2Node_FunctionEntry>(Node))
                    Entry = FunctionEntry;
                if (const auto* Call = Cast<UK2Node_CallFunction>(Node))
                    Presenter = Call;
                if (Node)
                    for (const UEdGraphPin* Pin : Node->Pins)
                        TestTrue(TEXT("Tooltip initialization has no orphaned pins"), Pin && !Pin->bOrphanedPin);
            }

            TestEqual(TEXT("Tooltip initialization contains only its entry and native presentation"), Graph->Nodes.Num(), 2);
            if (!TestNotNull(TEXT("Function entry"), Entry) || !TestNotNull(TEXT("Presenter call"), Presenter))
                return false;
            const UFunction* Target = Presenter->GetTargetFunction();
            TestTrue(TEXT("Tooltip delegates to native PresentFromInventorySlot"),
                Target && Target->GetOuter() == UGridItemTooltipWidget::StaticClass() &&
                Target->GetFName() == GET_FUNCTION_NAME_CHECKED(UGridItemTooltipWidget, PresentFromInventorySlot));

            const UEdGraphPin* Exec = Entry->FindPin(UEdGraphSchema_K2::PN_Then);
            const UEdGraphPin* Slot = Entry->FindPin(TEXT("SlotWidget"));
            const UEdGraphPin* PresenterSlot = Presenter->FindPin(TEXT("InInventorySlot"));
            const UEdGraphPin* Then = Presenter->FindPin(UEdGraphSchema_K2::PN_Then);
            TestTrue(TEXT("Initialization executes native presentation exactly once"),
                Exec && Exec->LinkedTo.Num() == 1 && Exec->LinkedTo[0]->GetOwningNode() == Presenter &&
                Then && Then->LinkedTo.IsEmpty());
            TestTrue(TEXT("Native presentation receives the original slot"),
                Slot && PresenterSlot && PresenterSlot->LinkedTo.Num() == 1 && PresenterSlot->LinkedTo[0] == Slot);
        }
        FKismetEditorUtilities::CompileBlueprint(Widget);
        TestTrue(*FString::Printf(TEXT("Native tooltip Blueprint compiles without removed fields: %s"), Path),
            Widget->Status != BS_Error);
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
