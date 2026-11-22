#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062JVisualProjectionTest,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionVisualProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062JVisualProjectionTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Installed visual registry configured"), Fixture.bReady)) return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Production root visual catalogue"), Root) ||
        !TestNotNull(TEXT("Only canonical visual catalogue"), Root->WeaponVisualCatalogue.Get()))
        return false;
    const bool bRootDirty = Root->GetOutermost()->IsDirty();
    TArray<FGridWeaponPipelineRow> Rows;
    FString Error;
    if (!TestTrue(TEXT("Historical identity manifest parses"),
        FGridWeaponPipelineManifest::Load(FGridWeaponPipelineManifest::DefaultFile(), Rows, Error)))
        return false;
    TSet<FName> UsedArchetypes, UsedVisuals;
    int32 Bindings = 0;
    for (const FGridWeaponPipelineRow& Row : Rows)
    {
        if (Row.ProductionState != TEXT("Active")) continue;
        const FName Archetype(*Row.ArchetypeId), VisualId(*Row.VisualId);
        TestFalse(TEXT("Each real ARM has a single visual binding"),
            UsedArchetypes.Contains(Archetype));
        UsedArchetypes.Add(Archetype);
        FGridResolvedItem Item;
        if (!TestTrue(TEXT("Real canonical visual N1 resolves"),
            Fixture.Resolve(FName(Row.ItemDefinitionId), 1, Item))) continue;
        const FGridCanonicalWeaponVisual* Visual =
            Root->WeaponVisualCatalogue->FindForArchetype(Archetype);
        if (!TestNotNull(TEXT("Archetype visual binding is in unique catalogue"), Visual))
            continue;
        ++Bindings;
        UsedVisuals.Add(VisualId);
        TestEqual(TEXT("Stable visual ID"), Item.VisualId, VisualId);
        TestEqual(TEXT("Catalogue binding matches resolved view"), Visual->VisualId, Item.VisualId);
        TestTrue(TEXT("Icon and world mesh are authored and loadable"),
            Item.ItemIcon.LoadSynchronous() && Item.WorldMesh.LoadSynchronous());
        TestTrue(TEXT("Equipped mesh is authored and loadable"),
            Item.EquippedMesh.LoadSynchronous() != nullptr);
        TestTrue(TEXT("No invalid world mesh transform"),
            !Item.WorldVisualTransform.ContainsNaN());
        TestTrue(TEXT("Mesh reference matches single catalogue"),
            Item.WorldMesh.ToSoftObjectPath() == Visual->WorldMesh.ToSoftObjectPath());
    }
    TArray<FString> Errors;
    TestTrue(TEXT("Canonical visual catalogue passes native validation"),
        Root->WeaponVisualCatalogue->Validate(Errors));
    TestEqual(TEXT("All 22 ARM have a real visual join"), Bindings, 22);
    TestEqual(TEXT("All archetypes are unique"), UsedArchetypes.Num(), 22);
    TestTrue(TEXT("One or more distinct shared visual profiles"), !UsedVisuals.IsEmpty());
    TestEqual(TEXT("Read-only visual audit does not dirty package"),
        Root->GetOutermost()->IsDirty(), bRootDirty);
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
