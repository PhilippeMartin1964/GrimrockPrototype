#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Save/GrimrockPartySaveGame.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"

namespace ArmClean062AD
{
    // Isolated, realistic bag state. Split independent persisted dimensions
    // into separately named Automation tests to report the exact violation,
    // never publishing a player's actual SaveGame data or private logs.
    struct FMemoryFixture
    {
        FGridCanonicalRegistryFixture Registry;
        TStrongObjectPtr<UGridPartyInventoryComponent> Inventory;
        TStrongObjectPtr<UGrimrockPartySaveGame> Before;
        TStrongObjectPtr<UGrimrockPartySaveGame> After;
        FString Failure;
        TArray<uint8> Bytes;

        bool Create()
        {
            if (!Registry.bReady)
            {
                Failure = TEXT("registry");
                return false;
            }
            TArray<FGridWeaponPipelineRow> Rows;
            if (!FGridWeaponPipelineManifest::Load(
                FGridWeaponPipelineManifest::DefaultFile(), Rows, Failure) ||
                Rows.Num() != 22)
            {
                Failure = TEXT("manifest_shape");
                return false;
            }
            Before.Reset(NewObject<UGrimrockPartySaveGame>());
            if (!Before.IsValid())
            {
                Failure = TEXT("save_alloc");
                return false;
            }
            // Create the character through the same validated CC2 path as
            // MON12.8.1.SaveMemoryRoundTrip. Raw AddDefaulted_GetRef()
            // fails the v27 attribute-point/class identity save gate.
            FRPGAuthoringIdentityResolver::ResetRuntimeCache();
            Inventory.Reset(NewObject<UGridPartyInventoryComponent>());
            if (!Inventory.IsValid())
            {
                Failure = TEXT("inventory_alloc");
                return false;
            }
            Inventory->InitializeDefaultPartyIfNeeded();
            URPGRaceAsset* Race = NewObject<URPGRaceAsset>(Inventory.Get());
            URPGClassAsset* Class = NewObject<URPGClassAsset>(Inventory.Get());
            if (!Race || !Class)
            {
                Failure = TEXT("rpg_assets_alloc");
                return false;
            }
            Race->RaceId = TEXT("ARM06_MemoryHuman");
            Race->DisplayName = FText::FromString(TEXT("ARM06 Memory Human"));
            Race->AttributeBonuses = FRPGAttributes{0,0,0,0,0,0};
            Class->ClassId = TEXT("ARM06_MemoryWarrior");
            Class->DisplayName = FText::FromString(TEXT("ARM06 Memory Warrior"));
            Class->BaseAttributes = FRPGAttributes{12,11,12,9,9,9};
            Class->HealthAtLevelOne = 18;
            Class->HealthPerLevel = 8;
            Class->ManaAtLevelOne = 5;
            Class->ManaPerLevel = 2;
            FRPGCharacterCreationRequest Request;
            Request.DisplayName = FText::FromString(TEXT("ARM06 Memory Hero"));
            Request.RaceDefinition = Race;
            Request.ClassDefinition = Class;
            FText CreateError;
            if (!Inventory->CreateInitialCharacter(Request, CreateError) ||
                !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(0))
            {
                Failure = TEXT("character_create");
                return false;
            }
            Before->PartyInventoryState = Inventory->PartyInventoryState;
            FGridCharacterInventoryState& Hero =
                Before->PartyInventoryState.ActiveCharacters[0];
            Hero.InventorySlots.SetNum(44);
            for (FGridInventorySlot& Slot : Hero.InventorySlots)
                Slot = FGridInventorySlot();
            int32 Index = 0;
            for (const FGridWeaponPipelineRow& Row : Rows)
            {
                FGridResolvedItem N1, N20;
                if (!Registry.Resolve(FName(Row.ItemDefinitionId), 1, N1) ||
                    !Registry.Resolve(FName(Row.ItemDefinitionId), 20, N20))
                {
                    Failure = TEXT("physical_arms");
                    return false;
                }
                for (int32 Level : {1,20})
                {
                    FGridInventorySlot& Slot = Hero.InventorySlots[Index++];
                    Slot.bOccupied = true;
                    FGridItemInstance& Item = Slot.Item;
                    Item.RuntimeObjectId = FGuid(0xAD06, Index, Level, 0x2026);
                    Item.ItemDefinitionId = N1.ItemId;
                    Item.ItemLevel = Level;
                    Item.Quantity = 1;
                    Item.AbsorbedEquipmentPhysicalArmor = Row.IsPassive() ? 3 : 0;
                    Item.AbsorbedEquipmentMagicalArmor = Row.IsFocus() ? 1 : 0;
                    Item.OwnerType = EGridItemOwnerType::CharacterInventory;
                    Item.OwnerGuid = Hero.CharacterId;
                    Item.OwnerCharacterIndex = 0;
                    Item.EquipmentSlot = EGridEquipmentSlot::None;
                    Item.bLightsEnabled = false;
                    Item.ReadableContentId = NAME_None;
                    Item.ReadTitleOverride = FText();
                    Item.ReadTextOverride = FText();
                    Item.LastWorldTransform = FTransform::Identity;
                }
            }
            FText ValidationError;
            if (!Before->ValidateCurrentState(ValidationError))
            {
                Failure = TEXT("validation_preflight");
                return false;
            }
            if (!UGameplayStatics::SaveGameToMemory(Before.Get(), Bytes) || Bytes.IsEmpty())
            {
                Failure = TEXT("encode");
                return false;
            }
            After.Reset(Cast<UGrimrockPartySaveGame>(
                UGameplayStatics::LoadGameFromMemory(Bytes)));
            if (!After.IsValid())
            {
                Failure = TEXT("decode");
                return false;
            }
            if (After->PartyInventoryState.ActiveCharacters.Num() != 1 ||
                After->PartyInventoryState.ActiveCharacters[0].InventorySlots.Num() != 44)
            {
                Failure = TEXT("memory_shape");
                return false;
            }
            return true;
        }

