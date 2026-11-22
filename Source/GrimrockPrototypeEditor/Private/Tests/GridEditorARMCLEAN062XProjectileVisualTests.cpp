#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"

// Permanent read-only regression of the installed canonical ItemSystem.
// The obsolete transient production candidate builders were one-shot tools.

namespace GridCanonicalThrowVisuals
{
    constexpr const TCHAR* IDs[] =
    {
        TEXT("Weapon_ThrowingKnife_01"),
        TEXT("Weapon_Dart_01"),
        TEXT("Weapon_Javelin_01")
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062XRealProjectileVisuals,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionProjectileVisualParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062XRealProjectileVisuals::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Registry;
    if (!TestTrue(TEXT("Production ItemSystem configured"), Registry.bReady)) return false;
    int32 Complete = 0;
    for (const TCHAR* Id : GridCanonicalThrowVisuals::IDs)
    {
        FGridResolvedItem Item;
        if (!TestTrue(TEXT("Real canonical thrown ARM N1"),
            Registry.Resolve(FName(Id), 1, Item))) continue;
        const bool bValid = Item.Weapon.Projectile.bEnabled &&
            Item.ProjectileAppearance.bEnabled &&
            Item.ProjectileMesh.LoadSynchronous() != nullptr;
        TestTrue(TEXT("Physics and visual projectile profiles are paired"), bValid);
        Complete += bValid ? 1 : 0;
    }
    TestEqual(TEXT("Three complete native thrown weapon visuals"), Complete, 3);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062XProjectileVisualSafety,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProjectileVisualProjectionSafety",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062XProjectileVisualSafety::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Registry;
    if (!TestTrue(TEXT("Registry configured"), Registry.bReady)) return false;
    FGridResolvedItem Sword;
    if (TestTrue(TEXT("Real standard weapon resolves"),
        Registry.Resolve(TEXT("Weapon_StraightSword_01"), 1, Sword)))
    {
        // Ordinary hand tosses may have their own visual presentation,
        // but cannot acquire the specialized recoverable attack permission.
        TestFalse(TEXT("Ordinary ARM never borrows specialized projectile attack"),
            Sword.Weapon.Projectile.bEnabled);
    }
    FGridResolvedItem Invalid;
    TestFalse(TEXT("Invalid physical weapon level cannot project visuals"),
        Registry.Registry->ResolveItem(TEXT("Weapon_Dart_01"), 21, Invalid));
    TestFalse(TEXT("Unknown weapon cannot acquire visual projectile"),
        Registry.Registry->ResolveItem(TEXT("UnknownProjectileVisual"), 1, Invalid));
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
