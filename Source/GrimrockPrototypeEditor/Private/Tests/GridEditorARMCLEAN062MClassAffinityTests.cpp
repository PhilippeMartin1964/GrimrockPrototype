#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062MClassAffinityProjection,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.ClassAffinityProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062MClassAffinityProjection::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Production canonical registry"), Fixture.bReady)) return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Single native weapon catalogue"), Root) ||
        !TestNotNull(TEXT("Canonical class affinity source"), Root->WeaponCatalogue.Get()))
        return false;
    const bool bWasDirty = Root->GetOutermost()->IsDirty();
    TArray<FString> Errors;
    TestTrue(TEXT("Native class and archetype catalogue validates"),
        Root->WeaponCatalogue->Validate(Errors));
    TestEqual(TEXT("Six authentic class preference records"),
        Root->WeaponCatalogue->ClassPreferences.Num(), 6);
    TSet<FName> Classes, Archetypes;
    for (const FGridCanonicalClassPreference& Class : Root->WeaponCatalogue->ClassPreferences)
    {
        TestTrue(TEXT("Class ID is explicitly authored"), !Class.ClassId.IsNone());
        TestFalse(TEXT("No duplicate global class advice"), Classes.Contains(Class.ClassId));
        Classes.Add(Class.ClassId);
        TestTrue(TEXT("Class branches are not an inferred fallback"),
            !Class.CanonicalBranches.IsEmpty());
    }
    for (const FGridCanonicalWeaponArchetype& Arm : Root->WeaponCatalogue->Archetypes)
    {
        TestFalse(TEXT("No duplicate canonical numeric archetype"), Archetypes.Contains(Arm.ArchetypeId));
        Archetypes.Add(Arm.ArchetypeId);
    }
    TestEqual(TEXT("Six unique production class preferences"), Classes.Num(), 6);
    TestEqual(TEXT("Original 82 numeric profiles retain one archetype identity each"),
        Archetypes.Num(), 82);
    TestEqual(TEXT("Class affinity audit is read-only"),
        Root->GetOutermost()->IsDirty(), bWasDirty);
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
