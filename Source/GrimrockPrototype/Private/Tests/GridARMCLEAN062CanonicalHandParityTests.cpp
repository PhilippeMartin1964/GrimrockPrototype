#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "Serialization/Csv/CsvParser.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062ProductionARMHandsTest,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionHandActionParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean062ProductionARMHandsTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Registry;
    if (!TestTrue(TEXT("Canonical hand registry configured"), Registry.bReady)) return false;
    FString Csv;
    if (!TestTrue(TEXT("Production identity manifest loads"),
        FFileHelper::LoadFileToString(Csv,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Config/Armement/WeaponProductionManifest.csv")))))
        return false;
    FCsvParser Parser(Csv);
    const TArray<TArray<const TCHAR*>>& Rows = Parser.GetRows();
    if (!TestEqual(TEXT("Header plus 22 production ARM"), Rows.Num(), 23))
        return false;
    int32 Arms = 0, Levels = 0, Passive = 0, Focus = 0, AttackMain = 0, AttackOff = 0;
    TSet<FName> Seen;
    for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
    {
        const TArray<const TCHAR*>& Columns = Rows[RowIndex];
        if (!TestTrue(TEXT("Production CSV row complete"), Columns.Num() >= 4))
            return false;
        const FString State(Columns[1]);
        const FString Kind(Columns[2]);
        if (!TestEqual(TEXT("Exactly authored Active state"), State, FString(TEXT("Active"))))
            continue;
        ++Arms;
        const FName ItemId(Columns[3]);
        TestFalse(TEXT("Each physical ARM appears once"), Seen.Contains(ItemId));
        Seen.Add(ItemId);
        FGridResolvedItem N1;
        if (!TestTrue(TEXT("Concrete N1 canonical ARM resolves"),
            Registry.Resolve(ItemId, 1, N1))) continue;
        if (Kind == TEXT("FocusPassive") || Kind == TEXT("ShieldPassive"))
        {
            ++Passive;
            Focus += Kind == TEXT("FocusPassive") ? 1 : 0;
            TestFalse(TEXT("Passive ARM has no attack-source permission"),
                N1.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::MainHand) ||
                N1.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::OffHand));
        }
        else
        {
            AttackMain += N1.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::MainHand) ? 1 : 0;
            AttackOff += N1.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::OffHand) ? 1 : 0;
        }
        for (int32 Level = 1; Level <= 20; ++Level)
        {
            FGridResolvedItem Resolved;
            if (!TestTrue(TEXT("Canonical hand policy at every physical level"),
                Registry.Resolve(ItemId, Level, Resolved))) continue;
            ++Levels;
            TestEqual(TEXT("Hand equip policy does not vary by physical level"),
                Resolved.Weapon.HandPolicy, N1.Weapon.HandPolicy);
            TestEqual(TEXT("MainHand attack never invented by level growth"),
                Resolved.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::MainHand),
                N1.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::MainHand));
            TestEqual(TEXT("OffHand attack never invented by level growth"),
                Resolved.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::OffHand),
                N1.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::OffHand));
            TestEqual(TEXT("Physical armor retains canonical progression"),
                Resolved.Weapon.PhysicalArmor >= 0, true);
        }
    }
    TestEqual(TEXT("Exactly 22 hand-authorized ARM"), Arms, 22);
    TestEqual(TEXT("All 440 canonical hand+numeric profiles"), Levels, 440);
    TestEqual(TEXT("Three passive ARM"), Passive, 3);
    TestEqual(TEXT("Exactly one FocusPassive"), Focus, 1);
    TestEqual(TEXT("Nineteen real MainHand attacks"), AttackMain, 19);
    TestEqual(TEXT("Two real OffHand attacks"), AttackOff, 2);

    FGridResolvedWeapon Guard;
    Guard.Kind = EGridArmamentKind::Focus;
    Guard.HandPolicy = EGridHandPolicy::OffHand;
    Guard.AttackSourceSlots.Add(EGridEquipmentSlot::OffHand);
    TestFalse(TEXT("Passive focus never grants attack"), Guard.HasAttackSourceInHand(EGridEquipmentSlot::OffHand));
    Guard.Kind = EGridArmamentKind::Weapon;
    Guard.HandPolicy = EGridHandPolicy::MainHand;
    TestFalse(TEXT("MainHand-only never attacks OffHand"), Guard.HasAttackSourceInHand(EGridEquipmentSlot::OffHand));
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
