#if WITH_DEV_AUTOMATION_TESTS

#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UI/GridCombatHudWidget.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON128MoveOrSwapHotbarTest,
    "Grimrock.Monsters.MON12.8.2.MoveOrSwapBindings",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON128MoveOrSwapHotbarTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    UGridPartyInventoryComponent* Component = CreateMON128Inventory ();
    if (!Component)
    {
        return false;
    }

    const FGuid WeaponRuntimeId = FGuid::NewGuid ();
    TestTrue (TEXT ("Slot zero accepts the unarmed shortcut"),
        Component->SetCharacterCombatHotbarBinding (
            0,
            0,
            MakeMON128UnarmedBinding ()));
    TestTrue (TEXT ("Slot one accepts the weapon shortcut"),
        Component->SetCharacterCombatHotbarBinding (
            0,
            1,
            MakeMON128EquipmentBinding (WeaponRuntimeId)));
    TestTrue (TEXT ("Dropping onto an occupied slot swaps atomically"),
        Component->MoveOrSwapCharacterCombatHotbarBinding (0, 0, 1));

    FGridCombatHotbarBinding SlotZero;
    FGridCombatHotbarBinding SlotOne;
    Component->GetCharacterCombatHotbarBinding (0, 0, SlotZero);
    Component->GetCharacterCombatHotbarBinding (0, 1, SlotOne);
    TestEqual (TEXT ("The weapon moved to slot zero"),
        SlotZero.ActionId, FName (TEXT ("Attack_Shuriken")));
    TestEqual (TEXT ("The unarmed action moved to slot one"),
        SlotOne.ActionId, FName (TEXT ("Attack_Unarmed")));
    TestEqual (TEXT ("The swapped first index is normalized"),
        SlotZero.SlotIndex, 0);
    TestEqual (TEXT ("The swapped second index is normalized"),
        SlotOne.SlotIndex, 1);

    TestTrue (TEXT ("Dropping onto an empty slot moves the binding"),
        Component->MoveOrSwapCharacterCombatHotbarBinding (0, 1, 2));
    FGridCombatHotbarBinding SlotTwo;
    Component->GetCharacterCombatHotbarBinding (0, 1, SlotOne);
    Component->GetCharacterCombatHotbarBinding (0, 2, SlotTwo);
    TestTrue (TEXT ("The move clears its source"), SlotOne.IsEmpty ());
    TestEqual (TEXT ("The empty target receives the action"),
        SlotTwo.ActionId, FName (TEXT ("Attack_Unarmed")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON128QuickItemDropTest,
    "Grimrock.Monsters.MON12.8.2.InventoryQuickItemBinding",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON128QuickItemDropTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    UGridPartyInventoryComponent* Component = CreateMON128Inventory ();
    if (!Component)
    {
        return false;
    }

    UGridItemDefinitionAsset* PotionDefinition =
        NewObject<UGridItemDefinitionAsset> (Component);
    PotionDefinition->ItemDefinitionId = TEXT ("Potion_MON1282");
    PotionDefinition->DisplayName =
        FText::FromString (TEXT ("Potion de test"));
    PotionDefinition->ItemType = EGridItemType::Potion;
    PotionDefinition->bStackable = true;
    PotionDefinition->MaxStackSize = 10;
    TestTrue (TEXT ("The potion definition is registered"),
        Component->RegisterItemDefinition (PotionDefinition));

    FGridItemInstance Potion;
    Potion.RuntimeObjectId = FGuid::NewGuid ();
    Potion.ItemDefinitionId = PotionDefinition->ItemDefinitionId;
    Potion.DisplayName = PotionDefinition->DisplayName;
    Potion.Quantity = 3;
    Potion.OwnerType = EGridItemOwnerType::CharacterInventory;
    Potion.OwnerCharacterIndex = 0;
    FGridInventorySlot& InventorySlot = Component->PartyInventoryState
        .ActiveCharacters[0].InventorySlots[0];
    InventorySlot.bOccupied = true;
    InventorySlot.Item = Potion;

    TestTrue (TEXT ("Dropping a potion creates a type-based shortcut"),
        Component->SetCharacterCombatHotbarBindingFromItem (
            0,
            4,
            Potion,
            EGridEquipmentSlot::None));
    FGridCombatHotbarBinding Binding;
    Component->GetCharacterCombatHotbarBinding (0, 4, Binding);
    TestEqual (TEXT ("The quick-item action id is stable"),
        Binding.ActionId,
        FName (TEXT ("Use_Potion_MON1282")));
    TestTrue (TEXT ("The binding uses quick-item policy"),
        Binding.SourcePolicy == EGridCombatActionSourcePolicy::QuickItem);
    TestEqual (TEXT ("The item definition identifies future stacks"),
        Binding.SourceDefinitionId,
        PotionDefinition->ItemDefinitionId);
    TestFalse (TEXT ("A stack runtime id is deliberately not persisted"),
        Binding.PreferredSourceRuntimeId.IsValid ());
    TestTrue (TEXT ("Creating the shortcut does not move the potion"),
        !InventorySlot.IsEmpty () && InventorySlot.Item.Quantity == 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON128StableEquipmentResolutionTest,
    "Grimrock.Monsters.MON12.8.2.EquipmentBindingFollowsRuntimeItem",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON128StableEquipmentResolutionTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    const FGuid WeaponRuntimeId = FGuid::NewGuid ();
    FGridCombatHotbarBinding Binding =
        MakeMON128EquipmentBinding (WeaponRuntimeId);
    Binding.SlotIndex = 0;
    TArray<FGridCombatHotbarBinding> Bindings;
    Bindings.SetNum (FGridCombatHotbarBinding::SlotCount);
    for (int32 SlotIndex = 0;
        SlotIndex < Bindings.Num ();
        ++SlotIndex)
    {
        Bindings[SlotIndex].Reset (SlotIndex);
    }
    Bindings[0] = Binding;

    FGridAvailableCombatAction Action;
    Action.Definition.ActionId = Binding.ActionId;
    Action.Definition.SourcePolicy = Binding.SourcePolicy;
    Action.SourceDefinitionId = Binding.SourceDefinitionId;
    Action.SourceRuntimeId = WeaponRuntimeId;
    Action.SourceEquipmentSlot = EGridEquipmentSlot::OffHand;
    Action.bEnabled = true;
    TArray<FGridAvailableCombatAction> AvailableActions = { Action };
    TArray<FGridCombatHudActionView> Views;
    FGridCombatHudViewModelBuilder::BuildHotbarActions (
        Bindings,
        AvailableActions,
        Views);

    TestEqual (TEXT ("The projection still contains ten fixed slots"),
        Views.Num (), 10);
    TestTrue (TEXT ("The same runtime weapon resolves after changing hand"),
        Views[0].bResolved);
    TestTrue (TEXT ("The resolved action uses its current hand"),
        Views[0].Action.SourceEquipmentSlot ==
            EGridEquipmentSlot::OffHand);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON1284ZeroQuantityDefinitionRehydrationTest,
    "Grimrock.Monsters.MON12.8.4.ZeroQuantityDefinitionRehydration",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON1284ZeroQuantityDefinitionRehydrationTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    UGridPartyInventoryComponent* Component = CreateMON128Inventory ();
    if (!Component)
    {
        return false;
    }

    const FName PotionDefinitionId =
        TEXT ("Potion_MON1284_Rehydrate");
    FGridCombatHotbarBinding Binding;
    Binding.ActionId =
        FGridCombatHotbarBinding::MakeQuickItemActionId (
            PotionDefinitionId);
    Binding.SourcePolicy =
        EGridCombatActionSourcePolicy::QuickItem;
    Binding.SourceDefinitionId = PotionDefinitionId;
    TestTrue (TEXT ("A zero-quantity quick item binding is stored"),
        Component->SetCharacterCombatHotbarBinding (
            0,
            0,
            Binding));

    UGridItemDefinitionAsset* PotionDefinition =
        NewObject<UGridItemDefinitionAsset> (Component);
    PotionDefinition->ItemDefinitionId = PotionDefinitionId;
    PotionDefinition->ItemType = EGridItemType::Potion;
    int32 ResolverCallCount = 0;
    FName MissingDefinitionId;
    TestTrue (TEXT ("Hotbar-only definitions are rehydrated"),
        Component->RehydrateOwnedItemDefinitions (
            [&ResolverCallCount,
                PotionDefinition,
                PotionDefinitionId]
            (FName RequestedDefinitionId)
            {
                ++ResolverCallCount;
                return RequestedDefinitionId == PotionDefinitionId
                    ? PotionDefinition
                    : nullptr;
            },
            MissingDefinitionId));
    TestEqual (TEXT ("The hotbar definition is resolved exactly once"),
        ResolverCallCount,
        1);
    TestTrue (TEXT ("No definition is reported missing"),
        MissingDefinitionId.IsNone ());
    TestEqual (TEXT ("The rehydrated definition remains registered"),
        Component->FindItemDefinition (PotionDefinitionId),
        PotionDefinition);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST (
    FGridMON1288ClearShortcutKeepsItemSourceTest,
    "Grimrock.Monsters.MON12.8.8.ClearShortcutKeepsItemSource",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::EngineFilter)

bool FGridMON1288ClearShortcutKeepsItemSourceTest::RunTest (
    const FString& Parameters)
{
    (void)Parameters;
    UGridPartyInventoryComponent* Component = CreateMON128Inventory ();
    if (!TestNotNull (
        TEXT ("The inventory component is created"),
        Component))
    {
        return false;
    }

    FGridCharacterInventoryState& Character =
        Component->PartyInventoryState.ActiveCharacters[0];
    FGridInventorySlot& InventorySlot = Character.InventorySlots[0];
    FGridItemInstance Potion;
    Potion.RuntimeObjectId = FGuid::NewGuid ();
    Potion.ItemDefinitionId = TEXT ("Potion_MON1288");
    Potion.DisplayName = FText::FromString (TEXT ("Potion MON12.8.8"));
    Potion.Quantity = 3;
    Potion.OwnerType = EGridItemOwnerType::CharacterInventory;
    Potion.OwnerCharacterIndex = 0;
    InventorySlot.bOccupied = true;
    InventorySlot.Item = Potion;

    FGridCombatHotbarBinding PotionBinding;
    PotionBinding.ActionId =
        FGridCombatHotbarBinding::MakeQuickItemActionId (
            Potion.ItemDefinitionId);
    PotionBinding.SourcePolicy =
        EGridCombatActionSourcePolicy::QuickItem;
    PotionBinding.SourceDefinitionId = Potion.ItemDefinitionId;
    TestTrue (TEXT ("The inventory item shortcut is assigned"),
        Component->SetCharacterCombatHotbarBinding (
            0,
            0,
            PotionBinding));
    TestTrue (TEXT ("The inventory item shortcut is cleared"),
        Component->ClearCharacterCombatHotbarBinding (0, 0));
    TestFalse (TEXT ("Clearing does not remove the inventory item"),
        InventorySlot.IsEmpty ());
    TestTrue (TEXT ("Clearing preserves the inventory item instance"),
        InventorySlot.Item.RuntimeObjectId == Potion.RuntimeObjectId);
    TestEqual (TEXT ("Clearing preserves the inventory stack quantity"),
        InventorySlot.Item.Quantity,
        3);

    FGridItemInstance EquippedWeapon;
    EquippedWeapon.RuntimeObjectId = FGuid::NewGuid ();
    EquippedWeapon.ItemDefinitionId = TEXT ("Sword_MON1288");
    EquippedWeapon.DisplayName = FText::FromString (TEXT ("Épée MON12.8.8"));
    EquippedWeapon.Quantity = 1;
    EquippedWeapon.OwnerType = EGridItemOwnerType::EquipmentSlot;
    EquippedWeapon.OwnerCharacterIndex = 0;
    EquippedWeapon.EquipmentSlot = EGridEquipmentSlot::MainHand;
    Component->PartyInventoryState.ActiveEquipment[0].MainHand =
        EquippedWeapon;

    TestTrue (TEXT ("The equipped item shortcut is assigned"),
        Component->SetCharacterCombatHotbarBinding (
            0,
            1,
            MakeMON128EquipmentBinding (
                EquippedWeapon.RuntimeObjectId)));
    TestTrue (TEXT ("The equipped item shortcut is cleared"),
        Component->ClearCharacterCombatHotbarBinding (0, 1));

    FGridItemInstance StillEquippedWeapon;
    TestTrue (TEXT ("Clearing does not unequip the source item"),
        Component->GetEquippedItem (
            0,
            EGridEquipmentSlot::MainHand,
            StillEquippedWeapon));
    TestTrue (TEXT ("Clearing preserves the equipped item instance"),
        StillEquippedWeapon.RuntimeObjectId ==
            EquippedWeapon.RuntimeObjectId);

    FGridCombatHotbarBinding ClearedBinding;
    TestTrue (TEXT ("The cleared shortcut remains readable"),
        Component->GetCharacterCombatHotbarBinding (
            0,
            1,
            ClearedBinding));
    TestTrue (TEXT ("Only the shortcut binding is empty"),
        ClearedBinding.IsEmpty ());
    return true;
}

#endif
