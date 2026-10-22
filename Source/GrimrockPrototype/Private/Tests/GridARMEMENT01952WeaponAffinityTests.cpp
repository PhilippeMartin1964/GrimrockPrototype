#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "UObject/UObjectGlobals.h"

namespace ARMEMENT01952
{
    UGridWeaponCatalogueAsset* LoadProductionCatalogue()
    {
        return LoadObject<UGridWeaponCatalogueAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
    }

    void Verify(FAutomationTestBase& Test, UGridWeaponCatalogueAsset& Catalogue,
        const TCHAR* Archetype, const TCHAR* Class, EGridWeaponClassAffinity Expected)
    {
        Test.TestEqual(*FString::Printf(TEXT("%s / %s"), Archetype, Class),
            Catalogue.GetClassAffinity(FName(Archetype), FName(Class)), Expected);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT01952ProjectionCoverage,
    "Grimrock.RPG.ARMEMENT01.9.WeaponAffinity.ProductionProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT01952ProjectionCoverage::RunTest(const FString&)
{
    UGridWeaponCatalogueAsset* Catalogue = ARMEMENT01952::LoadProductionCatalogue();
    if (!TestNotNull(TEXT("Original production catalogue exists"), Catalogue)) return false;
    TestTrue(TEXT("1640 original weapon-level profiles stay valid"), Catalogue->IsValidCatalogue());
    TestTrue(TEXT("82 weapon affinities and six class preferences materialized from XLSX"),
        Catalogue->IsValidClassAffinityData());
    TestEqual(TEXT("82 archetypes (not 1640 duplicated metadata records)"),
        Catalogue->ClassAffinities.Num(), 82);
    TestEqual(TEXT("Six production class preferences"), Catalogue->ClassPreferences.Num(), 6);
    for (int32 Ordinal = 1; Ordinal <= 82; ++Ordinal)
    {
        const FName Id(*FString::Printf(TEXT("ARM%03d"), Ordinal));
        TestNotNull(TEXT("Every archetype still has level-one profile"), Catalogue->FindProfile(Id, 1));
        TestNotNull(TEXT("Every archetype still has level-twenty profile"), Catalogue->FindProfile(Id, 20));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT01952Examples,
    "Grimrock.RPG.ARMEMENT01.9.WeaponAffinity.CanonicalClassExamples",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT01952Examples::RunTest(const FString&)
{
    using A = EGridWeaponClassAffinity;
    UGridWeaponCatalogueAsset* Catalogue = ARMEMENT01952::LoadProductionCatalogue();
    if (!TestNotNull(TEXT("Catalogue"), Catalogue)) return false;
    if (!TestTrue(TEXT("Imported class affinities valid"), Catalogue->IsValidClassAffinityData())) return false;

    ARMEMENT01952::Verify(*this, *Catalogue, TEXT("ARM001"), TEXT("Rogue"), A::Primary);
    ARMEMENT01952::Verify(*this, *Catalogue, TEXT("ARM001"), TEXT("Ranger"), A::Secondary);
    ARMEMENT01952::Verify(*this, *Catalogue, TEXT("ARM001"), TEXT("Alchemist"), A::Secondary);
    ARMEMENT01952::Verify(*this, *Catalogue, TEXT("ARM001"), TEXT("Warrior"), A::Unspecified);
    ARMEMENT01952::Verify(*this, *Catalogue, TEXT("ARM045"), TEXT("Ranger"), A::Primary);
    ARMEMENT01952::Verify(*this, *Catalogue, TEXT("ARM054"), TEXT("Ranger"), A::Primary);
    ARMEMENT01952::Verify(*this, *Catalogue, TEXT("ARM054"), TEXT("Rogue"), A::Secondary);
    ARMEMENT01952::Verify(*this, *Catalogue, TEXT("ARM054"), TEXT("Warrior"), A::Unspecified);
    ARMEMENT01952::Verify(*this, *Catalogue, TEXT("ARM075"), TEXT("Warrior"), A::Primary);
    ARMEMENT01952::Verify(*this, *Catalogue, TEXT("ARM075"), TEXT("Priest"), A::Secondary);
    ARMEMENT01952::Verify(*this, *Catalogue, TEXT("ARM061"), TEXT("Mage"), A::Primary);
    ARMEMENT01952::Verify(*this, *Catalogue, TEXT("ARM061"), TEXT("Priest"), A::Secondary);
    ARMEMENT01952::Verify(*this, *Catalogue, TEXT("ARM069"), TEXT("Priest"), A::Primary);
    ARMEMENT01952::Verify(*this, *Catalogue, TEXT("ARM069"), TEXT("Mage"), A::Secondary);
    TestEqual(TEXT("Unknown archetype does not disallow equipment"),
        Catalogue->GetClassAffinity(TEXT("ARM999"), TEXT("Warrior")), A::Unspecified);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMEMENT01952Preferences,
    "Grimrock.RPG.ARMEMENT01.9.WeaponAffinity.ClassesTabEvidence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMEMENT01952Preferences::RunTest(const FString&)
{
    UGridWeaponCatalogueAsset* Catalogue = ARMEMENT01952::LoadProductionCatalogue();
    if (!TestNotNull(TEXT("Catalogue"), Catalogue)) return false;
    if (!TestTrue(TEXT("Imported preferences valid"), Catalogue->IsValidClassAffinityData())) return false;
    const FGridWeaponClassPreference* Warrior = Catalogue->FindClassPreference(TEXT("Warrior"));
    const FGridWeaponClassPreference* Mage = Catalogue->FindClassPreference(TEXT("Mage"));
    const FGridWeaponClassPreference* Priest = Catalogue->FindClassPreference(TEXT("Priest"));
    if (!TestNotNull(TEXT("Warrior preference"), Warrior) ||
        !TestNotNull(TEXT("Mage preference"), Mage) ||
        !TestNotNull(TEXT("Priest preference"), Priest)) return false;

    TestTrue(TEXT("Warrior class priority from spreadsheet includes shield"),
        Warrior->PriorityWeapons.Contains(TEXT("écu")));
    TestTrue(TEXT("Mage class priority includes elemental wand"),
        Mage->PriorityWeapons.Contains(TEXT("Baguettes")));
    TestTrue(TEXT("Priest class priority includes sacred sceptre"),
        Priest->PriorityWeapons.Contains(TEXT("sceptre sacré")));
    TestFalse(TEXT("Unknown class has no preference"),
        Catalogue->FindClassPreference(TEXT("UnknownClass")) != nullptr);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
