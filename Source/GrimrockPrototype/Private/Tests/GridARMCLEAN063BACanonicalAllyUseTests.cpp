#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "RPG/StatusEffects/GridCombatStatusApplicationResolver.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/Combat/GridCombatTargetingResolver.h"
#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridItemInteractionReadModel.h"
#include "../Runtime/GridQuickItemUseReadModel.h"

namespace ARMCLEAN063BA
{
    static FName ItemId() { return TEXT("Item_ARM063BA_AllyUse"); }
    static FName ActionId() { return TEXT("Use_ARM063BA_AllyUse"); }
    static FName StatusId() { return TEXT("Status_ARM063BA_AllyEffect"); }

    struct FFixture
    {
        UWorld* World = nullptr;
        UGameInstance* GI = nullptr;
        UGridItemSubsystem* Registry = nullptr;
        UGridItemSystemAsset* System = nullptr;
        UGridConsumableDefinitionAsset* Item = nullptr;
        UGridStatusEffectDefinitionAsset* Status = nullptr;
        AGrimrockPartyPawn* Pawn = nullptr;
        UGridPartyInventoryComponent* Inventory = nullptr;

        explicit FFixture(EGridEffectKind Kind)
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM063BA_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (!World || !GEngine) return;
            FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
            WorldContext.SetCurrentWorld(World);
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
            Item->DisplayName = FText::FromString(TEXT("BA synthetic ally item"));
            Item->WeightKg = 0.2f;
            Item->MaxStack = 4;
            FGridItemActionBinding Binding;
            Binding.ActionId = ActionId();
            Binding.PresentationId = Production->ActionPresentationCatalogue->Presentations[0].PresentationId;
            Item->UseActions.Add(Binding);
            System->ItemDefinitions.Add(Item);
            FGridAction Action;
            Action.ActionId = ActionId();
            Action.Kind = EGridActionKind::Use;
            Action.Targeting = EGridCombatTargetingPolicy::Ally;
            Action.SourceItemQuantityCost = 1;
            FGridActionEffect Effect;
            Effect.Kind = Kind;
            Effect.Magnitude = Kind == EGridEffectKind::Heal ? 8 : 1;
            Effect.StatusId = Kind == EGridEffectKind::Heal ? NAME_None : StatusId();
            Action.Effects.Add(Effect);
            System->ActionCatalogue->Actions.Add(Action);
            Registry->Deinitialize();
            TArray<FString> Errors;
            if (!Registry->RegisterItemSystem(System, Errors)) return;

            Pawn = World->SpawnActor<AGrimrockPartyPawn>();
            if (!Pawn) return;
            Inventory = Pawn->PartyInventoryComponent.Get();
            if (!Inventory) return;
            Inventory->InitializeDefaultPartyIfNeeded();
            FGridCharacterInventoryState& First = Inventory->PartyInventoryState.ActiveCharacters[0];
            First.DerivedStats.MaxHealth = 30;
            First.Resources.CurrentHealth = 30; // Important: caster is FULL.
            First.InventorySlots.SetNum(6);

            FGridCharacterInventoryState Ally = First;
            Ally.CharacterId = FGuid::NewGuid();
            Ally.DisplayName = FText::FromString(TEXT("Synthetic target"));
            Ally.Resources.CurrentHealth = 10;
            Ally.InventorySlots.Reset();
            Ally.InventorySlots.SetNum(6);
            Inventory->PartyInventoryState.ActiveCharacters.Add(Ally);
            if (!Inventory->PartyInventoryState.ActiveEquipment.IsEmpty())
            {
                const auto Equipment = Inventory->PartyInventoryState.ActiveEquipment[0];
                Inventory->PartyInventoryState.ActiveEquipment.Add(Equipment);
            }

            Status = NewObject<UGridStatusEffectDefinitionAsset>(World);
            Status->EffectId = StatusId();
            Status->DisplayName = FText::FromString(TEXT("Synthetic ally status"));
            Status->Disposition = EGridStatusEffectDisposition::Buff;
            Status->DefaultDuration = 3;
            Status->DefaultPotency = 4;
            Status->StackPolicy = EGridStatusEffectStackPolicy::NoStack;
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
            return Registry && Item && Inventory && Status &&
                Inventory->IsValidCharacterIndex(0) &&
                Inventory->IsValidCharacterIndex(1) && Registry->ContainsItem(ItemId());
        }

