#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../ARMEMENT/GridEditorAlchemySaveCopy.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace
{
    namespace Copy = GridEditorAlchemySaveCopy;
    namespace Identity = GridEditorAlchemyActionIdentity;

    struct FIdentityGuard
    {
        FIdentityGuard() { FRPGAuthoringIdentityResolver::ResetRuntimeCache(); }
        ~FIdentityGuard() { FRPGAuthoringIdentityResolver::ResetRuntimeCache(); }
    };

    bool MakeFixture(UGridPartyInventoryComponent& Inventory, UGrimrockPartySaveGame& Save)
    {
        Inventory.InitializeDefaultPartyIfNeeded();
        auto* Race = NewObject<URPGRaceAsset>(&Inventory);
        Race->RaceId = TEXT("X64_Human");
        Race->DisplayName = FText::FromString(TEXT("X64 Human"));
        Race->AttributeBonuses = FRPGAttributes{0,0,0,0,0};
        auto* Class = NewObject<URPGClassAsset>(&Inventory);
        Class->ClassId = TEXT("X64_Warrior");
        Class->DisplayName = FText::FromString(TEXT("X64 Warrior"));
        Class->BaseAttributes = FRPGAttributes{12,11,12,9,9,9};
        Class->HealthAtLevelOne = 18;
        Class->HealthPerLevel = 8;
        Class->ManaAtLevelOne = 5;
        Class->ManaPerLevel = 2;
        FRPGCharacterCreationRequest Request;
        Request.DisplayName = FText::FromString(TEXT("X64 Hero"));
        Request.RaceDefinition = Race;
        Request.ClassDefinition = Class;
        FText Error;
        if (!Inventory.CreateInitialCharacter(Request, Error)) return false;

        Save.PartyInventoryState = Inventory.PartyInventoryState;
        auto& Party = Save.PartyInventoryState;
        const auto Template = Party.ActiveCharacters[0];
        Party.ActiveCharacters.SetNum(2);
        Party.ActiveEquipment.SetNum(2);
        Party.CharacterPool.SetNum(2);
        for (int32 I = 0; I < 4; ++I)
        {
            auto& C = I < 2 ? Party.ActiveCharacters[I] : Party.CharacterPool[I-2];
            C = Template;
            C.CharacterId = FGuid::NewGuid();
            C.InventorySlots.Reset();
            C.InventorySlots.SetNum(4);
            auto& Slot = C.InventorySlots[0];
            Slot.bOccupied = true;
            Slot.Item.RuntimeObjectId = FGuid::NewGuid();
            Slot.Item.ItemDefinitionId = Identity::Item(I);
            Slot.Item.ItemLevel = 0;
            Slot.Item.Quantity = 1;
            Slot.Item.OwnerGuid = C.CharacterId;
            Slot.Item.OwnerCharacterIndex = I < 2 ? I : INDEX_NONE;
            Slot.Item.OwnerType = EGridItemOwnerType::CharacterInventory;
            Slot.Item.DisplayName = FText::FromString(TEXT("Saved elixir name"));
            auto& Hotbar = C.CombatHotbarSlots[2];
            Hotbar.Reset(2);
            Hotbar.ActionId = Identity::SharedLegacyAction();
            Hotbar.SourceDefinitionId = Slot.Item.ItemDefinitionId;
            Hotbar.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
            Hotbar.PreferredSourceRuntimeId = Slot.Item.RuntimeObjectId;
        }
        Save.PartyPortraitOrder = {Party.ActiveCharacters[1].CharacterId, Party.ActiveCharacters[0].CharacterId};
        Save.CurrentDungeonLevelId = TEXT("X64_TestLevel");
        Save.PartyCellX = 12;
        Save.PartyCellY = 7;
        Save.PartyFacing = EGridEdge::East;
        Save.GameplayElapsedSeconds = 247.5;
        FGridLevelRuntimeState Level;
        Level.LevelId = Save.CurrentDungeonLevelId;
        Level.bHasBeenVisited = true;
        Level.IntVariables.Add(TEXT("RuneCount"), 3);
        FGridRuntimeItemState Dropped;
        Dropped.ObjectId = FGuid::NewGuid();
        Dropped.ItemDefinitionId = TEXT("Stone");
        Dropped.ItemLevel = 0;
        Dropped.Quantity = 2;
        Dropped.CellX = 13;
        Dropped.CellY = 7;
        Dropped.Transform.SetLocation(FVector(2600,1400,30));
        Level.Items.Add(Dropped.ObjectId, Dropped);
        FGridCombatTrapState Trap;
        Trap.TrapId = TEXT("X64_OtherTrap");
        Trap.RemainingRounds = 2;
        Trap.SourceActionId = TEXT("Action_Unchanged_Trap");
        Trap.RawDamage = 5;
        Level.CombatTraps.Add(FIntPoint(9,8), Trap);
        Save.DungeonRuntimeState.LevelStates.Add(Level.LevelId, Level);
        return true;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X64SaveCopy,
    "Grimrock.RPG.ARM.CLEAN06_4X64.SaveBinaryCopyRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN064X64SaveCopy::RunTest(const FString&)
{
    FIdentityGuard Guard;
    TStrongObjectPtr<UGridPartyInventoryComponent> Inventory(NewObject<UGridPartyInventoryComponent>());
    TStrongObjectPtr<UGrimrockPartySaveGame> Save(NewObject<UGrimrockPartySaveGame>());
    if (!TestTrue(TEXT("Valid character creation and v27 snapshot"), MakeFixture(*Inventory.Get(), *Save.Get())))
        return false;
    TArray<uint8> Input;
    if (!TestTrue(TEXT("Real UE SaveGame byte serialization"), UGameplayStatics::SaveGameToMemory(Save.Get(), Input)) ||
        !TestFalse(TEXT("Nonempty archive"), Input.IsEmpty())) return false;
    const TArray<uint8> Original = Input;
    FString Error;
    TStrongObjectPtr<UGrimrockPartySaveGame> Expected(Copy::LoadExact(Input, Error));
    if (!TestNotNull(TEXT("Original binary accepted by actual v27 loader"), Expected.Get())) return false;
    for (int32 I = 0; I < 4; ++I)
    {
        auto& C = I < 2 ? Expected->PartyInventoryState.ActiveCharacters[I] : Expected->PartyInventoryState.CharacterPool[I-2];
        C.CombatHotbarSlots[2].ActionId = Identity::ActionForItem(Identity::Item(I));
    }
    TArray<uint8> Converted;
    int32 Count = INDEX_NONE;
    const bool bConverted = Copy::Convert(Input, Converted, Count, Error);
    if (!TestTrue(*FString::Printf(TEXT("Actual v27 binary converts: %s"), *Error), bConverted)) return false;
    TestEqual(TEXT("Four bindings, two active and two reserve"), Count, 4);
    TestTrue(TEXT("Original bytes unchanged"), Input == Original);
    TStrongObjectPtr<UGrimrockPartySaveGame> Loaded(Copy::LoadExact(Converted, Error));
    if (!TestNotNull(TEXT("Converted binary accepted by same real loader"), Loaded.Get())) return false;
    TestTrue(TEXT("Every other reflected saved field retained"), Copy::SameSaveProperties(*Expected.Get(), *Loaded.Get(), Error));
    TestEqual(TEXT("Version remains 27"), Loaded->SaveVersion, 27);
    TestEqual(TEXT("World time kept"), Loaded->GameplayElapsedSeconds, 247.5);
    TestEqual(TEXT("Position kept"), Loaded->PartyCellX, 12);
    TestTrue(TEXT("Portrait order kept"), Loaded->PartyPortraitOrder == Save->PartyPortraitOrder);
    TArray<uint8> Again;
    TestTrue(TEXT("Already converted archive is accepted"), Copy::Convert(Converted, Again, Count, Error));
    TestEqual(TEXT("No additional modifications"), Count, 0);
    TestTrue(TEXT("No-op returns exactly original bytes, not another serialization"), Again == Converted);

    // Refuse aliasing without clearing original data.
    TArray<uint8> Alias = Original;
    TestFalse(TEXT("Aliased output refused"), Copy::Convert(Alias, Alias, Count, Error));
    TestTrue(TEXT("Aliased original unchanged"), Alias == Original);
    const TArray<uint8> Empty;
    TestFalse(TEXT("Empty input refused"), Copy::Convert(Empty, Again, Count, Error));
    TestTrue(TEXT("Failure returns no candidate"), Again.IsEmpty());

    // Wrong source identity is structurally accepted by the v27 archive
    // reader but is ambiguous for this migration and must be refused.
    Save->PartyInventoryState.CharacterPool[1].CombatHotbarSlots[2].SourceDefinitionId = TEXT("Unknown");
    TArray<uint8> Bad;
    if (!TestTrue(TEXT("Serialize ambiguous binding fixture"), UGameplayStatics::SaveGameToMemory(Save.Get(), Bad))) return false;
    TestFalse(TEXT("Unknown item for shared action refused atomically"), Copy::Convert(Bad, Again, Count, Error));
    TestTrue(TEXT("No partial file returned"), Count == 0 && Again.IsEmpty());

    Save->PartyInventoryState.CharacterPool[1].CombatHotbarSlots[2].SourceDefinitionId = Identity::Item(3);
    auto& Level = Save->DungeonRuntimeState.LevelStates.FindChecked(Save->CurrentDungeonLevelId);
    Level.CombatTraps.FindChecked(FIntPoint(9,8)).SourceActionId = Identity::SharedLegacyAction();
    Bad.Reset();
    if (!TestTrue(TEXT("Serialize ambiguous trap fixture"), UGameplayStatics::SaveGameToMemory(Save.Get(), Bad))) return false;
    TestFalse(TEXT("Ambiguous persisted source without ItemId refused"), Copy::Convert(Bad, Again, Count, Error));
    TestTrue(TEXT("No candidate on world-state conflict"), Again.IsEmpty());
    AddInfo(TEXT("[ARM-CLEAN06.4X64] UE v27 archive copy, exact action remapping and independent reload; original bytes preserved."));
    return !HasAnyErrors();
}
#endif
