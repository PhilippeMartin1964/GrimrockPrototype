#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENTLOT027MagicAndSpear,
    "Grimrock.RPG.ARMEMENT.LOT02.MagicAndSpearN1Contracts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENTLOT027MagicAndSpear::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Unique native ItemSystem configured"), Fixture.bReady)) return false;
    struct FExpected
    {
        const TCHAR* Item;
        const TCHAR* Arm;
        const TCHAR* Action;
        const TCHAR* Kind;
        bool bTwo;
        bool bRanged;
    };
    const FExpected Cases[] =
    {
        {TEXT("Weapon_ShortSpear_01"),TEXT("ARM038"),TEXT("Attack_ShortSpear_01"),TEXT("MeleeTwoHand"),true,false},
        {TEXT("Weapon_ArcaneWand_01"),TEXT("ARM061"),TEXT("Attack_ArcaneWand_01"),TEXT("MagicOneHand"),false,true},
        {TEXT("Weapon_FireWand_01"),TEXT("ARM062"),TEXT("Attack_FireWand_01"),TEXT("MagicOneHand"),false,true},
        {TEXT("Weapon_ElementalStaff_01"),TEXT("ARM067"),TEXT("Attack_ElementalStaff_01"),TEXT("MagicTwoHand"),true,true},
        {TEXT("Weapon_HolyScepter_01"),TEXT("ARM069"),TEXT("Attack_HolyScepter_01"),TEXT("MagicOneHand"),false,true}
    };
    int32 Checked = 0;
    for (const FExpected& Source : Cases)
    {
        FGridWeaponPipelineRow Kind;
        Kind.BuildKind = Source.Kind;
        Kind.EquipmentSlots = TEXT("MainHand");
        Kind.AttackId = Source.Action;
        TestTrue(TEXT("Authored production kind remains supported"), Kind.IsSupportedBuildKind());
        TestEqual(TEXT("Authored kind handedness"), Kind.IsTwoHanded(), Source.bTwo);
        TestEqual(TEXT("Authored kind ranged attack semantics"), Kind.UsesRangedAction(), Source.bRanged);
        TestFalse(TEXT("Magic and spear are never consumed as throwables"), Kind.IsThrowable());
        FGridResolvedItem Item;
        if (!TestTrue(TEXT("Real magical or spear weapon N1 resolves"),
            Fixture.Resolve(FName(Source.Item), 1, Item))) continue;
        ++Checked;
        TestTrue(TEXT("Weapon numeric and physical handedness comes from canonical source"),
            Item.bIsArmament && Item.ArchetypeId == FName(Source.Arm) &&
            Item.Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand) &&
            (Item.Weapon.HandPolicy == EGridCanonicalHandPolicy::TwoHanded) == Source.bTwo &&
            Item.Weapon.DamageDice.bEnabled);
        FGridResolvedBoundAction Attack;
        if (TestTrue(TEXT("Actual MainHand attack resolves"),
            Fixture.Registry->ResolveBoundAttackForHand(
                Item.ItemId, 1, FName(Source.Action), EGridEquipmentSlot::MainHand, Attack)))
        {
            TestTrue(TEXT("Magic/staff attack remains equipment sourced"),
                Attack.IsValid() &&
                Attack.Action.bRangedAttack == Source.bRanged &&
                Attack.Action.SourceItemQuantityCost == 0);
        }
        FGridResolvedItem N20;
        if (TestTrue(TEXT("Level twenty still has same weapon archetype"),
            Fixture.Resolve(Item.ItemId, 20, N20)))
            TestTrue(TEXT("Level twenty still has same hand and damage mode"),
                N20.ArchetypeId == Item.ArchetypeId &&
                N20.Weapon.HandPolicy == Item.Weapon.HandPolicy &&
                N20.Weapon.DamageDice.bEnabled);
    }
    TestEqual(TEXT("Five genuine magic/spear source identities"), Checked, 5);
    FGridWeaponPipelineRow Focus;
    Focus.BuildKind = TEXT("FocusPassive");
    TestTrue(TEXT("Focus stays passive and has no generated magic attack"),
        Focus.IsSupportedBuildKind() && Focus.IsFocus() &&
        !Focus.UsesRangedAction());
    return !HasAnyErrors();
}
#endif
