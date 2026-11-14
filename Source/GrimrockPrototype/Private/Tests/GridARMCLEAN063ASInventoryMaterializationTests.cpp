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
        Item.Weight = 999.0f;
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
    TestTrue(TEXT("Mass comes from canonical"), FMath::IsNearlyEqual(Materialized.Weight, Ammo.WeightKg));
    TestEqual(TEXT("Canonical name from root"), Materialized.DisplayName.ToString(), Ammo.DisplayName.ToString());
    TestEqual(TEXT("Original source is not modified"), Incoming.Weight, Original.Weight);
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
    TestTrue(TEXT("Canonical stack has matching source weight"),
        FMath::IsNearlyEqual(Character.InventorySlots[1].Item.Weight, Ammo.WeightKg));
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
    TestTrue(TEXT("Contradictory old icon/name do not affect canonical metadata"),
        FMath::IsNearlyEqual(Conflict.Weight, Ammo.WeightKg));
    TestEqual(TEXT("Old max stack is ignored for registered ID"),
        FGridCanonicalInventoryStackReadModel::Resolve(Fixture.Registry, WrongOld, Conflict).MaxStackSize,
        Ammo.MaxStack);

    // N0 on a REAL ARM without authored old default fails, never silently N1.
    Fixture.ClearSlots();
    const FName SwordId(TEXT("Weapon_StraightSword_01"));
    FGridItemInstance SwordN0 = ARMCLEAN063AS::MakeItem(SwordId, 0);
    const FGridItemInstance OriginalN0 = SwordN0;
    TestFalse(TEXT("Registered ARM N0 requires explicit fixed level without old author"),
        Fixture.Inventory->ApplyItemDefinitionToInstance(SwordN0));
    TestEqual(TEXT("Rejected level unchanged"), SwordN0.ItemLevel, OriginalN0.ItemLevel);
    TestEqual(TEXT("Rejected weight unchanged"), SwordN0.Weight, OriginalN0.Weight);
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
    TestTrue(TEXT("ARM N19 weight from canonical root"),
        FMath::IsNearlyEqual(Character.InventorySlots[0].Item.Weight, Sword19.WeightKg));
    TestFalse(TEXT("ARM cannot gain a SplitStack menu"),
        ARMCLEAN063AS::HasSplitAction(Fixture.Pawn, Character.InventorySlots[0].Item));

    // Unregistered objects maintain their former definition-driven capacities.
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
    TestTrue(TEXT("Legacy pickup retains old stack policy"),
        Fixture.Inventory->AddItemToCharacterInventory(0, OldStack));
    TestEqual(TEXT("Legacy first stack ten"), Character.InventorySlots[0].Item.Quantity, 10);
    TestEqual(TEXT("Legacy remainder three"), Character.InventorySlots[1].Item.Quantity, 3);
    TestTrue(TEXT("Legacy stack old unit weight"),
        FMath::IsNearlyEqual(Character.InventorySlots[0].Item.Weight, 1.5f));
    return true;
}
#endif
