#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/Combat/GridCombatTargetingResolver.h"
#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridItemInteractionReadModel.h"
#include "../Runtime/GridQuickItemUseReadModel.h"

namespace ARMCLEAN063AY
{
    static FName ItemId() { return TEXT("Item_ARM063AY_StatusCleanse"); }
    static FName ActionId() { return TEXT("Use_ARM063AY_StatusCleanse"); }
    static FName StatusId() { return TEXT("Status_ARM063AY_Toxin"); }

    struct FFixture
    {
        UWorld* World = nullptr;
        UGameInstance* GI = nullptr;
        UGridItemSubsystem* Registry = nullptr;
        UGridItemSystemAsset* System = nullptr;
        UGridConsumableDefinitionAsset* Item = nullptr;
        AGrimrockPartyPawn* Pawn = nullptr;
        UGridPartyInventoryComponent* Inventory = nullptr;

        FFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM063AY_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (!World || !GEngine) return;
            FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
            Context.SetCurrentWorld(World);
            GI = NewObject<UGameInstance>(GetTransientPackage());
            if (!GI) return;
            GI->Init();
            World->SetGameInstance(GI);
            Registry = GI->GetSubsystem<UGridItemSubsystem>();
            if (!Registry) return;
            UGridItemSystemAsset* Production = Registry->LoadConfiguredItemSystemForPreview();
            if (!Production || !Production->ActionCatalogue ||
                !Production->ActionPresentationCatalogue ||
                Production->ActionPresentationCatalogue->Presentations.IsEmpty()) return;

            System = DuplicateObject<UGridItemSystemAsset>(Production, GetTransientPackage());
            System->ActionCatalogue = DuplicateObject<UGridActionCatalogueAsset>(
                Production->ActionCatalogue, System);
            Item = NewObject<UGridConsumableDefinitionAsset>(System);
            Item->ItemId = ItemId();
            Item->DisplayName = FText::FromString(TEXT("AX/AY diagnostic cleanse"));
            Item->WeightKg = 0.2f;
            Item->MaxStack = 4;
            FGridItemActionBinding Binding;
            Binding.ActionId = ActionId();
            Binding.PresentationId =
                Production->ActionPresentationCatalogue->Presentations[0].PresentationId;
            Item->UseActions.Add(Binding);
            System->ItemDefinitions.Add(Item);

            FGridAction Use;
            Use.ActionId = ActionId();
            Use.Kind = EGridActionKind::Use;
            Use.Targeting = EGridCombatTargetingPolicy::Self;
            Use.SourceItemQuantityCost = 1;
            FGridActionEffect Effect;
            Effect.Kind = EGridEffectKind::RemoveStatus;
            Effect.StatusId = StatusId();
            Effect.Magnitude = 1;
            Use.Effects.Add(Effect);
            System->ActionCatalogue->Actions.Add(Use);
            Registry->Deinitialize();
            TArray<FString> Errors;
            if (!Registry->RegisterItemSystem(System, Errors)) return;

            Pawn = World->SpawnActor<AGrimrockPartyPawn>();
            if (!Pawn) return;
            Inventory = Pawn->PartyInventoryComponent.Get();
            if (!Inventory) return;
            Inventory->InitializeDefaultPartyIfNeeded();
            Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots.SetNum(6);
            Inventory->PartyInventoryState.ActiveCharacters[0].DerivedStats.MaxHealth = 30;
            Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth = 25;
        }

        ~FFixture()
        {
            if (World) World->SetGameInstance(nullptr);
            if (GI) GI->Shutdown();
            if (World)
            {
                World->DestroyWorld(false);
                if (GEngine) GEngine->DestroyWorldContext(World);
            }
        }

        bool Ready() const
        {
            return Registry && Item && Inventory && Inventory->IsValidCharacterIndex(0) &&
                Registry->ContainsItem(ItemId());
        }

