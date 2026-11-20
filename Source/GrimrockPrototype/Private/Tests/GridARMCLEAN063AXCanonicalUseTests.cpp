#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridCanonicalQuickItemUseReadModel.h"

namespace ARMCLEAN063AX
{
    struct FFixture
    {
        UWorld* World = nullptr;
        UGameInstance* GI = nullptr;
        AGrimrockPartyPawn* Pawn = nullptr;
        UGridPartyInventoryComponent* Inventory = nullptr;
        UGridItemSubsystem* Registry = nullptr;
        UGridConsumableDefinitionAsset* Consumable = nullptr;
        UGridItemSystemAsset* System = nullptr;
        static FName ItemId() { return TEXT("Item_ARM063AX_SyntheticHeal"); }
        static FName ActionId() { return TEXT("Use_ARM063AX_SyntheticHeal"); }

        FFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM063AX_%s"),
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

            // Only the isolated test GI's registry is replaced. The genuine
            // production catalogues remain referenced; no .uasset is edited.
            UGridItemSystemAsset* Production = Registry->LoadConfiguredItemSystemForPreview();
            if (!Production || !Production->ActionCatalogue ||
                !Production->ActionPresentationCatalogue ||
                Production->ActionPresentationCatalogue->Presentations.IsEmpty()) return;

            System = DuplicateObject<UGridItemSystemAsset>(Production, GetTransientPackage());
            System->ActionCatalogue =
                DuplicateObject<UGridActionCatalogueAsset>(Production->ActionCatalogue, System);
            Consumable = NewObject<UGridConsumableDefinitionAsset>(System);
            Consumable->ItemId = ItemId();
            Consumable->DisplayName = FText::FromString(TEXT("Potion de test canonique"));
            Consumable->WeightKg = 0.2f;
            Consumable->MaxStack = 5;
            FGridItemActionBinding Binding;
            Binding.ActionId = ActionId();
            Binding.PresentationId =
                Production->ActionPresentationCatalogue->Presentations[0].PresentationId;
            Consumable->UseActions.Add(Binding);
            System->ItemDefinitions.Add(Consumable);
            FGridCanonicalAction Heal;
            Heal.ActionId = ActionId();
            Heal.Kind = EGridCanonicalActionKind::Use;
            Heal.Targeting = EGridCombatTargetingPolicy::Self;
            Heal.SourceItemQuantityCost = 1;
            FGridCanonicalActionEffect Effect;
            Effect.Kind = EGridCanonicalEffectKind::Heal;
            Effect.Magnitude = 8;
            Heal.Effects.Add(Effect);
            System->ActionCatalogue->Actions.Add(Heal);
            Registry->Deinitialize();
            TArray<FString> Errors;
            if (!Registry->RegisterItemSystem(System, Errors)) return;

            Pawn = World->SpawnActor<AGrimrockPartyPawn>();
            if (!Pawn) return;
            Inventory = Pawn->PartyInventoryComponent.Get();
            if (!Inventory) return;
            Inventory->InitializeDefaultPartyIfNeeded();
            Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots.SetNum(5);
            // An uncreated default character has only 1 maximum HP by design
            // (FRPGDerivedStats), so it cannot meaningfully test healing.
            // Supply a valid completed-character health capacity without
            // modifying any production rule or data asset.
            Inventory->PartyInventoryState.ActiveCharacters[0].DerivedStats.MaxHealth = 30;
            Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentHealth = 30;
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
            return Registry && Consumable && Inventory && Inventory->IsValidCharacterIndex(0) &&
                Registry->HasCanonicalItem(ItemId());
        }

        FGridItemInstance Make(int32 Level = 0, int32 Quantity = 1) const
        {
            FGridItemInstance I;
            I.RuntimeObjectId = FGuid::NewGuid();
            I.ItemDefinitionId = ItemId();
            I.ItemLevel = Level;
            I.Quantity = Quantity;
            I.OwnerType = EGridItemOwnerType::World;
            I.Weight = 900.0f; // Deliberately stale saved weight.
            return I;
        }
    };
}

