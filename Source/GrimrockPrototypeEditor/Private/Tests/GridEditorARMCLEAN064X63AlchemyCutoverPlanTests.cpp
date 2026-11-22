#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../ARMEMENT/GridEditorAlchemyActionIdentity.h"
#include "Tests/GridCanonicalRegistryFixture.h"
#include "Save/GrimrockPartySaveGame.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridPartyInventoryComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X63AlchemyJointProjection,
    "Grimrock.RPG.ARM.CLEAN06_4X63.JointAlchemyProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN064X63AlchemyJointProjection::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Single canonical runtime ItemSystem"), Fixture.bReady)) return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Real ItemSystem root"), Root) ||
        !TestNotNull(TEXT("Real detailed action catalogue"), Root->ActionCatalogue.Get()) ||
        !TestNotNull(TEXT("Real action presentation catalogue"), Root->ActionPresentationCatalogue.Get()))
        return false;
    const bool bRootDirty = Root->GetOutermost()->IsDirty();
    const bool bActionDirty = Root->ActionCatalogue->GetOutermost()->IsDirty();
    TestEqual(TEXT("All 48 final canonical item identities"), Root->ItemDefinitions.Num(), 48);
    TestEqual(TEXT("Fourteen source-specific detailed actions"), Root->ActionCatalogue->DetailedActions.Num(), 14);
    TestEqual(TEXT("Original 19 compact ARM attacks still installed"), Root->ActionCatalogue->Actions.Num(), 19);
    TestNull(TEXT("Original shared elixir ID is not an alias in native catalogue"),
        Root->ActionCatalogue->FindDetailed(GridEditorAlchemyActionIdentity::SharedLegacyAction()));
    const FName ItemIds[] = {
        TEXT("Item_Antidote"), TEXT("Item_Bomb_Fire"), TEXT("Item_Bomb_Toxic"),
        TEXT("Item_DefensiveElixir_Fire"), TEXT("Item_DefensiveElixir_Ice"),
        TEXT("Item_DefensiveElixir_Lightning"), TEXT("Item_DefensiveElixir_Poison"),
        TEXT("Item_Flask_Acid"), TEXT("Item_Flask_CorrosiveCloud"), TEXT("Item_Panacea"),
        TEXT("Item_Flask_Oil")
    };
    TSet<FName> Used;
    int32 Checked = 0;
    for (FName Id : ItemIds)
    {
        FGridResolvedItem Item;
        if (!TestTrue(TEXT("Native N0 alchemy ItemId resolves"),
            Fixture.Resolve(Id, 0, Item))) continue;
        if (!TestEqual(TEXT("Exactly one explicit alchemy action"), Item.AvailableActions.Num(), 1))
            continue;
        const FName ExpectedRenamed = GridEditorAlchemyActionIdentity::ActionForItem(Id);
        const FName ActionId = Item.AvailableActions[0].ActionId;
        if (!ExpectedRenamed.IsNone())
            TestEqual(TEXT("Four former shared elixir IDs have their exact stable replacement"),
                ActionId, ExpectedRenamed);
        TestFalse(TEXT("No two alchemy actions share a colliding identity"), Used.Contains(ActionId));
        Used.Add(ActionId);
        FGridCombatActionDefinition Action;
        if (!TestTrue(TEXT("Authored detailed QuickItem action resolves"),
            Fixture.Registry->ResolveDetailedNonArmAction(Id, 0, ActionId, Action)))
            continue;
        ++Checked;
        TestEqual(TEXT("Native source policy retains QuickItem"),
            Action.SourcePolicy, EGridCombatActionSourcePolicy::QuickItem);
        TestEqual(TEXT("Exactly one physical source consumed"),
            Action.ResourceCosts.SourceItemQuantityCost, 1);
        TestEqual(TEXT("Alchemy N0 has no per-weapon level"), Item.ItemLevel, 0);
        TestEqual(TEXT("Alchemy does not occupy an equipment hand"), Item.NonArmEquipmentSlots.Num(), 0);
        TestTrue(TEXT("No copied armor profile on alchemy item"), !Item.bIsArmament);
        FGridCombatActionDefinition Invalid;
        TestFalse(TEXT("N21 alchemy is not a real physical instance"),
            Fixture.Registry->ResolveDetailedNonArmAction(Id, 21, ActionId, Invalid));
        for (int32 J = 0; J < 4; ++J)
        {
            const FName OtherId = GridEditorAlchemyActionIdentity::Item(J);
            if (OtherId == Id) continue;
            TestFalse(TEXT("A potion cannot borrow another elixir's specific action"),
                Fixture.Registry->ResolveDetailedNonArmAction(
                    Id, 0, GridEditorAlchemyActionIdentity::ActionForItem(OtherId), Invalid));
        }
    }
    TestEqual(TEXT("All eleven explicit physical alchemy owners"), Checked, 11);
    TestEqual(TEXT("No duplicate alchemy ActionId"), Used.Num(), 11);
    TestEqual(TEXT("Production ItemSystem unchanged"), Root->GetOutermost()->IsDirty(), bRootDirty);
    TestEqual(TEXT("Detailed catalogue unchanged"), Root->ActionCatalogue->GetOutermost()->IsDirty(), bActionDirty);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X63SavedElixirIdentity,
    "Grimrock.RPG.ARM.CLEAN06_4X63.SavedElixirBindingCopy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN064X63SavedElixirIdentity::RunTest(const FString&)
{
    namespace Identity = GridEditorAlchemyActionIdentity;
    TStrongObjectPtr<UGrimrockPartySaveGame> Save(NewObject<UGrimrockPartySaveGame>());
    TestEqual(TEXT("Old schema is still v27"), Save->SaveVersion, 27);
    Save->PartyInventoryState.SelectedCharacterIndex = 0;
    Save->PartyInventoryState.ActiveCharacters.SetNum(2);
    Save->PartyInventoryState.CharacterPool.SetNum(2);
    Save->CurrentDungeonLevelId = TEXT("X63_CopyOnly");
    Save->PartyCellX = 7;
    Save->PartyCellY = 9;
    Save->GameplayElapsedSeconds = 123.5;
    auto Add = [](FGridCharacterInventoryState& Character, int32 Variant)
    {
        Character.CharacterId = FGuid::NewGuid();
        Character.CombatHotbarSlots.SetNum(4);
        for (int32 I = 0; I < 4; ++I) Character.CombatHotbarSlots[I].Reset(I);
        auto& Primary = Character.CombatHotbarSlots[0];
        Primary.ActionId = FGridCombatHotbarBinding::MakePrimaryAttackActionId();
        Primary.SourcePolicy = EGridCombatActionSourcePolicy::Universal;
        auto& Binding = Character.CombatHotbarSlots[2];
        Binding.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
        Binding.ActionId = Identity::SharedLegacyAction();
        Binding.SourceDefinitionId = Identity::Item(Variant);
        Binding.PreferredSourceRuntimeId = FGuid::NewGuid();
        auto& Unrelated = Character.CombatHotbarSlots[3];
        Unrelated.SourcePolicy = EGridCombatActionSourcePolicy::QuickItem;
        Unrelated.ActionId = TEXT("Action_Alchemist_OilSlick");
        Unrelated.SourceDefinitionId = TEXT("Item_Flask_Oil");
        Character.InventorySlots.SetNum(1);
        Character.InventorySlots[0].bOccupied = true;
        Character.InventorySlots[0].Item.RuntimeObjectId = Binding.PreferredSourceRuntimeId;
        Character.InventorySlots[0].Item.ItemDefinitionId = Binding.SourceDefinitionId;
        Character.InventorySlots[0].Item.Quantity = 1;
        Character.InventorySlots[0].Item.ItemLevel = 0;
    };
    for (int32 I = 0; I < 2; ++I)
    {
        Add(Save->PartyInventoryState.ActiveCharacters[I], I);
        Add(Save->PartyInventoryState.CharacterPool[I], I+2);
    }
    const FGridPartyInventoryState Original = Save->PartyInventoryState;
    FGridPartyInventoryState Expected = Original;
    for (int32 I = 0; I < 2; ++I)
    {
        Expected.ActiveCharacters[I].CombatHotbarSlots[2].ActionId = Identity::ActionForItem(Identity::Item(I));
        Expected.CharacterPool[I].CombatHotbarSlots[2].ActionId = Identity::ActionForItem(Identity::Item(I+2));
    }
    FGridPartyInventoryState Working = Original;
    int32 Count = INDEX_NONE;
    FString Error;
    if (!TestTrue(TEXT("All active and reserve bindings converted in copy"), Identity::ConvertPartyCopy(Working, Count, Error))) return false;
    TestEqual(TEXT("Four exact replacements"), Count, 4);
    TestTrue(TEXT("Compare every reflected party field against four planned ID changes"),
        FGridPartyInventoryState::StaticStruct()->CompareScriptStruct(&Working, &Expected, 0));
    TestTrue(TEXT("Input v27 save remains unchanged"),
        FGridPartyInventoryState::StaticStruct()->CompareScriptStruct(&Save->PartyInventoryState, &Original, 0));
    TestEqual(TEXT("Version unchanged"), Save->SaveVersion, 27);
    TestEqual(TEXT("Dungeon coordinates unchanged"), Save->PartyCellX, 7);
    TestEqual(TEXT("Gameplay clock unchanged"), Save->GameplayElapsedSeconds, 123.5);
    TestTrue(TEXT("Repeat conversion is idempotent"), Identity::ConvertPartyCopy(Working, Count, Error));
    TestEqual(TEXT("No second conversion"), Count, 0);

    // Even an error in the last reserve character may not partially convert
    // previous active characters. No guesses based on the first element.
    auto Bad = Original;
    Bad.CharacterPool[1].CombatHotbarSlots[2].SourceDefinitionId = TEXT("UnrelatedItem");
    const auto BeforeBad = Bad;
    TestFalse(TEXT("Ambiguous old ActionId rejected"), Identity::ConvertPartyCopy(Bad, Count, Error));
    TestEqual(TEXT("No partial conversion"), Count, 0);
    TestTrue(TEXT("Original invalid input left untouched"),
        FGridPartyInventoryState::StaticStruct()->CompareScriptStruct(&Bad, &BeforeBad, 0));
    Bad = Expected;
    Bad.ActiveCharacters[0].CombatHotbarSlots[2].ActionId = Identity::ActionForItem(Identity::Item(1));
    TestFalse(TEXT("An already renamed but wrong element rejected"), Identity::ConvertPartyCopy(Bad, Count, Error));
    Bad = Original;
    Bad.ActiveCharacters[0].CombatHotbarSlots[2].SourcePolicy = EGridCombatActionSourcePolicy::Ability;
    TestFalse(TEXT("Shared legacy ID not valid for Ability source"), Identity::ConvertPartyCopy(Bad, Count, Error));
    AddInfo(TEXT("[ARM-CLEAN06.4X63] Four source-aware binding changes in memory only; no SaveGame file opened or rewritten."));
    return !HasAnyErrors();
}

