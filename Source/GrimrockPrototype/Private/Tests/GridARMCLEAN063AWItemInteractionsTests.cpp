#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridReadableContentAsset.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridCanonicalItemInteractionReadModel.h"

namespace ARMCLEAN063AW
{
    struct FFixture
    {
        UWorld* World = nullptr;
        UGameInstance* GI = nullptr;
        AGrimrockPartyPawn* Pawn = nullptr;
        UGridPartyInventoryComponent* Inventory = nullptr;
        const UGridItemSubsystem* Registry = nullptr;

        FFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM063AW_%s"),
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
            Pawn = World->SpawnActor<AGrimrockPartyPawn>();
            if (!Pawn) return;
            Inventory = Pawn->PartyInventoryComponent.Get();
            if (!Inventory) return;
            Inventory->InitializeDefaultPartyIfNeeded();
            Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots.SetNum(5);
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
            return Registry && Pawn && Inventory && Inventory->IsValidCharacterIndex(0) &&
                Registry->HasCanonicalItem(TEXT("Weapon_ThrowingKnife_01")) &&
                Registry->HasCanonicalItem(TEXT("Item_Arrows_01"));
        }

        FGridItemInstance Make(FName Id, int32 Level) const
        {
            FGridItemInstance I;
            I.RuntimeObjectId = FGuid::NewGuid();
            I.ItemDefinitionId = Id;
            I.ItemLevel = Level;
            I.Quantity = 1;
            I.OwnerType = EGridItemOwnerType::World;
            return I;
        }