        const FGridItemInstance& Original(int32 I) const
        {
            return Before->PartyInventoryState.ActiveCharacters[0].InventorySlots[I].Item;
        }
        const FGridItemInstance& Restored(int32 I) const
        {
            return After->PartyInventoryState.ActiveCharacters[0].InventorySlots[I].Item;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062ADSaveInstanceParity,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionARMInstanceSaveRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062ADSaveInstanceParity::RunTest(const FString&)
{
    ArmClean062AD::FMemoryFixture Test;
    if (!TestTrue(TEXT("UE native 44-object memory snapshot: structure"),
        Test.Create()))
    {
        AddError(Test.Failure);
        return false;
    }
    for (int32 I = 0; I < 44; ++I)
    {
        const FGridItemInstance& A = Test.Original(I);
        const FGridItemInstance& B = Test.Restored(I);
        TestTrue(TEXT("Durable physical identity, level and stack"),
            A.RuntimeObjectId == B.RuntimeObjectId &&
            A.ItemDefinitionId == B.ItemDefinitionId &&
            A.ItemLevel == B.ItemLevel && A.Quantity == B.Quantity);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062ADArmorDebtRoundTrip,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.SaveArmorDebtDimension",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062ADArmorDebtRoundTrip::RunTest(const FString&)
{
    ArmClean062AD::FMemoryFixture Test;
    if (!TestTrue(TEXT("Native 44-object memory snapshot"), Test.Create()))
        return false;
    for (int32 I = 0; I < 44; ++I)
        TestTrue(TEXT("Physical and magical spent armor persists independently"),
            Test.Original(I).AbsorbedEquipmentPhysicalArmor ==
                Test.Restored(I).AbsorbedEquipmentPhysicalArmor &&
            Test.Original(I).AbsorbedEquipmentMagicalArmor ==
                Test.Restored(I).AbsorbedEquipmentMagicalArmor);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062ADOwnershipRoundTrip,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.SaveOwnershipDimension",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062ADOwnershipRoundTrip::RunTest(const FString&)
{
    ArmClean062AD::FMemoryFixture Test;
    if (!TestTrue(TEXT("Native 44-object memory snapshot"), Test.Create()))
        return false;
    for (int32 I = 0; I < 44; ++I)
    {
        const FGridItemInstance& A = Test.Original(I);
        const FGridItemInstance& B = Test.Restored(I);
        TestTrue(TEXT("Owner, index and equipment slot persist"),
            A.OwnerType == B.OwnerType && A.OwnerGuid == B.OwnerGuid &&
            A.OwnerCharacterIndex == B.OwnerCharacterIndex &&
            A.EquipmentSlot == B.EquipmentSlot);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062ADOptionalDimensionRoundTrip,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.SaveOptionalPresentationDimension",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062ADOptionalDimensionRoundTrip::RunTest(const FString&)
{
    ArmClean062AD::FMemoryFixture Test;
    if (!TestTrue(TEXT("Native 44-object memory snapshot"), Test.Create()))
        return false;
    for (int32 I = 0; I < 44; ++I)
    {
        const FGridItemInstance& A = Test.Original(I);
        const FGridItemInstance& B = Test.Restored(I);
        TestTrue(TEXT("Readable, light and world pose persist"),
            A.bLightsEnabled == B.bLightsEnabled &&
            A.ReadableContentAsset == B.ReadableContentAsset &&
            A.ReadableContentId == B.ReadableContentId &&
            A.ReadTitleOverride.EqualTo(B.ReadTitleOverride) &&
            A.ReadTextOverride.EqualTo(B.ReadTextOverride) &&
            A.LastWorldTransform.Equals(B.LastWorldTransform));
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062ADRehydrationRoundTrip,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.SaveCanonicalRehydrationDimension",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062ADRehydrationRoundTrip::RunTest(const FString&)
{
    ArmClean062AD::FMemoryFixture Test;
    if (!TestTrue(TEXT("Native 44-object memory snapshot"), Test.Create()))
        return false;
    for (int32 I = 0; I < 44; ++I)
    {
        FGridResolvedItem Item;
        TestTrue(TEXT("Every restored item resolves from one canonical registry"),
            Test.Registry.Resolve(
                Test.Restored(I).ItemDefinitionId, Test.Restored(I).ItemLevel, Item));
    }
    TestNull(TEXT("No static DisplayName on mutable item"),
        FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("DisplayName")));
    TestNull(TEXT("No static Weight cache on mutable item"),
        FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("Weight")));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062ADRejectLossySave,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.RejectLossySaveInstanceMigration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062ADRejectLossySave::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Registry;
    if (!TestTrue(TEXT("Canonical ItemSystem"), Registry.bReady)) return false;
    FGridResolvedItem Valid, Invalid;
    TestTrue(TEXT("N20 physical arm exists"),
        Registry.Resolve(TEXT("Weapon_StraightSword_01"), 20, Valid));
    TestFalse(TEXT("N0 never promoted to N1"),
        Registry.Registry->ResolveItem(Valid.ItemId, 0, Invalid));
    TestFalse(TEXT("N21 never truncated to N20"),
        Registry.Registry->ResolveItem(Valid.ItemId, 21, Invalid));
    TestFalse(TEXT("Unknown ItemId cannot guess archetype"),
        Registry.Registry->ResolveItem(TEXT("Weapon_WrongIdentity"), 20, Invalid));
    FGridCanonicalArmorReserveView Reserve;
    TestFalse(TEXT("Negative physical debt rejected"),
        FGridCanonicalArmorReserveResolver::TryCompute(Valid.Weapon, -1, 0, Reserve));
    TestFalse(TEXT("Negative magical debt rejected"),
        FGridCanonicalArmorReserveResolver::TryCompute(Valid.Weapon, 0, -1, Reserve));
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
