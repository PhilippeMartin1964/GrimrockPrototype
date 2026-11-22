#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMCLEAN03LosslessCatalogueTest,
    "Grimrock.RPG.ARMEMENT.CLEAN03.LosslessCompactCatalogue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridARMCLEAN03LosslessCatalogueTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Canonical numeric source configured"), Fixture.bReady))
        return false;
    UGridItemSystemAsset* Root = GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Unique authored numeric catalogue"), Root) ||
        !TestNotNull(TEXT("Weapon numeric authority"), Root->WeaponCatalogue.Get()))
        return false;
    TArray<FString> Errors;
    TestTrue(TEXT("Native 82-archetype growth catalogue validates"),
        Root->WeaponCatalogue->Validate(Errors));
    TestEqual(TEXT("All original 82 compact growth profiles are retained"),
        Root->WeaponCatalogue->Archetypes.Num(), 82);
    int32 Profiles = 0;
    TSet<FName> Ids;
    for (const FGridCanonicalWeaponArchetype& Arm : Root->WeaponCatalogue->Archetypes)
    {
        TestFalse(TEXT("No duplicate numeric archetype"), Ids.Contains(Arm.ArchetypeId));
        Ids.Add(Arm.ArchetypeId);
        for (int32 Level = 1; Level <= 20; ++Level)
        {
            FGridCanonicalResolvedWeapon N;
            if (!TestTrue(TEXT("One physical N1-N20 numeric profile"),
                Arm.TryResolve(Level, N))) continue;
            ++Profiles;
            TestTrue(TEXT("Finite and valid action numbers"),
                N.ActionPointCost >= 0 && N.RangeCells >= 0);
            TestTrue(TEXT("No negative armor from growth rules"),
                N.PhysicalArmor >= 0 && N.MagicalArmor >= 0);
        }
        FGridCanonicalResolvedWeapon Bad;
        TestFalse(TEXT("No level-zero growth profile"), Arm.TryResolve(0, Bad));
        TestFalse(TEXT("No level-twenty-one growth profile"), Arm.TryResolve(21, Bad));
    }
    TestEqual(TEXT("All 1640 numeric profiles resolve from one authority"), Profiles, 1640);
    TestEqual(TEXT("All 82 numeric identities are unique"), Ids.Num(), 82);
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
