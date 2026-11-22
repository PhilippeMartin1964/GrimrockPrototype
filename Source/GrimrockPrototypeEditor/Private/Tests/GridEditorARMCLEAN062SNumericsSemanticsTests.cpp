#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"

// Permanent read-only regression of the installed canonical ItemSystem.
// The obsolete transient production candidate builders were one-shot tools.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062SProductionNumbers,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.Numeric82WithProductionSemantics22",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062SProductionNumbers::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Registry;
    if (!TestTrue(TEXT("Production numeric registry validates"), Registry.bReady)) return false;
    TArray<FGridWeaponPipelineRow> Manifest;
    FString Error;
    if (!TestTrue(TEXT("Canonical ARM identity manifest available"),
        FGridWeaponPipelineManifest::Load(FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
        return false;
    int32 Active = 0, Profiles = 0, Passive = 0;
    for (const FGridWeaponPipelineRow& Row : Manifest)
    {
        if (Row.ProductionState != TEXT("Active")) continue;
        ++Active;
        for (int32 Level = 1; Level <= 20; ++Level)
        {
            FGridResolvedItem Item;
            if (!TestTrue(TEXT("Every authored physical N1-N20 level resolves"),
                Registry.Resolve(FName(Row.ItemDefinitionId), Level, Item))) continue;
            ++Profiles;
            TestEqual(TEXT("Archetype stable across physical levels"),
                Item.ArchetypeId, FName(Row.ArchetypeId));
            TestTrue(TEXT("Finite physical weight and numeric AP/range"),
                FMath::IsFinite(Item.WeightKg) && Item.WeightKg >= 0.f &&
                Item.Weapon.ActionPointCost >= 0 && Item.Weapon.RangeCells >= 0);
            if (Row.IsPassive())
            {
                ++Passive;
                TestFalse(TEXT("Passive numeric profiles have no attack dice"),
                    Item.Weapon.DamageDice.bEnabled);
            }
            else
                TestTrue(TEXT("Offensive numerics have damage dice"),
                    Item.Weapon.DamageDice.bEnabled);
        }
    }
    TestEqual(TEXT("22 ARM have proven concrete semantics"), Active, 22);
    TestEqual(TEXT("440 item-level numeric profiles from one catalogue"), Profiles, 440);
    TestTrue(TEXT("Passive N1-N20 profiles remain represented"), Passive >= 40);
    FGridResolvedItem Invalid;
    TestFalse(TEXT("N0 cannot borrow N1 weapon numbers"),
        Registry.Registry->ResolveItem(TEXT("Weapon_StraightSword_01"), 0, Invalid));
    TestFalse(TEXT("N21 cannot borrow N20 numbers"),
        Registry.Registry->ResolveItem(TEXT("Weapon_StraightSword_01"), 21, Invalid));
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
