#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "Tests/GridCanonicalLootRegistryTestUtils.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"

// The original migration preview compared obsolete 82-archetype workbooks
// and 23 partial assets. After cutover the only meaningful production proof
// is the actual, validated four-catalogue 48-item DA_ItemSystem binary.
// Keep the original Automation test identity for regression continuity.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062YCompleteProductionPreview,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.CompleteProductionPreview23",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062YCompleteProductionPreview::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Production ItemSystem can be configured"), Fixture.bReady))
        return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Real canonical root on disk"), Root))
        return false;
    const bool bDirty = Root->GetOutermost()->IsDirty();
    TArray<FString> Errors;
    TestTrue(TEXT("All four installed catalogues validate"),
        Root->Validate(Errors));
    TestTrue(TEXT("Strict production shape/order of 48 canonical definitions"),
        GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*Root));
    TestNotNull(TEXT("The only weapon number source exists"), Root->WeaponCatalogue.Get());
    TestNotNull(TEXT("The only weapon visual source exists"), Root->WeaponVisualCatalogue.Get());
    TestNotNull(TEXT("The only compact/detailed action source exists"), Root->ActionCatalogue.Get());
    TestNotNull(TEXT("The only action presentation source exists"),
        Root->ActionPresentationCatalogue.Get());

    TArray<FGridWeaponPipelineRow> Manifest;
    FString Error;
    if (!TestTrue(TEXT("Production ARM identity manifest parses"),
        FGridWeaponPipelineManifest::Load(
            FGridWeaponPipelineManifest::DefaultFile(), Manifest, Error)))
    {
        AddError(Error);
        return false;
    }
    TArray<FName> ItemIds;
    Fixture.Registry->GetItemIds(ItemIds);
    TestEqual(TEXT("Canonical index holds exactly 48 identities"), ItemIds.Num(), 48);
    int32 Arms = 0, CompleteProfiles = 0, SpecializedFlights = 0, VisualFlights = 0;
    for (const FGridWeaponPipelineRow& Row : Manifest)
    {
        if (Row.ProductionState != TEXT("Active")) continue;
        ++Arms;
        for (int32 Level = 1; Level <= 20; ++Level)
        {
            FGridResolvedItem Item;
            if (!TestTrue(TEXT("Every ARM N1 to N20 resolves with exact physical level"),
                Fixture.Resolve(FName(Row.ItemDefinitionId), Level, Item)))
                continue;
            ++CompleteProfiles;
            TestTrue(TEXT("No obsolete source supplies the numeric profile"),
                Item.bIsArmament && Item.ArchetypeId == FName(Row.ArchetypeId));
            TestTrue(TEXT("One valid visual identity for all ARM levels"),
                Item.VisualId == FName(Row.VisualId) && !Item.WorldMesh.IsNull());
            if (Level == 1)
            {
                SpecializedFlights += Item.Weapon.Projectile.bEnabled ? 1 : 0;
                VisualFlights += Item.ProjectileAppearance.bEnabled ? 1 : 0;
            }
        }
    }
    TestEqual(TEXT("22 active ARM in the canonical registry"), Arms, 22);
    TestEqual(TEXT("All 440 physical ARM levels present"), CompleteProfiles, 440);
    TestEqual(TEXT("Three authorized specialized flights"), SpecializedFlights, 3);
    TestEqual(TEXT("Three authored projectile visual presentations"), VisualFlights, 3);

    int32 NonArms = 0;
    for (FName Id : ItemIds)
    {
        FGridResolvedItem N0;
        if (Fixture.Resolve(Id, 0, N0))
        {
            ++NonArms;
            TestFalse(TEXT("N0 cannot masquerade as an ARM"), N0.bIsArmament);
            TestTrue(TEXT("NonARM ID keeps one physical level"), N0.ItemLevel == 0);
        }
    }
    TestEqual(TEXT("All 26 non-ARM sources are present"), NonArms, 26);
    TestEqual(TEXT("Exactly 48 persisted source definitions"),
        Root->ItemDefinitions.Num(), 48);
    TestEqual(TEXT("Nineteen compact ARM action definitions"),
        Root->ActionCatalogue->Actions.Num(), 19);
    TestEqual(TEXT("Fourteen explicit detailed N0 actions"),
        Root->ActionCatalogue->DetailedActions.Num(), 14);
    TestEqual(TEXT("No native audit mutated canonical binary"),
        Root->GetOutermost()->IsDirty(), bDirty);
    FGridResolvedItem Rejected;
    TestFalse(TEXT("No invented N21 ARM level"),
        Fixture.Registry->ResolveItem(TEXT("Weapon_ThrowingKnife_01"), 21, Rejected));
    TestFalse(TEXT("No unknown historical ItemId silently materialized"),
        Fixture.Registry->ResolveItem(TEXT("Not_An_Authored_Item"), 0, Rejected));
    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
