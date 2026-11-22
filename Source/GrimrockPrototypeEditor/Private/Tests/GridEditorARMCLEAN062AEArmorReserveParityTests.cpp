#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062AEProductionArmorReserves,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionInstanceArmorReserveParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062AEProductionArmorReserves::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Canonical numeric source configured"), Fixture.bReady)) return false;
    TArray<FGridWeaponPipelineRow> Manifest;
    FString Error;
    if (!TestTrue(TEXT("Official 22 ARM identity manifest loads"),
        FGridWeaponPipelineManifest::Load(
            FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
        return false;
    if (!TestEqual(TEXT("Exactly 22 physical ARM entries"), Manifest.Num(), 22))
        return false;
    int32 Profiles = 0, Passive = 0, Depleted = 0;
    for (const FGridWeaponPipelineRow& Row : Manifest)
    {
        if (Row.IsPassive()) ++Passive;
        for (int32 Level : {1, 20})
        {
            FGridResolvedItem Item;
            if (!TestTrue(TEXT("Real N1/N20 armor comes from unique ItemSystem"),
                Fixture.Resolve(FName(Row.ItemDefinitionId), Level, Item))) continue;
            TestEqual(TEXT("Exact canonical armor archetype"),
                Item.ArchetypeId, FName(Row.ArchetypeId));
            const int32 Physical = FMath::Min(3, Item.Weapon.PhysicalArmor);
            const int32 Magical = FMath::Min(2, Item.Weapon.MagicalArmor);
            FGridCanonicalArmorReserveView Reserve;
            if (!TestTrue(TEXT("Spent armor never duplicates nominal armor"),
                FGridCanonicalArmorReserveResolver::TryCompute(
                    Item.Weapon, Physical, Magical, Reserve)))
                continue;
            TestEqual(TEXT("Real nominal physical armor preserved"),
                Reserve.NominalPhysicalArmor, Item.Weapon.PhysicalArmor);
            TestEqual(TEXT("Real nominal magical armor preserved"),
                Reserve.NominalMagicalArmor, Item.Weapon.MagicalArmor);
            TestEqual(TEXT("Physical debt remains exact"),
                Reserve.SpentPhysicalArmor, Physical);
            TestEqual(TEXT("Magical debt remains exact"),
                Reserve.SpentMagicalArmor, Magical);
            TestEqual(TEXT("No physical armor refill"),
                Reserve.RemainingPhysicalArmor,
                FMath::Max(0, Item.Weapon.PhysicalArmor - Physical));
            TestEqual(TEXT("No magical armor refill"),
                Reserve.RemainingMagicalArmor,
                FMath::Max(0, Item.Weapon.MagicalArmor - Magical));
            if (Physical > 0 || Magical > 0) ++Depleted;
            ++Profiles;
        }
    }
    TestEqual(TEXT("44 actual numeric N1 and N20 armor levels"), Profiles, 44);
    TestEqual(TEXT("Three passive ARM classes included"), Passive, 3);
    TestTrue(TEXT("Actual nonzero depletion cases examined"), Depleted > 0);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062AENeverRefillShield,
	"Grimrock.RPG.ARM.CLEAN06_2.Canonical.NoFreeArmorRefillDuringCanonicalResolution",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062AENeverRefillShield::RunTest(const FString&)
{
	FGridCanonicalResolvedWeapon Shield;
	Shield.Kind = EGridCanonicalArmamentKind::Shield;
	Shield.PhysicalArmor = 5;
	Shield.MagicalArmor = 2;
	FGridCanonicalArmorReserveView View;
	if (!TestTrue(TEXT("A 5-point physical shield can be three points spent"),
		FGridCanonicalArmorReserveResolver::TryCompute(Shield, 3, 1, View)))
	{
		return false;
	}
	TestEqual(TEXT("Only two physical armor points remain"), View.RemainingPhysicalArmor, 2);
	TestEqual(TEXT("One magical armor point remains"), View.RemainingMagicalArmor, 1);
	TestEqual(TEXT("Three physical points remain consumed"), View.SpentPhysicalArmor, 3);

	TestTrue(TEXT("Exhausted physical reserve remains exhausted"),
		FGridCanonicalArmorReserveResolver::TryCompute(Shield, 5, 2, View));
	TestEqual(TEXT("No free physical refill"), View.RemainingPhysicalArmor, 0);
	TestEqual(TEXT("No free magical refill"), View.RemainingMagicalArmor, 0);

	TestTrue(TEXT("Outdated higher spent amount is retained and remaining is clamped"),
		FGridCanonicalArmorReserveResolver::TryCompute(Shield, 9, 3, View));
	TestEqual(TEXT("Spent amount is never silently clamped"),
		View.SpentPhysicalArmor, 9);
	TestEqual(TEXT("Overdepletion still leaves no shield armor"),
		View.RemainingPhysicalArmor, 0);
	TestEqual(TEXT("Excess spent magic also retained"), View.SpentMagicalArmor, 3);
	TestEqual(TEXT("No negative remaining magical armor"), View.RemainingMagicalArmor, 0);

	TestFalse(TEXT("Negative persisted physical debt is invalid"),
		FGridCanonicalArmorReserveResolver::TryCompute(Shield, -1, 0, View));
	TestEqual(TEXT("Failure clears the previous successful physical result"),
		View.SpentPhysicalArmor, 0);
	TestEqual(TEXT("Failure clears previous magical result"),
		View.SpentMagicalArmor, 0);
	Shield.PhysicalArmor = -1;
	TestFalse(TEXT("Negative nominal armor is invalid"),
		FGridCanonicalArmorReserveResolver::TryCompute(Shield, 0, 0, View));
	return true;
}

#endif
