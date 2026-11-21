#pragma once

/** Only isolated Automation fixtures may reproduce the old 18-kit data
 * without DA_ItemSystem. Editor/Debug packaging with Automation compiled in
 * is not itself an authorisation to load historical DA_Weapon_/DA_Item_.
 */
struct FGridApprovedStarterLegacyFixtureGate
{
    static bool Allows(bool bActuallyRunningAutomation)
    {
#if WITH_DEV_AUTOMATION_TESTS
        return bActuallyRunningAutomation;
#else
        (void)bActuallyRunningAutomation;
        return false;
#endif
    }
};
