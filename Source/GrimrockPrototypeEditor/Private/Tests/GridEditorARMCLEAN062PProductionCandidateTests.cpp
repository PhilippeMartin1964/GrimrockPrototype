#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"

// Permanent read-only regression of the installed canonical ItemSystem.
// The obsolete transient production candidate builders were one-shot tools.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062PProductionCandidate,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionCandidate22",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062PProductionCandidate::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Registry;
    if (!TestTrue(TEXT("Production ItemSystem configured"), Registry.bReady)) return false;
    TArray<FGridWeaponPipelineRow> Manifest;
    FString Error;
    if (!TestTrue(TEXT("ARM identity manifest parses"),
        FGridWeaponPipelineManifest::Load(FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
        return false;
    TSet<FName> Ids;
    int32 Arms = 0;
    for (const FGridWeaponPipelineRow& Row : Manifest)
    {
        if (Row.ProductionState != TEXT("Active")) continue;
        ++Arms;
        FGridResolvedItem Item;
        if (!TestTrue(TEXT("Canonical N1 armament and visual resolve"),
            Registry.Resolve(FName(Row.ItemDefinitionId), 1, Item))) continue;
        TestFalse(TEXT("No duplicate ARM item identity"), Ids.Contains(Item.ItemId));
        Ids.Add(Item.ItemId);
        TestEqual(TEXT("Source manifest still identifies same archetype"),
            Item.ArchetypeId, FName(Row.ArchetypeId));
        TestEqual(TEXT("One visual identity per ARM"), Item.VisualId, FName(Row.VisualId));
        TestTrue(TEXT("Actual item icon, world and equipped mesh load"),
            Item.ItemIcon.LoadSynchronous() && Item.WorldMesh.LoadSynchronous() &&
            Item.EquippedMesh.LoadSynchronous());
        TestEqual(TEXT("Weapon is not stackable"), Item.MaxStack, 1);
        TestTrue(TEXT("ARM N1 physical weight is finite"), FMath::IsFinite(Item.WeightKg) && Item.WeightKg >= 0.f);
    }
    TestEqual(TEXT("Exactly 22 ARM N1 canonical definitions"), Arms, 22);
    TestEqual(TEXT("22 distinct production IDs"), Ids.Num(), Arms);
    TArray<FName> All;
    Registry.Registry->GetCanonicalItemIds(All);
    TestEqual(TEXT("No obsolete 23-item candidate: 48 real identities"), All.Num(), 48);
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
