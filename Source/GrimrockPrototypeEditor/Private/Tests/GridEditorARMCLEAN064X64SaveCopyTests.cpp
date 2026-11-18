#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "../ARMEMENT/GridEditorAlchemySaveCopy.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGCharacterPortraitSetAsset.h"
#include "RPG/RPGClassVisualAsset.h"
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
        Race->AttributeBonuses = FRPGAttributes{0,0,0,0,0,0};
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
        if (!Party.ActiveCharacters.IsValidIndex(0) ||
            !Party.ActiveCharacters[0].CombatHotbarSlots.IsValidIndex(2)) return false;
        const FGridCharacterInventoryState Template = Party.ActiveCharacters[0];
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
        Level.bLevelVariablesInitialized = true;
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
    FString Error;
    FText Validation;
    if (!TestTrue(TEXT("Complete fixture accepted by actual SaveGame validator"), Save->ValidateCurrentState(Validation)))
    {
        AddError(Validation.ToString());
        return false;
    }
    TArray<uint8> Input;
    const bool bEncoded = Copy::EncodeChecked(*Save.Get(), Input, Error);
    if (!TestTrue(*FString::Printf(TEXT("Real UE SaveGame byte serialization: %s"), *Error), bEncoded) ||
        !TestFalse(TEXT("Nonempty archive"), Input.IsEmpty())) return false;
    const TArray<uint8> Original = Input;
    TStrongObjectPtr<UGrimrockPartySaveGame> Expected(Copy::LoadExact(Input, Error));
    if (!TestNotNull(TEXT("Original binary accepted by actual v27 loader"), Expected.Get())) return false;
    if (!TestEqual(TEXT("Active character count"), Expected->PartyInventoryState.ActiveCharacters.Num(), 2) ||
        !TestEqual(TEXT("Reserve character count"), Expected->PartyInventoryState.CharacterPool.Num(), 2)) return false;
    for (int32 I = 0; I < 4; ++I)
    {
        auto& C = I < 2 ? Expected->PartyInventoryState.ActiveCharacters[I] : Expected->PartyInventoryState.CharacterPool[I-2];
        if (!TestTrue(TEXT("Expected saved slot exists"), C.CombatHotbarSlots.IsValidIndex(2))) return false;
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
    if (!TestTrue(TEXT("Serialize ambiguous binding fixture"), Copy::EncodeChecked(*Save.Get(), Bad, Error))) return false;
    TestFalse(TEXT("Unknown item for shared action refused atomically"), Copy::Convert(Bad, Again, Count, Error));
    TestTrue(TEXT("No partial file returned"), Count == 0 && Again.IsEmpty());

    Save->PartyInventoryState.CharacterPool[1].CombatHotbarSlots[2].SourceDefinitionId = Identity::Item(3);
    auto* Level = Save->DungeonRuntimeState.LevelStates.Find(Save->CurrentDungeonLevelId);
    if (!TestNotNull(TEXT("World snapshot still exists"), Level)) return false;
    auto* Trap = Level->CombatTraps.Find(FIntPoint(9,8));
    if (!TestNotNull(TEXT("Trap still exists"), Trap)) return false;
    Trap->SourceActionId = Identity::SharedLegacyAction();
    Bad.Reset();
    if (!TestTrue(TEXT("Serialize ambiguous trap fixture"), Copy::EncodeChecked(*Save.Get(), Bad, Error))) return false;
    TestFalse(TEXT("Ambiguous persisted source without ItemId refused"), Copy::Convert(Bad, Again, Count, Error));
    TestTrue(TEXT("No candidate on world-state conflict"), Again.IsEmpty());
    // Prove invalid save state never reaches the raw serializer. This
    // exercises the exact previously missed MON19.2.2 fixture invariant.
    Trap->SourceActionId = TEXT("Action_Unchanged_Trap");
    Level->bLevelVariablesInitialized = false;
    TArray<uint8> Partial = Original;
    TestFalse(TEXT("Uninitialized level variable state refuses encoding"),
        Copy::EncodeChecked(*Save.Get(), Partial, Error));
    TestTrue(TEXT("No partial bytes can be passed to LoadGameFromMemory"), Partial.IsEmpty());
    AddInfo(TEXT("[ARM-CLEAN06.4X64] UE v27 archive copy, exact action remapping and independent reload; original bytes preserved."));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X66ColdNativeSave,
    "Grimrock.RPG.ARM.CLEAN06_4X66.ColdNativeRPGSaveCopy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN064X66ColdNativeSave::RunTest(const FString&)
{
    FIdentityGuard Guard;
    using FResolver = FRPGAuthoringIdentityResolver;
    const FString Root = TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/");
    const TCHAR* Races[] = { TEXT("Dwarf"), TEXT("Elf"), TEXT("Gnome"), TEXT("HalfOrc"), TEXT("Halfling"), TEXT("Human") };
    const TCHAR* Classes[] = { TEXT("Alchemist"), TEXT("Mage"), TEXT("Priest"), TEXT("Ranger"), TEXT("Rogue"), TEXT("Warrior") };
    auto ObjectPath = [&](const FString& Directory, const FString& Name)
    { return Root + Directory + Name + TEXT(".") + Name; };

    // Use actual native assets as the oracle, not transient fixtures or
    // file names as durable semantic identities. Reset the runtime cache
    // separately before each lookup so a remembered character creation
    // definition cannot conceal the commandlet's cold-load defect.
    for (const TCHAR* Name : Races)
    {
        FResolver::ResetRuntimeCache();
        const auto* ResolvedRace = FResolver::ResolveRaceById(FName(Name));
        const FString RaceName = FString(TEXT("DA_Race_")) + Name;
        const auto* Race = LoadObject<URPGRaceAsset>(nullptr, *ObjectPath(TEXT(""), RaceName));
        if (!TestNotNull(TEXT("Native race asset"), Race)) return false;
        TestTrue(*FString::Printf(TEXT("Cold race %s resolves to native asset"), Name),
            ResolvedRace == Race && Race->RaceId == FName(Name));

        FResolver::ResetRuntimeCache();
        const auto* ResolvedPortrait = FResolver::ResolvePortraitSetByRaceId(FName(Name));
        const FString PortraitName = FString(TEXT("DA_PortraitSet_")) + Name;
        const auto* Portrait = LoadObject<URPGCharacterPortraitSetAsset>(nullptr,
            *ObjectPath(TEXT("Visuals/PortraitSets/"), PortraitName));
        if (!TestNotNull(TEXT("Native portrait set"), Portrait)) return false;
        TestTrue(*FString::Printf(TEXT("Cold portrait set %s resolves to native asset"), Name),
            ResolvedPortrait == Portrait && Portrait->RaceId == FName(Name));
    }
    for (const TCHAR* Name : Classes)
    {
        const auto* Class = LoadObject<URPGClassAsset>(nullptr,
            *ObjectPath(TEXT(""), FString(TEXT("DA_Class_")) + Name));
        if (!TestNotNull(TEXT("Native RPG class"), Class)) return false;
        FResolver::ResetRuntimeCache();
        TestTrue(*FString::Printf(TEXT("Cold class %s resolves to native asset"), Name),
            FResolver::ResolveClassById(Class->ClassId) == Class);
        const auto* Visual = LoadObject<URPGClassVisualAsset>(nullptr,
            *ObjectPath(TEXT("Visuals/ClassVisuals/"), FString(TEXT("DA_ClassVisual_")) + Name));
        if (!TestNotNull(TEXT("Native class visual"), Visual)) return false;
        FResolver::ResetRuntimeCache();
        TestTrue(*FString::Printf(TEXT("Cold class visual %s resolves to native asset"), Name),
            FResolver::ResolveClassVisualByClassId(Visual->ClassId) == Visual);
    }
    FResolver::ResetRuntimeCache();
    TestNull(TEXT("Unknown race is not guessed"), FResolver::ResolveRaceById(TEXT("X66_NoSuchRace")));

    auto* Race = LoadObject<URPGRaceAsset>(nullptr, *ObjectPath(TEXT(""), TEXT("DA_Race_Dwarf")));
    auto* Class = LoadObject<URPGClassAsset>(nullptr, *ObjectPath(TEXT(""), TEXT("DA_Class_Warrior")));
    auto* Portrait = LoadObject<URPGCharacterPortraitSetAsset>(nullptr,
        *ObjectPath(TEXT("Visuals/PortraitSets/"), TEXT("DA_PortraitSet_Dwarf")));
    if (!Race || !Class || !Portrait) return false;
    FResolver::ResetRuntimeCache();
    TStrongObjectPtr<UGridPartyInventoryComponent> Inventory(NewObject<UGridPartyInventoryComponent>());
    Inventory->InitializeDefaultPartyIfNeeded();
    FRPGCharacterCreationRequest Request;
    Request.DisplayName = FText::FromString(TEXT("X66 Native Hero"));
    Request.RaceDefinition = Race;
    Request.ClassDefinition = Class;
    FRPGCharacterPortraitVariant Variant;
    if (!TestTrue(TEXT("Actual Dwarf portrait"), Portrait->GetFirstValidPortrait(ERPGCharacterPortraitGender::Male, Variant)))
        return false;
    Request.PortraitGender = ERPGCharacterPortraitGender::Male;
    Request.PortraitVariantId = Variant.VariantId;
    Request.Portrait = Variant.Portrait;
    FText CreationError;
    const bool bCreated = Inventory->CreateInitialCharacter(Request, CreationError);
    if (!TestTrue(*FString::Printf(TEXT("Create native character: %s"), *CreationError.ToString()), bCreated))
        return false;
    TStrongObjectPtr<UGrimrockPartySaveGame> Save(NewObject<UGrimrockPartySaveGame>());
    Save->PartyInventoryState = Inventory->PartyInventoryState;
    if (!Save->PartyInventoryState.ActiveCharacters.IsValidIndex(0)) return false;
    auto& Character = Save->PartyInventoryState.ActiveCharacters[0];
    if (!Character.CombatHotbarSlots.IsValidIndex(2)) return false;
    auto& Slot = Character.InventorySlots.AddDefaulted_GetRef();
    Slot.bOccupied = true;
    Slot.Item.ItemDefinitionId = Identity::Item(0);
    Slot.Item.ItemLevel = 0;
    Slot.Item.Quantity = 1;
    Slot.Item.RuntimeObjectId = FGuid::NewGuid();
    Slot.Item.OwnerGuid = Character.CharacterId;
    Slot.Item.OwnerCharacterIndex = 0;
    Slot.Item.OwnerType = EGridItemOwnerType::CharacterInventory;
    auto& Binding = Character.CombatHotbarSlots[2];
    Binding.Reset(2);
    Binding.ActionId = Identity::SharedLegacyAction();
    Binding.SourceDefinitionId = Slot.Item.ItemDefinitionId;
    Binding.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
    Binding.PreferredSourceRuntimeId = Slot.Item.RuntimeObjectId;
    Save->CurrentDungeonLevelId = TEXT("X66_ColdNative");
    FString Error;
    TArray<uint8> Bytes, Converted;
    const bool bEncoded = Copy::EncodeChecked(*Save.Get(), Bytes, Error);
    if (!TestTrue(*FString::Printf(TEXT("Encode native v27: %s"), *Error), bEncoded)) return false;
    const auto Original = Bytes;
    FResolver::ResetRuntimeCache();
    int32 Count = INDEX_NONE;
    const bool bConverted = Copy::Convert(Bytes, Converted, Count, Error);
    if (!TestTrue(*FString::Printf(TEXT("Cold native conversion: %s"), *Error), bConverted)) return false;
    TestEqual(TEXT("Exactly one elixir binding rewritten"), Count, 1);
    TestTrue(TEXT("Original archive untouched"), Bytes == Original);
    FResolver::ResetRuntimeCache();
    TStrongObjectPtr<UGrimrockPartySaveGame> Loaded(Copy::LoadExact(Converted, Error));
    if (!TestNotNull(TEXT("Cold converted save accepted"), Loaded.Get()) ||
        !Loaded->PartyInventoryState.ActiveCharacters.IsValidIndex(0)) return false;
    const auto& Restored = Loaded->PartyInventoryState.ActiveCharacters[0];
    TestTrue(TEXT("Durable attributes unchanged"), FRPGAttributes::StaticStruct()->CompareScriptStruct(
        &Character.Attributes, &Restored.Attributes, 0));
    TestEqual(TEXT("Same character identity"), Restored.CharacterId, Character.CharacterId);
    TestTrue(TEXT("Native class and race resolve"), FResolver::ResolveClassById(Restored.ClassId) == Class &&
        FResolver::ResolveRaceById(Restored.RaceId) == Race);
    TestTrue(TEXT("Portrait preserved"), Restored.Portrait.ToSoftObjectPath() == Variant.Portrait.ToSoftObjectPath());
    FRPGAttributePointBalance Balance;
    TestTrue(TEXT("Native v27 budget retains strict rules"), FRPGAttributePointService::TryGetBalance(Restored, Balance));
    auto Invalid = Restored;
    Invalid.Attributes.Strength = 99;
    TestFalse(TEXT("Genuinely invalid attribute still rejected"), FRPGAttributePointService::TryGetBalance(Invalid, Balance));
    TestTrue(TEXT("Native assets untouched"), !Class->GetOutermost()->IsDirty() && !Race->GetOutermost()->IsDirty());
    return !HasAnyErrors();
}

#endif
