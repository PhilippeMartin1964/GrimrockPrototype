#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"

// Permanent final-state regression for the installed ItemSystem. The one-shot
// HandThrowPlan is retired; this test never opens a legacy DA_Weapon_* asset.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN065D3NativeProjection,
    "Grimrock.RPG.ARM.CLEAN06_5D3.RealHandThrowProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridEditorARMCLEAN065D3NativeProjection::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Real installed ItemSystem registry"), Fixture.bReady))
        return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Only canonical root"), Root) ||
        !TestNotNull(TEXT("Numeric archetype catalogue"), Root->WeaponCatalogue.Get()) ||
        !TestNotNull(TEXT("Weapon visual catalogue"), Root->WeaponVisualCatalogue.Get()))
        return false;
    const bool bNumericDirty = Root->WeaponCatalogue->GetOutermost()->IsDirty();
    const bool bVisualDirty = Root->WeaponVisualCatalogue->GetOutermost()->IsDirty();
    TArray<FGridWeaponPipelineRow> Rows;
    FString Error;
    if (!TestTrue(TEXT("22 real ARM identity rows parse"),
        FGridWeaponPipelineManifest::Load(
            FGridWeaponPipelineManifest::DefaultFile(), Rows, Error)))
        return false;
    int32 Ordinary = 0, Specialized = 0, Levels = 0, Physical = 0;
    TSet<FName> Seen;
    for (const FGridWeaponPipelineRow& Row : Rows)
    {
        if (Row.ProductionState != TEXT("Active")) continue;
        const FName ItemId(Row.ItemDefinitionId);
        TestFalse(TEXT("No duplicate physical ItemId"), Seen.Contains(ItemId));
        Seen.Add(ItemId);
        FGridResolvedItem N1;
        if (!TestTrue(TEXT("Real first level resolves from canonical registry"),
            Fixture.Resolve(ItemId, 1, N1))) continue;
        const auto& P = N1.Weapon.Projectile;
        const auto& V = N1.ProjectileAppearance;
        const bool bSpecialized = Row.IsThrowable();
        TestEqual(TEXT("Only three specialized combat projectiles"),
            P.bEnabled, bSpecialized);
        TestEqual(TEXT("Ordinary ARM has explicit hand-toss policy"),
            P.bGenericThrowAuthored, !bSpecialized);
        TestEqual(TEXT("Ordinary throw has no dedicated projectile mesh"),
            N1.ProjectileMesh.IsNull(), !bSpecialized);
        if (bSpecialized)
        {
            ++Specialized;
            TestFalse(TEXT("Specialized action does not double-grant generic throw"),
                P.bGenericThrowOneHanded);
            TestTrue(TEXT("Dedicated flight has authored visual"),
                V.bEnabled && !N1.ProjectileMesh.IsNull());
        }
        else
        {
            ++Ordinary;
            TestFalse(TEXT("Ordinary hand toss does not grant combat attack source"),
                P.bEnabled || V.bEnabled && !P.bGenericThrowAuthored);
            TestTrue(TEXT("Ordinary authored speed is finite and nonnegative"),
                FMath::IsFinite(P.InitialSpeed) && P.InitialSpeed >= 0.0f);
            if (P.bGenericThrowOneHanded)
            {
                ++Physical;
                TestTrue(TEXT("Eligible hand toss has a real positive flight speed"),
                    P.InitialSpeed > KINDA_SMALL_NUMBER);
            }
            if (Row.IsPassive())
                TestFalse(TEXT("Passive shield/focus is not a generic hand throw"),
                    P.bGenericThrowOneHanded);
        }
        TestTrue(TEXT("Flight arc/lifetime/offset finite and valid"),
            FMath::IsFinite(P.Arc) && FMath::IsFinite(P.FlightLifetime) &&
            P.FlightLifetime >= 0.0f &&
            FMath::IsFinite(P.ImpactDropOffset) && P.ImpactDropOffset >= 0.0f);
        TestTrue(TEXT("Visual transform and angular velocities are finite"),
            !V.RelativeRotation.ContainsNaN() && !V.RelativeScale.ContainsNaN() &&
            !V.TumbleAxis.ContainsNaN() &&
            FMath::IsFinite(V.TumbleDegreesPerSecond) &&
            FMath::IsFinite(V.SpinDegreesPerSecond));
        for (int32 Level = 1; Level <= 20; ++Level)
        {
            FGridResolvedItem Item;
            if (!TestTrue(TEXT("N1-N20 physical flight policy resolves"),
                Fixture.Resolve(ItemId, Level, Item))) continue;
            ++Levels;
            TestEqual(TEXT("Hand throw is authored per archetype, not level"),
                Item.Weapon.Projectile.bGenericThrowOneHanded, P.bGenericThrowOneHanded);
            TestEqual(TEXT("Combat throw permission stays fixed at all levels"),
                Item.Weapon.Projectile.bEnabled, P.bEnabled);
            TestEqual(TEXT("No visual identity drift from N1 to N20"),
                Item.VisualId, N1.VisualId);
        }
    }
    TestEqual(TEXT("All 22 concrete ARM identities"), Seen.Num(), 22);
    TestEqual(TEXT("19 ordinary generic flights"), Ordinary, 19);
    TestEqual(TEXT("Three specialized combat projectiles"), Specialized, 3);
    TestEqual(TEXT("440 real N1-N20 physical weapon sources"), Levels, 440);
    TestTrue(TEXT("At least one real one-handed weapon is throwable"), Physical > 0);
    TestEqual(TEXT("Numeric catalogue never dirtied"), Root->WeaponCatalogue->GetOutermost()->IsDirty(), bNumericDirty);
    TestEqual(TEXT("Visual catalogue never dirtied"), Root->WeaponVisualCatalogue->GetOutermost()->IsDirty(), bVisualDirty);
    FGridResolvedItem Invalid;
    TestFalse(TEXT("No bad physical N21 source can project flight"),
        Fixture.Registry->ResolveItem(TEXT("Weapon_ThrowingKnife_01"), 21, Invalid));
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
