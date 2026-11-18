#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../ARMEMENT/GridEditorAlchemyCutoverPlan.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X62AlchemyEvidence,
    "Grimrock.RPG.ARM.CLEAN06_4X62.NativeAlchemyEvidenceReadOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN064X62AlchemyEvidence::RunTest(const FString&)
{
    namespace Audit = GridEditorAlchemyMigrationAudit;
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr, Audit::RootPath()));
    if (!TestNotNull(TEXT("Real X.61B canonical root"), Root.Get()) ||
        !TestNotNull(TEXT("Action catalogue"), Root->ActionCatalogue.Get()) ||
        !TestNotNull(TEXT("Presentation catalogue"), Root->ActionPresentationCatalogue.Get())) return false;
    TStrongObjectPtr<UGridItemSystemAsset> Historical(NewObject<UGridItemSystemAsset>());
    TStrongObjectPtr<UGridItemSystemAsset> Verified(NewObject<UGridItemSystemAsset>());
    UGridItemSystemAsset* EvidenceRoot = Root.Get();
    if (Root->ItemDefinitions.Num() == 48)
    {
        FString VerifyError;
        if (!TestTrue(TEXT("Persisted extension equals fresh projection"),
            GridEditorAlchemyCutoverPlan::BuildForCurrentState(*Root.Get(), *Verified.Get(), VerifyError)) ||
            !GridEditorAlchemyCutoverPlan::HistoricalView(*Root.Get(), *Historical.Get(), VerifyError))
        { AddError(VerifyError); return false; }
        EvidenceRoot = Historical.Get();
    }
    const auto Original = Root->ItemDefinitions;
    const auto ExistingActions = Root->ActionCatalogue->DetailedActions.Num();
    const auto ExistingPresentations = Root->ActionPresentationCatalogue->DetailedPresentations.Num();
    FString Error;
    TArray<Audit::FRow> Rows;
    const bool bBuilt = Audit::Build(*EvidenceRoot, Rows, Error, EvidenceRoot != Root.Get());
    if (!TestTrue(*FString::Printf(TEXT("Read-only native evidence: %s"), *Error), bBuilt) ||
        !TestEqual(TEXT("Ten legacy sources and canonical Oil control"), Rows.Num(), Audit::TotalCount))
        return false;

    for (int32 I = 0; I < Rows.Num(); ++I)
    {
        const auto& Row = Rows[I];
        if (!TestNotNull(TEXT("Exact source loaded"), Row.Source)) return false;
        TestEqual(TEXT("Stable source identity"), Row.Source->ItemDefinitionId, Audit::Id(I));
        TestEqual(TEXT("Only oil is in original root"), Row.bRegistered, I == Audit::LegacyCount);
        TestTrue(TEXT("Action export is complete"), !Audit::ActionExport(Row.Action).IsEmpty());
        for (int32 J = 0; J < Rows.Num(); ++J)
        {
            const bool bSameId = I != J && Row.Action.ActionId == Rows[J].Action.ActionId;
            TestEqual(TEXT("Every same-id pair recorded"), Row.SameIdPeers.Contains(J), bSameId);
        }
        FGridCombatActionDefinition Rebuilt;
        TestTrue(TEXT("Source still builds identical QuickItem"), Row.Source->BuildQuickItemCombatActionDefinition(Rebuilt) &&
            Audit::DifferentActionFields(Row.Action, Rebuilt).IsEmpty());
        if (!Row.bRegistered)
        {
            TestTrue(TEXT("Successful isolated projection or explicit reason"), Row.bIsolatedParity || !Row.ProjectionReason.IsEmpty());
        }
        FGridCombatActionDefinition Changed = Row.Action;
        Changed.ActionPointCost += 1;
        TestTrue(TEXT("A changed action cost cannot pass reflected parity"),
            Audit::DifferentActionFields(Row.Action, Changed).Contains(TEXT("ActionPointCost")));
        TestFalse(TEXT("Source package not dirtied"), Row.Source->GetOutermost()->IsDirty());
        AddInfo(FString::Printf(TEXT("[ARM-CLEAN06.4X62] %s Action=%s isolated=%d peers=%d reason=%s"),
            *Audit::Id(I).ToString(), *Row.Action.ActionId.ToString(),
            Row.bIsolatedParity, Row.SameIdPeers.Num(), *Row.ProjectionReason));
    }
    // Independently exercise the shared-id hazard even if the native
    // sources have since been edited to different action identifiers.
    TStrongObjectPtr<UGridActionCatalogueAsset> Collision(NewObject<UGridActionCatalogueAsset>());
    FGridCanonicalRuntimeAction First;
    First.Definition = Rows[0].Action;
    FGridCanonicalRuntimeAction Second = First;
    Second.Definition.ActionPointCost += 1;
    Collision->DetailedActions.Add(First);
    Collision->DetailedActions.Add(Second);
    TArray<FString> Errors;
    TestFalse(TEXT("Global catalogue rejects same action ID with different mechanics"), Collision->Validate(Errors));
    TestTrue(TEXT("Duplicate refusal has a diagnostic"), !Errors.IsEmpty());
    TestTrue(TEXT("Ordered root definitions unchanged"), Root->ItemDefinitions == Original);
    TestEqual(TEXT("Existing action catalogue unchanged"), Root->ActionCatalogue->DetailedActions.Num(), ExistingActions);
    TestEqual(TEXT("Existing presentation catalogue unchanged"), Root->ActionPresentationCatalogue->DetailedPresentations.Num(), ExistingPresentations);
    TestFalse(TEXT("Root and catalogues remain clean"), Root->GetOutermost()->IsDirty() ||
        Root->ActionCatalogue->GetOutermost()->IsDirty() ||
        Root->ActionPresentationCatalogue->GetOutermost()->IsDirty());
    return !HasAnyErrors();
}
#endif
