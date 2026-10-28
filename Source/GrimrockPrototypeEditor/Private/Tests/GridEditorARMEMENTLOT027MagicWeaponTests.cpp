#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENTLOT027MagicAndSpear,
    "Grimrock.RPG.ARMEMENT.LOT02.MagicAndSpearN1Contracts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENTLOT027MagicAndSpear::RunTest(const FString&)
{
    struct FExpected
    {
        const TCHAR* Archetype;
        const TCHAR* Kind;
        bool bTwoHanded;
        bool bRangedAction;
    };
    const FExpected Cases[] =
    {
        { TEXT("ARM038"), TEXT("MeleeTwoHand"), true, false },
        { TEXT("ARM061"), TEXT("MagicOneHand"), false, true },
        { TEXT("ARM062"), TEXT("MagicOneHand"), false, true },
        { TEXT("ARM067"), TEXT("MagicTwoHand"), true, true },
        { TEXT("ARM069"), TEXT("MagicOneHand"), false, true }
    };
    UGridWeaponCatalogueAsset* Catalogue = LoadObject<UGridWeaponCatalogueAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
    if (!TestNotNull(TEXT("Canonical weapon catalogue"), Catalogue)) return false;
    const bool bDirty = Catalogue->GetOutermost()->IsDirty();

    for (const FExpected& Case : Cases)
    {
        FGridWeaponPipelineRow Route;
        Route.BuildKind = Case.Kind;
        Route.EquipmentSlots = TEXT("MainHand");
        Route.AttackId = TEXT("Test_N1_Attack");
        const FString Label(Case.Archetype);
        TestTrue(*FString::Printf(TEXT("%s supported mode"), *Label), Route.IsSupportedBuildKind());
        TestEqual(*FString::Printf(TEXT("%s handedness"), *Label), Route.IsTwoHanded(), Case.bTwoHanded);
        TestEqual(*FString::Printf(TEXT("%s action routing"), *Label), Route.UsesRangedAction(), Case.bRangedAction);
        TestFalse(*FString::Printf(TEXT("%s never consumed"), *Label), Route.IsThrowable());
        TestFalse(*FString::Printf(TEXT("%s not passive"), *Label), Route.IsPassive());

        const FGridWeaponLevelProfile* N1 = Catalogue->FindProfile(FName(Case.Archetype), 1);
        if (!TestNotNull(*FString::Printf(TEXT("%s canonical N1"), *Label), N1)) continue;
        TestTrue(*FString::Printf(TEXT("%s valid N1"), *Label), N1->IsValidProfile());
        TestEqual(*FString::Printf(TEXT("%s source hand usage"), *Label), N1->HandUsage,
            Case.bTwoHanded ? FString(TEXT("2 mains")) : FString(TEXT("1 main")));
        FGridAttackDefinition Damage;
        if (!TestTrue(*FString::Printf(TEXT("%s original damage dice"), *Label),
            N1->TryBuildAttackDefinition(Damage))) continue;

        UGridItemDefinitionAsset* Item = NewObject<UGridItemDefinitionAsset>();
        Item->ItemDefinitionId = FName(*FString::Printf(TEXT("TestWeapon_%s"), *Label));
        Item->ItemType = EGridItemType::Weapon;
        Item->HandUsage = Case.bTwoHanded
            ? EGridItemHandUsage::TwoHanded : EGridItemHandUsage::OneHanded;
        Item->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::MainHand);
        Item->WeaponArchetypeId = FName(Case.Archetype);
        Item->WeaponCatalogue = Catalogue;
        Item->bUseWeaponCatalogue = true;
        Item->DefaultWeaponItemLevel = 1;
        Item->Weight = N1->WeightKg;
        FGridCombatActionDefinition Action;
        Action.ActionId = FName(*FString::Printf(TEXT("TestAttack_%s"), *Label));
        Action.ActionType = Case.bRangedAction
            ? EGridCombatActionType::RangedAttack : EGridCombatActionType::MeleeAttack;
        Action.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
        Action.TargetingPolicy = EGridCombatTargetingPolicy::FirstAxialTarget;
        Action.ResolutionProfile = EGridCombatActionResolutionProfile::Attack;
        Action.ActionPointCost = N1->ActionPointCost;
        Action.RangeCells = N1->RangeCells;
        Action.bRequiresLineOfSight = Case.bRangedAction;
        Action.ResourceCosts.SourceItemQuantityCost = 0;
        Action.OffensiveProfile.AttackId = Action.ActionId;
        Action.OffensiveProfile.AttackDefinition = Damage;
        Action.OffensiveProfile.DamageScalingAttribute = N1->ScalingAttribute;
        Action.OffensiveProfile.RangeCells = N1->RangeCells;
        Item->CombatActions.Add(Action);
        TestTrue(*FString::Printf(TEXT("%s equipped attack is valid"), *Label),
            Action.IsValid() && Item->IsValidDefinition() &&
            Item->CanProvideAttackFromSlot(EGridEquipmentSlot::MainHand));
        TestEqual(*FString::Printf(TEXT("%s source damage type unchanged"), *Label),
            Action.OffensiveProfile.AttackDefinition.DamageType, N1->DamageType);
        TestTrue(*FString::Printf(TEXT("%s no spell or consumable granted"), *Label),
            Action.SourcePolicy == EGridCombatActionSourcePolicy::Equipment &&
            Item->CombatActions.Num() == 1 && !Item->bProvidesQuickItemCombatAction);
    }
    FGridWeaponPipelineRow Unsupported;
    Unsupported.BuildKind = TEXT("FocusPassive");
    TestTrue(TEXT("Offhand focus is now a passive production kind"), Unsupported.IsSupportedBuildKind());
    TestTrue(TEXT("Focus has no attack action"), Unsupported.IsPassive() && Unsupported.IsFocus() &&
        !Unsupported.UsesRangedAction());
    TestEqual(TEXT("Catalogue remains unchanged"), Catalogue->GetOutermost()->IsDirty(), bDirty);
    return true;
}
#endif
