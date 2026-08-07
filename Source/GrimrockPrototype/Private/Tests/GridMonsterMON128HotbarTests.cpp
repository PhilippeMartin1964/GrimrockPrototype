#if WITH_DEV_AUTOMATION_TESTS

#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UObjectGlobals.h"

namespace
{
    UGridPartyInventoryComponent* CreateMON128Inventory ()
    {
        UGridPartyInventoryComponent* Component =
            NewObject<UGridPartyInventoryComponent> ();
        Component->InitializeDefaultPartyIfNeeded ();
        return Component;
    }

    FGridCombatHotbarBinding MakeMON128UnarmedBinding ()
    {
        FGridCombatHotbarBinding Binding;
        Binding.ActionId = TEXT ("Attack_Unarmed");
        Binding.SourcePolicy = EGridCombatActionSourcePolicy::Universal;
        return Binding;
    }

    FGridCombatHotbarBinding MakeMON128EquipmentBinding (
        const FGuid& RuntimeId)
    {
        FGridCombatHotbarBinding Binding;
        Binding.ActionId = TEXT ("Attack_Shuriken");
        Binding.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
        Binding.SourceDefinitionId = TEXT ("Shuriken");
        Binding.PreferredSourceRuntimeId = RuntimeId;
        Binding.PreferredEquipmentSlot = EGridEquipmentSlot::MainHand;
        return Binding;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON128DefaultHotbarTest,
    "Grimrock.Monsters.MON12.8.1.DefaultHotbarIsEmpty",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON128DefaultHotbarTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    UGridPartyInventoryComponent* Component = CreateMON128Inventory ();
    if (!TestNotNull (TEXT ("The inventory component is created"), Component))
    {
        return false;
    }

    TestEqual (
        TEXT ("The component exposes ten hotbar slots"),
        Component->GetCombatHotbarSlotCount (),
        10);
    const FGridCharacterInventoryState& Character =
        Component->PartyInventoryState.ActiveCharacters[0];
    TestEqual (
        TEXT ("A new character owns ten hotbar slots"),
        Character.CombatHotbarSlots.Num (),
        10);
    for (int32 SlotIndex = 0;
        SlotIndex < Character.CombatHotbarSlots.Num ();
        ++SlotIndex)
    {
        const FGridCombatHotbarBinding& Binding =
            Character.CombatHotbarSlots[SlotIndex];
        TestEqual (
            FString::Printf (TEXT ("Hotbar slot %d keeps its index"), SlotIndex),
            Binding.SlotIndex,
            SlotIndex);
        TestTrue (
            FString::Printf (TEXT ("Hotbar slot %d starts empty"), SlotIndex),
            Binding.IsEmpty ());
        TestTrue (
            FString::Printf (TEXT ("Hotbar slot %d is structurally valid"), SlotIndex),
            Binding.IsValid ());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON128PerCharacterHotbarTest,
    "Grimrock.Monsters.MON12.8.1.PerCharacterBindings",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON128PerCharacterHotbarTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    UGridPartyInventoryComponent* Component = CreateMON128Inventory ();
    if (!Component)
    {
        return false;
    }

    FGridCharacterInventoryState SecondCharacter;
    SecondCharacter.CharacterId = FGuid::NewGuid ();
    SecondCharacter.DisplayName = FText::FromString (TEXT ("Second"));
    Component->PartyInventoryState.ActiveCharacters.Add (SecondCharacter);
    Component->PartyInventoryState.ActiveEquipment.AddDefaulted ();
    Component->InitializeDefaultPartyIfNeeded ();

    TestTrue (
        TEXT ("Character zero accepts an unarmed shortcut"),
        Component->SetCharacterCombatHotbarBinding (
            0,
            0,
            MakeMON128UnarmedBinding ()));

    FGridCombatHotbarBinding FirstBinding;
    FGridCombatHotbarBinding SecondBinding;
    TestTrue (
        TEXT ("Character zero shortcut can be read"),
        Component->GetCharacterCombatHotbarBinding (
            0,
            0,
            FirstBinding));
    TestTrue (
        TEXT ("Character one shortcut can be read"),
        Component->GetCharacterCombatHotbarBinding (
            1,
            0,
            SecondBinding));
    TestEqual (
        TEXT ("Character zero keeps the assigned action"),
        FirstBinding.ActionId,
        FName (TEXT ("Attack_Unarmed")));
    TestTrue (
        TEXT ("Character one's corresponding slot remains empty"),
        SecondBinding.IsEmpty ());

    TestTrue (
        TEXT ("The shortcut can be cleared explicitly"),
        Component->ClearCharacterCombatHotbarBinding (0, 0));
    TestTrue (
        TEXT ("The cleared shortcut reads successfully"),
        Component->GetCharacterCombatHotbarBinding (
            0,
            0,
            FirstBinding));
    TestTrue (TEXT ("The cleared shortcut is empty"), FirstBinding.IsEmpty ());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON128HotbarSaveRoundTripTest,
    "Grimrock.Monsters.MON12.8.1.SaveMemoryRoundTrip",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON128HotbarSaveRoundTripTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    UGridPartyInventoryComponent* SourceComponent = CreateMON128Inventory ();
    if (!SourceComponent)
    {
        return false;
    }
    SourceComponent->PartyInventoryState.bInitialCharacterCreationCompleted = true;

    const FGuid WeaponRuntimeId = FGuid::NewGuid ();
    TestTrue (
        TEXT ("An equipment shortcut is assigned before saving"),
        SourceComponent->SetCharacterCombatHotbarBinding (
            0,
            9,
            MakeMON128EquipmentBinding (WeaponRuntimeId)));

    UGrimrockPartySaveGame* SourceSave =
        NewObject<UGrimrockPartySaveGame> ();
    SourceSave->PartyInventoryState = SourceComponent->PartyInventoryState;

    TArray<uint8> SaveBytes;
    TestTrue (
        TEXT ("The hotbar SaveGame serializes to memory"),
        UGameplayStatics::SaveGameToMemory (SourceSave, SaveBytes));
    UGrimrockPartySaveGame* LoadedSave = Cast<UGrimrockPartySaveGame> (
        UGameplayStatics::LoadGameFromMemory (SaveBytes));
    if (!TestNotNull (TEXT ("The hotbar SaveGame deserializes"), LoadedSave))
    {
        return false;
    }

    const FGridCombatHotbarBinding& LoadedBinding =
        LoadedSave->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[9];
    TestEqual (TEXT ("The slot index survives serialization"), LoadedBinding.SlotIndex, 9);
    TestEqual (
        TEXT ("The action identity survives serialization"),
        LoadedBinding.ActionId,
        FName (TEXT ("Attack_Shuriken")));
    TestTrue (
        TEXT ("The source policy survives serialization"),
        LoadedBinding.SourcePolicy == EGridCombatActionSourcePolicy::Equipment);
    TestEqual (
        TEXT ("The source definition survives serialization"),
        LoadedBinding.SourceDefinitionId,
        FName (TEXT ("Shuriken")));
    TestTrue (
        TEXT ("The preferred runtime source survives serialization"),
        LoadedBinding.PreferredSourceRuntimeId == WeaponRuntimeId);
    TestTrue (
        TEXT ("The preferred equipment slot survives serialization"),
        LoadedBinding.PreferredEquipmentSlot == EGridEquipmentSlot::MainHand);

    UGridPartyInventoryComponent* RestoredComponent =
        NewObject<UGridPartyInventoryComponent> ();
    FText RestoreError;
    TestTrue (
        TEXT ("The deserialized party and hotbar restore atomically"),
        RestoredComponent->RestorePartyInventoryState (
            LoadedSave->PartyInventoryState,
            RestoreError));
    FGridCombatHotbarBinding RestoredBinding;
    TestTrue (
        TEXT ("The restored binding is exposed by the component"),
        RestoredComponent->GetCharacterCombatHotbarBinding (
            0,
            9,
            RestoredBinding));
    TestTrue (
        TEXT ("The restored runtime identity is unchanged"),
        RestoredBinding.PreferredSourceRuntimeId == WeaponRuntimeId);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON128LegacySaveMigrationTest,
    "Grimrock.Monsters.MON12.8.1.LegacySaveGetsEmptyHotbar",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON128LegacySaveMigrationTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    UGridPartyInventoryComponent* SourceComponent = CreateMON128Inventory ();
    if (!SourceComponent)
    {
        return false;
    }

    FGridPartyInventoryState LegacyState = SourceComponent->PartyInventoryState;
    LegacyState.bInitialCharacterCreationCompleted = true;
    LegacyState.ActiveCharacters[0].CombatHotbarSlots.Reset ();

    UGridPartyInventoryComponent* RestoredComponent =
        NewObject<UGridPartyInventoryComponent> ();
    FText RestoreError;
    TestTrue (
        TEXT ("A legacy snapshot without hotbar data is accepted"),
        RestoredComponent->RestorePartyInventoryState (
            LegacyState,
            RestoreError));
    const TArray<FGridCombatHotbarBinding>& MigratedSlots =
        RestoredComponent->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots;
    TestEqual (
        TEXT ("The legacy character receives ten slots"),
        MigratedSlots.Num (),
        10);
    TestTrue (
        TEXT ("All migrated shortcuts are empty"),
        MigratedSlots.ContainsByPredicate (
            [] (const FGridCombatHotbarBinding& Binding)
            {
                return !Binding.IsEmpty ();
            }) == false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON128RejectInvalidHotbarTest,
    "Grimrock.Monsters.MON12.8.1.RejectInvalidHotbarAtomically",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON128RejectInvalidHotbarTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    UGridPartyInventoryComponent* Component = CreateMON128Inventory ();
    if (!Component)
    {
        return false;
    }
    const FGuid OriginalCharacterId =
        Component->PartyInventoryState.ActiveCharacters[0].CharacterId;

    FGridPartyInventoryState InvalidState = Component->PartyInventoryState;
    InvalidState.bInitialCharacterCreationCompleted = true;
    FGridCombatHotbarBinding& InvalidBinding =
        InvalidState.ActiveCharacters[0].CombatHotbarSlots[3];
    InvalidBinding.ActionId = TEXT ("Attack_BrokenWeapon");
    InvalidBinding.SourcePolicy = EGridCombatActionSourcePolicy::Equipment;
    InvalidBinding.SourceDefinitionId = TEXT ("BrokenWeapon");
    InvalidBinding.PreferredSourceRuntimeId.Invalidate ();

    FText RestoreError;
    TestFalse (
        TEXT ("An invalid populated hotbar is rejected"),
        Component->RestorePartyInventoryState (InvalidState, RestoreError));
    TestTrue (TEXT ("The invalid hotbar returns an error"), !RestoreError.IsEmpty ());
    TestTrue (
        TEXT ("The previous party remains intact"),
        Component->PartyInventoryState.ActiveCharacters[0].CharacterId ==
            OriginalCharacterId);
    TestTrue (
        TEXT ("The previous party hotbar remains empty"),
        Component->PartyInventoryState.ActiveCharacters[0].CombatHotbarSlots[3].IsEmpty ());
    return true;
}

#endif
