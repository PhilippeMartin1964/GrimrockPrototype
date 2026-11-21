#pragma once

/** Materialization from a legacy UGridItemDefinitionAsset is only useful
 * for explicitly running Automation characterization tests. Compiling
 * WITH_DEV_AUTOMATION_TESTS into Editor does not by itself grant access.
 */
struct FGridLegacyWorldActorInitGate
{
    static bool Allows(bool bRunningAutomation)
    {
#if WITH_DEV_AUTOMATION_TESTS
        return bRunningAutomation;
#else
        (void)bRunningAutomation;
        return false;
#endif
    }
};
