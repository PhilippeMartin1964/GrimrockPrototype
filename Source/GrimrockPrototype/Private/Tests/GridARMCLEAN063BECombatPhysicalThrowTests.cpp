#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/Combat/GridCombatPhysicalThrowSourceReadModel.h"
#include "UObject/StrongObjectPtr.h"

namespace ARMCLEAN063BE
{
    FGridItemInstance MakeItem(FName Id, int32 Level)
    {
        FGridItemInstance Item;
        Item.RuntimeObjectId = FGuid::NewGuid();
        Item.ItemDefinitionId = Id;
        Item.ItemLevel = Level;
        Item.Quantity = 1;
        return Item;
    }

    bool Prepare(FAutomationTestBase& Test,
        TStrongObjectPtr<UGridItemSystemAsset>& Root,
        TStrongObjectPtr<UGameInstance>& GI,
        TStrongObjectPtr<UGridItemSubsystem>& Registry)
    {
        Root.Reset(LoadObject<UGridItemSystemAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
        if (!Test.TestNotNull(TEXT("Production DA_ItemSystem"), Root.Get()))
            return false;
        GI.Reset(NewObject<UGameInstance>());
        Registry.Reset(NewObject<UGridItemSubsystem>(GI.Get()));
        TArray<FString> Errors;
        if (!Test.TestTrue(TEXT("Production registry is valid"),
            Registry->RegisterItemSystem(Root.Get(), Errors)))
        {
            for (const FString& Error : Errors) Test.AddError(Error);
            return false;
        }
        return true;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridBECombatThrowCanonicalActions,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalCombatPhysicalThrow.AuthoredActions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridBECombatThrowCanonicalActions::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root;
    TStrongObjectPtr<UGameInstance> GI;
    TStrongObjectPtr<UGridItemSubsystem> Registry;
    if (!ARMCLEAN063BE::Prepare(*this, Root, GI, Registry)) return false;
    const FName Ids[] = {
        TEXT("Weapon_ThrowingKnife_01"),
        TEXT("Weapon_Javelin_01"), TEXT("Weapon_Dart_01") };
    for (FName Id : Ids)
    {
        for (int32 Level : {1, 20})
        {
            const FGridItemInstance Item = ARMCLEAN063BE::MakeItem(Id, Level);
            FGridResolvedItem Resolved;
            if (!TestTrue(TEXT("Real ARM source N1/N20"),
                Registry->ResolveItem(Id, Level, Resolved))) return false;
            if (!TestTrue(TEXT("Actual throwable has bound action"),
                Resolved.bIsArmament && Resolved.Weapon.Projectile.bEnabled &&
                Resolved.AvailableActions.Num() > 0)) return false;

            const FName ActionId = Resolved.AvailableActions[0].ActionId;
            FGridResolvedBoundAction Bound;
            if (!TestTrue(TEXT("One real attack action resolves"),
                Registry->ResolveBoundAction(Id, Level, ActionId, Bound))) return false;
            if (!TestTrue(TEXT("The actual authored throw consumes one physical item"),
                Bound.Action.Kind == EGridActionKind::Attack &&
                Bound.Action.SourceItemQuantityCost == 1)) return false;

            const auto NoLegacy = FGridCombatPhysicalThrowSourceReadModel::Resolve(
                Registry.Get(), nullptr, Item,
                Bound.Action.SourceItemQuantityCost, Bound.Action.bRangedAttack);
            TestTrue(TEXT("N1/N20 canonical combat throw launches without old definition"),
                NoLegacy.bRegisteredIdentity && NoLegacy.bResolved &&
                NoLegacy.bLaunchPhysicalProjectile && !NoLegacy.bRejectAction);

            UGridItemDefinitionAsset* Old = NewObject<UGridItemDefinitionAsset>(GI.Get());
            Old->ItemDefinitionId = Id;
            Old->bCombatThrowWeapon = false;
            Old->ThrowSpeed = 0.f;
            const auto Contradictory = FGridCombatPhysicalThrowSourceReadModel::Resolve(
                Registry.Get(), Old, Item, 1, Bound.Action.bRangedAttack);
            TestTrue(TEXT("Old bCombatThrowWeapon=false cannot stop a canonical combat throw"),
                Contradictory.bLaunchPhysicalProjectile && !Contradictory.bRejectAction);

            Old->bCombatThrowWeapon = true;
            const auto NoCost = FGridCombatPhysicalThrowSourceReadModel::Resolve(
                Registry.Get(), Old, Item, 0, true);
            TestTrue(TEXT("No authored source cost cannot permit a free ranged projectile"),
                NoCost.bRegisteredIdentity && NoCost.bRejectAction &&
                !NoCost.bLaunchPhysicalProjectile);
            const auto BadCost = FGridCombatPhysicalThrowSourceReadModel::Resolve(
                Registry.Get(), Old, Item, 2, true);
            TestTrue(TEXT("No double debit or unrepresented physical quantity"),
                BadCost.bRejectAction && !BadCost.bLaunchPhysicalProjectile);
        }
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridBECombatThrowInvalidKnown,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalCombatPhysicalThrow.InvalidKnown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridBECombatThrowInvalidKnown::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root;
    TStrongObjectPtr<UGameInstance> GI;
    TStrongObjectPtr<UGridItemSubsystem> Registry;
    if (!ARMCLEAN063BE::Prepare(*this, Root, GI, Registry)) return false;
    TStrongObjectPtr<UGridItemDefinitionAsset> Old(NewObject<UGridItemDefinitionAsset>());
    Old->ItemDefinitionId = TEXT("Weapon_ThrowingKnife_01");
    Old->bCombatThrowWeapon = true;
    for (int32 Level : {0, 21})
    {
        const auto Bad = FGridCombatPhysicalThrowSourceReadModel::Resolve(
            Registry.Get(), Old.Get(),
            ARMCLEAN063BE::MakeItem(Old->ItemDefinitionId, Level), 1, true);
        TestTrue(TEXT("Known invalid ARM fails closed without old throw"),
            Bad.bRegisteredIdentity && Bad.bRejectAction &&
            !Bad.bLaunchPhysicalProjectile);
    }
    Old->ItemDefinitionId = TEXT("Weapon_StraightSword_01");
    const auto Sword = FGridCombatPhysicalThrowSourceReadModel::Resolve(
        Registry.Get(), Old.Get(),
        ARMCLEAN063BE::MakeItem(Old->ItemDefinitionId, 1), 0, false);
    TestTrue(TEXT("Old flag cannot create projectile for canonical straight sword"),
        Sword.bRegisteredIdentity && Sword.bResolved &&
        !Sword.bLaunchPhysicalProjectile && !Sword.bRejectAction);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridBECombatThrowLegacy,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalCombatPhysicalThrow.LegacyCompatibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridBECombatThrowLegacy::RunTest(const FString&)
{
    TStrongObjectPtr<UGridItemSystemAsset> Root;
    TStrongObjectPtr<UGameInstance> GI;
    TStrongObjectPtr<UGridItemSubsystem> Registry;
    if (!ARMCLEAN063BE::Prepare(*this, Root, GI, Registry)) return false;
    TStrongObjectPtr<UGridItemDefinitionAsset> Old(NewObject<UGridItemDefinitionAsset>());
    Old->ItemDefinitionId = TEXT("BE_LegacyCombatThrow");
    Old->bCombatThrowWeapon = true;
    const FGridItemInstance Item = ARMCLEAN063BE::MakeItem(Old->ItemDefinitionId, 0);
    const auto UnknownConfigured = FGridCombatPhysicalThrowSourceReadModel::Resolve(
        Registry.Get(), Old.Get(), Item, 0, false);
    TestTrue(TEXT("Configured registry never borrows unknown bCombatThrowWeapon"),
        !UnknownConfigured.bRegisteredIdentity && UnknownConfigured.bRejectAction &&
        !UnknownConfigured.bLaunchPhysicalProjectile);
    const auto Legacy = FGridCombatPhysicalThrowSourceReadModel::Resolve(
        nullptr, Old.Get(), Item, 0, false);
    TestTrue(TEXT("Only isolated Automation may characterize a historical combat throw"),
        !Legacy.bRegisteredIdentity && Legacy.bResolved &&
        Legacy.bLaunchPhysicalProjectile && !Legacy.bRejectAction);
    const auto Missing = FGridCombatPhysicalThrowSourceReadModel::Resolve(
        Registry.Get(), nullptr, Item, 0, true);
    TestFalse(TEXT("Missing old unregistered definition cannot launch"), Missing.bLaunchPhysicalProjectile);
    Old->ItemDefinitionId = TEXT("BE_DifferentIdentity");
    const auto Mismatched = FGridCombatPhysicalThrowSourceReadModel::Resolve(
        Registry.Get(), Old.Get(), Item, 0, false);
    TestFalse(TEXT("Mismatched legacy source never launches"), Mismatched.bLaunchPhysicalProjectile);
    return !HasAnyErrors();
}

#endif