        bool HasAction(const FGridItemInstance& Item,
            UGridItemDefinitionAsset* Definition, EGridItemActionType Type, bool* OutEnabled = nullptr) const
        {
            FGridItemActionContext Source;
            Source.PartyPawn = Pawn;
            Source.Item = Item;
            Source.ItemDefinition = Definition;
            Source.CharacterIndex = 0;
            Source.InventorySlotIndex = 0;
            FGridFacingTargetContext Facing;
            TArray<FGridItemContextAction> Actions;
            UGridItemContextActionLibrary::BuildItemContextActions(Source, Facing, Actions);
            const FGridItemContextAction* Found = Actions.FindByPredicate(
                [Type](const FGridItemContextAction& A) { return A.ActionType == Type; });
            if (OutEnabled) *OutEnabled = Found && Found->bEnabled;
            return Found != nullptr;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridARMCLEAN063AWInteractionIdentityTest,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalItemInteractionCapabilities",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridARMCLEAN063AWInteractionIdentityTest::RunTest(const FString&)
{
    ARMCLEAN063AW::FFixture F;
    if (!TestTrue(TEXT("Production canonical ItemSystem and party"), F.Ready())) return false;

    const FName KnifeId(TEXT("Weapon_ThrowingKnife_01"));
    UGridItemDefinitionAsset* Contradictory = NewObject<UGridItemDefinitionAsset>(F.Inventory);
    Contradictory->ItemDefinitionId = KnifeId;
    Contradictory->ItemType = EGridItemType::Potion;
    Contradictory->ReadText = FText::FromString(TEXT("Obsolete readable text"));
    Contradictory->Weight = 1500.0f;
    Contradictory->ThrowSpeed = 0.0f;
    Contradictory->bProvidesQuickItemCombatAction = true;

    for (int32 Level : {1, 20})
    {
        const FGridItemInstance Knife = F.Make(KnifeId, Level);
        const auto Canonical =
            FGridCanonicalItemInteractionReadModel::Resolve(F.Registry, nullptr, Knife);
        const auto Wrong =
            FGridCanonicalItemInteractionReadModel::Resolve(F.Registry, Contradictory, Knife);
        TestTrue(TEXT("Actual N1/N20 armament has canonical identity"),
            Canonical.bCanonicalIdentity && Canonical.bResolved);
        TestTrue(TEXT("Specialized throwing ARM supports physical utility hotbar"),
            Canonical.bCanAddToHotbar && Canonical.bPhysicalThrowHotbar);
        TestTrue(TEXT("Contradictory legacy asset cannot add Consume or Read"),
            !Wrong.bCanConsumeLegacy && !Wrong.bCanRead);
        TestTrue(TEXT("Contradictory legacy asset cannot revoke the physical throw"),
            Wrong.bCanAddToHotbar && Wrong.bPhysicalThrowHotbar);
        TestEqual(TEXT("Canonical item name ignores old asset"),
            Wrong.DisplayName.ToString(), Canonical.DisplayName.ToString());
        TestFalse(TEXT("Context menu never adds Consume for registered ARM"),
            F.HasAction(Knife, Contradictory, EGridItemActionType::Consume));
        TestFalse(TEXT("Context menu never reads old registered ARM ReadText"),
            F.HasAction(Knife, Contradictory, EGridItemActionType::Read));
        bool Enabled = false;
        TestTrue(TEXT("Context menu offers physical throw shortcut without legacy"),
            F.HasAction(Knife, nullptr, EGridItemActionType::AddToHotbar, &Enabled) && Enabled);
    }

    for (int32 InvalidLevel : {0, 21})
    {
        const auto Bad = FGridCanonicalItemInteractionReadModel::Resolve(
            F.Registry, Contradictory, F.Make(KnifeId, InvalidLevel));
        TestTrue(TEXT("Known N0/N21 is classified before instance validation"), Bad.bCanonicalIdentity);
        TestFalse(TEXT("Invalid registered level refuses legacy capabilities"), Bad.bResolved ||
            Bad.bCanRead || Bad.bCanConsumeLegacy || Bad.bCanAddToHotbar);
    }

    FGridItemInstance Arrow = F.Make(TEXT("Item_Arrows_01"), 0);
    const auto Ammo = FGridCanonicalItemInteractionReadModel::Resolve(F.Registry, Contradictory, Arrow);
    TestTrue(TEXT("Canonical N0 ammo resolves"), Ammo.bCanonicalIdentity && Ammo.bResolved);
    TestFalse(TEXT("Ammo receives no fabricated physical throw shortcut"), Ammo.bCanAddToHotbar);

    FGridItemInstance WithText = F.Make(KnifeId, 1);
    WithText.ReadTextOverride = FText::FromString(TEXT("Genuine instance text"));
    const auto GenuineRead = FGridCanonicalItemInteractionReadModel::Resolve(
        F.Registry, Contradictory, WithText);
    TestTrue(TEXT("Canonical item retains truly authored instance text"), GenuineRead.bCanRead);
    TestTrue(TEXT("Context offers Read only for genuine canonical instance content"),
        F.HasAction(WithText, Contradictory, EGridItemActionType::Read));

    UGridItemDefinitionAsset* Book = NewObject<UGridItemDefinitionAsset>(F.Inventory);
    Book->ItemDefinitionId = TEXT("ARM063AW_UnregisteredBook");
    Book->ItemType = EGridItemType::Book;
    Book->ReadText = FText::FromString(TEXT("Legacy readable body"));
    Book->DisplayName = FText::FromString(TEXT("Legacy book"));
    const auto Legacy = FGridCanonicalItemInteractionReadModel::Resolve(
        F.Registry, Book, F.Make(Book->ItemDefinitionId, 0));
    TestFalse(TEXT("Unregistered reading is not canonical"), Legacy.bCanonicalIdentity);
    TestFalse(TEXT("Configured registry refuses unknown readable source"),
        Legacy.bResolved || Legacy.bCanRead || Legacy.bCanConsumeLegacy ||
        Legacy.bCanConsumeCanonical || Legacy.bCanAddToHotbar || Legacy.bPhysicalThrowHotbar);
    TestTrue(TEXT("Unknown reader exposes no historical presentation"),
        Legacy.DisplayName.IsEmpty() && Legacy.DefaultReadText.IsEmpty());
    FGridItemInstance UnknownWithText = F.Make(Book->ItemDefinitionId, 0);
    UnknownWithText.ReadTextOverride = FText::FromString(TEXT("Unknown source text"));
    TestFalse(TEXT("Instance text cannot authorize an unknown identity"),
        FGridCanonicalItemInteractionReadModel::Resolve(F.Registry, Book, UnknownWithText).bCanRead);
    TestFalse(TEXT("Real menu refuses unknown Read"),
        F.HasAction(UnknownWithText, Book, EGridItemActionType::Read));
    Book->ItemType = EGridItemType::Potion;
    Book->bProvidesQuickItemCombatAction = true;
    TestFalse(TEXT("Historical potion cannot grant unknown Consume"),
        FGridCanonicalItemInteractionReadModel::Resolve(F.Registry, Book, UnknownWithText).bCanConsumeLegacy);
    TestFalse(TEXT("Real menu refuses unknown Consume"),
        F.HasAction(UnknownWithText, Book, EGridItemActionType::Consume));
    Book->ItemType = EGridItemType::Book;
    const auto FixtureRead = FGridCanonicalItemInteractionReadModel::Resolve(
        nullptr, Book, F.Make(Book->ItemDefinitionId, 0));
    TestTrue(TEXT("Isolated registry-less readable fixture remains supported"),
        FixtureRead.bResolved && FixtureRead.bCanRead);

    const FGridItemInstance Incoming = F.Make(KnifeId, 20);
    if (!TestTrue(TEXT("Real canonical knife enters inventory without old DA"),
        F.Inventory->AddItemToCharacterInventory(0, Incoming))) return false;
    const FGridCharacterInventoryState& Character = F.Inventory->PartyInventoryState.ActiveCharacters[0];
    const FGridInventorySlot* Slot = Character.InventorySlots.FindByPredicate(
        [KnifeId](const FGridInventorySlot& S)
        {
            return !S.IsEmpty() && S.Item.ItemDefinitionId == KnifeId;
        });
    if (!TestNotNull(TEXT("N20 physical inventory slot"), Slot)) return false;
    if (!TestTrue(TEXT("Context menu can bind the real inventory item"),
        F.HasAction(Slot->Item, nullptr, EGridItemActionType::AddToHotbar))) return false;
    FGridItemInstance Stale = Slot->Item;
    Stale.ItemLevel = 1;
    TestFalse(TEXT("Same GUID and ID cannot bind a stale N1 drag of an N20 knife"),
        F.Inventory->SetCharacterCombatHotbarBindingFromItem(
            0, 2, Stale, EGridEquipmentSlot::None));
    FGridCombatHotbarBinding Before;
    TestTrue(TEXT("Rejected stale drag retains empty hotbar slot"),
        F.Inventory->GetCharacterCombatHotbarBinding(0, 2, Before) && Before.IsEmpty());
    auto* MutableSlot = F.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots.FindByPredicate(
        [KnifeId](const FGridInventorySlot& S)
        { return !S.IsEmpty() && S.Item.ItemDefinitionId == KnifeId; });
    if (!TestNotNull(TEXT("Owned knife slot is mutable"), MutableSlot)) return false;
    MutableSlot->Item.Quantity = MAX_int32;
    TestFalse(TEXT("Over-capacity owned stack cannot bind from a plausible snapshot"),
        F.Inventory->SetCharacterCombatHotbarBindingFromItem(
            0, 2, Incoming, EGridEquipmentSlot::None));
    MutableSlot->Item.Quantity = 1;

    TestTrue(TEXT("Real hotbar binding requires no obsolete DA_Weapon"),
        F.Inventory->SetCharacterCombatHotbarBindingFromItem(
            0, 2, Slot->Item, EGridEquipmentSlot::None));
    FGridCombatHotbarBinding Binding;
    if (TestTrue(TEXT("Hotbar slot is readable"), F.Inventory->GetCharacterCombatHotbarBinding(0, 2, Binding)))
    {
        TestTrue(TEXT("Canonical shortcut is physical throw, not old Use_ action"),
            Binding.IsPhysicalThrowItemBinding());
        TestEqual(TEXT("Shortcut points to original ItemId"), Binding.SourceDefinitionId, KnifeId);
    }
    bool CanBindAgain = true;
    TestTrue(TEXT("Menu detects already-bound canonical physical throw"),
        F.HasAction(Slot->Item, nullptr, EGridItemActionType::AddToHotbar, &CanBindAgain));
    TestFalse(TEXT("Duplicate shortcut is disabled"), CanBindAgain);

    FText ConsumeError;
    TestFalse(TEXT("Legacy self-consume executor rejects registered ARM"),
        F.Inventory->TryConsumeSelfRestorativeQuickItem(0, Slot->Item.RuntimeObjectId, ConsumeError));
    TestTrue(TEXT("Failed self-consumption retains source item"),
        F.Inventory->CountItemDefinitionInCharacterInventory(0, KnifeId) == 1);
    // The same contract applies to equipped weapon shortcuts.
    FGridItemInstance Sword = F.Make(TEXT("Weapon_StraightSword_01"), 20);
    Sword.OwnerGuid = Character.CharacterId;
    Sword.OwnerCharacterIndex = 0;
    Sword.OwnerType = EGridItemOwnerType::EquipmentSlot;
    Sword.EquipmentSlot = EGridEquipmentSlot::MainHand;
    if (!TestTrue(TEXT("Equipment row available"), F.Inventory->PartyInventoryState.ActiveEquipment.IsValidIndex(0)))
        return false;
    auto& Main = F.Inventory->PartyInventoryState.ActiveEquipment[0].MainHand;
    Main = Sword;
    FGridItemInstance StaleSword = Sword;
    StaleSword.ItemLevel = 1;
    TestFalse(TEXT("Stale N1 equipped snapshot cannot authorize an N20 shortcut"),
        F.Inventory->SetCharacterCombatHotbarBindingFromItem(0, 3, StaleSword, EGridEquipmentSlot::MainHand));
    Main.Quantity = 2;
    TestFalse(TEXT("Malformed equipped quantity cannot authorize a shortcut"),
        F.Inventory->SetCharacterCombatHotbarBindingFromItem(0, 3, Sword, EGridEquipmentSlot::MainHand));
    Main.Quantity = 1;
    Main.OwnerGuid = FGuid::NewGuid();
    TestFalse(TEXT("Actual equipped source must belong to selected character"),
        F.Inventory->SetCharacterCombatHotbarBindingFromItem(0, 3, Sword, EGridEquipmentSlot::MainHand));
    Main.OwnerGuid = Sword.OwnerGuid;
    TestTrue(TEXT("Actual N20 equipped item binds a canonical attack"),
        F.Inventory->SetCharacterCombatHotbarBindingFromItem(0, 3, Sword, EGridEquipmentSlot::MainHand));
    return !HasAnyErrors();
}
#endif
