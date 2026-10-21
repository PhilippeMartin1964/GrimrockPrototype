#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/Combat/GridWeaponVisualCatalogueAsset.h"
#include "Runtime/Combat/GridWorldItemVisualResolver.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"

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
    UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
    for (const ARMEMENT0166Tests::FSpec& Spec : ARMEMENT0166Tests::Specs)
    {
        UGridItemDefinitionAsset* Item = ARMEMENT0166Tests::LoadItem(Spec);
        if (!TestNotNull(TEXT("Production item exists"), Item)) { continue; }
        TestTrue(TEXT("Item registration"), Inventory->RegisterItemDefinition(Item));
        FGridItemInstance Instance;
        Instance.RuntimeObjectId = FGuid::NewGuid();
        Instance.ItemDefinitionId = Item->ItemDefinitionId;
        Instance.Quantity = 1;
        TestTrue(TEXT("Create fixed-level instance"), Inventory->ApplyItemDefinitionToInstance(Instance));
        TestEqual(TEXT("Minted level = 1"), Instance.ItemLevel, 1);
        const UGridWeaponCatalogueAsset* Catalogue = Item->WeaponCatalogue.LoadSynchronous();
        if (!TestNotNull(TEXT("RPG catalogue loaded"), Catalogue)) { continue; }
        const FGridWeaponLevelProfile* Profile = Catalogue->FindProfile(Item->WeaponArchetypeId, 1);
        if (!TestNotNull(TEXT("Level one profile exists"), Profile)) { continue; }
        TestTrue(TEXT("Instance weight is catalogue weight"), FMath::IsNearlyEqual(Instance.Weight, Profile->WeightKg));
        TestEqual(TEXT("Combat action availability"), Item->CanProvideAttackFromSlot(Spec.Slot), Spec.bAttack);
        if (Spec.bAttack)
        {
            const FGridCombatActionDefinition& Action = Item->CombatActions[0];
            TestTrue(TEXT("Authored attack is valid"), Action.IsValid());
            TestEqual(TEXT("Cost is catalogue AP cost"), Action.ActionPointCost, Profile->ActionPointCost);
            TestEqual(TEXT("Attack range from catalogue"), Action.RangeCells, Profile->RangeCells);
            FGridOffensiveEquipmentProfile Offensive;
            TestTrue(TEXT("Attack resolves using the real item level"),
                Item->ResolveEquippedOffensiveProfile(Instance, Action, Offensive));
            TestTrue(TEXT("Resolved offensive dice are valid"), Offensive.AttackDefinition.DamageDice.IsValid());
            TestEqual(TEXT("Attack range stays catalogue-derived"), Offensive.RangeCells, Profile->RangeCells);
        }
        else
        {
            TestFalse(TEXT("Shield has no main-hand attack"), Item->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand));
            TestFalse(TEXT("Shield cannot be equipped in main hand"), Item->CanEquipToSlot(EGridEquipmentSlot::MainHand));
            TestFalse(TEXT("Shield cannot be physically thrown"), Item->IsPhysicallyThrowable());
            TestTrue(TEXT("Shield remains a passive item"), Item->CombatActions.IsEmpty());
            TestEqual(TEXT("Level-1 armor comes from catalogue"), Item->EquipmentStatBonus.ArmorBonus, Profile->PhysicalArmor);
        }
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
