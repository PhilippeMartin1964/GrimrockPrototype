#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "Runtime/Combat/GridWorldItemVisualResolver.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "GridCanonicalWeightTestWorld.h"

namespace ARMEMENT0166Tests
{
    struct FSpec
    {
        const TCHAR* AssetName;
        const TCHAR* ItemId;
        const TCHAR* Archetype;
        const TCHAR* VisualId;
        EGridItemType ItemType;
        EGridEquipmentSlot Slot;
        bool bAttack;
    };

    static const FSpec Specs[] =
    {
        {TEXT("DA_Weapon_Dagger_01"), TEXT("Weapon_Dagger_01"), TEXT("ARM001"), TEXT("VIS_Dagger_01"),
         EGridItemType::Weapon, EGridEquipmentSlot::MainHand, true},
        {TEXT("DA_Weapon_ShortBow_01"), TEXT("Weapon_ShortBow_01"), TEXT("ARM045"), TEXT("VIS_ShortBow_01"),
         EGridItemType::Weapon, EGridEquipmentSlot::MainHand, true},
        {TEXT("DA_Weapon_RoundShield_01"), TEXT("Weapon_RoundShield_01"), TEXT("ARM075"), TEXT("VIS_RoundShield_01"),
         EGridItemType::Shield, EGridEquipmentSlot::OffHand, false}
    };

