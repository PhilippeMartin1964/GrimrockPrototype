#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "Tests/GridCanonicalLootRegistryTestUtils.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"

// Historical X.37 is complete. Keep the Automation identity, but verify
// persisted canonical visuals/physics rather than reproject 22 old assets.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X37WorldPresentationProjection,
    "Grimrock.RPG.ARM.CLEAN06_4X37.NativeArmWorldProfileStaging",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridEditorARMCLEAN064X37WorldPresentationProjection::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Only canonical ItemSystem configured"), Fixture.bReady)) return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Canonical native root"), Root) ||
        !TestNotNull(TEXT("Native visuals"), Root->WeaponVisualCatalogue.Get()) ||
        !TestNotNull(TEXT("Native weapon numbers"), Root->WeaponCatalogue.Get()))
        return false;
    TestTrue(TEXT("Final shape is 48 items, not obsolete 34/38"),
        GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*Root));
    const bool bVisualDirty = Root->WeaponVisualCatalogue->GetOutermost()->IsDirty();
    const bool bNumericDirty = Root->WeaponCatalogue->GetOutermost()->IsDirty();

    TArray<FGridWeaponPipelineRow> Rows;
    FString Error;
    if (!TestTrue(TEXT("Historical ARM identity manifest available"),
        FGridWeaponPipelineManifest::Load(
            FGridWeaponPipelineManifest::DefaultFile(), Rows, Error)))
        return false;
    int32 Checked = 0;
    TSet<FName> ItemIds, VisualIds;
    for (const FGridWeaponPipelineRow& Row : Rows)
    {
        if (Row.ProductionState != TEXT("Active")) continue;
        const FName Id(Row.ItemDefinitionId), Archetype(Row.ArchetypeId), VisualId(Row.VisualId);
        TestFalse(TEXT("Single canonical concrete ItemId"), ItemIds.Contains(Id));
        TestFalse(TEXT("Single authored visual profile per ARM"), VisualIds.Contains(VisualId));
        ItemIds.Add(Id);
        VisualIds.Add(VisualId);
        const FGridWeaponArchetype* Numeric = Root->WeaponCatalogue->Find(Archetype);
        const FGridWeaponVisual* Visual =
            Root->WeaponVisualCatalogue->FindForArchetype(Archetype);
        if (!TestNotNull(TEXT("Native numeric physics authoring"), Numeric) ||
            !TestNotNull(TEXT("Native world visual authoring"), Visual))
            continue;
        FGridResolvedItem N1, N20;
        if (!TestTrue(TEXT("Native physical ARM N1/N20 resolve"),
            Fixture.Resolve(Id, 1, N1) && Fixture.Resolve(Id, 20, N20)))
            continue;
        ++Checked;
        TestEqual(TEXT("Authoring identity exact join"), N1.ArchetypeId, Archetype);
        TestEqual(TEXT("Resolved VisualId same as authored visual"), N1.VisualId, VisualId);
        TestEqual(TEXT("Resolved world mesh from exact canonical visual"),
            N1.WorldMesh.ToSoftObjectPath(), Visual->WorldMesh.ToSoftObjectPath());
        TestTrue(TEXT("World transform preserves actual authored offset"),
            N1.WorldVisualTransform.Equals(Visual->WorldTransform));
        TestEqual(TEXT("Authored world physics mass use is preserved"),
            N1.bUseItemWeightAsWorldPhysicsMass, Numeric->bUseItemWeightAsWorldPhysicsMass);
        TestEqual(TEXT("Authored first-place tilt is preserved"),
            N1.WorldPhysicsInitialTiltDegrees, Numeric->WorldPhysicsInitialTiltDegrees);
        TestTrue(TEXT("Authored tilt finite and bounded"),
            FMath::IsFinite(N1.WorldPhysicsInitialTiltDegrees) &&
            N1.WorldPhysicsInitialTiltDegrees >= 0.f &&
            N1.WorldPhysicsInitialTiltDegrees <= 45.f);
        TestEqual(TEXT("Authored sparkle permission is preserved"),
            N1.bEnableWorldSparkle, Visual->bEnableWorldSparkle);
        TestTrue(TEXT("Authored sparkle material identity is preserved"),
            N1.WorldSparkleMaterial.ToSoftObjectPath() ==
                Visual->WorldSparkleMaterial.ToSoftObjectPath());
        TestTrue(TEXT("Light data belongs to unique visual source"),
            FGridLightEmitterConfig::StaticStruct()->CompareScriptStruct(
                &N1.LightEmitter, &Visual->LightEmitter, 0));
		TestTrue(TEXT("Niagara world presentation belongs to unique visual source"),
			FGridItemVisualEffectsConfig::StaticStruct()->CompareScriptStruct(&N20.VisualEffects, &Visual->VisualEffects, 0));
		TestTrue(TEXT("N1 only projects the authored persistent Niagara threshold"),
			Visual->VisualEffectsMinimumItemLevel == 1
				? FGridItemVisualEffectsConfig::StaticStruct()->CompareScriptStruct(&N1.VisualEffects, &Visual->VisualEffects, 0)
				: N1.VisualEffects.Effects.IsEmpty());
		TestTrue(TEXT("World profile is not duplicated by item level"),
            N20.WorldMesh.ToSoftObjectPath() == N1.WorldMesh.ToSoftObjectPath() &&
            N20.WorldVisualTransform.Equals(N1.WorldVisualTransform) &&
            N20.bEnableWorldSparkle == N1.bEnableWorldSparkle);
    }
    TestEqual(TEXT("All 22 native ARM visual and physics joins"), Checked, 22);
    TestEqual(TEXT("All 22 concrete item identities unique"), ItemIds.Num(), 22);
    TestEqual(TEXT("22 native real visuals uniquely authored"), VisualIds.Num(), 22);
    TestEqual(TEXT("Never dirty real visual catalogue"), Root->WeaponVisualCatalogue->GetOutermost()->IsDirty(), bVisualDirty);
    TestEqual(TEXT("Never dirty real numeric catalogue"), Root->WeaponCatalogue->GetOutermost()->IsDirty(), bNumericDirty);
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
