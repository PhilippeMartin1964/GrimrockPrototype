#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "Runtime/Combat/GridRangedAmmunitionPolicy.h"

// The old AddExistingArrows candidate was a one-time migration tool.
// Validate the installed single ItemSystem and actual bow links instead.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062VRealAmmo,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionAmmoIdentityAndBowLinks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062VRealAmmo::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Registry;
    if (!TestTrue(TEXT("Real production ItemSystem validates"), Registry.bReady))
        return false;
    TArray<FName> Ids;
    Registry.Registry->GetCanonicalItemIds(Ids);
    TestEqual(TEXT("One 48-item canonical item registry"), Ids.Num(), 48);

    TArray<FGridWeaponPipelineRow> Manifest;
    FString Error;
    if (!TestTrue(TEXT("Production manifest parses"),
        FGridWeaponPipelineManifest::Load(
            FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
    {
        AddError(Error);
        return false;
    }
    FGridResolvedItem Arrows;
    if (!TestTrue(TEXT("The single real arrow identity resolves as an ammo kind"),
        Registry.Registry->ResolveAmmoKind(TEXT("Item_Arrows_01"), Arrows)))
        return false;
    TestEqual(TEXT("Canonical ammo ItemId"), Arrows.ItemId, FName(TEXT("Item_Arrows_01")));
    TestEqual(TEXT("Arrow kind is its exact physical source"), Arrows.AmmoKindId, Arrows.ItemId);
    TestTrue(TEXT("Real arrows are physical N0 and stackable"),
        Arrows.ItemLevel == 0 && Arrows.MaxStack >= 12 && !Arrows.bIsArmament);

    int32 Arms = 0, ReferencingBows = 0;
    for (const FGridWeaponPipelineRow& Row : Manifest)
    {
        if (Row.ProductionState != TEXT("Active")) continue;
        ++Arms;
        FGridResolvedItem Weapon;
        if (!TestTrue(*FString::Printf(TEXT("ARM %s resolves at N1"), *Row.ArchetypeId),
            Registry.Resolve(FName(Row.ItemDefinitionId), 1, Weapon))) continue;
        TestTrue(TEXT("Canonical weapon owns exact archetype"),
            Weapon.bIsArmament && Weapon.ArchetypeId == FName(Row.ArchetypeId));
        const bool bRequiresAmmo = !Weapon.Weapon.RequiredAmmoKindId.IsNone();
        if (!bRequiresAmmo) continue;
        ++ReferencingBows;
        TestEqual(TEXT("Bow ammo join is the unique canonical arrow kind"),
            Weapon.Weapon.RequiredAmmoKindId, Arrows.AmmoKindId);
        FGridRangedAmmunitionRequirement Requirement =
            FGridRangedAmmunitionPolicy::ResolveForAttack(
                Registry.Registry.Get(), Weapon.ItemId, 1, nullptr, true);
        TestTrue(TEXT("Actual bow preflight rejects historical lookup"),
            Requirement.bValid && Requirement.bCanonicalSource &&
            Requirement.bCanonicalAmmunition &&
            Requirement.AmmunitionItemId == Arrows.ItemId);
        TestTrue(TEXT("Only ranged rows require an arrow kind"), Row.BuildKind == TEXT("Ranged"));
    }
    TestEqual(TEXT("Full production manifest contains 22 ARM"), Arms, 22);
    TestEqual(TEXT("Both original bows require real arrows"), ReferencingBows, 2);
    FGridResolvedItem Wrong;
    TestFalse(TEXT("Non-ARM arrows cannot resolve at physical N1"),
        Registry.Registry->ResolveItem(Arrows.ItemId, 1, Wrong));
    TestFalse(TEXT("Unknown ammo kinds do not invent legacy assets"),
        Registry.Registry->ResolveAmmoKind(TEXT("Unknown_Ammo_Kind"), Wrong));
    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
