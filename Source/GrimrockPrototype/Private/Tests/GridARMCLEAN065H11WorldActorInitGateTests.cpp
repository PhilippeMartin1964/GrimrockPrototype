#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../Runtime/GridLegacyWorldActorInitGate.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean065H11LegacyActorGate,
    "Grimrock.RPG.ARM.CLEAN06_5H.OldWorldActorInitializationGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean065H11LegacyActorGate::RunTest(const FString&)
{
    TestFalse(TEXT("No obsolete world actor initialization in ordinary Editor gameplay"),
        FGridLegacyWorldActorInitGate::Allows(false));
    TestTrue(TEXT("Historical VFX/receptacle characterization remains available in Automation"),
        FGridLegacyWorldActorInitGate::Allows(true));
    // Existing GI-backed TorchRuntimeAndHeldVisual, LegacyIdentity and
    // CanonicalReceptacleInsertRestore exercise the materialization itself.
    return !HasAnyErrors();
}

#endif
