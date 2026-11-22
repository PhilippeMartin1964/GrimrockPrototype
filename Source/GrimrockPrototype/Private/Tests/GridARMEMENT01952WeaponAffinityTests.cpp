#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"

namespace ARMEMENT01952
{
    const FGridCanonicalWeaponArchetype* Find(FName Id)
    {
        UGridItemSystemAsset* Root = GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
        return Root && Root->WeaponCatalogue ? Root->WeaponCatalogue->Find(Id) : nullptr;
    }
    const FGridCanonicalClassPreference* Preference(FName Id)
    {
        UGridItemSystemAsset* Root = GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
        return Root && Root->WeaponCatalogue
            ? Root->WeaponCatalogue->ClassPreferences.FindByPredicate(
                [Id](const FGridCanonicalClassPreference& Row){return Row.ClassId == Id;})
            : nullptr;
    }
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT01952ProjectionCoverage,
    "Grimrock.RPG.ARMEMENT01.9.WeaponAffinity.ProductionProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridARMEMENT01952ProjectionCoverage::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Single production ItemSystem"), Fixture.bReady)) return false;
    UGridItemSystemAsset* Root = GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Canonical numeric root"), Root) ||
        !TestNotNull(TEXT("Canonical numeric catalogue"), Root->WeaponCatalogue.Get())) return false;
    TestEqual(TEXT("82 named archetype class affinities"), Root->WeaponCatalogue->Archetypes.Num(), 82);
    TestEqual(TEXT("Six original class preferences"), Root->WeaponCatalogue->ClassPreferences.Num(), 6);
    int32 NumericProfiles = 0;
    for (int32 Index = 1; Index <= 82; ++Index)
    {
        const FName Id(*FString::Printf(TEXT("ARM%03d"), Index));
        const FGridCanonicalWeaponArchetype* Row = Root->WeaponCatalogue->Find(Id);
        if (!TestNotNull(TEXT("Canonical numeric archetype is authored"), Row)) continue;
        FGridCanonicalResolvedWeapon N1, N20;
        if (TestTrue(TEXT("Both physical edge profiles from canonical authoring"),
            Row->TryResolve(1, N1) && Row->TryResolve(20, N20))) NumericProfiles += 20;
    }
    TestEqual(TEXT("1640 complete source profiles"), NumericProfiles, 1640);
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT01952Examples,
    "Grimrock.RPG.ARMEMENT01.9.WeaponAffinity.CanonicalClassExamples",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridARMEMENT01952Examples::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Canonical class catalogue configured"), Fixture.bReady)) return false;
    struct FExpected {const TCHAR* Arm; const TCHAR* Primary; const TCHAR* Secondary;};
    const FExpected Examples[] = {
        {TEXT("ARM001"),TEXT("Rogue"),TEXT("Ranger")},
        {TEXT("ARM045"),TEXT("Ranger"),nullptr},
        {TEXT("ARM054"),TEXT("Ranger"),TEXT("Rogue")},
        {TEXT("ARM075"),TEXT("Warrior"),TEXT("Priest")},
        {TEXT("ARM061"),TEXT("Mage"),TEXT("Priest")},
        {TEXT("ARM069"),TEXT("Priest"),TEXT("Mage")}
    };
    for (const FExpected& Expected : Examples)
    {
        const FGridCanonicalWeaponArchetype* Arm = ARMEMENT01952::Find(FName(Expected.Arm));
        if (!TestNotNull(TEXT("Canonical weapon affinity row"), Arm)) continue;
        TestEqual(TEXT("Exact primary class"), Arm->PrimaryClassId, FName(Expected.Primary));
        if (Expected.Secondary)
            TestTrue(TEXT("Exact secondary class retained"),
                Arm->SecondaryClassIds.Contains(FName(Expected.Secondary)));
    }
    TestNull(TEXT("No invented ARM999 affinity"), ARMEMENT01952::Find(TEXT("ARM999")));
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT01952Preferences,
    "Grimrock.RPG.ARMEMENT01.9.WeaponAffinity.ClassesTabEvidence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridARMEMENT01952Preferences::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Canonical ItemSystem"), Fixture.bReady)) return false;
    const FGridCanonicalClassPreference* Warrior = ARMEMENT01952::Preference(TEXT("Warrior"));
    const FGridCanonicalClassPreference* Mage = ARMEMENT01952::Preference(TEXT("Mage"));
    const FGridCanonicalClassPreference* Priest = ARMEMENT01952::Preference(TEXT("Priest"));
    if (!TestNotNull(TEXT("Warrior advice exists"), Warrior) ||
        !TestNotNull(TEXT("Mage advice exists"), Mage) ||
        !TestNotNull(TEXT("Priest advice exists"), Priest)) return false;
    TestTrue(TEXT("Warrior retains its shield priority"), Warrior->PriorityWeapons.Contains(TEXT("écu")));
    TestTrue(TEXT("Mage retains elemental wand priority"), Mage->PriorityWeapons.Contains(TEXT("Baguettes")));
    TestTrue(TEXT("Priest retains sacred sceptre priority"), Priest->PriorityWeapons.Contains(TEXT("sceptre sacré")));
    TestNull(TEXT("Unknown class has no manufactured advice"),
        ARMEMENT01952::Preference(TEXT("UnknownClass")));
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