namespace ARMCLEAN063AX
{
    bool RunStage(FAutomationTestBase& Test, const TCHAR* Stage)
    {
    FFixture F;
    if (!Test.TestTrue(TEXT("Isolated registered system with genuine catalogues"), F.Ready())) return false;
    if (FCString::Strcmp(Stage, TEXT("Registry")) == 0) return !Test.HasAnyErrors();

    FGridResolvedItem ResolvedItem;
    if (!Test.TestTrue(TEXT("Canonical N0 consumable resolves"),
        F.Registry->ResolveItem(ARMCLEAN063AX::FFixture::ItemId(), 0, ResolvedItem))) return false;
    Test.TestEqual(TEXT("Canonical stack policy kept"), ResolvedItem.MaxStack, 5);
    const auto View = FGridCanonicalQuickItemUseReadModel::Resolve(F.Registry, F.Make());
    Test.TestTrue(TEXT("One registered Use/Heal action resolves"), View.bCanonicalIdentity && View.bResolved);
    Test.TestTrue(TEXT("Self heal with zero mana/cooldown is usable in exploration"), View.bExplorationCompatible);
    Test.TestEqual(TEXT("Exact authored magnitude"), View.Action.EffectProfile.RestoreHealth, 8);
    Test.TestEqual(TEXT("Actual ActionId, no old Use_* identity"), View.Action.ActionId,
        ARMCLEAN063AX::FFixture::ActionId());
    Test.TestEqual(TEXT("Exactly one source item is spent"), View.Action.ResourceCosts.SourceItemQuantityCost, 1);
    Test.TestEqual(TEXT("No fabricated AP cost"), View.Action.ActionPointCost, 0);
    Test.TestTrue(TEXT("Transient action is accepted by the existing combat executor"), View.Action.IsValid());
    if (FCString::Strcmp(Stage, TEXT("Projection")) == 0) return !Test.HasAnyErrors();

    const FGridItemInstance Incoming = F.Make(0, 2);
    if (!Test.TestTrue(TEXT("Canonical consumable enters real inventory without legacy"),
        F.Inventory->AddItemToCharacterInventory(0, Incoming))) return false;
    FGridCharacterInventoryState& Character = F.Inventory->PartyInventoryState.ActiveCharacters[0];
    FGridInventorySlot* Slot = Character.InventorySlots.FindByPredicate(
        [](const FGridInventorySlot& Entry)
        {
            return !Entry.IsEmpty() &&
                Entry.Item.ItemDefinitionId == ARMCLEAN063AX::FFixture::ItemId();
        });
    if (!Test.TestNotNull(TEXT("Physical stack exists"), Slot)) return false;
    Test.TestEqual(TEXT("Quantity stays two"), Slot->Item.Quantity, 2);
    Test.TestEqual(TEXT("Slot has no obsolete cached weight"), Slot->Item.Weight, 0.0f);
    if (FCString::Strcmp(Stage, TEXT("Inventory")) == 0) return !Test.HasAnyErrors();

    Character.Resources.CurrentHealth = 1;
    FGridInventoryCharacterSummary Summary;
    if (!Test.TestTrue(TEXT("Character summary available"), F.Inventory->GetCharacterSummary(0, Summary))) return false;
    if (!Test.TestTrue(TEXT("Character has available health to restore"), Summary.DerivedStats.MaxHealth > 1)) return false;

    FGridItemActionContext Ctx;
    Ctx.PartyPawn = F.Pawn;
    Ctx.CharacterIndex = 0;
    Ctx.InventorySlotIndex = 0;
    Ctx.Item = Slot->Item;
    FGridFacingTargetContext Facing;
    TArray<FGridItemContextAction> Actions;
    if (!Test.TestTrue(TEXT("Canonical context actions build"),
        UGridItemContextActionLibrary::BuildItemContextActions(Ctx, Facing, Actions))) return false;
    const FGridItemContextAction* Consume = Actions.FindByPredicate(
        [](const FGridItemContextAction& A) { return A.ActionType == EGridItemActionType::Consume; });
    const FGridItemContextAction* Hotbar = Actions.FindByPredicate(
        [](const FGridItemContextAction& A) { return A.ActionType == EGridItemActionType::AddToHotbar; });
    Test.TestTrue(TEXT("Self-heal action is enabled in exploration"), Consume && Consume->bEnabled);
    Test.TestTrue(TEXT("Canonical consumable is available for hotbar"), Hotbar && Hotbar->bEnabled);
    if (FCString::Strcmp(Stage, TEXT("Context")) == 0) return !Test.HasAnyErrors();

    Test.TestTrue(TEXT("Real inventory hotbar binds exact canonical ActionId"),
        F.Inventory->SetCharacterCombatHotbarBindingFromItem(
            0, 3, Slot->Item, EGridEquipmentSlot::None));
    FGridCombatHotbarBinding Binding;
    if (Test.TestTrue(TEXT("Quick-item binding readable"),
        F.Inventory->GetCharacterCombatHotbarBinding(0, 3, Binding)))
    {
        Test.TestEqual(TEXT("Exact canonical action identity"), Binding.ActionId,
            ARMCLEAN063AX::FFixture::ActionId());
        Test.TestFalse(TEXT("Canonical self-heal is not a throw shortcut"),
            Binding.IsPhysicalThrowItemBinding());
    }

    if (FCString::Strcmp(Stage, TEXT("Hotbar")) == 0) return !Test.HasAnyErrors();
    const FGuid SourceGuid = Slot->Item.RuntimeObjectId;
    FText Error;
    Test.TestTrue(TEXT("Existing exploration executor consumes canonical Heal"),
        F.Inventory->TryConsumeSelfRestorativeQuickItem(0, SourceGuid, Error));
    Test.TestEqual(TEXT("Exactly one item consumed"),
        F.Inventory->CountItemDefinitionInCharacterInventory(0, ARMCLEAN063AX::FFixture::ItemId()), 1);
    Test.TestEqual(TEXT("Health increased by authored amount"), Character.Resources.CurrentHealth,
        FMath::Min(Summary.DerivedStats.MaxHealth, 9));

    if (FCString::Strcmp(Stage, TEXT("Exploration")) == 0) return !Test.HasAnyErrors();
    const FGridItemInstance Invalid = F.Make(21);
    const auto Bad = FGridCanonicalQuickItemUseReadModel::Resolve(F.Registry, Invalid);
    Test.TestTrue(TEXT("Invalid known ID is still canonical"), Bad.bCanonicalIdentity);
    Test.TestFalse(TEXT("Invalid N21 has no executable action"), Bad.bResolved);

    FGridResolvedBoundAction Bound;
    if (!Test.TestTrue(TEXT("Synthetic binding is checked against real registry"),
        F.Registry->ResolveBoundAction(ARMCLEAN063AX::FFixture::ItemId(), 0,
            ARMCLEAN063AX::FFixture::ActionId(), Bound))) return false;
    Bound.Action.Effects[0].Kind = EGridCanonicalEffectKind::Teleport;
    FGridCombatActionDefinition Unsupported;
    Test.TestFalse(TEXT("Unsupported teleport is rejected, not converted into a heal"),
        FGridCanonicalQuickItemUseReadModel::TryProjectSingleHeal(ResolvedItem, Bound, Unsupported));
    Bound.Action.Effects[0].Kind = EGridCanonicalEffectKind::Heal;
    Bound.Action.Effects[0].Magnitude = 0;
    Test.TestFalse(TEXT("Zero-magnitude heal rejected"),
        FGridCanonicalQuickItemUseReadModel::TryProjectSingleHeal(ResolvedItem, Bound, Unsupported));
    Bound.Action.Effects[0].Magnitude = 8;
    Bound.Action.Targeting = EGridCombatTargetingPolicy::Area;
    Test.TestFalse(TEXT("Area action cannot masquerade as self-restoration"),
        FGridCanonicalQuickItemUseReadModel::TryProjectSingleHeal(ResolvedItem, Bound, Unsupported));
    return !Test.HasAnyErrors();
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AXRegistration,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalSelfHealUseActions.Registry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean063AXRegistration::RunTest(const FString&)
{
    return ARMCLEAN063AX::RunStage(*this, TEXT("Registry"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AXProjection,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalSelfHealUseActions.Projection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean063AXProjection::RunTest(const FString&)
{
    return ARMCLEAN063AX::RunStage(*this, TEXT("Projection"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AXInventory,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalSelfHealUseActions.Inventory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean063AXInventory::RunTest(const FString&)
{
    return ARMCLEAN063AX::RunStage(*this, TEXT("Inventory"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AXContext,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalSelfHealUseActions.Context",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean063AXContext::RunTest(const FString&)
{
    return ARMCLEAN063AX::RunStage(*this, TEXT("Context"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AXHotbar,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalSelfHealUseActions.Hotbar",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean063AXHotbar::RunTest(const FString&)
{
    return ARMCLEAN063AX::RunStage(*this, TEXT("Hotbar"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AXExploration,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalSelfHealUseActions.Exploration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean063AXExploration::RunTest(const FString&)
{
    return ARMCLEAN063AX::RunStage(*this, TEXT("Exploration"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AXRejections,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalSelfHealUseActions.Rejections",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGridClean063AXRejections::RunTest(const FString&)
{
    return ARMCLEAN063AX::RunStage(*this, TEXT("Rejections"));
}
#endif
