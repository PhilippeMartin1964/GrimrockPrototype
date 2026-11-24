#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Runtime/Combat/GridCombatResolver.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace ARMEMENT011031
{
    struct FWorldFixture
    {
        UWorld* World = nullptr;
        UGameInstance* GI = nullptr;
        UGridItemSubsystem* Registry = nullptr;

        FWorldFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARMH40_Armor_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (!World || !GEngine) return;
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
            GI = NewObject<UGameInstance>(GetTransientPackage());
            GI->Init();
            World->SetGameInstance(GI);
            Registry = GI->GetSubsystem<UGridItemSubsystem>();
        }

        ~FWorldFixture()
        {
            if (World) World->SetGameInstance(nullptr);
            if (GI) GI->Shutdown();
            if (World)
            {
                World->DestroyWorld(false);
                if (GEngine) GEngine->DestroyWorldContext(World);
            }
        }

        bool ResolveShield(FGridResolvedItem& Out) const
        {
            return Registry && Registry->ResolveItem(TEXT("Weapon_RoundShield_01"), 1, Out) &&
                Out.bIsArmament && Out.Weapon.PhysicalArmor == 5;
        }

        UGridPartyInventoryComponent* NewInventory(int32 BaseArmor) const
        {
            AGrimrockPartyPawn* Pawn = World ? World->SpawnActor<AGrimrockPartyPawn>() : nullptr;
            UGridPartyInventoryComponent* Inventory = Pawn ? Pawn->PartyInventoryComponent.Get() : nullptr;
            if (!Inventory) return nullptr;
            Inventory->InitializeDefaultPartyIfNeeded();
            if (!Inventory->IsValidCharacterIndex(0)) return nullptr;
            Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentPhysicalArmor = BaseArmor;
            Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth = 20;
            return Inventory;
        }
    };

    FGridItemInstance MakeInstance(UGridPartyInventoryComponent* Inventory, FName ShieldId)
    {
        FGridItemInstance Item;
        Item.RuntimeObjectId = FGuid::NewGuid();
        Item.ItemDefinitionId = ShieldId;
        Item.ItemLevel = 1;
        Item.Quantity = 1;
        if (!Inventory->ApplyItemDefinitionToInstance(Item))
        {
            return FGridItemInstance();
        }
        return Item;
    }

    int32 FindBagSlot(const UGridPartyInventoryComponent* Inventory, const FGuid& Id)
    {
        if (!Inventory || Inventory->PartyInventoryState.ActiveCharacters.IsEmpty()) return INDEX_NONE;
        const TArray<FGridInventorySlot>& Slots = Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots;
        for (int32 Index = 0; Index < Slots.Num(); ++Index)
        {
            if (!Slots[Index].IsEmpty() && Slots[Index].Item.RuntimeObjectId == Id) return Index;
        }
        return INDEX_NONE;
    }

    bool EquipShield(UGridPartyInventoryComponent* Inventory, FName ShieldId, FGuid& OutRuntimeId)
    {
        if (!Inventory || ShieldId.IsNone()) return false;
        FGridItemInstance Instance = MakeInstance(Inventory, ShieldId);
        if (!Instance.IsValid() || !Inventory->AddItemToCharacterInventory(0, Instance)) return false;
        OutRuntimeId = Instance.RuntimeObjectId;
        const int32 Index = FindBagSlot(Inventory, OutRuntimeId);
        return Index != INDEX_NONE && Inventory->EquipItemFromInventorySlot(0, Index, EGridEquipmentSlot::OffHand);
    }

    FGridAttackResult HitWithPhysicalDamage(UGridPartyInventoryComponent* Inventory, int32 RawDamage)
    {
        FGridAttackTargetStats Target;
        Target.CurrentHealth = Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth;
        Target.PhysicalArmor = Inventory->GetEffectiveCurrentPhysicalArmor(0);
        const FGridAttackResult Result = FGridCombatResolver::ResolveDirectDamage(
            Target, EGridDamageType::Physical, RawDamage, EGridPhysicalDamageSubtype::Bludgeoning);
        if (Result.bHit)
        {
            Inventory->ApplyPhysicalArmorDamage(0, Result.PhysicalArmorDamage);
            Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth =
                FMath::Max(0, Target.CurrentHealth - Result.HealthDamage);
        }
        return Result;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT011031RealShieldDamage,
    "Grimrock.RPG.ARMEMENT01.10.3.1.RealShieldAbsorbsBeforeHealth",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT011031RealShieldDamage::RunTest(const FString&)
{
    ARMEMENT011031::FWorldFixture Fixture;
    FGridResolvedItem Shield;
    if (!TestNotNull(TEXT("Canonical test world"), Fixture.World) ||
        !TestTrue(TEXT("Real ARM075 N1 resolves from ItemSystem"), Fixture.ResolveShield(Shield)))
        return false;
    TestEqual(TEXT("Canonical level-one round shield armor is five"), Shield.Weapon.PhysicalArmor, 5);

    UGridPartyInventoryComponent* Inventory = Fixture.NewInventory(0);
    if (!TestNotNull(TEXT("World inventory for canonical shield"), Inventory)) return false;
    FGuid ShieldId;
    if (!TestTrue(TEXT("Real shield equipped in OffHand"),
        ARMEMENT011031::EquipShield(Inventory, Shield.ItemId, ShieldId))) return false;
    TestEqual(TEXT("Effective reserve starts with five"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 5);

    FGridInventoryCharacterSummary Summary;
    TestTrue(TEXT("Summary loads before the first attack"), Inventory->GetCharacterSummary(0, Summary));
    TestEqual(TEXT("UI projection agrees with combat before hit"), Summary.Resources.CurrentPhysicalArmor, 5);

    const FGridAttackResult First = ARMEMENT011031::HitWithPhysicalDamage(Inventory, 4);
    TestTrue(TEXT("First physical attack resolves"), First.bHit);
    TestEqual(TEXT("First bite absorbs four physical armor"), First.PhysicalArmorDamage, 4);
    TestEqual(TEXT("First bite inflicts no health damage"), First.HealthDamage, 0);
    TestEqual(TEXT("Armor remaining after first bite"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 1);
    TestEqual(TEXT("Health remains intact"), Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth, 20);

    const FGridAttackResult Second = ARMEMENT011031::HitWithPhysicalDamage(Inventory, 5);
    TestTrue(TEXT("Second attack resolves"), Second.bHit);
    TestEqual(TEXT("Second bite absorbs last armor point"), Second.PhysicalArmorDamage, 1);
    TestEqual(TEXT("Second bite damages health by four"), Second.HealthDamage, 4);
    TestEqual(TEXT("Shield reserve is exhausted"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 0);
    TestEqual(TEXT("Health is now sixteen"), Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth, 16);
    TestTrue(TEXT("Summary loads after both attacks"), Inventory->GetCharacterSummary(0, Summary));
    TestEqual(TEXT("UI and combat both see depleted armor"), Summary.Resources.CurrentPhysicalArmor, 0);
    TestEqual(TEXT("Stored unmodified base reserve"), Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentPhysicalArmor, 0);
    TestEqual(TEXT("Shield absorbed five"), Inventory->PartyInventoryState.ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT011031Persistence,
    "Grimrock.RPG.ARMEMENT01.10.3.1.EquipUnequipAndStateCopy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT011031Persistence::RunTest(const FString&)
{
    ARMEMENT011031::FWorldFixture Fixture;
    FGridResolvedItem Shield;
    if (!TestTrue(TEXT("Real ARM075 N1 resolves from ItemSystem"), Fixture.ResolveShield(Shield)))
        return false;
    UGridPartyInventoryComponent* Inventory = Fixture.NewInventory(0);
    if (!TestNotNull(TEXT("Canonical inventory"), Inventory)) return false;
    FGuid ShieldId;
    if (!TestTrue(TEXT("Equip round shield"), ARMEMENT011031::EquipShield(Inventory, Shield.ItemId, ShieldId))) return false;
    const FGridAttackResult Hit = ARMEMENT011031::HitWithPhysicalDamage(Inventory, 3);
    TestEqual(TEXT("Three armor absorbed"), Hit.PhysicalArmorDamage, 3);
    TestEqual(TEXT("Two armor remain"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 2);

    TestTrue(TEXT("Unequip depleted shield"), Inventory->UnequipItemToInventory(0, EGridEquipmentSlot::OffHand));
    TestEqual(TEXT("No armor without the shield equipped"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 0);
    const int32 BagIndex = ARMEMENT011031::FindBagSlot(Inventory, ShieldId);
    if (!TestTrue(TEXT("Original runtime item in bag"), BagIndex != INDEX_NONE)) return false;
    TestEqual(TEXT("Depletion follows item into bag"), Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[BagIndex].Item.AbsorbedEquipmentPhysicalArmor, 3);
    TestTrue(TEXT("Re-equip same item without free refill"),
        Inventory->EquipItemFromInventorySlot(0, BagIndex, EGridEquipmentSlot::OffHand));
    TestEqual(TEXT("Only two armor after re-equipping"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 2);
    TestEqual(TEXT("Equipped instance preserved GUID"), Inventory->PartyInventoryState.ActiveEquipment[0].OffHand.RuntimeObjectId, ShieldId);

    const FGridPartyInventoryState SerializedStateModel = Inventory->PartyInventoryState;
    UGridPartyInventoryComponent* Loaded = Fixture.NewInventory(0);
    if (!TestNotNull(TEXT("Fresh inventory for state restore"), Loaded)) return false;
    Loaded->PartyInventoryState = SerializedStateModel;
    TestEqual(TEXT("State copy retains worn shield armor"), Loaded->GetEffectiveCurrentPhysicalArmor(0), 2);
    TestEqual(TEXT("State copy retains damage on same runtime item"),
        Loaded->PartyInventoryState.ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, 3);
    // Deliberately not a disk SaveGame/PIE test: actual save/load is a separate acceptance step.
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT011031Restore,
    "Grimrock.RPG.ARMEMENT01.10.3.1.BaseReserveAndEquipmentRestoration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT011031Restore::RunTest(const FString&)
{
    ARMEMENT011031::FWorldFixture Fixture;
    FGridResolvedItem Shield;
    if (!TestTrue(TEXT("Real ARM075 N1 resolves from ItemSystem"), Fixture.ResolveShield(Shield)))
        return false;
    UGridPartyInventoryComponent* Inventory = Fixture.NewInventory(2);
    if (!TestNotNull(TEXT("Canonical inventory"), Inventory)) return false;
    FGuid ShieldId;
    if (!TestTrue(TEXT("Equip shield"), ARMEMENT011031::EquipShield(Inventory, Shield.ItemId, ShieldId))) return false;
    TestEqual(TEXT("Base armor two plus shield five"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 7);

    const FGridAttackResult Hit = ARMEMENT011031::HitWithPhysicalDamage(Inventory, 6);
    TestEqual(TEXT("Damage consumed six armor"), Hit.PhysicalArmorDamage, 6);
    TestEqual(TEXT("No health damage"), Hit.HealthDamage, 0);
    TestEqual(TEXT("Remaining reserve is one"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 1);
    TestEqual(TEXT("Shield spent all five"), Inventory->PartyInventoryState.ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, 5);
    TestEqual(TEXT("Base reserve spent one"), Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentPhysicalArmor, 1);

    TestEqual(TEXT("Restore four"), Inventory->RestorePhysicalArmor(0, 4), 4);
    TestEqual(TEXT("Four points return to worn shield"), Inventory->PartyInventoryState.ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, 1);
    TestEqual(TEXT("Base reserve unchanged by shield restoration"),
        Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentPhysicalArmor, 1);
    TestEqual(TEXT("Effective physical armor is now five"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 5);

    TestTrue(TEXT("Unequip shield again"), Inventory->UnequipItemToInventory(0, EGridEquipmentSlot::OffHand));
    TestEqual(TEXT("Only base reserve remains after unequipping"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 1);
    const int32 Slot = ARMEMENT011031::FindBagSlot(Inventory, ShieldId);
    TestTrue(TEXT("Previously depleted shield remains identifiable"), Slot != INDEX_NONE);
    if (Slot != INDEX_NONE)
    {
        TestTrue(TEXT("Re-equips without unearned reserve"), Inventory->EquipItemFromInventorySlot(0, Slot, EGridEquipmentSlot::OffHand));
        TestEqual(TEXT("Restored shield contributes four, base contributes one"),
            Inventory->GetEffectiveCurrentPhysicalArmor(0), 5);
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