namespace
{
    struct FX63World
    {
        UWorld* World = nullptr;
        UGameInstance* GI = nullptr;
        UGridItemSubsystem* Registry = nullptr;
        UGridPartyInventoryComponent* Inventory = nullptr;

        explicit FX63World(UGridItemSystemAsset* Preview)
        {
            const UWorld::InitializationValues Settings = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM064X63_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Settings);
            if (!World || !GEngine) return;
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
            GI = NewObject<UGameInstance>(GetTransientPackage());
            GI->Init();
            World->SetGameInstance(GI);
            Registry = GI->GetSubsystem<UGridItemSubsystem>();
            if (!Registry) return;
            Registry->Deinitialize();
            TArray<FString> Errors;
            if (!Registry->RegisterItemSystem(Preview, Errors)) return;
            auto* Pawn = World->SpawnActor<AGrimrockPartyPawn>();
            if (!Pawn) return;
            Inventory = Pawn->PartyInventoryComponent.Get();
            if (Inventory) Inventory->InitializeDefaultPartyIfNeeded();
        }
        ~FX63World()
        {
            if (World) World->SetGameInstance(nullptr);
            if (GI) GI->Shutdown();
            if (World)
            {
                World->DestroyWorld(false);
                if (GEngine) GEngine->DestroyWorldContext(World);
            }
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridEditorARMCLEAN064X63CanonicalAlchemyHotbar,
    "Grimrock.RPG.ARM.CLEAN06_4X63.JointAlchemyHotbar",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridEditorARMCLEAN064X63CanonicalAlchemyHotbar::RunTest(const FString&)
{
    FGridCanonicalRegistryFixture Fixture;
    if (!TestTrue(TEXT("Authoritative production ItemSystem"), Fixture.bReady)) return false;
    UGridItemSystemAsset* Root =
        GetDefault<UGridItemSubsystem>()->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Real ItemSystem root"), Root)) return false;
    FX63World World(Root);
    if (!TestTrue(TEXT("World with production registry and physical inventory"),
        World.Registry && World.Inventory &&
        World.Inventory->IsValidCharacterIndex(0))) return false;
    const FName Ids[] = {
        TEXT("Item_Antidote"), TEXT("Item_Bomb_Fire"), TEXT("Item_Bomb_Toxic"),
        TEXT("Item_DefensiveElixir_Fire"), TEXT("Item_DefensiveElixir_Ice"),
        TEXT("Item_DefensiveElixir_Lightning"), TEXT("Item_DefensiveElixir_Poison"),
        TEXT("Item_Flask_Acid"), TEXT("Item_Flask_CorrosiveCloud"),
        TEXT("Item_Panacea"), TEXT("Item_Flask_Oil")
    };
    FGridCharacterInventoryState& Hero =
        World.Inventory->PartyInventoryState.ActiveCharacters[0];
    Hero.InventorySlots.Reset();
    Hero.InventorySlots.SetNum(UE_ARRAY_COUNT(Ids));
    if (!TestTrue(TEXT("Physical hotbar has twelve positions"),
        World.Inventory->EnsureCharacterCombatHotbarCapacity(
            0, UE_ARRAY_COUNT(Ids) + 1))) return false;
    for (int32 I = 0; I < UE_ARRAY_COUNT(Ids); ++I)
    {
        FGridResolvedItem Canonical;
        if (!TestTrue(TEXT("Real N0 alchemy item resolves"),
            World.Registry->ResolveItem(Ids[I], 0, Canonical))) continue;
        if (!TestEqual(TEXT("One explicit action available"),
            Canonical.AvailableActions.Num(), 1)) continue;
        FGridInventorySlot& Slot = Hero.InventorySlots[I];
        Slot.bOccupied = true;
        Slot.Item.RuntimeObjectId = FGuid::NewGuid();
        Slot.Item.ItemDefinitionId = Ids[I];
        Slot.Item.ItemLevel = 0;
        Slot.Item.Quantity = 1;
        Slot.Item.OwnerType = EGridItemOwnerType::CharacterInventory;
        Slot.Item.OwnerGuid = Hero.CharacterId;
        Slot.Item.OwnerCharacterIndex = 0;
        const FGuid OriginalGuid = Slot.Item.RuntimeObjectId;
        if (!TestTrue(TEXT("Real inventory creates canonical QuickItem binding"),
            World.Inventory->SetCharacterCombatHotbarBindingFromItem(
                0, I + 1, Slot.Item, EGridEquipmentSlot::None))) continue;
        FGridCombatHotbarBinding Bound;
        if (!TestTrue(TEXT("Actual QuickItem binding retrievable"),
            World.Inventory->GetCharacterCombatHotbarBinding(0, I + 1, Bound))) continue;
        TestEqual(TEXT("Bound canonical action matches item owner"),
            Bound.ActionId, Canonical.AvailableActions[0].ActionId);
        TestEqual(TEXT("Item identity retained"), Bound.SourceDefinitionId, Ids[I]);
        TestEqual(TEXT("Source policy is a QuickItem"),
            Bound.SourcePolicy, EGridCombatActionSourcePolicy::QuickItem);
        TestEqual(TEXT("Binding does not consume the assigned physical instance"),
            Hero.InventorySlots[I].Item.RuntimeObjectId, OriginalGuid);
    }
    return !HasAnyErrors();
}

#endif
