#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"

// Permanent LOT02.6 guard against regressing the three authored
// recoverable throwing weapons into a generic consumable or spell.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENTLOT026Throwing,
    "Grimrock.RPG.ARMEMENT.LOT02.ThrowingWeaponN1Batch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENTLOT026Throwing::RunTest(const FString&)
{
    FGridWeaponPipelineRow Kind;
    Kind.BuildKind = TEXT("ThrowingOneHand");
    TestTrue(TEXT("Throwing production kind supported"), Kind.IsSupportedBuildKind());
    TestTrue(TEXT("Throwing is a dedicated physical projectile"), Kind.IsThrowable());
    TestFalse(TEXT("Throwing never passive"), Kind.IsPassive());
    Kind.BuildKind = TEXT("MeleeOneHand");
    TestFalse(TEXT("Melee does not silently become a specialized thrown source"), Kind.IsThrowable());
    Kind.BuildKind = TEXT("FocusPassive");
    Kind.EquipmentSlots = TEXT("OffHand");
    TestTrue(TEXT("Passive focus remains a supported physical source"),
        Kind.IsSupportedBuildKind() && Kind.IsPassive() && Kind.IsFocus());
    Kind.BuildKind = TEXT("MagicOneHand");
    TestTrue(TEXT("Magic is a direct attack, not a thrown consumable"),
        Kind.IsSupportedBuildKind() && Kind.UsesRangedAction() && !Kind.IsThrowable());

    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Real installed ItemSystem"), Fixture.bReady)) return false;
    struct FSource { const TCHAR* Item; const TCHAR* Arm; const TCHAR* Attack; };
    const FSource Sources[] =
    {
        {TEXT("Weapon_ThrowingKnife_01"),TEXT("ARM054"),TEXT("Attack_ThrowingKnife_01")},
        {TEXT("Weapon_Javelin_01"),TEXT("ARM057"),TEXT("Attack_Javelin_01")},
        {TEXT("Weapon_Dart_01"),TEXT("ARM058"),TEXT("Attack_Dart_01")}
    };
    int32 ValidSources = 0;
    for (const FSource& Source : Sources)
    {
        FGridResolvedItem N1, N20;
        if (!TestTrue(TEXT("N1/N20 canonical thrown source"),
            Fixture.Resolve(FName(Source.Item), 1, N1) &&
            Fixture.Resolve(FName(Source.Item), 20, N20))) continue;
        ++ValidSources;
        TestTrue(TEXT("Exactly the three approved specialized projectile identities"),
            N1.bIsArmament && N1.ArchetypeId == FName(Source.Arm) &&
            N1.Weapon.Projectile.bEnabled && N1.Weapon.DamageDice.bEnabled &&
            N1.Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand) &&
            N1.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::MainHand));
        TestTrue(TEXT("N1 thrown physics are real and finite"),
            FMath::IsFinite(N1.Weapon.Projectile.InitialSpeed) &&
            N1.Weapon.Projectile.InitialSpeed > 0.f &&
            FMath::IsFinite(N1.Weapon.Projectile.Arc) &&
            N1.ProjectileAppearance.bEnabled && !N1.ProjectileMesh.IsNull());
        FGridResolvedBoundAction Attack;
        if (!TestTrue(TEXT("One actually bound physical attack"),
            Fixture.Registry->ResolveBoundAttackForHand(
                N1.ItemId, 1, FName(Source.Attack), EGridEquipmentSlot::MainHand, Attack)))
            continue;
        TestTrue(TEXT("Recoverable thrown attack consumes exactly one source"),
            Attack.IsValid() && Attack.Action.bRangedAttack &&
            Attack.Action.SourceItemQuantityCost == 1);
        TestTrue(TEXT("N20 does not invent a different projectile authority"),
            N20.Weapon.Projectile.bEnabled &&
            N20.ArchetypeId == N1.ArchetypeId &&
            N20.VisualId == N1.VisualId);
    }
    TestEqual(TEXT("Exactly three real specialized thrown weapons"), ValidSources, 3);
    FGridResolvedItem Wrong;
    TestFalse(TEXT("Thrown ARM cannot be promoted from legacy N0"),
        Fixture.Registry->ResolveItem(TEXT("Weapon_ThrowingKnife_01"), 0, Wrong));
    return !HasAnyErrors();
}
#endif
