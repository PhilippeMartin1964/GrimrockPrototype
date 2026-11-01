#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Kismet/GameplayStatics.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGStartingEquipmentPresetCatalog.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GrimrockPartySaveGame.h"

namespace ARMEMENT01171
{
    UGridItemDefinitionAsset* OilFlask()
    {
        return LoadObject<UGridItemDefinitionAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Flask_Oil.DA_Item_Flask_Oil"));
    }

    URPGClassAsset* Alchemist()
    {
        return LoadObject<URPGClassAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Alchemist.DA_Class_Alchemist"));
    }

    URPGRaceAsset* Human()
    {
        return LoadObject<URPGRaceAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Race_Human.DA_Race_Human"));
    }

    const FGridItemInstance* FindInBag(const FGridCharacterInventoryState& Hero, FName ItemId)
    {
        for (const FGridInventorySlot& Slot : Hero.InventorySlots)
        {
            if (!Slot.IsEmpty() && Slot.Item.ItemDefinitionId == ItemId)
            {
                return &Slot.Item;
            }
        }
        return nullptr;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01171OilContract,
    "Grimrock.RPG.ARMEMENT01.17.1.RealOilFlaskNoTalentBypass",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01171OilContract::RunTest(const FString&)
{
    UGridItemDefinitionAsset* Oil = ARMEMENT01171::OilFlask();
    if (!TestNotNull(TEXT("Real RPG03 oil-flask DataAsset is materialized"), Oil))
        return false;

    TestEqual(TEXT("Canonical oil-flask inventory identity"), Oil->ItemDefinitionId,
        FName(TEXT("Item_Flask_Oil")));
    TestTrue(TEXT("Oil flask is a genuine approved item definition"), Oil->IsValidDefinition());
    TestTrue(TEXT("Oil is explicitly a QuickItem"), Oil->bProvidesQuickItemCombatAction);
    TestTrue(TEXT("Alchemy source tag exists"), Oil->ItemTags.Contains(FName(TEXT("QuickItem.Alchemy"))));
    TestTrue(TEXT("Flask source tag exists"), Oil->ItemTags.Contains(FName(TEXT("QuickItem.Flask"))));

    FGridCombatActionDefinition Action;
    if (!TestTrue(TEXT("Oil flask already builds a runnable QuickItem action"),
        Oil->BuildQuickItemCombatActionDefinition(Action))) return false;
    TestEqual(TEXT("Correct existing action identity"), Action.ActionId,
        FName(TEXT("Action_Alchemist_OilSlick")));
    TestEqual(TEXT("Uses established inventory QuickItem authority"), Action.SourcePolicy,
        EGridCombatActionSourcePolicy::QuickItem);
    TestEqual(TEXT("Targets ground area"), Action.TargetingPolicy,
        EGridCombatTargetingPolicy::Area);
    TestEqual(TEXT("Two action points, not a free spell"), Action.ActionPointCost, 2);
    TestEqual(TEXT("Throws up to four cells"), Action.RangeCells, 4);
    TestEqual(TEXT("One physical flask is spent on accepted execution"),
        Action.ResourceCosts.SourceItemQuantityCost, 1);
    TestTrue(TEXT("QuickItem source does not grant or demand a level-two talent"),
        Action.Requirements.IsEmpty());
    if (!TestEqual(TEXT("One canonical surface contribution"), Action.SurfaceEffects.Num(), 1))
        return false;
    TestEqual(TEXT("Exactly the existing Oil surface"), Action.SurfaceEffects[0].SurfaceType,
        EGridCombatSurfaceType::Oil);
    TestEqual(TEXT("Existing oil lasts four rounds"), Action.SurfaceEffects[0].DurationRounds, 4);
    TestEqual(TEXT("Oil terrain traversal penalty is preserved"),
        Action.SurfaceEffects[0].TraversalCostModifier, 1);
    TestTrue(TEXT("Exact ID is found by the authorized starter rehydration catalogue"),
        FRPGStartingEquipmentPresetCatalog::ResolveKnownStartingItemDefinition(Oil->ItemDefinitionId) == Oil);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01171StarterAndSave,
    "Grimrock.RPG.ARMEMENT01.17.1.TransmuterNewGameSaveContinue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01171StarterAndSave::RunTest(const FString&)
{
    URPGClassAsset* Original = ARMEMENT01171::Alchemist();
    URPGRaceAsset* Race = ARMEMENT01171::Human();
    UGridItemDefinitionAsset* Oil = ARMEMENT01171::OilFlask();
    if (!TestNotNull(TEXT("Production Alchemist class"), Original) ||
        !TestNotNull(TEXT("Production Human race"), Race) ||
        !TestNotNull(TEXT("Approved real Oil item"), Oil)) return false;

    const int32 OriginalKitCount = Original->StartingEquipmentKits.Num();
    const FName OriginalDefault = Original->DefaultStartingEquipmentKitId;
    URPGClassAsset* Preview = DuplicateObject<URPGClassAsset>(Original, GetTransientPackage());
    FString Error;
    if (!TestTrue(TEXT("Approved Alchemist kits remain materializable"),
        Preview && FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass(*Preview, Error)))
    {
        AddError(Error);
        return false;
    }
    TestEqual(TEXT("Production Alchemist class is unchanged"), Original->StartingEquipmentKits.Num(), OriginalKitCount);
    TestEqual(TEXT("Production default is unchanged"), Original->DefaultStartingEquipmentKitId, OriginalDefault);
    if (!TestEqual(TEXT("Three Alchemist orientations are retained"), Preview->StartingEquipmentKits.Num(), 3))
        return false;
    for (const FRPGStartingEquipmentKit& Kit : Preview->StartingEquipmentKits)
    {
        const bool bTransmuter = Kit.KitId == FName(TEXT("Kit_Alchemist_Transmuter"));
        int32 OilEntries = 0;
        for (const FRPGStartingEquipmentEntry& Entry : Kit.Items)
        {
            if (Entry.ItemDefinition && Entry.ItemDefinition->ItemDefinitionId == Oil->ItemDefinitionId)
            {
                ++OilEntries;
                TestTrue(TEXT("Same published Oil DataAsset is referenced"), Entry.ItemDefinition.Get() == Oil);
                TestEqual(TEXT("A single finished oil flask is granted"), Entry.Quantity, 1);
                TestEqual(TEXT("Starter item stays in bag"), Entry.TargetSlot, EGridEquipmentSlot::None);
                TestEqual(TEXT("Non-weapon oil flask has no item level"), Entry.ItemLevel, 0);
            }
        }
        TestEqual(TEXT("Only Transmuter gets the oil flask"), OilEntries, bTransmuter ? 1 : 0);
        if (bTransmuter)
        {
            TestEqual(TEXT("Transmuter retains dagger, orb and adds oil"), Kit.Items.Num(), 3);
            TestEqual(TEXT("Canonical dagger stays main hand"),
                Kit.Items[0].ItemDefinition->ItemDefinitionId, FName(TEXT("Weapon_Dagger_01")));
            TestEqual(TEXT("Orb is still passive off-hand equipment"),
                Kit.Items[1].ItemDefinition->ItemDefinitionId, FName(TEXT("Weapon_ArcaneOrb_01")));
        }
    }

    UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
    Inventory->InitializeDefaultPartyIfNeeded();
    FRPGCharacterCreationRequest Request;
    Request.DisplayName = FText::FromString(TEXT("Transmuter N1"));
    Request.ClassDefinition = Original;
    Request.CombatActionSourceClassDefinition = Original;
    Request.RaceDefinition = Race;
    Request.StartingEquipmentKitId = TEXT("Kit_Alchemist_Transmuter");
    FText CreationError;
    if (!TestTrue(TEXT("New Game creates playable Transmuter with actual starter"),
        Inventory->CreateInitialCharacter(Request, CreationError)))
    {
        AddError(CreationError.ToString());
        return false;
    }
    const FGridCharacterInventoryState& Hero = Inventory->PartyInventoryState.ActiveCharacters[0];
    const FGridItemInstance* Flask = ARMEMENT01171::FindInBag(Hero, Oil->ItemDefinitionId);
    if (!TestNotNull(TEXT("Oil flask is physically in Transmuter bag"), Flask))
        return false;
    const FGuid Guid = Flask->RuntimeObjectId;
    TestTrue(TEXT("Starter oil item has a unique GUID"), Guid.IsValid());
    TestEqual(TEXT("Starter has one oil flask"), Flask->Quantity, 1);
    TestEqual(TEXT("Oil flask owned by hero"), Flask->OwnerGuid, Hero.CharacterId);
    TestEqual(TEXT("Physical owner is bag"), Flask->OwnerType, EGridItemOwnerType::CharacterInventory);
    TestEqual(TEXT("Exact source definition registered"), Inventory->FindItemDefinition(Oil->ItemDefinitionId), Oil);
    TestEqual(TEXT("Dagger remains equipped"),
        Inventory->PartyInventoryState.ActiveEquipment[0].MainHand.ItemDefinitionId,
        FName(TEXT("Weapon_Dagger_01")));
    TestEqual(TEXT("Arcane orb remains in off hand"),
        Inventory->PartyInventoryState.ActiveEquipment[0].OffHand.ItemDefinitionId,
        FName(TEXT("Weapon_ArcaneOrb_01")));
    TestEqual(TEXT("N1 starts before the OilSlick talent is unlockable"), Hero.Level, 1);
    TestEqual(TEXT("Stable kit identifier"), Hero.StartingEquipmentKitId,
        FName(TEXT("Kit_Alchemist_Transmuter")));
    FString OwnershipError;
    TestTrue(TEXT("Starter ownership remains canonical"), Inventory->ValidateInventoryOwnership(OwnershipError));

    UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
    Save->PartyInventoryState = Inventory->PartyInventoryState;
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("Complete SaveGame binary serializes starter oil"), UGameplayStatics::SaveGameToMemory(Save, Bytes)))
        return false;
    const UGrimrockPartySaveGame* Loaded =
        Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Binary SaveGame reload succeeds"), Loaded)) return false;
    const FGridCharacterInventoryState& SavedHero = Loaded->PartyInventoryState.ActiveCharacters[0];
    const FGridItemInstance* SavedFlask = ARMEMENT01171::FindInBag(SavedHero, Oil->ItemDefinitionId);
    if (!TestNotNull(TEXT("Saved Transmuter retains physical flask"), SavedFlask)) return false;
    TestEqual(TEXT("Oil flask GUID persists"), SavedFlask->RuntimeObjectId, Guid);
    TestEqual(TEXT("Kit identity persists"), SavedHero.StartingEquipmentKitId,
        FName(TEXT("Kit_Alchemist_Transmuter")));
    UGridPartyInventoryComponent* Continued = NewObject<UGridPartyInventoryComponent>();
    Continued->InitializeDefaultPartyIfNeeded();
    FText RestoreError;
    if (!TestTrue(TEXT("Continue restores actual player inventory"), Continued->RestorePartyInventoryState(
        Loaded->PartyInventoryState, RestoreError)))
    {
        AddError(RestoreError.ToString());
        return false;
    }
    TestEqual(TEXT("Restored Transmuter carries one oil flask"),
        Continued->CountItemDefinitionInCharacterInventory(0, Oil->ItemDefinitionId), 1);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
