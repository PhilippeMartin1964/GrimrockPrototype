#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "../Runtime/GridCanonicalInventoryStackReadModel.h"
#include "../Runtime/GridItemInstanceMaterializationReadModel.h"
#include "../UI/GridInventoryItemMetadataReadModel.h"

namespace ARMCLEAN063AS
{
    struct FWorldFixture
    {
        UWorld* World = nullptr;
        UGameInstance* GameInstance = nullptr;
        AGrimrockPartyPawn* Pawn = nullptr;
        UGridPartyInventoryComponent* Inventory = nullptr;
        const UGridItemSubsystem* Registry = nullptr;

        FWorldFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM063AS_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (!World || !GEngine) return;
            FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
            Context.SetCurrentWorld(World);
            GameInstance = NewObject<UGameInstance>(GetTransientPackage());
            if (!GameInstance) return;
            GameInstance->Init();
            World->SetGameInstance(GameInstance);
            Registry = GameInstance->GetSubsystem<UGridItemSubsystem>();
            Pawn = World->SpawnActor<AGrimrockPartyPawn>();
            if (!Pawn) return;
            Inventory = Pawn->PartyInventoryComponent.Get();
            if (!Inventory) return;
            Inventory->InitializeDefaultPartyIfNeeded();
            Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots.SetNum(4);
        }
        ~FWorldFixture()
        {
            if (World) World->SetGameInstance(nullptr);
            if (GameInstance) GameInstance->Shutdown();
            if (World)
            {
                World->DestroyWorld(false);
                if (GEngine) GEngine->DestroyWorldContext(World);
            }
        }
        bool Ready() const
        {
            return Registry && Inventory && Inventory->IsValidCharacterIndex(0) &&
                Registry->HasCanonicalItem(TEXT("Item_Arrows_01")) &&
                Registry->HasCanonicalItem(TEXT("Weapon_StraightSword_01"));
        }
        void ClearSlots() const
        {
            if (!Inventory) return;
            for (FGridInventorySlot& Slot : Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots)
            {
                Slot = FGridInventorySlot();
            }
            if (Inventory->HasCursorItem()) Inventory->ClearCursorItem();
        }
    };

    FGridItemInstance MakeItem(FName Id, int32 Level, int32 Quantity = 1)
    {
        FGridItemInstance Item;
        Item.RuntimeObjectId = FGuid::NewGuid();
        Item.ItemDefinitionId = Id;
        Item.ItemLevel = Level;
        Item.Quantity = Quantity;
        Item.OwnerType = EGridItemOwnerType::World;
        Item.AbsorbedEquipmentPhysicalArmor = 3;
        Item.AbsorbedEquipmentMagicalArmor = 2;
        Item.bLightsEnabled = true;
        return Item;
    }

    bool HasSplitAction(AGrimrockPartyPawn* Pawn, FGridItemInstance Item)
    {
        FGridItemActionContext Context;
        Context.PartyPawn = Pawn;
        Context.CharacterIndex = 0;
        Context.InventorySlotIndex = 0;
        Context.Item = Item;
        FGridFacingTargetContext Facing;
        TArray<FGridItemContextAction> Actions;
        UGridItemContextActionLibrary::BuildItemContextActions(Context, Facing, Actions);
        return Actions.ContainsByPredicate([](const FGridItemContextAction& Action)
        {
            return Action.ActionType == EGridItemActionType::SplitStack;
        });
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063ASNoLegacyInventory,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalInventoryMaterializationWithoutLegacyDefinition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063ASNoLegacyInventory::RunTest(const FString&)
{
    ARMCLEAN063AS::FWorldFixture Fixture;
    if (!TestTrue(TEXT("World GI canonical registry and inventory available"), Fixture.Ready())) return false;

    const FName AmmoId(TEXT("Item_Arrows_01"));
    FGridResolvedItem Ammo;
    if (!TestTrue(TEXT("Production N0 arrow stack resolves"), Fixture.Registry->ResolveItem(AmmoId, 0, Ammo))) return false;
    if (!TestTrue(TEXT("Real arrow MaxStack is stackable"), Ammo.MaxStack > 1)) return false;

    FGridItemInstance Incoming = ARMCLEAN063AS::MakeItem(AmmoId, 0, Ammo.MaxStack + 3);
    const FGuid OriginalGuid = Incoming.RuntimeObjectId;
    const FGridItemInstance Original = Incoming;
    FGridItemInstance Materialized = Incoming;
    if (!TestTrue(TEXT("Canonical N0 arrow materializes with nullptr old definition"),
        FGridItemInstanceMaterializationReadModel::TryApply(Fixture.Registry, nullptr, Materialized))) return false;
    TestEqual(TEXT("True N0 remains zero"), Materialized.ItemLevel, 0);
    TestTrue(TEXT("All materialized durable fields unchanged"),
        FGridItemInstance::StaticStruct()->CompareScriptStruct(&Materialized, &Original, 0));
    const auto MaterializedMeta = FGridInventoryItemMetadataReadModel::Resolve(Fixture.Registry, nullptr, Materialized);
    TestTrue(TEXT("Canonical metadata resolves"), MaterializedMeta.bResolved);
    TestTrue(TEXT("Mass comes from canonical"), FMath::IsNearlyEqual(MaterializedMeta.UnitWeightKg, Ammo.WeightKg));
    TestEqual(TEXT("Canonical name from root"), MaterializedMeta.DisplayName.ToString(), Ammo.DisplayName.ToString());
    TestTrue(TEXT("Original source is not modified"),
        FGridItemInstance::StaticStruct()->CompareScriptStruct(&Incoming, &Original, 0));
    TestEqual(TEXT("Armor physical debt preserved"), Materialized.AbsorbedEquipmentPhysicalArmor, 3);
    TestEqual(TEXT("Armor magic debt preserved"), Materialized.AbsorbedEquipmentMagicalArmor, 2);
    TestTrue(TEXT("Live light state preserved"), Materialized.bLightsEnabled);

    const auto Policy = FGridCanonicalInventoryStackReadModel::Resolve(Fixture.Registry, nullptr, Materialized);
    TestTrue(TEXT("Stack reads canonical identity without old DA"), Policy.bCanonicalIdentity && Policy.bResolved);
    TestEqual(TEXT("Stack capacity from real root"), Policy.MaxStackSize, Ammo.MaxStack);
    TestTrue(TEXT("Inventory can accept full canonical ammo quantity"),
        Fixture.Inventory->CanAddItemToCharacterInventory(0, Incoming));
    TestTrue(TEXT("Inventory accepts canonical ammo without RegisterItemDefinition"),
        Fixture.Inventory->AddItemToCharacterInventory(0, Incoming));

    FGridCharacterInventoryState& Character = Fixture.Inventory->PartyInventoryState.ActiveCharacters[0];
    TestEqual(TEXT("First stack reaches canonical capacity"),
        Character.InventorySlots[0].Item.Quantity, Ammo.MaxStack);
    TestEqual(TEXT("Second stack stores the remainder"), Character.InventorySlots[1].Item.Quantity, 3);
    TestEqual(TEXT("First GUID preserved"), Character.InventorySlots[0].Item.RuntimeObjectId, OriginalGuid);
    TestTrue(TEXT("Second stack owns a fresh GUID"),
        Character.InventorySlots[1].Item.RuntimeObjectId != OriginalGuid);
    TestEqual(TEXT("Canonical arrow stays N0 in bag"), Character.InventorySlots[0].Item.ItemLevel, 0);
    const auto StackMeta = FGridInventoryItemMetadataReadModel::Resolve(Fixture.Registry, nullptr, Character.InventorySlots[1].Item);
    TestTrue(TEXT("Canonical stack has matching source weight"), StackMeta.bResolved &&
        FMath::IsNearlyEqual(StackMeta.UnitWeightKg, Ammo.WeightKg));
    TestTrue(TEXT("Gameplay still offers split action without old definition"),
        ARMCLEAN063AS::HasSplitAction(Fixture.Pawn, Character.InventorySlots[1].Item));
    TestTrue(TEXT("Split uses authoritative stack policy without old DA"),
        Fixture.Inventory->TrySplitInventoryStackToFirstFreeSlot(0, 1));
    TestEqual(TEXT("Split keeps ceiling half"), Character.InventorySlots[1].Item.Quantity, 2);
    TestEqual(TEXT("Split puts floor half in first free slot"), Character.InventorySlots[2].Item.Quantity, 1);
    TestTrue(TEXT("Split preserves original slot GUID"),
        Character.InventorySlots[1].Item.RuntimeObjectId != Character.InventorySlots[2].Item.RuntimeObjectId);

    // Cursor/inventory merge works against the SAME MaxStack without old assets.
    TestTrue(TEXT("Take one arrow to cursor uses canonical stack"),
        Fixture.Inventory->TryTakeInventorySlotQuantityToCursor(0, 2, 1));
    TestTrue(TEXT("Cursor holds the selected arrow"), Fixture.Inventory->HasCursorItem());
    TestTrue(TEXT("Merge cursor arrow into partial stack"),
        Fixture.Inventory->TryPlaceCursorItemInCharacterInventorySlot(0, 1));
    TestEqual(TEXT("Second stack is restored to three"), Character.InventorySlots[1].Item.Quantity, 3);
    TestFalse(TEXT("Cursor cleared after full merge"), Fixture.Inventory->HasCursorItem());
    FGridItemInstance More = ARMCLEAN063AS::MakeItem(AmmoId, 0, 1);
    TestTrue(TEXT("Capacity check uses partial stack even without legacy"),
        Fixture.Inventory->CanAddItemToCharacterInventory(0, More));
    TestTrue(TEXT("Pickup merges into partial stack"),
        Fixture.Inventory->AddItemToCharacterInventory(0, More));
    TestEqual(TEXT("Second stack now contains four"), Character.InventorySlots[1].Item.Quantity, 4);

    // Contradicting legacy capacity MUST NOT change registered ammo policy.
    UGridItemDefinitionAsset* WrongOld = NewObject<UGridItemDefinitionAsset>(Fixture.Inventory);
    WrongOld->ItemDefinitionId = AmmoId;
    WrongOld->DisplayName = FText::FromString(TEXT("Old wrong arrows"));
    WrongOld->Weight = 1234.0f;
    WrongOld->bStackable = false;
    WrongOld->MaxStackSize = 1;
    if (!TestTrue(TEXT("Contradictory legacy registered as transitional asset"),
        Fixture.Inventory->RegisterItemDefinition(WrongOld))) return false;
    FGridItemInstance Conflict = ARMCLEAN063AS::MakeItem(AmmoId, 0, 7);
    TestTrue(TEXT("Canonical materialization ignores old unstackable flag"),
        Fixture.Inventory->ApplyItemDefinitionToInstance(Conflict));
    TestEqual(TEXT("Canonical capacity still allows seven"), Conflict.Quantity, 7);
    const auto ConflictMeta = FGridInventoryItemMetadataReadModel::Resolve(Fixture.Registry, WrongOld, Conflict);
    TestTrue(TEXT("Contradictory old metadata does not affect canonical mass"), ConflictMeta.bResolved &&
        FMath::IsNearlyEqual(ConflictMeta.UnitWeightKg, Ammo.WeightKg));
    TestEqual(TEXT("Old max stack is ignored for registered ID"),
        FGridCanonicalInventoryStackReadModel::Resolve(Fixture.Registry, WrongOld, Conflict).MaxStackSize,
        Ammo.MaxStack);

    FGridItemInstance InvalidAmmo = ARMCLEAN063AS::MakeItem(AmmoId, 1, 1);
    const auto InvalidPolicy = FGridCanonicalInventoryStackReadModel::Resolve(Fixture.Registry, WrongOld, InvalidAmmo);
    TestTrue(TEXT("Invalid ammo still recognized canonical"), InvalidPolicy.bCanonicalIdentity);
    TestFalse(TEXT("Physical ammo N1 stack policy rejected"), InvalidPolicy.bResolved);
    TestFalse(TEXT("Physical ammo N1 cannot materialize"), Fixture.Inventory->ApplyItemDefinitionToInstance(InvalidAmmo));
    TestFalse(TEXT("Physical ammo N1 cannot enter inventory"), Fixture.Inventory->CanAddItemToCharacterInventory(0, InvalidAmmo));
    TestEqual(TEXT("Invalid ammo level unchanged"), InvalidAmmo.ItemLevel, 1);

    // N0 on a REAL ARM fails, never silently N1.
    Fixture.ClearSlots();
    const FName SwordId(TEXT("Weapon_StraightSword_01"));
    FGridItemInstance SwordN0 = ARMCLEAN063AS::MakeItem(SwordId, 0);
    const FGridItemInstance OriginalN0 = SwordN0;
    TestFalse(TEXT("Registered ARM N0 requires explicit fixed level without old author"),
        Fixture.Inventory->ApplyItemDefinitionToInstance(SwordN0));
    TestEqual(TEXT("Rejected level unchanged"), SwordN0.ItemLevel, OriginalN0.ItemLevel);
    TestTrue(TEXT("Rejected durable state unchanged"),
        FGridItemInstance::StaticStruct()->CompareScriptStruct(&SwordN0, &OriginalN0, 0));
    TestFalse(TEXT("CanAdd refuses ARM N0 even with empty bag"),
        Fixture.Inventory->CanAddItemToCharacterInventory(0, SwordN0));

    FGridResolvedItem Sword19;
    if (!TestTrue(TEXT("Registered ARM N19 resolves"),
        Fixture.Registry->ResolveItem(SwordId, 19, Sword19))) return false;
    FGridItemInstance Sword = ARMCLEAN063AS::MakeItem(SwordId, 19);
    TestTrue(TEXT("Canonical ARM N19 is addable with no old definition"),
        Fixture.Inventory->AddItemToCharacterInventory(0, Sword));
    TestEqual(TEXT("N19 level preserved"), Character.InventorySlots[0].Item.ItemLevel, 19);
    TestEqual(TEXT("ARM remains nonstackable"), Character.InventorySlots[0].Item.Quantity, 1);
    const auto SwordMeta = FGridInventoryItemMetadataReadModel::Resolve(Fixture.Registry, nullptr, Character.InventorySlots[0].Item);
    TestTrue(TEXT("ARM N19 weight from canonical root"), SwordMeta.bResolved &&
        FMath::IsNearlyEqual(SwordMeta.UnitWeightKg, Sword19.WeightKg));
    TestFalse(TEXT("ARM cannot gain a SplitStack menu"),
        ARMCLEAN063AS::HasSplitAction(Fixture.Pawn, Character.InventorySlots[0].Item));

    // An unknown ID with plausible cached metadata must not enter a configured inventory.
    Fixture.ClearSlots();
    const FGridItemInstance Unknown = ARMCLEAN063AS::MakeItem(TEXT("CLEAN06_5G42_Unknown"), 0, 2);
    TestFalse(TEXT("Unknown ID has no bag capacity"), Fixture.Inventory->CanAddItemToCharacterInventory(0, Unknown));
    TestFalse(TEXT("Unknown ID cannot enter a bag"), Fixture.Inventory->AddItemToCharacterInventory(0, Unknown));
    TestFalse(TEXT("Unknown ID cannot enter cursor"), Fixture.Inventory->SetCursorItem(Unknown));
    TestFalse(TEXT("Invalid physical ARM N0 cannot enter cursor"), Fixture.Inventory->SetCursorItem(SwordN0));
    TestFalse(TEXT("Single cursor cannot clamp excess canonical units"), Fixture.Inventory->SetCursorItem(Incoming));
    TestFalse(TEXT("Rejected cursor is empty"), Fixture.Inventory->HasCursorItem());
    TestTrue(TEXT("Rejected bag remains empty"), Character.InventorySlots[0].IsEmpty());
    // Simulate a malformed already-owned cursor, bypassing its public setter.
    Fixture.Inventory->PartyInventoryState.CursorItem = Unknown;
    Fixture.Inventory->PartyInventoryState.bHasCursorItem = true;
    TestFalse(TEXT("Unknown cursor cannot move into explicit slot"), Fixture.Inventory->TryPlaceCursorItemInCharacterInventorySlot(0, 0));
    TestFalse(TEXT("Unknown cursor cannot move into first free slot"), Fixture.Inventory->TryPlaceCursorItemInCharacterInventory(0));
    TestTrue(TEXT("Rejection preserves cursor identity and quantity"), Fixture.Inventory->HasCursorItem() &&
        Fixture.Inventory->GetCursorItem().RuntimeObjectId == Unknown.RuntimeObjectId &&
        Fixture.Inventory->GetCursorItem().Quantity == Unknown.Quantity);
    TestTrue(TEXT("No partial bag transfer"), Character.InventorySlots[0].IsEmpty());
    Fixture.ClearSlots();
    TestTrue(TEXT("Valid canonical single stack may enter cursor"), Fixture.Inventory->SetCursorItem(More));
    TestTrue(TEXT("Canonical single stack enters free bag slot"), Fixture.Inventory->TryPlaceCursorItemInCharacterInventory(0));
    TestEqual(TEXT("Canonical transfer preserves units"), Character.InventorySlots[0].Item.Quantity, More.Quantity);
    TestFalse(TEXT("Successful transfer clears cursor"), Fixture.Inventory->HasCursorItem());

    // Equipment/world rollback is a physical transfer, never a new item grant.
    Fixture.ClearSlots();
    FGridItemInstance& Hand = Fixture.Inventory->PartyInventoryState.ActiveEquipment[0].MainHand;
    Hand = SwordN0;
    FGridItemInstance WorldItem;
    TestFalse(TEXT("Invalid physical N0 cannot leave equipped slot"), Fixture.Inventory->TryExtractOneEquippedItemForWorldTransfer(0, EGridEquipmentSlot::MainHand, SwordId, WorldItem));
    TestEqual(TEXT("Invalid source remains owned"), Hand.RuntimeObjectId, SwordN0.RuntimeObjectId);
    TestFalse(TEXT("No world item produced on refusal"), WorldItem.IsValid());
    Hand = FGridItemInstance();
    TestTrue(TEXT("Valid N19 sword enters bag"), Fixture.Inventory->AddItemToCharacterInventory(0, Sword));
    TestTrue(TEXT("Valid sword equips"), Fixture.Inventory->EquipItemFromInventorySlot(0, 0, EGridEquipmentSlot::MainHand));
    if (!TestTrue(TEXT("Valid N19 leaves physical hand"), Fixture.Inventory->TryExtractOneEquippedItemForWorldTransfer(0, EGridEquipmentSlot::MainHand, SwordId, WorldItem))) return false;
    TestTrue(TEXT("Hand empty after extraction"), !Hand.IsValid());
    TestEqual(TEXT("World item preserves level"), WorldItem.ItemLevel, 19);
    TestTrue(TEXT("Valid world item restores"), Fixture.Inventory->TryRestoreExtractedItemToEquipment(0, EGridEquipmentSlot::MainHand, WorldItem));
    TestEqual(TEXT("Rollback preserves GUID"), Hand.RuntimeObjectId, Sword.RuntimeObjectId);
    TestEqual(TEXT("Rollback preserves level"), Hand.ItemLevel, 19);
    TestEqual(TEXT("Rollback preserves armor debt"), Hand.AbsorbedEquipmentPhysicalArmor, Sword.AbsorbedEquipmentPhysicalArmor);
    TestFalse(TEXT("Cannot replay same GUID into occupied hand"), Fixture.Inventory->TryRestoreExtractedItemToEquipment(0, EGridEquipmentSlot::MainHand, WorldItem));
    FGridItemInstance OtherLevel = WorldItem;
    OtherLevel.RuntimeObjectId = FGuid::NewGuid();
    OtherLevel.ItemLevel = 20;
    TestFalse(TEXT("Cannot merge a different physical level"), Fixture.Inventory->TryRestoreExtractedItemToEquipment(0, EGridEquipmentSlot::MainHand, OtherLevel));
    OtherLevel.ItemLevel = 19;
    TestFalse(TEXT("Cannot exceed canonical non-stackable capacity"), Fixture.Inventory->TryRestoreExtractedItemToEquipment(0, EGridEquipmentSlot::MainHand, OtherLevel));
    TestEqual(TEXT("Rejected restores do not duplicate sword"), Hand.Quantity, 1);
    Hand = FGridItemInstance();
    TestFalse(TEXT("Unknown world identity cannot enter empty equipment"), Fixture.Inventory->TryRestoreExtractedItemToEquipment(0, EGridEquipmentSlot::MainHand, Unknown));
    TestFalse(TEXT("Invalid N0 cannot restore into empty hand"), Fixture.Inventory->TryRestoreExtractedItemToEquipment(0, EGridEquipmentSlot::MainHand, SwordN0));
    TestFalse(TEXT("Unknown or invalid restore leaves hand empty"), Hand.IsValid());

    // An unknown item may not gain inventory capacity from an obsolete definition.
    Fixture.ClearSlots();
    UGridItemDefinitionAsset* Legacy = NewObject<UGridItemDefinitionAsset>(Fixture.Inventory);
    Legacy->ItemDefinitionId = TEXT("ARM063AS_Unregistered_Stack");
    Legacy->DisplayName = FText::FromString(TEXT("Old stack"));
    Legacy->bStackable = true;
    Legacy->MaxStackSize = 10;
    Legacy->Weight = 1.5f;
    if (!TestTrue(TEXT("Legacy item definition registered"),
        Fixture.Inventory->RegisterItemDefinition(Legacy))) return false;
    FGridItemInstance OldStack = ARMCLEAN063AS::MakeItem(Legacy->ItemDefinitionId, 0, 13);
    const auto RefusedPolicy =
        FGridCanonicalInventoryStackReadModel::Resolve(Fixture.Registry, Legacy, OldStack);
    TestFalse(TEXT("Configured registry refuses unknown legacy stack capacity"), RefusedPolicy.bResolved);
    TestFalse(TEXT("Unknown stack cannot enter canonical inventory"),
        Fixture.Inventory->CanAddItemToCharacterInventory(0, OldStack));
    TestFalse(TEXT("Unknown stack cannot be granted"),
        Fixture.Inventory->AddItemToCharacterInventory(0, OldStack));
    OldStack.Quantity = 1;
    TestFalse(TEXT("Single unknown item is refused, not just oversized stacks"),
        Fixture.Inventory->CanAddItemToCharacterInventory(0, OldStack));
    TestFalse(TEXT("Single unknown item cannot be granted"),
        Fixture.Inventory->AddItemToCharacterInventory(0, OldStack));
    TestTrue(TEXT("Rejected inventory leaves all slots empty"),
        Character.InventorySlots[0].IsEmpty() && Character.InventorySlots[1].IsEmpty());
    OldStack.Quantity = 13;
    const auto IsolatedPolicy =
        FGridCanonicalInventoryStackReadModel::Resolve(nullptr, Legacy, OldStack);
    TestTrue(TEXT("Registry-less Automation fixture still has old stack limit"),
        IsolatedPolicy.bResolved && IsolatedPolicy.MaxStackSize == 10 &&
        IsolatedPolicy.IsStackable());
    const auto HistoricalMetadata = FGridInventoryItemMetadataReadModel::Resolve(
        nullptr, Legacy, OldStack);
    TestTrue(TEXT("Isolated definition still owns its unit mass"),
        HistoricalMetadata.bResolved && FMath::IsNearlyEqual(HistoricalMetadata.UnitWeightKg, 1.5f));
    TestTrue(TEXT("Isolated definition still owns its name"),
        HistoricalMetadata.DisplayName.EqualTo(Legacy->DisplayName));
    return true;
}
#endif
