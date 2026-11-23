#pragma once

/** The obsolete inventory definition map exists only for native Automation
 * characterization. Editor builds may compile these functions, but ordinary
 * Editor gameplay is not itself permission to register historical assets.
 * A world with a GameInstance always uses its ItemSystem.
 */
struct FGridLegacyInventoryRegistrationGate
{
    static bool Allows(bool bRunningAutomation, bool bHasGameInstance)
    {
#if WITH_DEV_AUTOMATION_TESTS
        return bRunningAutomation && !bHasGameInstance;
#else
        (void)bRunningAutomation;
        (void)bHasGameInstance;
        return false;
#endif
    }
};
