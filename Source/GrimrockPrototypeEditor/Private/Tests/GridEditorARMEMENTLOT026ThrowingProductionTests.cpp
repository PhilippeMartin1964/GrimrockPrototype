#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENTLOT026Throwing,
    "Grimrock.RPG.ARMEMENT.LOT02.ThrowingWeaponN1Batch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENTLOT026Throwing::RunTest(const FString&)
{
    FGridWeaponPipelineRow Kind;
    Kind.BuildKind = TEXT("ThrowingOneHand");
    TestTrue(TEXT("Throwing build kind supported"), Kind.IsSupportedBuildKind());
    TestTrue(TEXT("Throwing is a physical projectile"), Kind.IsThrowable());
    TestFalse(TEXT("Throwing is not passive"), Kind.IsPassive());
    Kind.BuildKind = TEXT("MeleeOneHand");
    TestFalse(TEXT("Melee cannot become thrown silently"), Kind.IsThrowable());
    Kind.BuildKind = TEXT("FocusPassive");
    TestFalse(TEXT("Unimplemented focus cannot enter standard pipeline"), Kind.IsSupportedBuildKind());

    UGridWeaponCatalogueAsset* Catalog = LoadObject<UGridWeaponCatalogueAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
    if (!TestNotNull(TEXT("N1 weapon catalogue"), Catalog)) return false;
    const bool bDirty = Catalog->GetOutermost()->IsDirty();
    for (const TCHAR* Id : { TEXT("ARM054"), TEXT("ARM057"), TEXT("ARM058") })
    {
        const FGridWeaponLevelProfile* N1 = Catalog->FindProfile(FName(Id), 1);
        if (!TestNotNull(*FString::Printf(TEXT("N1 %s"), Id), N1)) continue;
        TestTrue(TEXT("Canonical N1 valid"), N1->IsValidProfile());
        TestEqual(TEXT("Throwing profile held in one hand"), N1->HandUsage, FString(TEXT("1 main")));
        FGridAttackDefinition Attack;
        if (!TestTrue(TEXT("Canonical damage dice"), N1->TryBuildAttackDefinition(Attack))) continue;

        UGridItemDefinitionAsset* Item = NewObject<UGridItemDefinitionAsset>();
        Item->ItemDefinitionId = FName(*FString::Printf(TEXT("TestThrow_%s"), Id));
        Item->ItemType = EGridItemType::Weapon;
        Item->HandUsage = EGridItemHandUsage::OneHanded;
        Item->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::MainHand);
        Item->bUseWeaponCatalogue = true;
        Item->WeaponArchetypeId = FName(Id);
        Item->WeaponCatalogue = Catalog;
        Item->DefaultWeaponItemLevel = 1;
        Item->Weight = N1->WeightKg;
        Item->bCombatThrowWeapon = true;
        FGridCombatActionDefinition Action;
        Action.ActionId = FName(*FString::Printf(TEXT("Attack_Throw_%s"), Id));
        Action.ActionType = EGridCombatActionType::RangedAttack;
        Action.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
        Action.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
        Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
        Action.ActionPointCost = N1->ActionPointCost;
        Action.RangeCells = N1->RangeCells;
        Action.bRequiresLineOfSight = true;
        Action.ResourceCosts.SourceItemQuantityCost = 1;
        Action.OffensiveProfile.AttackId = Action.ActionId;
        Action.OffensiveProfile.AttackDefinition = Attack;
        Action.OffensiveProfile.DamageScalingAttribute = N1->ScalingAttribute;
        Action.OffensiveProfile.RangeCells = N1->RangeCells;
        Item->CombatActions.Add(Action);
        TestTrue(TEXT("Equipped throw definition valid"), Item->IsValidDefinition() &&
            Item->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand));
        TestTrue(TEXT("Throwable object"), Item->IsCombatThrowable() && Item->IsPhysicallyThrowable());
        FGridCombatActionDefinition Quick;
        if (TestTrue(TEXT("Throw from inventory has action"), Item->BuildInventoryCombatActionDefinition(Quick)))
        {
            TestEqual(TEXT("Inventory action consumes one"), Quick.ResourceCosts.SourceItemQuantityCost, 1);
            TestEqual(TEXT("Inventory action source policy"), Quick.SourcePolicy,
                EGridCombatActionSourcePolicy::QuickItem);
        }
        AddInfo(FString::Printf(TEXT("[LOT02.6] %s N1 Range=%d AP=%d"), Id, N1->RangeCells, N1->ActionPointCost));
    }
    TestEqual(TEXT("Read-only catalogue"), Catalog->GetOutermost()->IsDirty(), bDirty);
    return true;
}
#endif
