#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "../ARMEMENT/GridArmClean064X37ArmWorldProfileProjection.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X37WorldPresentationProjection,
    "Grimrock.RPG.ARM.CLEAN06_4X37.NativeArmWorldProfileStaging",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN064X37WorldPresentationProjection::RunTest(const FString&)
{
    UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem"));
    if (!TestNotNull(TEXT("Real canonical ItemSystem"), Root) ||
        !TestNotNull(TEXT("Real weapon visual catalogue"), Root->WeaponVisualCatalogue.Get()) ||
        !TestNotNull(TEXT("Real weapon catalogue"), Root->WeaponCatalogue.Get()))
    {
        return false;
    }
    const bool bVisualDirty = Root->WeaponVisualCatalogue->GetOutermost()->IsDirty();
    const bool bWeaponDirty = Root->WeaponCatalogue->GetOutermost()->IsDirty();

    ARMCLEAN064X37::FPlan Plan;
    FString Error;
    if (!TestTrue(TEXT("All 22 sources join by exact ItemId, ArchetypeId, VisualId and copy without loss"),
        ARMCLEAN064X37::BuildPlan(Root, Plan, Error)))
    {
        AddError(Error);
        return false;
    }
    bool bOk = TestEqual(TEXT("Exactly 22 historical ARM sources"), Plan.SourceCount, 22) &&
        TestEqual(TEXT("Existing visual profile count unchanged"), Plan.Visuals.Num(), 22) &&
        TestEqual(TEXT("All 82 numeric archetypes preserved"), Plan.Archetypes.Num(), 82);

    // Prove the staging helper detects an actual discrepancy, rather than
    // rubber-stamping every value. No source or real catalogue is edited.
    const TCHAR* FirstSuffix = ARMCLEAN064X37::LegacySuffixes[0];
    UGridItemDefinitionAsset* First = LoadObject<UGridItemDefinitionAsset>(
        nullptr, *ARMCLEAN064X37::SourceObjectPath(FirstSuffix));
    if (!TestNotNull(TEXT("One real source for corruption probe"), First))
    {
        return false;
    }
    FGridCanonicalWeaponVisual Probe;
    FGridCanonicalWeaponArchetype ProbeArchetype;
    bOk &= TestTrue(TEXT("Pure copy accepts real source property types"),
        ARMCLEAN064X37::Project(*First, Probe, ProbeArchetype));
    bOk &= TestTrue(TEXT("Copied values exactly match original"),
        ARMCLEAN064X37::Matches(*First, Probe, ProbeArchetype));
    ProbeArchetype.bUseItemWeightAsWorldPhysicsMass =
        !ProbeArchetype.bUseItemWeightAsWorldPhysicsMass;
    bOk &= TestFalse(TEXT("Modified physics policy is detected"),
        ARMCLEAN064X37::Matches(*First, Probe, ProbeArchetype));

    bOk &= TestEqual(TEXT("No native visual package made dirty by staging"),
        Root->WeaponVisualCatalogue->GetOutermost()->IsDirty(), bVisualDirty);
    bOk &= TestEqual(TEXT("No native weapon package made dirty by staging"),
        Root->WeaponCatalogue->GetOutermost()->IsDirty(), bWeaponDirty);
    AddInfo(TEXT("[ARM-CLEAN06.4X.37] 22 exact transient world-profile joins; 8 visuals + 2 physics fields per source. No SavePackage, .uasset or SaveGame edits."));
    return bOk && !HasAnyErrors();
}
#endif
