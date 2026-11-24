#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "RPG/StatusEffects/GridCombatStatusApplicationResolver.h"
#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridItemInteractionReadModel.h"
#include "../Runtime/GridQuickItemUseReadModel.h"

namespace ARMCLEAN063AZ
{
    static FName ItemId() { return TEXT("Item_ARM063AZ_StatusBuff"); }
    static FName ActionId() { return TEXT("Use_ARM063AZ_StatusBuff"); }
    static FName StatusId() { return TEXT("Status_ARM063AZ_Aegis"); }

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
                FName(*FString::Printf(TEXT("ARM063AZ_%s"),
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
            Item->DisplayName = FText::FromString(TEXT("AX/AY diagnostic buff"));
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
            Effect.Kind = EGridEffectKind::ApplyStatus;
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


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AZStatusProjection,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalUseApplyStatus.ProjectionAndRejections",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean063AZStatusProjection::RunTest(const FString&)
{
    ARMCLEAN063AZ::FFixture F;
    if (!TestTrue(TEXT("Synthetic registered ApplyStatus Use action"), F.Ready())) return false;

    FGridResolvedItem Item;
    if (!TestTrue(TEXT("Canonical N0 consumable resolves"),
        F.Registry->ResolveItem(ARMCLEAN063AZ::ItemId(), 0, Item))) return false;
    FGridResolvedBoundAction Bound;
    if (!TestTrue(TEXT("Authored ActionId and PresentationId resolve"),
        F.Registry->ResolveBoundAction(ARMCLEAN063AZ::ItemId(), 0,
            ARMCLEAN063AZ::ActionId(), Bound))) return false;
    const auto View = FGridQuickItemUseReadModel::Resolve(F.Registry, F.Make());
    if (!TestTrue(TEXT("Registered self-status action is projected"), View.bRegisteredIdentity && View.bResolved))
        return false;
    TestTrue(TEXT("Projected action accepted by the combat schema"), View.Action.IsValid());
    TestEqual(TEXT("Authoritative ActionId"), View.Action.ActionId, ARMCLEAN063AZ::ActionId());
    TestEqual(TEXT("Exactly one physical item cost"), View.Action.ResourceCosts.SourceItemQuantityCost, 1);
    TestEqual(TEXT("No AP cost invented"), View.Action.ActionPointCost, 0);
    TestFalse(TEXT("No exploration status application"), View.bExplorationCompatible);
    TestEqual(TEXT("No fabricated direct healing"), View.Action.EffectProfile.RestoreHealth, 0);
    TestTrue(TEXT("No fabricated status removal"), View.Action.StatusRemovals.IsEmpty());
    if (TestEqual(TEXT("Exactly one status application"), View.Action.StatusApplications.Num(), 1))
    {
        const auto& Application = View.Action.StatusApplications[0];
        TestEqual(TEXT("Exact authored status identity"), Application.StatusEffectId, ARMCLEAN063AZ::StatusId());
        TestEqual(TEXT("One initial stack"), Application.InitialStackCount, 1);
        TestEqual(TEXT("Default duration belongs to status definition"), Application.DurationOverride, INDEX_NONE);
        TestEqual(TEXT("Default potency belongs to status definition"), Application.PotencyOverride, INDEX_NONE);
        TestEqual(TEXT("Apply after successful action resolution"),
            Application.Trigger, EGridCombatStatusApplicationTrigger::AfterResolution);
        TestEqual(TEXT("No inferred armor gate"), Application.ArmorGate, EGridCombatStatusArmorGate::None);
    }

    const auto Capabilities = FGridItemInteractionReadModel::Resolve(F.Registry, F.Make());
    TestTrue(TEXT("Combat quick-item hotbar accepts registered ApplyStatus"), Capabilities.bCanAddToHotbar);
    TestFalse(TEXT("Combat-only status does not masquerade as inline Consume"), Capabilities.bCanConsumeItem);
    TestFalse(TEXT("Status item is not a throwing weapon"), Capabilities.bPhysicalThrowHotbar);

    const auto Try = [&Item](const FGridResolvedBoundAction& Source)
    {
        FGridCombatActionDefinition Candidate;
        return FGridQuickItemUseReadModel::TryProjectSingleStatusApplication(Item, Source, Candidate);
    };

    FGridResolvedBoundAction Invalid = Bound;
    Invalid.Action.Effects[0].Magnitude = 0;
    TestFalse(TEXT("Zero stacks fail closed"), Try(Invalid));
    Invalid = Bound;
    Invalid.Action.Effects[0].Magnitude = 2;
    TestFalse(TEXT("Ambiguous number of stacks fails closed"), Try(Invalid));
    Invalid = Bound;
    Invalid.Action.Effects[0].StatusId = NAME_None;
    TestFalse(TEXT("Status identity is required"), Try(Invalid));
    Invalid = Bound;
    Invalid.Action.Targeting = EGridCombatTargetingPolicy::Ally;
    TestTrue(TEXT("BA: explicit single Ally is now supported"), Try(Invalid));
    Invalid = Bound;
    Invalid.Action.Targeting = EGridCombatTargetingPolicy::AllyOrHostile;
    TestFalse(TEXT("AllyOrHostile target remains forbidden"), Try(Invalid));
    Invalid = Bound;
    Invalid.Action.Targeting = EGridCombatTargetingPolicy::Party;
    TestFalse(TEXT("Party target is not silently authorized"), Try(Invalid));
    Invalid = Bound;
    Invalid.Action.Targeting = EGridCombatTargetingPolicy::Hostile;
    TestFalse(TEXT("Hostile target is never inferred"), Try(Invalid));
    Invalid = Bound;
    Invalid.Action.SourceItemQuantityCost = 0;
    TestFalse(TEXT("Unpaid status application disallowed"), Try(Invalid));
    Invalid = Bound;
    Invalid.Action.Effects[0].Kind = EGridEffectKind::RemoveStatus;
    TestFalse(TEXT("AY RemoveStatus cannot be misclassified"), Try(Invalid));
    Invalid = Bound;
    Invalid.Action.Effects[0].Kind = EGridEffectKind::Heal;
    TestFalse(TEXT("AX Heal cannot be misclassified"), Try(Invalid));
    Invalid = Bound;
    const FGridActionEffect Copy = Invalid.Action.Effects[0];
    Invalid.Action.Effects.Add(Copy); // Copy before TArray mutation: no self-append reference.
    TestFalse(TEXT("Multiple effects remain unsupported"), Try(Invalid));
    Invalid = Bound;
    Invalid.ItemLevel = 21;
    TestFalse(TEXT("Item level cannot be forged"), Try(Invalid));

    const auto KnownInvalid = FGridQuickItemUseReadModel::Resolve(F.Registry, F.Make(21));
    TestTrue(TEXT("Invalid registered identity is recognized"), KnownInvalid.bRegisteredIdentity);
    TestFalse(TEXT("No fallback on invalid item level"), KnownInvalid.bResolved);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AZStatusCombat,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalUseApplyStatus.CombatCatalogueAndHotbar",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean063AZStatusCombat::RunTest(const FString&)
{
    ARMCLEAN063AZ::FFixture F;
    if (!TestTrue(TEXT("Synthetic canonical ApplyStatus registered"), F.Ready())) return false;

    if (!TestTrue(TEXT("Canonical consumable materializes in real inventory"),
        F.Inventory->AddItemToCharacterInventory(0, F.Make()))) return false;
    const auto& Character = F.Inventory->PartyInventoryState.ActiveCharacters[0];
    const FGridInventorySlot* Slot = Character.InventorySlots.FindByPredicate(
        [](const FGridInventorySlot& Entry)
        {
            return !Entry.IsEmpty() && Entry.Item.ItemDefinitionId == ARMCLEAN063AZ::ItemId();
        });
    if (!TestNotNull(TEXT("Canonical physical stack exists"), Slot)) return false;

    UGridItemDefinitionAsset* Contradictory = NewObject<UGridItemDefinitionAsset>(F.Inventory);
    Contradictory->ItemDefinitionId = ARMCLEAN063AZ::ItemId();
    Contradictory->ItemType = EGridItemType::Potion;
    FGridItemActionContext Context;
    Context.PartyPawn = F.Pawn;
    Context.ItemDefinition = Contradictory;
    Context.Item = Slot->Item;
    Context.InventorySlotIndex = 0;
    Context.CharacterIndex = 0;
    FGridFacingTargetContext Facing;
    TArray<FGridItemContextAction> Actions;
    TestTrue(TEXT("Canonical context actions build despite contradictory legacy type"),
        UGridItemContextActionLibrary::BuildItemContextActions(Context, Facing, Actions));
    const FGridItemContextAction* Consume = Actions.FindByPredicate(
        [](const FGridItemContextAction& A) { return A.ActionType == EGridItemActionType::Consume; });
    const FGridItemContextAction* Hotbar = Actions.FindByPredicate(
        [](const FGridItemContextAction& A) { return A.ActionType == EGridItemActionType::AddToHotbar; });
    TestNull(TEXT("No inline consumption for combat-only status buff"), Consume);
    TestTrue(TEXT("Actual canonical status usage offers hotbar"), Hotbar && Hotbar->bEnabled);

    TestTrue(TEXT("Hotbar binds actual item and canonical action without old asset"),
        F.Inventory->SetCharacterCombatHotbarBindingFromItem(0, 5, Slot->Item, EGridEquipmentSlot::None));
    FGridCombatHotbarBinding Binding;
    if (TestTrue(TEXT("Hotbar slot remains bound"), F.Inventory->GetCharacterCombatHotbarBinding(0, 5, Binding)))
    {
        TestEqual(TEXT("ActionId preserved, no old QuickItem shadow"), Binding.ActionId, ARMCLEAN063AZ::ActionId());
        TestFalse(TEXT("Status buff is not physical throw"), Binding.IsPhysicalThrowItemBinding());
    }

    const auto Use = FGridQuickItemUseReadModel::Resolve(F.Registry, Slot->Item);
    if (!TestTrue(TEXT("Canonical usage resolved for actual stored item"), Use.bResolved)) return false;

    // The real status resolver must locate a valid, independent status asset.
    // No extra status duration/potency is copied into DA_ItemSystem.
    UGridStatusEffectDefinitionAsset* Status = NewObject<UGridStatusEffectDefinitionAsset>(F.World);
    Status->EffectId = ARMCLEAN063AZ::StatusId();
    Status->DisplayName = FText::FromString(TEXT("Synthetic protection buff"));
    Status->Disposition = EGridStatusEffectDisposition::Buff;
    Status->DefaultDuration = 3;
    Status->DefaultPotency = 4;
    Status->StackPolicy = EGridStatusEffectStackPolicy::NoStack;
    if (!TestTrue(TEXT("Authoritative status asset passes validation"), Status->IsValidDefinition())) return false;
    TestTrue(TEXT("Status lookup resolves typed asset, not a legacy item"),
        FGridCombatStatusApplicationResolver::ResolveDefinition(ARMCLEAN063AZ::StatusId()) == Status);

    FGridCombatActionContribution Contribution;
    Contribution.Definition = Use.Action;
    Contribution.SourceDefinitionId = ARMCLEAN063AZ::ItemId();
    Contribution.AvailableSourceQuantity = 2;
    if (!TestTrue(TEXT("Real QuickItem combat contribution is valid"), Contribution.IsValid())) return false;

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
    FGridAttackTargetStats TargetBefore;
    TargetBefore.CurrentHealth = 25;
    const bool Before = FGridCombatStatusApplicationResolver::WouldAnyMutate(
        Use.Action.StatusApplications, Character.CharacterId, TargetBefore, nullptr,
        Catalogue.CurrentStatusEffects);
    TestTrue(TEXT("Status is applicable before first use"), Before);
    TArray<FGridCombatActionContribution> Input;
    Input.Add(Contribution);
    TArray<FGridAvailableCombatAction> Available;
    FGridCombatActionCatalog::Build(Catalogue, Input, Available);
    if (TestEqual(TEXT("Exactly one self-target QuickItem action"), Available.Num(), 1))
    {
        TestTrue(TEXT("Combat action available for first application"), Available[0].bEnabled);
        TestEqual(TEXT("Combat targeting is strictly Self"), Available[0].Definition.TargetingPolicy,
            EGridCombatTargetingPolicy::Self);
    }

    FGridStatusEffectApplyResult Applied;
    FString Error;
    TestTrue(TEXT("Actual status stack policy applies once to copied state"),
        Catalogue.CurrentStatusEffects.TryApply(*Status, Character.CharacterId,
            1, INDEX_NONE, INDEX_NONE, Applied, Error));
    TestTrue(TEXT("Initial status application changes state"), Applied.DidMutate());
    TestTrue(TEXT("Status now already present"), Catalogue.CurrentStatusEffects.ActiveEffects.Num() == 1);
    const bool After = FGridCombatStatusApplicationResolver::WouldAnyMutate(
        Use.Action.StatusApplications, Character.CharacterId, TargetBefore, nullptr,
        Catalogue.CurrentStatusEffects);
    TestFalse(TEXT("NoStack makes duplicate application useless"), After);
    Available.Reset();
    FGridCombatActionCatalog::Build(Catalogue, Input, Available);
    if (TestEqual(TEXT("Catalogue retains unavailable action without deleting binding"), Available.Num(), 1))
    {
        TestFalse(TEXT("No-op status use disabled before item is consumed"), Available[0].bEnabled);
        TestEqual(TEXT("NoApplicableEffect is precise refusal"), Available[0].AvailabilityReason,
            EGridCombatActionAvailabilityReason::NoApplicableEffect);
    }

    TestEqual(TEXT("Availability preview cannot spend or duplicate item"),
        F.Inventory->CountItemDefinitionInCharacterInventory(0, ARMCLEAN063AZ::ItemId()), 2);
    return !HasAnyErrors();
}
#endif
