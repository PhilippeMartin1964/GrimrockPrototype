#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "UI/GridWeaponTooltipProjection.h"

namespace ARMEMENT0164
{
    struct FPilot {const TCHAR* Id; const TCHAR* Arm; const TCHAR* Visual;};
    const FPilot Pilots[] =
    {
        {TEXT("Weapon_Dagger_01"), TEXT("ARM001"), TEXT("VIS_Dagger_01")},
        {TEXT("Weapon_ShortBow_01"), TEXT("ARM045"), TEXT("VIS_ShortBow_01")},
        {TEXT("Weapon_RoundShield_01"), TEXT("ARM075"), TEXT("VIS_RoundShield_01")}
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0164InventoryIconTest,
    "Grimrock.RPG.ARMEMENT01.6.UI.InventoryTooltipIconAndRealLevel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT0164InventoryIconTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Real ItemSystem for UI presentation"), Fixture.bReady))
        return false;
    for (const ARMEMENT0164::FPilot& Pilot : ARMEMENT0164::Pilots)
    {
        FGridResolvedItem Resolved;
        if (!TestTrue(TEXT("Approved canonical pilot N12"),
            Fixture.Resolve(FName(Pilot.Id), 12, Resolved))) continue;
        TestEqual(TEXT("Exact numeric visual ID"), Resolved.VisualId, FName(Pilot.Visual));
        UTexture2D* Icon = Resolved.ItemIcon.LoadSynchronous();
        if (!TestNotNull(TEXT("Inventory icon loads from single catalogue"), Icon)) continue;
        FGridItemInstance Instance;
        Instance.RuntimeObjectId = FGuid::NewGuid();
        Instance.ItemDefinitionId = Resolved.ItemId;
        Instance.ItemLevel = 12;
        Instance.Quantity = 1;
        Instance.OwnerType = EGridItemOwnerType::CharacterInventory;
        Instance.OwnerCharacterIndex = 0;
        FGridWeaponTooltipPresentation View;
        TestTrue(TEXT("UI tooltip reads actual physical item level"),
            FGridWeaponTooltipProjection::Build( Instance, View, Fixture.Registry.Get()) &&
            View.bHasProfile && View.ItemLevel == 12 &&
            View.ArchetypeId == FName(Pilot.Arm));
        FGridResolvedItem Higher;
        TestTrue(TEXT("N20 reuses same authoritative item icon"),
            Fixture.Resolve(Resolved.ItemId, 20, Higher) &&
            Higher.ItemIcon.ToSoftObjectPath() == Resolved.ItemIcon.ToSoftObjectPath());
        Instance.ItemLevel = 0;
        TestFalse(TEXT("Invalid ARM N0 never invents tooltip level"),
            FGridWeaponTooltipProjection::Build( Instance, View, Fixture.Registry.Get()));
        TestEqual(TEXT("Invalid request does not mutate physical instance"),
            Instance.ItemLevel, 0);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0164HotbarIconTest,
    "Grimrock.RPG.ARMEMENT01.6.UI.HotbarCatalogueIconAndLegacySafety",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FARMEMENT0164HotbarIconTest::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Canonical action/icon registry"), Fixture.bReady))
        return false;
    FGridResolvedItem Dagger, Shield;
    if (!TestTrue(TEXT("Real dagger and shield profiles"),
        Fixture.Resolve(TEXT("Weapon_Dagger_01"), 1, Dagger) &&
        Fixture.Resolve(TEXT("Weapon_RoundShield_01"), 1, Shield))) return false;
    TestTrue(TEXT("Dagger image belongs to item, independent from action kind"),
        Dagger.ItemIcon.LoadSynchronous() != nullptr);
    TestTrue(TEXT("Action icon comes from global visual catalogue"),
        Fixture.Registry->FindEquippedActionIcon(
            EGridEquippedWeaponActionIconKind::MeleeOneHand).LoadSynchronous() != nullptr);
    TestTrue(TEXT("Passive shield does not borrow a combat attack"),
        Shield.AvailableActions.IsEmpty());
    TestTrue(TEXT("Unarmed None does not overwrite an action icon"),
        Fixture.Registry->FindEquippedActionIcon(
            EGridEquippedWeaponActionIconKind::None).IsNull());
    FGridResolvedItem Unknown;
    TestFalse(TEXT("Unknown ARM cannot borrow dagger or shield icon"),
        Fixture.Registry->ResolveItem(TEXT("Missing_WeaponVisual_0164"), 1, Unknown));
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
