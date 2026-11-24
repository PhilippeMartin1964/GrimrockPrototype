#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GridEditorCanonicalStarterTestWorld.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace ARMEMENT01161Test
{
    const FName ToolId(TEXT("Item_DisarmTools_01"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01161DisarmToolAuthoring,
    "Grimrock.RPG.ARMEMENT01.16.DisarmTool.AuthoringContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01161DisarmToolAuthoring::RunTest(const FString&)
{
    FGridEditorCanonicalStarterTestWorld Fixture;
    if (!TestTrue(TEXT("Production ItemSystem configured"), Fixture.Ready()))
        return false;
    FGridResolvedItem Tool;
    if (!TestTrue(TEXT("Disarm tools resolve at canonical N0"),
        Fixture.Registry->ResolveItem(ARMEMENT01161Test::ToolId, 0, Tool)))
        return false;
    TestTrue(TEXT("Canonical tool is a valid non-armament item"), Tool.IsValid() && !Tool.bIsArmament);
    TestEqual(TEXT("Stable disarm tool ID"), Tool.ItemId, ARMEMENT01161Test::ToolId);
    TestTrue(TEXT("Disarm gameplay tag preserved"), Tool.Tags.Contains(TEXT("Tool.Disarm")));
    TestEqual(TEXT("Not hand held"), Tool.NonArmHandPolicy, EGridNonArmamentHandPolicy::NotHandHeld);
    TestEqual(TEXT("Non-stackable physical item"), Tool.MaxStack, 1);
    TestTrue(TEXT("Tool is not a QuickItem action"), Tool.AvailableActions.IsEmpty());
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01161SaboteurApprovedGrant,
    "Grimrock.RPG.ARMEMENT01.16.DisarmTool.ApprovedKitGrant",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01161SaboteurApprovedGrant::RunTest(const FString&)
{
    URPGClassAsset* Rogue = LoadObject<URPGClassAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Rogue.DA_Class_Rogue"));
    URPGRaceAsset* Human = LoadObject<URPGRaceAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Race_Human.DA_Race_Human"));
    if (!TestNotNull(TEXT("Production Rogue class exists"), Rogue) ||
        !TestNotNull(TEXT("Production Human race exists"), Human))
    {
        return false;
    }
    FGridEditorCanonicalStarterTestWorld Fixture;
    if (!TestTrue(TEXT("World-bound ItemSystem configured"), Fixture.Ready()))
        return false;
    UGridPartyInventoryComponent* Inventory = Fixture.MakeInventory();
    if (!TestNotNull(TEXT("Real world-bound inventory"), Inventory)) return false;
    FRPGCharacterCreationRequest Request;
    Request.DisplayName = FText::FromString(TEXT("Saboteur Test"));
    Request.ClassDefinition = Rogue;
    Request.CombatActionSourceClassDefinition = Rogue;
    Request.RaceDefinition = Human;
    Request.StartingEquipmentKitId = TEXT("Kit_Rogue_Saboteur");
    FText Error;
    if (!TestTrue(TEXT("Approved Saboteur remains playable"),
        Inventory->CreateInitialCharacter(Request, Error)))
    {
        AddError(Error.ToString());
        return false;
    }

    const FGridCharacterInventoryState& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
    TestEqual(TEXT("Character retains its approved orientation"),
        Character.StartingEquipmentKitId, FName(TEXT("Kit_Rogue_Saboteur")));
    TestEqual(TEXT("Existing N1 Stiletto remains equipped"),
        Inventory->PartyInventoryState.ActiveEquipment[0].MainHand.ItemDefinitionId,
        FName(TEXT("Weapon_Stiletto_01")));
    // ARMEMENT01.16.3: the once-pending starter grant is now approved.
    // Keep the authoring contract strict while verifying real New Game delivery.
    TestEqual(TEXT("Approved Saboteur receives exactly one reusable tool"),
        Inventory->CountItemDefinitionInCharacterInventory(0, ARMEMENT01161Test::ToolId), 1);
    TestTrue(TEXT("Canonical tool can be resolved from Saboteur's starting bag"),
        Inventory->HasItemDefinitionInCharacterInventory(0, ARMEMENT01161Test::ToolId));
    FGridResolvedItem ProductionTool;
    if (!TestTrue(TEXT("Tool resolves from sole configured ItemSystem"),
        Fixture.Registry->ResolveItem(ARMEMENT01161Test::ToolId, 0, ProductionTool)))
        return false;
    const FGridCharacterEquipmentState& Equipment = Inventory->PartyInventoryState.ActiveEquipment[0];
    TestFalse(TEXT("Reusable tools are not equipped into the off hand"),
        Equipment.OffHand.ItemDefinitionId == ARMEMENT01161Test::ToolId);
    TestFalse(TEXT("Reusable tools are not equipped into the main hand"),
        Equipment.MainHand.ItemDefinitionId == ARMEMENT01161Test::ToolId);
    TestTrue(TEXT("Approved tool remains non-consuming and non-stackable"),
        ProductionTool.IsValid() && ProductionTool.MaxStack == 1);

    FString OwnershipError;
    TestTrue(TEXT("No inventory ownership corruption"), Inventory->ValidateInventoryOwnership(OwnershipError));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
