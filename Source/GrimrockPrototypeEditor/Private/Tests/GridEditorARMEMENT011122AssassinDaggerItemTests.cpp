#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GridEditorCanonicalStarterTestWorld.h"

namespace ARMEMENT011122Tests
{
    constexpr TCHAR ItemId[] = TEXT("Weapon_AssassinDagger_01");
    constexpr TCHAR ArchetypeId[] = TEXT("ARM002");
    constexpr TCHAR VisualId[] = TEXT("VIS_AssassinDagger_01");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT011122SourceContract,
    "Grimrock.RPG.ARMEMENT01.11.2.2.AssassinDaggerSource",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT011122SourceContract::RunTest(const FString&)
{
    FGridEditorCanonicalStarterTestWorld Fixture;
    if (!TestTrue(TEXT("One production registry configured"), Fixture.Ready()))
        return false;
    FGridResolvedItem Dagger;
    if (!TestTrue(TEXT("ARM002 N1 is resolved by DA_ItemSystem"),
        Fixture.Registry->ResolveItem(ARMEMENT011122Tests::ItemId, 1, Dagger)))
        return false;
    TestTrue(TEXT("ARM002 canonical identity is valid"),
        Dagger.IsValid() && Dagger.bIsArmament &&
        Dagger.ItemLevel == 1 && Dagger.ArchetypeId == FName(ARMEMENT011122Tests::ArchetypeId));
    TestTrue(TEXT("Real ARM002 offensive dice remain authored"), Dagger.Weapon.DamageDice.bEnabled);
    TestTrue(TEXT("One-handed MainHand attack source"),
        Dagger.Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand) &&
        Dagger.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::MainHand) &&
        !Dagger.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::OffHand));
    TestTrue(TEXT("Positive authored action cost and range"),
        Dagger.Weapon.ActionPointCost > 0 && Dagger.Weapon.RangeCells > 0);
    TestEqual(TEXT("Canonical ARM002 has one visual identity"),
        Dagger.VisualId, FName(ARMEMENT011122Tests::VisualId));
    TestTrue(TEXT("Real icon can be loaded"),
        Dagger.ItemIcon.LoadSynchronous() != nullptr);
    TestTrue(TEXT("World mesh can be loaded"),
        Dagger.WorldMesh.LoadSynchronous() != nullptr);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT011122Definition,
    "Grimrock.RPG.ARMEMENT01.11.2.2.AssassinDaggerDefinition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT011122Definition::RunTest(const FString&)
{
    FGridEditorCanonicalStarterTestWorld Fixture;
    if (!TestTrue(TEXT("Canonical ItemSystem configured"), Fixture.Ready()))
        return false;
    TArray<FName> Ids;
    Fixture.Registry->GetItemIds(Ids);
    int32 Found = 0;
    for (FName Id : Ids)
        if (Id == FName(ARMEMENT011122Tests::ItemId)) ++Found;
    TestEqual(TEXT("Exactly one registered AssassinDagger identity"), Found, 1);
    FGridResolvedItem Item;
    if (!TestTrue(TEXT("Assassin dagger N1 resolves"),
        Fixture.Registry->ResolveItem(ARMEMENT011122Tests::ItemId, 1, Item)))
        return false;
    TestEqual(TEXT("Stable ItemId"), Item.ItemId, FName(ARMEMENT011122Tests::ItemId));
    TestEqual(TEXT("Stable ARM archetype"), Item.ArchetypeId, FName(ARMEMENT011122Tests::ArchetypeId));
    TestEqual(TEXT("One visual identity"), Item.VisualId, FName(ARMEMENT011122Tests::VisualId));
    TestTrue(TEXT("Positive canonical N1 weight"), FMath::IsFinite(Item.WeightKg) && Item.WeightKg > 0.f);
    TestEqual(TEXT("One non-stackable weapon"), Item.MaxStack, 1);
    TestTrue(TEXT("No invented thrown projectile"), !Item.Weapon.Projectile.bEnabled);
    TestTrue(TEXT("Physical MainHand attack exists"),
        Item.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::MainHand));
    TestFalse(TEXT("No OffHand attack invented"),
        Item.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::OffHand));
    TestTrue(TEXT("Only authored combat bindings remain"), Item.AvailableActions.Num() == 1);
    if (Item.AvailableActions.Num() == 1)
    {
        FGridResolvedBoundAction Bound;
        if (TestTrue(TEXT("Bound canonical attack resolves"),
            Fixture.Registry->ResolveBoundAttackForHand(Item.ItemId, 1,
                Item.AvailableActions[0].ActionId,
                EGridEquipmentSlot::MainHand, Bound)))
        {
            TestTrue(TEXT("Real attack resolved and authored"), Bound.IsValid());
            // Compact actions intentionally do not duplicate the N1 weapon AP
            // cost. Check the bound action's identity and its numeric owner.
            TestEqual(TEXT("Bound action keeps exact canonical identity"),
                Bound.Action.ActionId, Item.AvailableActions[0].ActionId);
            TestTrue(TEXT("N1 weapon alone owns the positive AP cost"),
                Item.Weapon.ActionPointCost > 0);
        }
    }
    TestTrue(TEXT("Canonical equipped mesh exists"),
        Item.EquippedMesh.LoadSynchronous() != nullptr);
    FGridResolvedItem Invalid;
    TestFalse(TEXT("No N21 level from historical catalogue"),
        Fixture.Registry->ResolveItem(Item.ItemId, 21, Invalid));
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
