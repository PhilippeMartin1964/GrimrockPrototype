#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062ClassParityTest,
    "Grimrock.RPG.ARM.CLEAN06_2.Semantics.ClassAffinityParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062ClassParityTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Production ItemSystem configured"), Fixture.bReady)) return false;
    UGridItemSystemAsset* Root = GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Real production root"), Root) ||
        !TestNotNull(TEXT("Real canonical weapon catalogue"), Root->WeaponCatalogue.Get())) return false;
    const UGridCanonicalWeaponCatalogueAsset* Catalogue = Root->WeaponCatalogue.Get();
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Full canonical class and numeric source valid"),
        Catalogue->Validate(Errors))) return false;
    TestEqual(TEXT("82 authoring archetype affinities retained"), Catalogue->Archetypes.Num(), 82);
    TestEqual(TEXT("Six distinct class preferences retained"), Catalogue->ClassPreferences.Num(), 6);
    TSet<FName> SeenClasses, SeenArms;
    for (const FGridClassPreference& Class : Catalogue->ClassPreferences)
    {
        TestFalse(TEXT("No duplicate class advice"), SeenClasses.Contains(Class.ClassId));
        SeenClasses.Add(Class.ClassId);
        TestTrue(TEXT("No unqualified empty class identity"), !Class.ClassId.IsNone());
    }
    for (const FGridWeaponArchetype& Arm : Catalogue->Archetypes)
    {
        TestFalse(TEXT("No duplicate ARM authoring identity"), SeenArms.Contains(Arm.ArchetypeId));
        SeenArms.Add(Arm.ArchetypeId);
        TestTrue(TEXT("Every known ARM affinity retains primary class"),
            !Arm.PrimaryClassId.IsNone());
    }
    TestEqual(TEXT("Six different canonical class advices"), SeenClasses.Num(), 6);
    TestEqual(TEXT("82 different canonical numeric/affinity rows"), SeenArms.Num(), 82);

    UGridCanonicalWeaponCatalogueAsset* Bad =
        DuplicateObject<UGridCanonicalWeaponCatalogueAsset>(Catalogue, GetTransientPackage());
    if (!TestNotNull(TEXT("Transient canonical catalogue clone"), Bad)) return false;
    const FName FirstClassId = Bad->ClassPreferences[0].ClassId;
    Bad->ClassPreferences[1].ClassId = FirstClassId;
    Errors.Reset();
    TestFalse(TEXT("Duplicated class ID must be rejected by actual canonical validation"),
        Bad->Validate(Errors));
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