        FGridItemInstance Make(int32 Level = 0) const
        {
            FGridItemInstance Entry;
            Entry.RuntimeObjectId = FGuid::NewGuid();
            Entry.ItemDefinitionId = ItemId();
            Entry.ItemLevel = Level;
            Entry.Quantity = 2;
            Entry.OwnerType = EGridItemOwnerType::World;
            return Entry;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AYStatusProjection,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalUseRemoveStatus.ProjectionAndRejections",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063AYStatusProjection::RunTest(const FString&)
{
    ARMCLEAN063AY::FFixture F;
    if (!TestTrue(TEXT("Registered synthetic RemoveStatus Use action"), F.Ready())) return false;

    FGridResolvedItem Item;
    if (!TestTrue(TEXT("N0 consumable resolves in actual registry"),
        F.Registry->ResolveItem(ARMCLEAN063AY::ItemId(), 0, Item))) return false;
    FGridResolvedBoundAction Bound;
    if (!TestTrue(TEXT("Actual ActionId and presentation link resolves"),
        F.Registry->ResolveBoundAction(ARMCLEAN063AY::ItemId(), 0,
            ARMCLEAN063AY::ActionId(), Bound))) return false;
    const auto View = FGridQuickItemUseReadModel::Resolve(F.Registry, F.Make());
    TestTrue(TEXT("Canonical status cleansing projects without historical item"),
        View.bRegisteredIdentity && View.bResolved);
    TestFalse(TEXT("Status cleansing never uses exploration self-heal executor"),
        View.bExplorationCompatible);
    TestTrue(TEXT("Projected QuickItem passes existing action validator"),
        View.Action.IsValid());
    TestEqual(TEXT("Exact authored ActionId"), View.Action.ActionId, ARMCLEAN063AY::ActionId());
    TestEqual(TEXT("Exactly one source item consumed"),
        View.Action.ResourceCosts.SourceItemQuantityCost, 1);
    TestEqual(TEXT("No invented AP cost"), View.Action.ActionPointCost, 0);
    TestEqual(TEXT("No invented healing"), View.Action.EffectProfile.RestoreHealth, 0);
    TestEqual(TEXT("One precise status removal filter"),
        View.Action.StatusRemovals.Num(), 1);
    if (View.Action.StatusRemovals.Num() == 1)
    {
        const auto& Removal = View.Action.StatusRemovals[0];
        TestEqual(TEXT("Only the authored StatusId matches"),
            Removal.EffectIds.Num(), 1);
        if (Removal.EffectIds.Num() == 1)
            TestEqual(TEXT("Authored status identity preserved"),
                Removal.EffectIds[0], ARMCLEAN063AY::StatusId());
        TestEqual(TEXT("One removal only"), Removal.MaximumRemovals, 1);
    }

    const auto Capability =
        FGridItemInteractionReadModel::Resolve(F.Registry, F.Make());
    TestTrue(TEXT("Status cleanse may be assigned to combat hotbar"),
        Capability.bCanAddToHotbar);
    TestFalse(TEXT("No inline Consume action for combat-only status removal"),
        Capability.bCanConsumeItem);
    TestFalse(TEXT("No physical throw shortcut"), Capability.bPhysicalThrowHotbar);

    auto TryProject = [&Item](FGridResolvedBoundAction Modified)
    {
        FGridCombatActionDefinition Output;
        return FGridQuickItemUseReadModel::TryProjectSingleStatusRemoval(
            Item, Modified, Output);
    };
    FGridResolvedBoundAction Modified = Bound;
    Modified.Action.Effects[0].Magnitude = 0;
    TestFalse(TEXT("Missing authored removal count is refused"), TryProject(Modified));
    Modified = Bound;
    Modified.Action.Effects[0].Magnitude = 2;
    TestFalse(TEXT("Multiple removals not inferred"), TryProject(Modified));
    Modified = Bound;
    Modified.Action.Effects[0].StatusId = NAME_None;
    TestFalse(TEXT("Missing StatusId is refused"), TryProject(Modified));
    Modified = Bound;
    Modified.Action.Targeting = EGridCombatTargetingPolicy::Ally;
    TestTrue(TEXT("BA: one explicit Ally is now accepted"), TryProject(Modified));
    Modified = Bound;
    Modified.Action.Targeting = EGridCombatTargetingPolicy::Party;
    TestFalse(TEXT("Party target remains forbidden"), TryProject(Modified));
    Modified = Bound;
    // UE TArray may reallocate on Add: never append an element by a
    // reference into the very array being mutated (assertion in Automation).
    const FGridActionEffect ExtraEffect = Modified.Action.Effects[0];
    Modified.Action.Effects.Add(ExtraEffect);
    TestFalse(TEXT("Mixed/multiple effects refuse ambiguous execution"), TryProject(Modified));
    Modified = Bound;
    Modified.Action.Effects[0].Kind = EGridEffectKind::ApplyStatus;
    TestFalse(TEXT("ApplyStatus cannot masquerade as RemoveStatus"), TryProject(Modified));
    Modified = Bound;
    Modified.Action.Effects[0].Kind = EGridEffectKind::Heal;
    TestFalse(TEXT("Heal not projected as RemoveStatus"), TryProject(Modified));

    const auto Invalid = FGridQuickItemUseReadModel::Resolve(F.Registry, F.Make(21));
    TestTrue(TEXT("N21 identity remains known"), Invalid.bRegisteredIdentity);
    TestFalse(TEXT("Invalid level has no executable action"), Invalid.bResolved);
    // AX self-healing remains a separate, supported projection.
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AYStatusCombat,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalUseRemoveStatus.CombatCatalogueAndHotbar",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063AYStatusCombat::RunTest(const FString&)
{
    ARMCLEAN063AY::FFixture F;
    if (!TestTrue(TEXT("Registered synthetic status cleanse"), F.Ready())) return false;
    const FGridItemInstance Incoming = F.Make();
    if (!TestTrue(TEXT("Canonical consumable materializes in inventory"),
        F.Inventory->AddItemToCharacterInventory(0, Incoming))) return false;
    const auto& Character = F.Inventory->PartyInventoryState.ActiveCharacters[0];
    const FGridInventorySlot* Slot = Character.InventorySlots.FindByPredicate(
        [](const FGridInventorySlot& Entry)
        {
            return !Entry.IsEmpty() &&
                Entry.Item.ItemDefinitionId == ARMCLEAN063AY::ItemId();
        });
    if (!TestNotNull(TEXT("Physical inventory source exists"), Slot)) return false;


    FGridItemActionContext Context;
    Context.PartyPawn = F.Pawn;
    Context.Item = Slot->Item;
    Context.InventorySlotIndex = 0;
    Context.CharacterIndex = 0;
    FGridFacingTargetContext Facing;
    TArray<FGridItemContextAction> Actions;
    TestTrue(TEXT("Context actions build"),
        UGridItemContextActionLibrary::BuildItemContextActions(Context, Facing, Actions));
    const auto* Consume = Actions.FindByPredicate(
        [](const FGridItemContextAction& A) { return A.ActionType == EGridItemActionType::Consume; });
    const auto* Hotbar = Actions.FindByPredicate(
        [](const FGridItemContextAction& A) { return A.ActionType == EGridItemActionType::AddToHotbar; });
    TestNull(TEXT("Obsolete potion type cannot offer Consume for canonical cleanser"), Consume);
    TestTrue(TEXT("Canonical status cleansing offers a real combat hotbar binding"),
        Hotbar && Hotbar->bEnabled);
    TestTrue(TEXT("Canonical status use can bind without legacy source"),
        F.Inventory->SetCharacterCombatHotbarBindingFromItem(
            0, 4, Slot->Item, EGridEquipmentSlot::None));
    FGridCombatHotbarBinding Binding;
    if (TestTrue(TEXT("Hotbar binding stored"), F.Inventory->GetCharacterCombatHotbarBinding(0, 4, Binding)))
        TestEqual(TEXT("Exact canonical ActionId used"), Binding.ActionId, ARMCLEAN063AY::ActionId());

    const auto Use = FGridQuickItemUseReadModel::Resolve(F.Registry, Slot->Item);
    if (!TestTrue(TEXT("Canonical combat action resolved"), Use.bResolved)) return false;
    UGridStatusEffectDefinitionAsset* Status = NewObject<UGridStatusEffectDefinitionAsset>(F.World);
    Status->EffectId = ARMCLEAN063AY::StatusId();
    Status->Disposition = EGridStatusEffectDisposition::Debuff;
    FGridStatusEffectRuntimeState State;
    State.EffectId = Status->EffectId;
    State.DefinitionAsset = Status;
    State.RemainingDuration = 2;
    State.StackCount = 1;
    State.Potency = 1;
    FGridCombatActionContribution Contribution;
    Contribution.Definition = Use.Action;
    Contribution.SourceDefinitionId = ARMCLEAN063AY::ItemId();
    Contribution.AvailableSourceQuantity = 2;
    if (!TestTrue(TEXT("Projected action is a valid combat contribution"), Contribution.IsValid()))
        return false;

    FGridCombatActionCatalogContext Catalogue;
    Catalogue.CharacterIndex = 0;
    Catalogue.CharacterId = Character.CharacterId;
    Catalogue.bCombatActive = true;
    Catalogue.bActiveCombatant = true;
    Catalogue.bEnableQuickItemExecutors = true;
    Catalogue.CurrentHealth = 25;
    Catalogue.MaximumHealth = 30;
    Catalogue.RemainingActionPoints = 2;
    Catalogue.CurrentMana = 0;
    Catalogue.MaximumMana = 0;
    Catalogue.CurrentStatusEffects.ActiveEffects.Add(State);
    TArray<FName> Matched;
    FGridCombatTargetingResolver::CollectStatusRemovalIds(
        Catalogue.CurrentStatusEffects, Use.Action.StatusRemovals, Matched);
    TestEqual(TEXT("Actual combat resolver recognizes one exact status"), Matched.Num(), 1);
    if (Matched.Num() == 1)
        TestEqual(TEXT("Matched exact StatusId"), Matched[0], ARMCLEAN063AY::StatusId());
    TArray<FGridCombatActionContribution> Input;
    Input.Add(Contribution);
    TArray<FGridAvailableCombatAction> Available;
    FGridCombatActionCatalog::Build(Catalogue, Input, Available);
    TestEqual(TEXT("One real quick-item combat action"), Available.Num(), 1);
    if (Available.Num() == 1)
        TestTrue(TEXT("Status-present action is enabled"), Available[0].bEnabled);

    Catalogue.CurrentStatusEffects.ActiveEffects.Reset();
    Matched.Reset();
    FGridCombatTargetingResolver::CollectStatusRemovalIds(
        Catalogue.CurrentStatusEffects, Use.Action.StatusRemovals, Matched);
    TestTrue(TEXT("No status means no candidate to remove"), Matched.IsEmpty());
    Available.Reset();
    FGridCombatActionCatalog::Build(Catalogue, Input, Available);
    if (TestEqual(TEXT("No-status catalogue still describes the action"), Available.Num(), 1))
    {
        TestFalse(TEXT("No-status action is disabled before consuming item"), Available[0].bEnabled);
        TestEqual(TEXT("No applicable effect is the exact reason"),
            Available[0].AvailabilityReason,
            EGridCombatActionAvailabilityReason::NoApplicableEffect);
    }

    // The exploration self-heal executor is deliberately NOT invoked: status
    // cleansing has no safe exploration lifecycle transaction in this milestone.
    TestEqual(TEXT("No item was spent merely by projecting the status action"),
        F.Inventory->CountItemDefinitionInCharacterInventory(0, ARMCLEAN063AY::ItemId()), 2);
    return !HasAnyErrors();
}

#endif
