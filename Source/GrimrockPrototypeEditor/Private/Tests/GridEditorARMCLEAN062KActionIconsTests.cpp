#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062KRealIconPaletteProjection,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionActionIconPalette",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062KRealIconPaletteProjection::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Registered ItemSystem for action icons"), Fixture.bReady))
        return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Production ItemSystem root"), Root) ||
        !TestNotNull(TEXT("Unique canonical visual catalogue"), Root->WeaponVisualCatalogue.Get()))
        return false;
    const bool bDirty = Root->GetOutermost()->IsDirty();
    TArray<FString> Errors;
    TestTrue(TEXT("Real native catalogue icons validate"),
        Root->WeaponVisualCatalogue->Validate(Errors));
    int32 Configured = 0, Empty = 0;
    for (const TPair<EGridEquippedWeaponActionIconKind, TSoftObjectPtr<UTexture2D>>& Entry :
        Root->WeaponVisualCatalogue->ActionIcons)
    {
        const TSoftObjectPtr<UTexture2D> Resolved =
            Fixture.Registry->FindEquippedActionIcon(Entry.Key);
        TestEqual(TEXT("ItemSystem resolves exact authored icon path"),
            Resolved.ToSoftObjectPath(), Entry.Value.ToSoftObjectPath());
        if (Entry.Value.IsNull()) ++Empty;
        else
        {
            ++Configured;
            TestTrue(TEXT("Authored action icon loads"), Resolved.LoadSynchronous() != nullptr);
        }
    }
    TestTrue(TEXT("One or more authored action icon kinds"), Configured > 0);
    AddInfo(FString::Printf(TEXT("[ARM-CLEAN06] Canonical action icons: %d configured, %d intentionally blank."),
        Configured, Empty));
    TestEqual(TEXT("Read-only icon check never dirties DA_ItemSystem"),
        Root->GetOutermost()->IsDirty(), bDirty);
    return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062KIconSafety,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.ActionIconProjectionSafety",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062KIconSafety::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Canonical icon registry"), Fixture.bReady)) return false;
    TestTrue(TEXT("None cannot acquire a decorative action icon"),
        Fixture.Registry->FindEquippedActionIcon(EGridEquippedWeaponActionIconKind::None).IsNull());
    TestTrue(TEXT("Unknown icon category never guesses a historical palette"),
        Fixture.Registry->FindEquippedActionIcon(
            static_cast<EGridEquippedWeaponActionIconKind>(254)).IsNull());
    FGridResolvedItem Unknown;
    TestFalse(TEXT("An invalid ItemId cannot acquire an action icon by fallback"),
        Fixture.Registry->ResolveItem(TEXT("Unknown_ActionIconItem"), 0, Unknown));
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
