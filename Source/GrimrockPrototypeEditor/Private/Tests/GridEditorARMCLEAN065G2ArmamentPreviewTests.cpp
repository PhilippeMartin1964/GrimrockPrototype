#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../Details/GridEditorArmamentPreview.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARM065G2Preview,
    "Grimrock.Editor.ARM.CLEAN06_5G2.CanonicalArmamentDetails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARM065G2Preview::RunTest(const FString&)
{
    UGridItemSystemAsset* Root = GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Configured native ItemSystem root"), Root)) return false;
    const bool bWasDirty = Root->GetOutermost()->IsDirty();
    int32 Count = 0;
    FGridResolvedItem N1, N20;
    FString Error;
    for (const auto& Definition : Root->ItemDefinitions)
    {
        const auto* Arm = Cast<UGridArmamentDefinitionAsset>(Definition.Get());
        if (!Arm) continue;
        ++Count;
        const bool bArmWasDirty = Arm->GetOutermost()->IsDirty();
        if (!TestTrue(TEXT("Exact registered asset resolves both levels"),
            GridEditorArmamentPreview::Resolve(Root, Arm, N1, N20, Error)))
            return false;
        TestEqual(TEXT("N1 identity"), N1.ItemId, Arm->ItemId);
        TestEqual(TEXT("N20 identity"), N20.ItemId, Arm->ItemId);
        TestEqual(TEXT("N1 level"), N1.ItemLevel, 1);
        TestEqual(TEXT("N20 level"), N20.ItemLevel, 20);
        TestEqual(TEXT("Same physical weight at both levels"), N1.WeightKg, N20.WeightKg);
        TestEqual(TEXT("No edits to actual arm package"), Arm->GetOutermost()->IsDirty(), bArmWasDirty);
        TStrongObjectPtr<UGridArmamentDefinitionAsset> Duplicate(NewObject<UGridArmamentDefinitionAsset>());
        Duplicate->ItemId = Arm->ItemId;
        Duplicate->ArchetypeId = Arm->ArchetypeId;
        TestFalse(TEXT("Unindexed duplicate identity refuses borrowed preview"),
            GridEditorArmamentPreview::Resolve(Root, Duplicate.Get(), N1, N20, Error));
        TestFalse(TEXT("Failure clears both prior resolved previews"), N1.IsValid() || N20.IsValid());
    }
    TestEqual(TEXT("All 22 persisted armaments covered"), Count, 22);
    TestFalse(TEXT("No registry means no preview"),
        GridEditorArmamentPreview::Resolve(nullptr, nullptr, N1, N20, Error));
    TestEqual(TEXT("No mutation of root"), Root->GetOutermost()->IsDirty(), bWasDirty);
    return !HasAnyErrors();
}
#endif
