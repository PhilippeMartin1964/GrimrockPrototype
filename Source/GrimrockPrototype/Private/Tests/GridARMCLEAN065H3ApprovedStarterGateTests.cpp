#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../RPG/GridApprovedStarterLegacyFixtureGate.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean065H3ApprovedStarterRegistrylessGate,
    "Grimrock.RPG.ARM.CLEAN06_5H.RegistrylessApprovedStarterFixture",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean065H3ApprovedStarterRegistrylessGate::RunTest(const FString&)
{
    TestFalse(TEXT("Development Editor outside Automation cannot read obsolete approved kits"),
        FGridApprovedStarterLegacyFixtureGate::Allows(false));
    TestTrue(TEXT("Isolated Automation-only source parity fixture retained"),
        FGridApprovedStarterLegacyFixtureGate::Allows(true));
    return !HasAnyErrors();
}

#endif
