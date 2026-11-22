#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean065H3ApprovedStarterRegistrylessGate,
    "Grimrock.RPG.ARM.CLEAN06_5H.RegistrylessApprovedStarterFixture",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean065H3ApprovedStarterRegistrylessGate::RunTest(const FString&)
{
    URPGClassAsset* Transient = NewObject<URPGClassAsset>(GetTransientPackage());
    if (!TestNotNull(TEXT("Transient production-class preview exists"), Transient)) return false;
    Transient->ClassId = TEXT("Warrior");

    // Even during Automation there is no permission to create a historical
    // definition or infer an unregistered source item.
    FString Error;
    TestFalse(TEXT("Automation refuses registryless approved starter recipes"),
        FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(
            *Transient, Error, nullptr));
    TestTrue(TEXT("Missing canonical ItemSystem is reported"),
        Error.Contains(TEXT("canonical ItemSystem")));
    TestTrue(TEXT("Registryless refusal does not grant a starting kit"),
        Transient->StartingEquipmentKits.IsEmpty());
    TestTrue(TEXT("Registryless refusal does not set a default kit"),
        Transient->DefaultStartingEquipmentKitId.IsNone());
    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
