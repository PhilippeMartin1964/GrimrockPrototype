#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"

// Permanent read-only regression of the installed canonical ItemSystem.
// The obsolete transient production candidate builders were one-shot tools.

#include "Tests/GridCanonicalLootRegistryTestUtils.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062UFullProductionCandidate,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.FullProductionCandidatePreflight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062UFullProductionCandidate::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Registry;
    if (!TestTrue(TEXT("Single installed ItemSystem validates"), Registry.bReady)) return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Authored ItemSystem is present"), Root)) return false;
    const bool bWasDirty = Root->GetOutermost()->IsDirty();
    TArray<FString> Errors;
    TestTrue(TEXT("Root plus four canonical catalogues structurally valid"),
        Root->Validate(Errors));
    TestTrue(TEXT("Exact 48-item native source order and classes"),
        GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*Root));
    TestNotNull(TEXT("Single numeric catalogue"), Root->WeaponCatalogue.Get());
    TestNotNull(TEXT("Single visual catalogue"), Root->WeaponVisualCatalogue.Get());
    TestNotNull(TEXT("Single compact/detailed action catalogue"), Root->ActionCatalogue.Get());
    TestNotNull(TEXT("Single presentation catalogue"), Root->ActionPresentationCatalogue.Get());
    TestEqual(TEXT("48 actual production items"), Root->ItemDefinitions.Num(), 48);
    TestEqual(TEXT("All 14 N0 runtime action definitions"),
        Root->ActionCatalogue->DetailedActions.Num(), 14);
    TestEqual(TEXT("Original 19 compact ARM action definitions"),
        Root->ActionCatalogue->Actions.Num(), 19);
    FGridResolvedItem Missing;
    TestFalse(TEXT("Unknown IDs do not load obsolete DataAssets"),
        Registry.Registry->ResolveItem(TEXT("ObsoleteCandidateItem"), 0, Missing));
    TestEqual(TEXT("Read-only preflight leaves native package unchanged"),
        Root->GetOutermost()->IsDirty(), bWasDirty);
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