        FGridItemInstance Make(int32 Level = 0) const
        {
            FGridItemInstance Source;
            Source.RuntimeObjectId = FGuid::NewGuid();
            Source.ItemDefinitionId = ItemId();
            Source.ItemLevel = Level;
            Source.Quantity = 2;
            Source.OwnerType = EGridItemOwnerType::World;
            return Source;
        }
    };

    static bool RunScenario(FAutomationTestBase& Test, EGridEffectKind Kind)
    {
        FFixture F(Kind);
        if (!Test.TestTrue(TEXT("Real isolated registry, two party characters and status asset"), F.Ready()))
            return false;
        FGridResolvedItem Item;
        if (!Test.TestTrue(TEXT("Canonical non-ARM N0 item resolves"),
            F.Registry->ResolveItem(ItemId(), 0, Item))) return false;
        FGridResolvedBoundAction Bound;
        if (!Test.TestTrue(TEXT("Use binding resolves through DA_ItemSystem"),
            F.Registry->ResolveBoundAction(ItemId(), 0, ActionId(), Bound))) return false;
        const FGridQuickItemUseReadView View =
            FGridQuickItemUseReadModel::Resolve(F.Registry, F.Make());
        if (!Test.TestTrue(TEXT("Explicit Ally action resolves"), View.bRegisteredIdentity && View.bResolved))
            return false;
        Test.TestTrue(TEXT("Projection accepted by existing QuickItem schema"), View.Action.IsValid());
        Test.TestFalse(TEXT("Ally action is never an exploration self-consume"), View.bExplorationCompatible);
        Test.TestEqual(TEXT("True targeting policy is Ally"), View.Action.TargetingPolicy,
            EGridCombatTargetingPolicy::Ally);
        Test.TestEqual(TEXT("Exactly one resolved target"), View.Action.MaximumResolvedTargets, 1);
        Test.TestEqual(TEXT("Actual canonical ActionId preserved"), View.Action.ActionId, ActionId());
        Test.TestEqual(TEXT("Exactly one item spent if executed"), View.Action.ResourceCosts.SourceItemQuantityCost, 1);
        Test.TestEqual(TEXT("No invented AP cost"), View.Action.ActionPointCost, 0);
        Test.TestEqual(TEXT("No fabricated mana cost"), View.Action.ResourceCosts.ManaCost, 0);
        Test.TestEqual(TEXT("Single authored effect survives"), Bound.Action.Effects.Num(), 1);
        if (Kind == EGridEffectKind::Heal)
        {
            Test.TestEqual(TEXT("Authored heal magnitude"), View.Action.EffectProfile.RestoreHealth, 8);
            Test.TestTrue(TEXT("No added statuses"), View.Action.StatusApplications.IsEmpty());
            Test.TestTrue(TEXT("No removed statuses"), View.Action.StatusRemovals.IsEmpty());
        }
        else if (Kind == EGridEffectKind::ApplyStatus)
        {
            if (Test.TestEqual(TEXT("One status application"), View.Action.StatusApplications.Num(), 1))
            {
                const auto& Applied = View.Action.StatusApplications[0];
                Test.TestEqual(TEXT("Exact status identity"), Applied.StatusEffectId, StatusId());
                Test.TestEqual(TEXT("One stack"), Applied.InitialStackCount, 1);
                Test.TestEqual(TEXT("Definition owns duration"), Applied.DurationOverride, INDEX_NONE);
                Test.TestEqual(TEXT("Definition owns potency"), Applied.PotencyOverride, INDEX_NONE);
            }
        }
        else if (Kind == EGridEffectKind::RemoveStatus)
        {
            if (Test.TestEqual(TEXT("One status removal"), View.Action.StatusRemovals.Num(), 1))
            {
                const auto& Removed = View.Action.StatusRemovals[0];
                Test.TestEqual(TEXT("Exact authored status identity"), Removed.EffectIds[0], StatusId());
                Test.TestEqual(TEXT("One removal only"), Removed.MaximumRemovals, 1);
            }
        }

        const FGridItemInteractionReadView Capabilities =
            FGridItemInteractionReadModel::Resolve(F.Registry, nullptr, F.Make());
        Test.TestFalse(TEXT("No context Consume shortcut for Ally"), Capabilities.bCanConsumeCanonical);
        Test.TestTrue(TEXT("Combat hotbar binding is authorized"), Capabilities.bCanAddToHotbar);

        FGridCombatActionDefinition Other;
        FGridResolvedBoundAction Invalid = Bound;
        Invalid.Action.Targeting = EGridCombatTargetingPolicy::Party;
        const bool bInvalidParty = Kind == EGridEffectKind::Heal
            ? FGridQuickItemUseReadModel::TryProjectSingleHeal(Item, Invalid, Other)
            : Kind == EGridEffectKind::ApplyStatus
                ? FGridQuickItemUseReadModel::TryProjectSingleStatusApplication(Item, Invalid, Other)
                : FGridQuickItemUseReadModel::TryProjectSingleStatusRemoval(Item, Invalid, Other);
        Test.TestFalse(TEXT("Party/area is not inferred as Ally"), bInvalidParty);
        const auto WrongLevel = FGridQuickItemUseReadModel::Resolve(F.Registry, F.Make(21));
        Test.TestTrue(TEXT("N21 registered identity remains known"), WrongLevel.bRegisteredIdentity);
        Test.TestFalse(TEXT("Invalid level fails closed"), WrongLevel.bResolved);

        if (!Test.TestTrue(TEXT("Real canonical item enters inventory"),
            F.Inventory->AddItemToCharacterInventory(0, F.Make()))) return false;
        const FGridCharacterInventoryState& Caster =
            F.Inventory->PartyInventoryState.ActiveCharacters[0];
        const FGridCharacterInventoryState& Target =
            F.Inventory->PartyInventoryState.ActiveCharacters[1];
        const FGridInventorySlot* Source = Caster.InventorySlots.FindByPredicate(
            [](const FGridInventorySlot& Slot)
            {
                return !Slot.IsEmpty() && Slot.Item.ItemDefinitionId == ItemId();
            });
        if (!Test.TestNotNull(TEXT("Physical source stack exists"), Source)) return false;

        // A deliberately misleading old Potion must not produce an exploration Consume.
        UGridItemDefinitionAsset* Legacy = NewObject<UGridItemDefinitionAsset>(F.Inventory);
        Legacy->ItemDefinitionId = ItemId();
        Legacy->ItemType = EGridItemType::Potion;
        FGridItemActionContext Context;
        Context.PartyPawn = F.Pawn;
        Context.ItemDefinition = Legacy;
        Context.Item = Source->Item;
        Context.CharacterIndex = 0;
        Context.InventorySlotIndex = 0;
        FGridFacingTargetContext Facing;
        TArray<FGridItemContextAction> Menu;
        Test.TestTrue(TEXT("Context menu builds"), UGridItemContextActionLibrary::BuildItemContextActions(
            Context, Facing, Menu));
        Test.TestFalse(TEXT("Misleading legacy Potion cannot grant Consume"),
            Menu.ContainsByPredicate([](const FGridItemContextAction& E)
            {
                return E.ActionType == EGridItemActionType::Consume;
            }));
        Test.TestTrue(TEXT("Canonical Ally item can be added to hotbar"),
            F.Inventory->SetCharacterCombatHotbarBindingFromItem(
                0, 6, Source->Item, EGridEquipmentSlot::None));
        FGridCombatHotbarBinding Hotbar;
        if (Test.TestTrue(TEXT("Stored binding is readable"),
            F.Inventory->GetCharacterCombatHotbarBinding(0, 6, Hotbar)))
            Test.TestEqual(TEXT("Original ActionId bound to hotbar"), Hotbar.ActionId, ActionId());

        TArray<int32> Targets;
        FGridCombatTargetingResolver::CollectPartyTargets(
            F.Inventory->PartyInventoryState, EGridCombatTargetingPolicy::Ally,
            0, 1, 3, Targets);
        Test.TestEqual(TEXT("Targeting resolver chooses one explicit ally"), Targets.Num(), 1);
        if (Targets.Num() == 1)
            Test.TestEqual(TEXT("Second member selected, not caster"), Targets[0], 1);
        FGridCombatTargetingResolver::CollectPartyTargets(
            F.Inventory->PartyInventoryState, EGridCombatTargetingPolicy::Ally,
            0, INDEX_NONE, 3, Targets);
        Test.TestTrue(TEXT("No implicit ally chosen when target is absent"), Targets.IsEmpty());

        FGridCombatActionContribution Contribution;
        Contribution.Definition = View.Action;
        Contribution.SourceDefinitionId = ItemId();
        Contribution.AvailableSourceQuantity = 2;
        if (!Test.TestTrue(TEXT("Combat contribution valid"), Contribution.IsValid())) return false;
        FGridCombatActionCatalogContext Catalogue;
        Catalogue.CharacterIndex = 0;
        Catalogue.CharacterId = Caster.CharacterId;
        Catalogue.bCombatActive = true;
        Catalogue.bActiveCombatant = true;
        Catalogue.bEnableQuickItemExecutors = true;
        Catalogue.CurrentHealth = 30;
        Catalogue.MaximumHealth = 30;
        Catalogue.RemainingActionPoints = 2;
        Catalogue.CurrentMana = 0;
        Catalogue.MaximumMana = 0;
        TArray<FGridCombatActionContribution> Contributions;
        Contributions.Add(Contribution);
        TArray<FGridAvailableCombatAction> Available;
        FGridCombatActionCatalog::Build(Catalogue, Contributions, Available);
        if (Test.TestEqual(TEXT("One valid ally action in combat catalogue"), Available.Num(), 1))
        {
            Test.TestTrue(TEXT("Full-health caster does not prevent Ally target selection"),
                Available[0].bEnabled);
        }
        if (Kind == EGridEffectKind::Heal)
        {
            Test.TestEqual(TEXT("Target, not caster, needs the healing"),
                FMath::Min(Target.DerivedStats.MaxHealth, Target.Resources.CurrentHealth +
                    View.Action.EffectProfile.RestoreHealth), 18);
        }
        else if (Kind == EGridEffectKind::ApplyStatus)
        {
            if (!Test.TestTrue(TEXT("Status definition is valid"), F.Status->IsValidDefinition())) return false;
            FGridAttackTargetStats TargetBefore;
            TargetBefore.CurrentHealth = Target.Resources.CurrentHealth;
            Test.TestTrue(TEXT("Actual status resolver accepts application on selected ally"),
                FGridCombatStatusApplicationResolver::WouldAnyMutate(
                    View.Action.StatusApplications, Caster.CharacterId,
                    TargetBefore, nullptr, Target.StatusEffects));
        }
        else
        {
            // Removal preflight reads the selected target's statuses, not the
            // caster's (who has none). This is the existing party executor.
            FGridStatusEffectRuntimeState State;
            State.EffectId = StatusId();
            State.DefinitionAsset = F.Status;
            State.StackCount = 1;
            State.Potency = 1;
            State.RemainingDuration = 2;
            FGridStatusEffectCollection TargetStatuses;
            TargetStatuses.ActiveEffects.Add(State);
            TArray<FName> Removable;
            FGridCombatTargetingResolver::CollectStatusRemovalIds(
                TargetStatuses, View.Action.StatusRemovals, Removable,
                EGridCombatStatusRemovalTargetSide::Party);
            Test.TestEqual(TEXT("Selected ally provides exactly one removable status"), Removable.Num(), 1);
            if (Removable.Num() == 1)
                Test.TestEqual(TEXT("Exact status to remove on ally"), Removable[0], StatusId());
        }

        Test.TestEqual(TEXT("Preview and selection do not consume the item"),
            F.Inventory->CountItemDefinitionInCharacterInventory(0, ItemId()), 2);
        Test.TestEqual(TEXT("Ally has no item mistakenly transferred"),
            F.Inventory->CountItemDefinitionInCharacterInventory(1, ItemId()), 0);
        return !Test.HasAnyErrors();
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063BAAllyHeal,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalAllyUse.Heal",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean063BAAllyHeal::RunTest(const FString&)
{
    return ARMCLEAN063BA::RunScenario(*this, EGridEffectKind::Heal);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063BAAllyApply,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalAllyUse.ApplyStatus",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean063BAAllyApply::RunTest(const FString&)
{
    return ARMCLEAN063BA::RunScenario(*this, EGridEffectKind::ApplyStatus);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063BAAllyRemove,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalAllyUse.RemoveStatus",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean063BAAllyRemove::RunTest(const FString&)
{
    return ARMCLEAN063BA::RunScenario(*this, EGridEffectKind::RemoveStatus);
}

#endif