    UGridItemDefinitionAsset* LoadItem(const FSpec& Spec)
    {
        const FString ObjectPath = FString::Printf(
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/%s.%s"), Spec.AssetName, Spec.AssetName);
        return LoadObject<UGridItemDefinitionAsset>(nullptr, *ObjectPath);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0166AuthoredDefinitionsTest,
    "Grimrock.RPG.ARMEMENT01.6.Items.AuthoredCataloguesAndMeshes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0166AuthoredDefinitionsTest::RunTest(const FString&)
{
    for (const ARMEMENT0166Tests::FSpec& Spec : ARMEMENT0166Tests::Specs)
    {
        UGridItemDefinitionAsset* Item = ARMEMENT0166Tests::LoadItem(Spec);
        if (!TestNotNull(TEXT("Production item asset loads"), Item)) { continue; }
        TestTrue(TEXT("Definition is valid"), Item->IsValidDefinition());
        TestEqual(TEXT("Stable item ID"), Item->ItemDefinitionId, FName(Spec.ItemId));
        TestEqual(TEXT("Correct type"), Item->ItemType, Spec.ItemType);
        TestFalse(TEXT("Equipment is not stackable"), Item->bStackable);
        TestTrue(TEXT("RPG catalogue opt-in"), Item->bUseWeaponCatalogue);
        TestTrue(TEXT("Visual catalogue opt-in"), Item->bUseWeaponVisualCatalogue);
        TestEqual(TEXT("Correct archetype"), Item->WeaponArchetypeId, FName(Spec.Archetype));
        TestEqual(TEXT("Starter item level"), Item->DefaultWeaponItemLevel, 1);
        TestTrue(TEXT("Authorized equipment slot"), Item->CanEquipToSlot(Spec.Slot));

        const UGridWeaponCatalogueAsset* Rpg = Item->WeaponCatalogue.LoadSynchronous();
        const UGridWeaponVisualCatalogueAsset* Visuals = Item->WeaponVisualCatalogue.LoadSynchronous();
        if (!TestNotNull(TEXT("RPG catalogue exists"), Rpg) ||
            !TestNotNull(TEXT("Visual catalogue exists"), Visuals)) { continue; }
        const FGridWeaponLevelProfile* Profile = Rpg->FindProfile(FName(Spec.Archetype), 1);
        const FGridWeaponVisualProfile* Look = Visuals->FindProfileForArchetype(FName(Spec.Archetype));
        if (!TestNotNull(TEXT("Level 1 RPG profile"), Profile) ||
            !TestNotNull(TEXT("Approved visual profile"), Look)) { continue; }
        TestTrue(TEXT("Real level-1 catalogue weight"), FMath::IsNearlyEqual(Item->Weight, Profile->WeightKg));
        TestEqual(TEXT("Numeric VisualId"), Look->VisualId, FName(Spec.VisualId));
        const FGridResolvedWorldItemMesh World = FGridWorldItemVisualResolver::Resolve(*Item);
        TestEqual(TEXT("Realistic world mesh is resolved"), World.Mesh, Look->WorldMesh.LoadSynchronous());
        TestEqual(TEXT("Held mesh is available to existing equipment presentation"), Item->LoadHeldMesh(), Item->EquippedMesh.LoadSynchronous());
        TestEqual(TEXT("Visual identity agrees with catalogue"), World.VisualId, Look->VisualId);
        TestTrue(TEXT("Rendered mesh keeps authored transform"), World.RelativeTransform.Equals(Look->WorldVisualTransform));
        TestEqual(TEXT("Action count does not fabricate shield attack"), Item->CombatActions.Num(), Spec.bAttack ? 1 : 0);
        TestEqual(TEXT("Attack authoring matches catalogue passive flag"), Profile->DamageDice.bEnabled, Spec.bAttack);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0166EquipmentAttackTest,
    "Grimrock.RPG.ARMEMENT01.6.Items.EquipAndFixedLevelCombat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0166EquipmentAttackTest::RunTest(const FString&)
{
    FGridCanonicalWeightTestWorld Fixture({});
    const UGridItemSubsystem* Registry = Fixture.Scope.Registry;
    if (!TestNotNull(TEXT("Configured production ItemSystem"), Registry))
        return false;
    UGridPartyInventoryComponent* Inventory = Fixture.MakeInventory();
    if (!TestNotNull(TEXT("World-bound inventory for canonical pilot weapons"), Inventory))
        return false;
    Inventory->InitializeDefaultPartyIfNeeded();

    for (const ARMEMENT0166Tests::FSpec& Spec : ARMEMENT0166Tests::Specs)
    {
        const FName ItemId(Spec.ItemId);
        FGridResolvedItem Resolved;
        if (!TestTrue(TEXT("Real pilot ARM resolves canonically at N1"),
                Registry->ResolveItem(ItemId, 1, Resolved)) ||
            !TestTrue(TEXT("Resolved item and archetype match the pilot manifest"),
                Resolved.IsValid() && Resolved.bIsArmament &&
                Resolved.ItemId == ItemId && Resolved.ItemLevel == 1 &&
                Resolved.ArchetypeId == FName(Spec.Archetype)))
            return false;

        TestTrue(TEXT("Resolved canonical mass is finite and nonnegative"),
            FMath::IsFinite(Resolved.WeightKg) && Resolved.WeightKg >= 0.f);
        TestTrue(TEXT("Canonical pilot supports its authored equipment hand"),
            Resolved.Weapon.CanEquipInHand(Spec.Slot));
        TestNull(TEXT("No legacy pilot definition is registered"),
            Inventory->FindItemDefinition(ItemId));
        FGridItemInstance Instance;
        Instance.RuntimeObjectId = FGuid::NewGuid();
        Instance.ItemDefinitionId = ItemId;
        Instance.ItemLevel = 1;
        Instance.Quantity = 1;
        if (!TestTrue(TEXT("Canonical fixed-level instance materializes"),
                Inventory->ApplyItemDefinitionToInstance(Instance)))
            return false;
        TestEqual(TEXT("Physical item identity is stable"), Instance.ItemDefinitionId, ItemId);
        TestEqual(TEXT("Physical item keeps explicit N1"), Instance.ItemLevel, 1);
        TestNull(TEXT("No redundant Weight field on physical instance"),
            FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("Weight")));

        TestEqual(TEXT("Canonical combat attack permission"),
            Resolved.Weapon.HasAttackSourceInHand(Spec.Slot), Spec.bAttack);
        if (Spec.bAttack)
        {
            TestTrue(TEXT("Pilot attack uses the canonical weapon kind"),
                Resolved.Weapon.Kind == EGridCanonicalArmamentKind::Weapon);
            TestTrue(TEXT("Pilot N1 attack costs action points"),
                Resolved.Weapon.ActionPointCost > 0);
            TestTrue(TEXT("Pilot attack range is authored"),
                Resolved.Weapon.RangeCells >= 1);
        }
        else
        {
            TestTrue(TEXT("Shield is a passive canonical ARM"),
                Resolved.Weapon.Kind == EGridCanonicalArmamentKind::Shield);
            TestFalse(TEXT("Passive shield does not attack from MainHand"),
                Resolved.Weapon.HasAttackSourceInHand(EGridEquipmentSlot::MainHand));
            TestFalse(TEXT("Passive shield is not equippable MainHand"),
                Resolved.Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand));
            TestEqual(TEXT("Round Shield N1 has five physical armor points"),
                Resolved.Weapon.PhysicalArmor, 5);
        }
    }
    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
