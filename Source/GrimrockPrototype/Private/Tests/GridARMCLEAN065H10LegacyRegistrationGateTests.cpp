#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GridPartyInventoryComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean065H10RegistrationGate,
    "Grimrock.RPG.ARM.CLEAN06_5H.LegacyInventoryRegistrationGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean065H10RegistrationGate::RunTest(const FString&)
{
    auto* Inventory = NewObject<UGridPartyInventoryComponent>();
    Inventory->InitializeDefaultPartyIfNeeded();
    FGridItemInstance Item;
    Item.RuntimeObjectId = FGuid::NewGuid();
    Item.ItemDefinitionId = TEXT("Item_MON11_UnknownWithoutRegistry");
    Item.Quantity = 1;
    const FGridItemInstance Before = Item;
    TestNull(TEXT("The retired transient property is no longer reflected"),
        Inventory->GetClass()->FindPropertyByName(TEXT("RuntimeItemDefinitionsById")));
    TestFalse(TEXT("Automation without a GameInstance cannot apply item metadata"),
        Inventory->ApplyItemDefinitionToInstance(Item));
    TestFalse(TEXT("Automation without a GameInstance cannot grant an unknown item"),
        Inventory->AddItemToCharacterInventory(0, Item));
    TestTrue(TEXT("Refusal preserves the item identity and all owned slots"),
        FGridItemInstance::StaticStruct()->CompareScriptStruct(&Item, &Before, 0) &&
        Inventory->CountItemDefinitionInCharacterInventory(0, Item.ItemDefinitionId) == 0);
    return !HasAnyErrors();
}

#endif
