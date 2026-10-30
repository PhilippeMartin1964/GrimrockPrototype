#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGStartingDisarmToolAuthoring.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace ARMEMENT01161Test
{
    const TCHAR* ToolPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_DisarmTools_01.DA_Item_DisarmTools_01");
    const FName ToolId(TEXT("Item_DisarmTools_01"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01161DisarmToolAuthoring,
    "Grimrock.RPG.ARMEMENT01.16.DisarmTool.AuthoringContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01161DisarmToolAuthoring::RunTest(const FString&)
{
    UGridItemDefinitionAsset* Tool = LoadObject<UGridItemDefinitionAsset>(
        nullptr, ARMEMENT01161Test::ToolPath);
    if (!TestNotNull(TEXT("N1 disarm tool has a real package"), Tool))
    {
        return false;
    }
    TestTrue(TEXT("Production tool meets the approved physical contract"),
        FRPGStartingDisarmToolAuthoring::Matches(*Tool));
    TestEqual(TEXT("Stable source item identity"), Tool->ItemDefinitionId, ARMEMENT01161Test::ToolId);
    TestTrue(TEXT("Disarm tool is reusable, not consumed"),
        !Tool->bStackable && Tool->MaxStackSize == 1);
    TestTrue(TEXT("Explicit disarm tag is present"), Tool->ItemTags.Contains(TEXT("Tool.Disarm")));
    TestFalse(TEXT("No fabricated QuickItem executor"), Tool->bProvidesQuickItemCombatAction);
    FGridCombatActionDefinition Fake;
    TestFalse(TEXT("No misleading hotbar action can be built"),
        Tool->BuildInventoryCombatActionDefinition(Fake));
    TestTrue(TEXT("Weapon stats are not silently granted"), Tool->CombatActions.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01161SaboteurIsolation,
    "Grimrock.RPG.ARMEMENT01.16.DisarmTool.NoPrematureKitGrant",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01161SaboteurIsolation::RunTest(const FString&)
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
    UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
    Inventory->InitializeDefaultPartyIfNeeded();
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
    TestEqual(TEXT("Unimplemented disarm tool is not prematurely granted"),
        Inventory->CountItemDefinitionInCharacterInventory(0, ARMEMENT01161Test::ToolId), 0);
    TestFalse(TEXT("Not-yet-implemented tool cannot be resolved as a starter item"),
        Inventory->HasItemDefinitionInCharacterInventory(0, ARMEMENT01161Test::ToolId));

    FString OwnershipError;
    TestTrue(TEXT("No inventory ownership corruption"), Inventory->ValidateInventoryOwnership(OwnershipError));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
