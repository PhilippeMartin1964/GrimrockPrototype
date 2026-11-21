#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../Runtime/GridLegacyInventoryRegistrationGate.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean065H10RegistrationGate,
    "Grimrock.RPG.ARM.CLEAN06_5H.LegacyInventoryRegistrationGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean065H10RegistrationGate::RunTest(const FString&)
{
    TestFalse(TEXT("Ordinary Editor runtime cannot register old definitions"),
        FGridLegacyInventoryRegistrationGate::Allows(false));
    TestTrue(TEXT("Actual Automation may characterize the historical registry"),
        FGridLegacyInventoryRegistrationGate::Allows(true));
    // Integration of the old native map remains covered by TD06.8 tests.
    // Game-world use of that map remains refused by canonical resolver gates.
    return !HasAnyErrors();
}

#endif
