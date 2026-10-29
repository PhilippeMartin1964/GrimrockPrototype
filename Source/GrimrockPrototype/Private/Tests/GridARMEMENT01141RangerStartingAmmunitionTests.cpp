#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/Combat/GridRangedAmmunitionPolicy.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01141RangerStartingAmmunition,
    "Grimrock.RPG.ARMEMENT01.14.RangerStarterAmmunition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01141RangerStartingAmmunition::RunTest(const FString&)
{
    URPGClassAsset* Ranger = LoadObject<URPGClassAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Ranger.DA_Class_Ranger"));
    URPGRaceAsset* Human = LoadObject<URPGRaceAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Race_Human.DA_Race_Human"));
    if (!TestNotNull(TEXT("Ranger production class exists"), Ranger) ||
        !TestNotNull(TEXT("Human race exists"), Human)) return false;

    const FName ArrowId(TEXT("Item_Arrows_01"));
    const FName KitIds[] = {TEXT("Kit_Ranger_Marksman"), TEXT("Kit_Ranger_Scout")};
    const FName BowIds[] = {TEXT("Weapon_ShortBow_01"), TEXT("Weapon_RecurveBow_01")};

    for (int32 KitIndex = 0; KitIndex < UE_ARRAY_COUNT(KitIds); ++KitIndex)
    {
        UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
        Inventory->InitializeDefaultPartyIfNeeded();
        FRPGCharacterCreationRequest Request;
        Request.DisplayName = FText::FromString(TEXT("Ranger Ammo Integration"));
        Request.ClassDefinition = Ranger;
        Request.CombatActionSourceClassDefinition = Ranger;
        Request.RaceDefinition = Human;
        Request.StartingEquipmentKitId = KitIds[KitIndex];

        FText CreationError;
        if (!Inventory->CreateInitialCharacter(Request, CreationError))
        {
            AddError(FString::Printf(TEXT("Could not create %s: %s"),
                *KitIds[KitIndex].ToString(), *CreationError.ToString()));
            return false;
        }
        const FGridCharacterEquipmentState& Gear = Inventory->PartyInventoryState.ActiveEquipment[0];
        if (!TestTrue(TEXT("Starting bow equipped to MainHand"), Gear.MainHand.IsValid())) return false;
        TestEqual(TEXT("Exact approved bow is equipped"), Gear.MainHand.ItemDefinitionId, BowIds[KitIndex]);
        TestFalse(TEXT("Two-handed bow does not duplicate OffHand item"), Gear.OffHand.IsValid());

        UGridItemDefinitionAsset* Bow = Inventory->FindItemDefinition(BowIds[KitIndex]);
        if (!TestNotNull(TEXT("Starting bow definition registered"), Bow)) return false;
        TestEqual(TEXT("Bow explicitly requires real arrow item ID"),
            Bow->RequiredAmmunitionItemDefinitionId, ArrowId);
        TestEqual(TEXT("New ranger starts with twelve arrows"),
            Inventory->CountItemDefinitionInCharacterInventory(0, ArrowId), 12);

        const FGuid HeroId = Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId;
        const FGuid BowGuid = Gear.MainHand.RuntimeObjectId;
        for (int32 Remaining = 12; Remaining > 0; --Remaining)
        {
            TestTrue(TEXT("Ranger can use one arrow"), 
                FGridRangedAmmunitionPolicy::HasRequiredAmmunition(*Inventory, 0, *Bow));
            TestTrue(TEXT("Accepted ranged attack consumes exactly one arrow"),
                FGridRangedAmmunitionPolicy::ConsumeRequiredAmmunition(*Inventory, 0, *Bow));
            TestEqual(TEXT("Arrow count decreases by one"),
                Inventory->CountItemDefinitionInCharacterInventory(0, ArrowId), Remaining - 1);
        }
        TestFalse(TEXT("Empty quiver blocks ranged attack"),
            FGridRangedAmmunitionPolicy::HasRequiredAmmunition(*Inventory, 0, *Bow));
        TestFalse(TEXT("No thirteenth arrow can be consumed"),
            FGridRangedAmmunitionPolicy::ConsumeRequiredAmmunition(*Inventory, 0, *Bow));
        TestEqual(TEXT("Arrow quantity stays at zero"),
            Inventory->CountItemDefinitionInCharacterInventory(0, ArrowId), 0);
        TestEqual(TEXT("Bow remains equipped after consuming all ammunition"),
            Inventory->PartyInventoryState.ActiveEquipment[0].MainHand.RuntimeObjectId, BowGuid);
        TestEqual(TEXT("Character identity remains unchanged"),
            Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId, HeroId);
        FString OwnershipError;
        TestTrue(TEXT("Inventory ownership still valid after ammo consumption"),
            Inventory->ValidateInventoryOwnership(OwnershipError));
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
