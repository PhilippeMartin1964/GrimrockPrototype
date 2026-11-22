#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Save/GrimrockPartySaveGame.h"
#include "Kismet/GameplayStatics.h"

namespace ArmClean062AD
{
    bool SameDurableItem(const FGridItemInstance& A, const FGridItemInstance& B)
    {
        return A.RuntimeObjectId == B.RuntimeObjectId &&
            A.ItemDefinitionId == B.ItemDefinitionId &&
            A.ItemLevel == B.ItemLevel && A.Quantity == B.Quantity &&
            A.AbsorbedEquipmentPhysicalArmor == B.AbsorbedEquipmentPhysicalArmor &&
            A.AbsorbedEquipmentMagicalArmor == B.AbsorbedEquipmentMagicalArmor &&
            A.OwnerType == B.OwnerType && A.OwnerGuid == B.OwnerGuid &&
            A.OwnerCharacterIndex == B.OwnerCharacterIndex &&
            A.EquipmentSlot == B.EquipmentSlot &&
            A.bLightsEnabled == B.bLightsEnabled &&
            A.ReadableContentAsset == B.ReadableContentAsset &&
            A.ReadableContentId == B.ReadableContentId &&
            A.ReadTitleOverride.EqualTo(B.ReadTitleOverride) &&
            A.ReadTextOverride.EqualTo(B.ReadTextOverride) &&
            A.LastWorldTransform.Equals(B.LastWorldTransform);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean062ADSaveInstanceParity,
    "Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionARMInstanceSaveRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean062ADSaveInstanceParity::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Registry;
    if (!TestTrue(TEXT("Real canonical registry"), Registry.bReady)) return false;
    TArray<FGridWeaponPipelineRow> Rows;
    FString Error;
    if (!TestTrue(TEXT("All ARM identity rows read"),
        FGridWeaponPipelineManifest::Load(
            FGridWeaponPipelineManifest::DefaultFile(), Rows, Error))) return false;
    if (!TestEqual(TEXT("Exactly twenty-two physical ARM"), Rows.Num(), 22)) return false;

    UGrimrockPartySaveGame* Source = NewObject<UGrimrockPartySaveGame>();
    FGridCharacterInventoryState& Hero =
        Source->PartyInventoryState.ActiveCharacters.AddDefaulted_GetRef();
    Hero.CharacterId = FGuid(0x2026, 0xAD06, 0x1, 0x1);
    Hero.DisplayName = FText::FromString(TEXT("ARM-CLEAN06 Memory Only"));
    Hero.InventorySlots.SetNum(44);
    int32 OriginalIndex = 0, Passive = 0, WorldOwners = 0;
    for (int32 Index = 0; Index < Rows.Num(); ++Index)
    {
        const FGridWeaponPipelineRow& Row = Rows[Index];
        FGridResolvedItem N1, N20;
        if (!TestTrue(TEXT("N1/N20 resolve without historical source"),
            Registry.Resolve(FName(Row.ItemDefinitionId), 1, N1) &&
            Registry.Resolve(FName(Row.ItemDefinitionId), 20, N20)))
            return false;
        TestEqual(TEXT("One actual ARM archetype"), N1.ArchetypeId, FName(Row.ArchetypeId));
        if (Row.IsPassive()) ++Passive;
        for (int32 Level : {1, 20})
        {
            FGridInventorySlot& Slot = Hero.InventorySlots[OriginalIndex++];
            Slot.bOccupied = true;
            FGridItemInstance& Item = Slot.Item;
            Item.RuntimeObjectId = FGuid(0xAD06, Index + 1, Level, 0x2026);
            Item.ItemDefinitionId = N1.ItemId;
            Item.ItemLevel = Level;
            Item.Quantity = 1;
            Item.AbsorbedEquipmentPhysicalArmor = Row.IsPassive() ? 3 : 0;
            Item.AbsorbedEquipmentMagicalArmor = Row.IsFocus() ? 1 : 0;
            Item.OwnerType = (Index % 4 == 0) ? EGridItemOwnerType::EquipmentSlot :
                (Index % 4 == 1) ? EGridItemOwnerType::CharacterInventory :
                (Index % 4 == 2) ? EGridItemOwnerType::Cursor : EGridItemOwnerType::World;
            Item.OwnerGuid = FGuid(0x2026, Index, Level, 0xAD);
            Item.OwnerCharacterIndex = Index % 3;
            Item.EquipmentSlot = Row.IsPassive()
                ? EGridEquipmentSlot::OffHand : EGridEquipmentSlot::MainHand;
            Item.bLightsEnabled = Index % 2 == 0;
            Item.ReadableContentId = Index == 0 ? FName(TEXT("TestReadingReference")) : NAME_None;
            Item.ReadTitleOverride = Index == 0 ? FText::FromString(TEXT("Private title")) : FText();
            Item.ReadTextOverride = Index == 0 ? FText::FromString(TEXT("Private readable text")) : FText();
            Item.LastWorldTransform = FTransform(
                FRotator(0.0f, Index * 5.0f, 0.0f),
                FVector(Index * 10.0f, 100.0f + Level, 3.0f),
                FVector(1.0f));
            if (Item.OwnerType == EGridItemOwnerType::World) ++WorldOwners;
        }
    }
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("Actual UE SaveGame serializes 44 instances in RAM"),
        UGameplayStatics::SaveGameToMemory(Source, Bytes) && !Bytes.IsEmpty()))
        return false;
    UGrimrockPartySaveGame* Rehydrated =
        Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Real UE SaveGame deserializes from memory"), Rehydrated))
        return false;
    if (!TestEqual(TEXT("Recovered one test hero"),
        Rehydrated->PartyInventoryState.ActiveCharacters.Num(), 1)) return false;
    const TArray<FGridInventorySlot>& Loaded =
        Rehydrated->PartyInventoryState.ActiveCharacters[0].InventorySlots;
    if (!TestEqual(TEXT("44 slot records survived"), Loaded.Num(), 44)) return false;
    for (int32 Index = 0; Index < 44; ++Index)
    {
        const FGridItemInstance& Original = Hero.InventorySlots[Index].Item;
        const FGridItemInstance& Restored = Loaded[Index].Item;
        TestTrue(TEXT("Each mutable GUID, debt, owner, readable, transform roundtrips"),
            ArmClean062AD::SameDurableItem(Original, Restored));
        FGridResolvedItem Item;
        TestTrue(TEXT("Restored physical ItemId+N resolves from canonical registry"),
            Registry.Resolve(Restored.ItemDefinitionId, Restored.ItemLevel, Item));
    }
    TestEqual(TEXT("All three passive ARM sources included"), Passive, 3);
    TestTrue(TEXT("Persisted world-owned entries included"), WorldOwners > 0);
    TestNull(TEXT("No stored DisplayName on mutable instance"),
        FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("DisplayName")));
    TestNull(TEXT("No stored Weight cache on mutable instance"),
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
    TestTrue(TEXT("Original physical level N20 exists"),
        Registry.Resolve(TEXT("Weapon_StraightSword_01"), 20, Valid));
    TestFalse(TEXT("Legacy N0 never promoted to N1"),
        Registry.Registry->ResolveItem(Valid.ItemId, 0, Invalid));
    TestFalse(TEXT("N21 never truncated to N20"),
        Registry.Registry->ResolveItem(Valid.ItemId, 21, Invalid));
    TestFalse(TEXT("Another ItemId never guessed from an archetype"),
        Registry.Registry->ResolveItem(TEXT("Weapon_WrongIdentity"), 20, Invalid));
    FGridCanonicalArmorReserveView Reserve;
    TestFalse(TEXT("Invalid negative physical depletion is refused"),
        FGridCanonicalArmorReserveResolver::TryCompute(Valid.Weapon, -1, 0, Reserve));
    TestFalse(TEXT("Invalid negative magical depletion is refused"),
        FGridCanonicalArmorReserveResolver::TryCompute(Valid.Weapon, 0, -1, Reserve));
    TestNull(TEXT("Mutable item has no stale static display cache"),
        FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("DisplayName")));
    TestNull(TEXT("Mutable item has no stale static weight cache"),
        FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("Weight")));
    return !HasAnyErrors();
}
#endif // WITH_DEV_AUTOMATION_TESTS
