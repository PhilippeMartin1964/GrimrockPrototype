#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "Runtime/Combat/GridRangedAmmunitionPolicy.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENTLOT025MarksmanStarter,
    "Grimrock.RPG.ARMEMENT.LOT02.MarksmanStarterAmmoCreationAndRestore",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENTLOT025MarksmanStarter::RunTest(const FString&)
{
    const FName ArrowId(TEXT("Item_Arrows_01"));
    UGridItemDefinitionAsset* Bow = LoadObject<UGridItemDefinitionAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_ShortBow_01.DA_Weapon_ShortBow_01"));
    UGridItemDefinitionAsset* Dagger = LoadObject<UGridItemDefinitionAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Dagger_01.DA_Weapon_Dagger_01"));
    UGridItemDefinitionAsset* Arrows =
        FRPGStartingEquipmentPresetCatalog::ResolveKnownStartingItemDefinition(ArrowId);
    if (!TestNotNull(TEXT("ARM045 has a definition"), Bow) ||
        !TestNotNull(TEXT("ARM001 has a definition"), Dagger) ||
        !TestNotNull(TEXT("Real arrow package resolves for SaveGame"), Arrows))
    {
        return false;
    }
    TestTrue(TEXT("Valid stackable arrow definition"),
        Arrows->IsValidDefinition() && Arrows->bStackable && Arrows->MaxStackSize >= 12);
    TestEqual(TEXT("Bow explicitly requires the authored arrow ID"),
        Bow->RequiredAmmunitionItemDefinitionId, ArrowId);
    if (!TestTrue(TEXT("Arrow requirement passes definition validation"),
        Bow->IsValidDefinition())) return false;

    URPGClassAsset* Ranger = LoadObject<URPGClassAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Ranger.DA_Class_Ranger"));
    URPGRaceAsset* Human = LoadObject<URPGRaceAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Race_Human.DA_Race_Human"));
    if (!TestNotNull(TEXT("Production Ranger"), Ranger) ||
        !TestNotNull(TEXT("Production Human"), Human)) return false;
    const int32 ProductionCount = Ranger->StartingEquipmentKits.Num();
    URPGClassAsset* Temporary = DuplicateObject<URPGClassAsset>(Ranger, GetTransientPackage());
    if (!TestNotNull(TEXT("Production class cloned"), Temporary)) return false;

    FRPGStartingEquipmentKit& Kit = Temporary->StartingEquipmentKits.AddDefaulted_GetRef();
    Kit.KitId = TEXT("Kit_Ranger_Marksman");
    Kit.DisplayName = FText::FromString(TEXT("Tireur (test)"));
    auto Add = [&Kit](UGridItemDefinitionAsset* Item, EGridEquipmentSlot Slot, int32 Level, int32 Count)
    {
        FRPGStartingEquipmentEntry& Entry = Kit.Items.AddDefaulted_GetRef();
        Entry.ItemDefinition = Item;
        Entry.TargetSlot = Slot;
        Entry.ItemLevel = Level;
        Entry.Quantity = Count;
    };
    Add(Bow, EGridEquipmentSlot::MainHand, 1, 1);
    Add(Dagger, EGridEquipmentSlot::None, 1, 1);
    Add(Arrows, EGridEquipmentSlot::None, 0, 12);
    Temporary->DefaultStartingEquipmentKitId = Kit.KitId;
    if (!TestTrue(TEXT("Transient Marksman kit passes class validation"),
        Temporary->IsValidDefinition())) return false;

    UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
    Inventory->InitializeDefaultPartyIfNeeded();
    FRPGCharacterCreationRequest Request;
    Request.DisplayName = FText::FromString(TEXT("Marksman test"));
    Request.RaceDefinition = Human;
    Request.ClassDefinition = Temporary;
    Request.CombatActionSourceClassDefinition = Temporary;
    Request.StartingEquipmentKitId = Kit.KitId;
    FText Error;
    if (!TestTrue(TEXT("Real CreateInitialCharacter grants the ranged kit"),
        Inventory->CreateInitialCharacter(Request, Error)))
    {
        AddError(Error.ToString());
        return false;
    }
    const FGridPartyInventoryState& State = Inventory->PartyInventoryState;
    const FGridCharacterInventoryState& Character = State.ActiveCharacters[0];
    const FGridCharacterEquipmentState& Gear = State.ActiveEquipment[0];
    TestEqual(TEXT("Approved Marksman kit identity"), Character.StartingEquipmentKitId, Kit.KitId);
    TestEqual(TEXT("Equipped two-handed bow"), Gear.MainHand.ItemDefinitionId, Bow->ItemDefinitionId);
    TestFalse(TEXT("Offhand physical slot remains empty"), Gear.OffHand.IsValid());
    TestEqual(TEXT("Exactly twelve arrows"), Inventory->CountItemDefinitionInCharacterInventory(0, ArrowId), 12);
    TestTrue(TEXT("Ready to fire with ammo"), FGridRangedAmmunitionPolicy::HasRequiredAmmunition(*Inventory, 0, *Bow));
    TestTrue(TEXT("One ammo consumed"), FGridRangedAmmunitionPolicy::ConsumeRequiredAmmunition(*Inventory, 0, *Bow));
    TestEqual(TEXT("Eleven arrows remain"), Inventory->CountItemDefinitionInCharacterInventory(0, ArrowId), 11);
    TestTrue(TEXT("Bow remains registered"), Inventory->FindItemDefinition(Bow->ItemDefinitionId) == Bow);
    TestTrue(TEXT("Arrows registered"), Inventory->FindItemDefinition(ArrowId) == Arrows);
    FString OwnershipError;
    TestTrue(TEXT("Ownership invariants"), Inventory->ValidateInventoryOwnership(OwnershipError));

    UGrimrockPartySaveGame* Snapshot = NewObject<UGrimrockPartySaveGame>();
    Snapshot->PartyInventoryState = Inventory->PartyInventoryState;
    UGridPartyInventoryComponent* Restored = NewObject<UGridPartyInventoryComponent>();
    Restored->InitializeDefaultPartyIfNeeded();
    if (!TestTrue(TEXT("SaveGame snapshot restores"), Restored->RestorePartyInventoryState(Snapshot->PartyInventoryState, Error)))
    {
        AddError(Error.ToString());
        return false;
    }
    FName MissingId;
    TestTrue(TEXT("Saved starting items rehydrate without dungeon placement"),
        Restored->RehydrateOwnedItemDefinitions(
            [](FName Id) { return FRPGStartingEquipmentPresetCatalog::ResolveKnownStartingItemDefinition(Id); },
            MissingId));
    TestTrue(TEXT("No missing item IDs"), MissingId.IsNone());
    TestEqual(TEXT("Saved stack quantity"), Restored->CountItemDefinitionInCharacterInventory(0, ArrowId), 11);
    TestEqual(TEXT("Saved kit ID"), Restored->PartyInventoryState.ActiveCharacters[0].StartingEquipmentKitId, Kit.KitId);
    TestEqual(TEXT("Saved bow GUID"), Restored->PartyInventoryState.ActiveEquipment[0].MainHand.RuntimeObjectId,
        Gear.MainHand.RuntimeObjectId);
    TestTrue(TEXT("Restored ammunition policy uses stack"), FGridRangedAmmunitionPolicy::HasRequiredAmmunition(*Restored, 0, *Bow));
    TestEqual(TEXT("No mutation to production class"), Ranger->StartingEquipmentKits.Num(), ProductionCount);
    return true;
}
#endif
