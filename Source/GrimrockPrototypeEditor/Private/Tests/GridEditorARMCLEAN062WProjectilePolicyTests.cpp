#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"

// Permanent read-only regression of the installed canonical ItemSystem.
// The obsolete transient production candidate builders were one-shot tools.

namespace GridCanonicalThrowTests
{
    constexpr const TCHAR* IDs[] =
    {
        TEXT("Weapon_ThrowingKnife_01"),
        TEXT("Weapon_Dart_01"),
        TEXT("Weapon_Javelin_01")
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062WRealCombatThrows,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionCombatThrowPhysics",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062WRealCombatThrows::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Registry;
    if (!TestTrue(TEXT("Real ItemSystem configured"), Registry.bReady)) return false;
    int32 Sources = 0;
    for (const TCHAR* Id : GridCanonicalThrowTests::IDs)
    {
        FGridResolvedItem Item;
        if (!TestTrue(TEXT("Physical thrown weapon exists at N1"),
            Registry.Resolve(FName(Id), 1, Item))) continue;
        ++Sources;
        TestTrue(TEXT("Specialized combat flight has finite authored dynamics"),
            Item.bIsArmament && Item.Weapon.Projectile.bEnabled &&
            FMath::IsFinite(Item.Weapon.Projectile.InitialSpeed) &&
            Item.Weapon.Projectile.InitialSpeed > 0.f &&
            FMath::IsFinite(Item.Weapon.Projectile.Arc));
        TestTrue(TEXT("Dedicated physical projectile mesh is resolved"),
            Item.ProjectileMesh.LoadSynchronous() != nullptr);
        TestTrue(TEXT("Exact visual flight profile exists"),
            Item.ProjectileAppearance.bEnabled);
    }
    TestEqual(TEXT("Three specialized physical throw sources"), Sources, 3);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062WRejectInvalidThrow,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.RejectInvalidThrowPhysics",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062WRejectInvalidThrow::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Registry;
    if (!TestTrue(TEXT("Canonical registry configured"), Registry.bReady)) return false;
    FGridResolvedItem Rejected;
    TestFalse(TEXT("N0 cannot synthesize thrown ARM N1"),
        Registry.Registry->ResolveItem(TEXT("Weapon_ThrowingKnife_01"), 0, Rejected));
    TestFalse(TEXT("N21 cannot synthesize thrown ARM"),
        Registry.Registry->ResolveItem(TEXT("Weapon_ThrowingKnife_01"), 21, Rejected));
    TestFalse(TEXT("Missing item cannot acquire a projectile"),
        Registry.Registry->ResolveItem(TEXT("Unknown_Throw_Weapon"), 1, Rejected));
    FGridResolvedItem Sword;
    if (TestTrue(TEXT("Ordinary sword resolves"),
        Registry.Resolve(TEXT("Weapon_StraightSword_01"), 1, Sword)))
    {
        TestFalse(TEXT("Sword does not borrow specialized throw policy"),
            Sword.Weapon.Projectile.bEnabled);
    }
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
