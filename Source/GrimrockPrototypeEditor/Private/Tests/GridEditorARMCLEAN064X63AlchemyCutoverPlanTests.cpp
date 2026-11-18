#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../ARMEMENT/GridEditorAlchemyCutoverPlan.h"
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
    namespace Plan = GridEditorAlchemyCutoverPlan;
    namespace Audit = GridEditorAlchemyMigrationAudit;
    namespace Identity = GridEditorAlchemyActionIdentity;
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr, Audit::RootPath()));
    TStrongObjectPtr<UGridItemSystemAsset> Preview(NewObject<UGridItemSystemAsset>());
    if (!TestNotNull(TEXT("Real X.61B registry"), Root.Get()) ||
        !TestNotNull(TEXT("Real action catalogue"), Root->ActionCatalogue.Get()) ||
        !TestNotNull(TEXT("Real presentation catalogue"), Root->ActionPresentationCatalogue.Get()))
        return false;
    const auto OriginalItems = Root->ItemDefinitions;
    const auto OriginalActions = Root->ActionCatalogue->DetailedActions;
    const auto OriginalPresentations = Root->ActionPresentationCatalogue->DetailedPresentations;
    const FString OriginalRootManifest = FGridEditorLootDefinitionProjection::RootManifest(*Root.Get());
    FString Error;
    const bool bBuilt = Plan::BuildForCurrentState(*Root.Get(), *Preview.Get(), Error);
    if (!TestTrue(*FString::Printf(TEXT("All ten items coexist: %s"), *Error), bBuilt)) return false;
    TestEqual(TEXT("48 definitions"), Preview->ItemDefinitions.Num(), 48);
    TestEqual(TEXT("14 detailed actions"), Preview->ActionCatalogue->DetailedActions.Num(), 14);
    TestEqual(TEXT("19 compact actions retained"), Preview->ActionCatalogue->Actions.Num(), 19);
    TestNull(TEXT("No legacy elixir ActionId in candidate catalogue"), Preview->ActionCatalogue->FindDetailed(Identity::SharedLegacyAction()));
    TestTrue(TEXT("Both armament catalogues retained"), Preview->WeaponCatalogue == Root->WeaponCatalogue &&
        Preview->WeaponVisualCatalogue == Root->WeaponVisualCatalogue);
    TestTrue(TEXT("Action catalogues are isolated copies"), Preview->ActionCatalogue != Root->ActionCatalogue &&
        Preview->ActionPresentationCatalogue != Root->ActionPresentationCatalogue);

    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Complete joint root resolves"), Registry->RegisterItemSystem(Preview.Get(), Errors)))
    {
        for (const FString& Detail : Errors) AddError(Detail);
        return false;
    }

    TSet<FName> Used;
    for (int32 I = 0; I < Audit::TotalCount; ++I)
    {
        const FName ItemId = Audit::Id(I);
        const FString SourcePath = FRPGAlchemistAuthoring::GetItemObjectPath(ItemId);
        const auto* Old = LoadObject<UGridItemDefinitionAsset>(nullptr, *SourcePath);
        if (!TestNotNull(TEXT("Real item source"), Old)) return false;
        FGridCombatActionDefinition Historical, Actual;
        FGridResolvedItem Item;
        if (!TestTrue(TEXT("Original action builds"), Old->BuildQuickItemCombatActionDefinition(Historical)) ||
            !TestTrue(TEXT("Known item resolves without a legacy pointer"), Registry->ResolveItem(ItemId, 0, Item)) ||
            !TestEqual(TEXT("One bound action"), Item.AvailableActions.Num(), 1))
            return false;
        const FName Renamed = Identity::ActionForItem(ItemId);
        const FName ExpectedId = Renamed.IsNone() ? Historical.ActionId : Renamed;
        TestEqual(TEXT("Exact new binding identity"), Item.AvailableActions[0].ActionId, ExpectedId);
        TestFalse(TEXT("Every alchemy action has a unique ID"), Used.Contains(ExpectedId));
        Used.Add(ExpectedId);

        if (!TestTrue(TEXT("Bound detailed QuickItem action resolves"), Registry->ResolveDetailedNonArmAction(ItemId, 0, ExpectedId, Actual)))
            return false;
        // No loss of mechanical, text or authored FX fields. The new stable
        // identifier is the sole action-level difference for the four elixirs.
        Historical.ActionId = ExpectedId;
        TestTrue(TEXT("Every reflected action field retained"), Audit::DifferentActionFields(Historical, Actual).IsEmpty());
        TestEqual(TEXT("Original source consumption"), Actual.ResourceCosts.SourceItemQuantityCost, 1);
        TestEqual(TEXT("Original cooldown"), Actual.CooldownRounds, Historical.CooldownRounds);
        TestTrue(TEXT("Gameplay modifier source tags unchanged"), Actual.SourceTags == Historical.SourceTags);
        TArray<FName> ExpectedTags = Old->ItemTags;
        if (Plan::IsPotion(I)) ExpectedTags.Add(GridCanonicalInventoryCategoryTags::Potion());
        TestTrue(TEXT("Item tags preserve old gameplay tags and only explicit category"), Item.Tags == ExpectedTags);
        TestEqual(TEXT("Native weight stays zero"), Item.WeightKg, 0.f);
        TestEqual(TEXT("Native stack capacity stays one"), Item.MaxStack, 1);
        TestEqual(TEXT("No hand permission introduced"), Item.NonArmEquipmentSlots.Num(), 0);
        TestEqual(TEXT("No mesh invented"), Item.WorldMesh.ToSoftObjectPath(), Old->WorldMesh.ToSoftObjectPath());
        TestEqual(TEXT("No icon invented"), Item.ItemIcon.ToSoftObjectPath(), Old->Icon.ToSoftObjectPath());
        FGridCombatActionDefinition Invalid;
        TestFalse(TEXT("Old shared action is not a runtime alias"), Registry->ResolveDetailedNonArmAction(ItemId, 0, Identity::SharedLegacyAction(), Invalid));
        for (int32 J = 0; J < 4; ++J)
            if (ItemId != Identity::Item(J))
                TestFalse(TEXT("One item cannot borrow an unrelated elixir action"),
                    Registry->ResolveDetailedNonArmAction(ItemId, 0, Identity::ActionForItem(Identity::Item(J)), Invalid));
        TestFalse(TEXT("Invalid level refused"), Registry->ResolveDetailedNonArmAction(ItemId, 21, ExpectedId, Invalid));
        TestFalse(TEXT("Source package untouched"), Old->GetOutermost()->IsDirty());
    }
    TestEqual(TEXT("All eleven exact owners including unchanged Oil"), Used.Num(), 11);

    // Independently reconstructed projection must be identical, and even
    // one changed item field or action cost must defeat the native gate.
    TStrongObjectPtr<UGridItemSystemAsset> Second(NewObject<UGridItemSystemAsset>());
    if (!TestTrue(TEXT("Second complete projection"), Plan::BuildForCurrentState(*Root.Get(), *Second.Get(), Error)) ||
        !TestTrue(TEXT("Exact native-equivalent projection comparison"), Plan::MatchesProjected(*Second.Get(), *Preview.Get(), Error, false)))
        return false;
    auto* ChangedItem = Cast<UGridConsumableDefinitionAsset>(Second->ItemDefinitions.Last().Get());
    if (!TestNotNull(TEXT("Final projected consumable"), ChangedItem)) return false;
    ChangedItem->WeightKg += 0.5f;
    TestFalse(TEXT("One changed authored field refuses conversion"), Plan::MatchesProjected(*Second.Get(), *Preview.Get(), Error, false));
    ChangedItem->WeightKg -= 0.5f;
    Second->ActionCatalogue->DetailedActions.Last().Definition.ActionPointCost += 1;
    TestFalse(TEXT("One changed action cost refuses conversion"), Plan::MatchesProjected(*Second.Get(), *Preview.Get(), Error, false));
    Second->ActionCatalogue->DetailedActions.Last().Definition.ActionPointCost -= 1;
    TestTrue(TEXT("Restored projection is identical"), Plan::MatchesProjected(*Second.Get(), *Preview.Get(), Error, false));

    // Re-run the same planner without changing the persisted root or any
    // already allocated destination. A partially populated root is refused.
    TestFalse(TEXT("Populated candidate cannot be appended again"), Plan::BuildForCurrentState(*Root.Get(), *Preview.Get(), Error));
    TestTrue(TEXT("Original root ordered definitions unchanged"), Root->ItemDefinitions == OriginalItems);
    TestTrue(TEXT("Original root manifest identical"), FGridEditorLootDefinitionProjection::RootManifest(*Root.Get()) == OriginalRootManifest);
    TestEqual(TEXT("Original detailed action count unchanged"), Root->ActionCatalogue->DetailedActions.Num(), OriginalActions.Num());
    TestEqual(TEXT("Original detailed presentation count unchanged"), Root->ActionPresentationCatalogue->DetailedPresentations.Num(), OriginalPresentations.Num());
    for (int32 I = 0; I < OriginalActions.Num(); ++I)
        TestTrue(TEXT("Existing detailed action fields unchanged"),
            Audit::DifferentActionFields(OriginalActions[I].Definition, Root->ActionCatalogue->DetailedActions[I].Definition).IsEmpty());
    TestFalse(TEXT("No native package dirty"), Root->GetOutermost()->IsDirty() ||
        Root->ActionCatalogue->GetOutermost()->IsDirty() || Root->ActionPresentationCatalogue->GetOutermost()->IsDirty());
    AddInfo(TEXT("[ARM-CLEAN06.4X63] 48 definitions and 14 details coexist in memory; no native migration."));
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
    namespace Plan = GridEditorAlchemyCutoverPlan;
    namespace Audit = GridEditorAlchemyMigrationAudit;
    namespace Identity = GridEditorAlchemyActionIdentity;
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr, Audit::RootPath()));
    TStrongObjectPtr<UGridItemSystemAsset> Preview(NewObject<UGridItemSystemAsset>());
    if (!TestNotNull(TEXT("Real registry"), Root.Get())) return false;
    FString Error;
    const bool bBuilt = Plan::BuildForCurrentState(*Root.Get(), *Preview.Get(), Error);
    if (!TestTrue(*FString::Printf(TEXT("Complete joint projection: %s"), *Error), bBuilt)) return false;
    FX63World Fixture(Preview.Get());
    if (!TestTrue(TEXT("World-backed inventory with joint registry"),
            Fixture.Registry && Fixture.Registry->HasCanonicalItem(Audit::Id(0)) &&
            Fixture.Inventory && Fixture.Inventory->IsValidCharacterIndex(0)))
        return false;

    auto& Character = Fixture.Inventory->PartyInventoryState.ActiveCharacters[0];
    Character.InventorySlots.Reset();
    Character.InventorySlots.SetNum(Audit::TotalCount);
    if (!TestTrue(TEXT("Enough hotbar slots"), Fixture.Inventory->EnsureCharacterCombatHotbarCapacity(
            0, Audit::TotalCount+1))) return false;
    for (int32 I = 0; I < Audit::TotalCount; ++I)
    {
        auto& Slot = Character.InventorySlots[I];
        Slot.bOccupied = true;
        Slot.Item.RuntimeObjectId = FGuid::NewGuid();
        Slot.Item.ItemDefinitionId = Audit::Id(I);
        Slot.Item.ItemLevel = 0;
        Slot.Item.Quantity = 1;
        Slot.Item.OwnerType = EGridItemOwnerType::CharacterInventory;
        Slot.Item.OwnerGuid = Character.CharacterId;
        Slot.Item.OwnerCharacterIndex = 0;
    }
    for (int32 I = 0; I < Audit::TotalCount; ++I)
    {
        const FGridItemInstance Item = Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[I].Item;
        const FName Renamed = Identity::ActionForItem(Item.ItemDefinitionId);
        const FName Expected = Renamed.IsNone() ? Plan::HistoricalAction(I) : Renamed;
        if (!TestTrue(TEXT("Actual inventory creates canonical QuickItem shortcut"),
                Fixture.Inventory->SetCharacterCombatHotbarBindingFromItem(0, I+1, Item, EGridEquipmentSlot::None))) return false;
        FGridCombatHotbarBinding Binding;
        if (!TestTrue(TEXT("Shortcut retrievable"), Fixture.Inventory->GetCharacterCombatHotbarBinding(0, I+1, Binding))) return false;
        TestEqual(TEXT("Correct source-specific action"), Binding.ActionId, Expected);
        TestEqual(TEXT("Correct item ID"), Binding.SourceDefinitionId, Item.ItemDefinitionId);
        TestEqual(TEXT("QuickItem policy"), Binding.SourcePolicy, EGridCombatActionSourcePolicy::QuickItem);
        TestTrue(TEXT("Item retained at assignment time"), Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[I].Item.RuntimeObjectId == Item.RuntimeObjectId);
    }
    return !HasAnyErrors();
}

#endif
