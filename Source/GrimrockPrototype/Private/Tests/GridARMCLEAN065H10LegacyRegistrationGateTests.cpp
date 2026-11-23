#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../Runtime/GridLegacyInventoryRegistrationGate.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean065H10RegistrationGate,
    "Grimrock.RPG.ARM.CLEAN06_5H.LegacyInventoryRegistrationGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean065H10RegistrationGate::RunTest(const FString&)
{
    TestFalse(TEXT("Ordinary Editor runtime cannot register old definitions"),
        FGridLegacyInventoryRegistrationGate::Allows(false, false));
    TestTrue(TEXT("Isolated Automation without a GameInstance may still use the historical map"),
        FGridLegacyInventoryRegistrationGate::Allows(true, false));
    TestFalse(TEXT("GameInstance-backed Automation cannot use the historical registry"),
        FGridLegacyInventoryRegistrationGate::Allows(true, true));
    return !HasAnyErrors();
}

#endif
