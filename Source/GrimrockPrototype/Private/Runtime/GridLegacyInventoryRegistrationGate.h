#pragma once

/** The obsolete inventory definition map exists only for native Automation
 * characterization. Editor builds may compile these functions, but ordinary
 * Editor gameplay is not itself permission to register historical assets.
 * GI-backed Automation fixtures are preserved; gameplay readers separately
 * enforce canonical authority.
 */
struct FGridLegacyInventoryRegistrationGate
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
