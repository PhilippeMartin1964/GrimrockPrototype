#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GridEditorCanonicalStarterTestWorld.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "Runtime/Combat/GridRangedAmmunitionPolicy.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENTLOT025MarksmanStarter,
    "Grimrock.RPG.ARMEMENT.LOT02.MarksmanStarterAmmoCreationAndRestore",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENTLOT025MarksmanStarter::RunTest(const FString&)
{
    FGridEditorCanonicalStarterTestWorld Fixture;
    if (!TestTrue(TEXT("World-bound canonical ItemSystem"), Fixture.Ready()))
        return false;
    const FName ArrowId(TEXT("Item_Arrows_01"));
    const FName BowId(TEXT("Weapon_ShortBow_01"));
    FGridResolvedItem Bow, Dagger, Arrows;
    if (!TestTrue(TEXT("Canonical ranged starter resolves completely"),
        Fixture.Registry->ResolveItem(BowId, 1, Bow) &&
        Fixture.Registry->ResolveItem(TEXT("Weapon_Dagger_01"), 1, Dagger) &&
        Fixture.Registry->ResolveItem(ArrowId, 0, Arrows)))
        return false;
    TestTrue(TEXT("Canonical arrows are a stackable N0 item"), Arrows.MaxStack >= 12);
    TestTrue(TEXT("Canonical short bow consumes real arrow identity"),
        Bow.Weapon.RequiredAmmoKindId == ArrowId);
    const FGridRangedAmmunitionRequirement Required =
        FGridRangedAmmunitionPolicy::ResolveForAttack(Fixture.Registry, BowId, 1, nullptr, true);
    TestTrue(TEXT("Valid canonical ammo preflight"),
        Required.bValid && Required.bRegisteredSource &&
        Required.bRegisteredAmmunition && Required.AmmunitionItemId == ArrowId);

    URPGClassAsset* Ranger = LoadObject<URPGClassAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Ranger.DA_Class_Ranger"));
    URPGRaceAsset* Human = LoadObject<URPGRaceAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Race_Human.DA_Race_Human"));
    if (!TestNotNull(TEXT("Production Ranger"), Ranger) ||
        !TestNotNull(TEXT("Production Human"), Human)) return false;
    const int32 ProductionCount = Ranger->StartingEquipmentKits.Num();
    const FName Default = Ranger->DefaultStartingEquipmentKitId;
    UGridPartyInventoryComponent* Inventory = Fixture.MakeInventory();
    if (!TestNotNull(TEXT("Canonical world-bound inventory"), Inventory)) return false;

    FRPGCharacterCreationRequest Request;
    Request.DisplayName = FText::FromString(TEXT("Marksman test"));
    Request.RaceDefinition = Human;
    Request.ClassDefinition = Ranger;
    Request.CombatActionSourceClassDefinition = Ranger;
    Request.StartingEquipmentKitId = TEXT("Kit_Ranger_Marksman");
    FText Error;
    if (!TestTrue(TEXT("Real NewGame grants real canonical ranger kit"),
        Inventory->CreateInitialCharacter(Request, Error)))
    {
        AddError(Error.ToString());
        return false;
    }

    const FGridPartyInventoryState& State = Inventory->PartyInventoryState;
    const FGridCharacterInventoryState& Character = State.ActiveCharacters[0];
    const FGridCharacterEquipmentState& Gear = State.ActiveEquipment[0];
    TestEqual(TEXT("Approved Marksman orientation"), Character.StartingEquipmentKitId, Request.StartingEquipmentKitId);
    TestEqual(TEXT("N1 two-handed short bow equipped"), Gear.MainHand.ItemDefinitionId, BowId);
    TestFalse(TEXT("OffHand is not a physical duplicate"), Gear.OffHand.IsValid());
    TestEqual(TEXT("Exactly 12 physical arrows"), Inventory->CountItemDefinitionInCharacterInventory(0, ArrowId), 12);
    TestTrue(TEXT("Shot requires real canonical arrows"),
        FGridRangedAmmunitionPolicy::HasRequiredAmmunitionForAttack(*Inventory, 0, Required));
    TestTrue(TEXT("One arrow consumed"),
        FGridRangedAmmunitionPolicy::ConsumeRequiredAmmunitionForAttack(*Inventory, 0, Required));
    TestEqual(TEXT("Eleven arrows remain"), Inventory->CountItemDefinitionInCharacterInventory(0, ArrowId), 11);
    FString OwnershipError;
    TestTrue(TEXT("Inventory ownership invariants"), Inventory->ValidateInventoryOwnership(OwnershipError));

    UGrimrockPartySaveGame* Snapshot = NewObject<UGrimrockPartySaveGame>();
    Snapshot->PartyInventoryState = Inventory->PartyInventoryState;
    UGridPartyInventoryComponent* Restored = Fixture.MakeInventory();
    if (!TestNotNull(TEXT("World-bound restore destination"), Restored)) return false;
    if (!TestTrue(TEXT("SaveGame state restores without old definition pointers"),
        Restored->RestorePartyInventoryState(Snapshot->PartyInventoryState, Error)))
    {
        AddError(Error.ToString());
        return false;
    }
    TestEqual(TEXT("Saved arrow quantity"), Restored->CountItemDefinitionInCharacterInventory(0, ArrowId), 11);
    TestEqual(TEXT("Saved kit identity"),
        Restored->PartyInventoryState.ActiveCharacters[0].StartingEquipmentKitId, Request.StartingEquipmentKitId);
    TestEqual(TEXT("Saved bow GUID"),
        Restored->PartyInventoryState.ActiveEquipment[0].MainHand.RuntimeObjectId, Gear.MainHand.RuntimeObjectId);
    TestTrue(TEXT("Restored bow still uses canonical ammunition policy"),
        FGridRangedAmmunitionPolicy::HasRequiredAmmunitionForAttack(*Restored, 0, Required));
    TestEqual(TEXT("Production class not modified"), Ranger->StartingEquipmentKits.Num(), ProductionCount);
    TestEqual(TEXT("Class default not modified"), Ranger->DefaultStartingEquipmentKitId, Default);
    return !HasAnyErrors();
}

#endif
