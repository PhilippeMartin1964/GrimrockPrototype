#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridItemContextActionLibrary.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace ARMEMENT0173
{
    struct FWorldFixture
    {
        UWorld* World = nullptr;
        AGrimrockPartyPawn* Pawn = nullptr;
        UGridPartyInventoryComponent* Inventory = nullptr;

        FWorldFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false)
                .RequiresHitProxies(false)
                .CreatePhysicsScene(false)
                .CreateNavigation(false)
                .CreateAISystem(false)
                .ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM0173_Context_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
                Context.SetCurrentWorld(World);
            }
            if (World)
            {
                Pawn = World->SpawnActor<AGrimrockPartyPawn>();
            }
            if (Pawn)
            {
                Inventory = Pawn->PartyInventoryComponent;
                if (Inventory)
                {
                    Inventory->InitializeDefaultPartyIfNeeded();
                }
            }
        }

        ~FWorldFixture()
        {
            if (!World)
            {
                return;
            }
            World->DestroyWorld(false);
            if (GEngine)
            {
                GEngine->DestroyWorldContext(World);
            }
        }

        bool Ready() const
        {
            return World && Pawn && Inventory && Inventory->IsValidCharacterIndex(0);
        }

        UGridItemDefinitionAsset* Define(FName Id, EGridItemHandUsage Usage)
        {
            if (!Inventory)
            {
                return nullptr;
            }
            UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Inventory);
            Definition->ItemDefinitionId = Id;
            Definition->DisplayName = FText::FromName(Id);
            Definition->ItemType = EGridItemType::Weapon;
            Definition->HandUsage = Usage;
            Definition->Weight = 1.0f;
            Definition->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::MainHand);
            return Inventory->RegisterItemDefinition(Definition) ? Definition : nullptr;
        }

        FGridItemInstance MakeItem(FName Id) const
        {
            FGridItemInstance Item;
            Item.RuntimeObjectId = FGuid::NewGuid();
            Item.ItemDefinitionId = Id;
            Item.DisplayName = FText::FromName(Id);
            Item.Quantity = 1;
            Item.Weight = 1.0f;
            return Item;
        }

        bool ActionsFor(const FGridItemInstance& Item, UGridItemDefinitionAsset* Definition,
            EGridEquipmentSlot Slot, TArray<FGridItemContextAction>& Out) const
        {
            FGridItemActionContext Context;
            Context.PartyPawn = Pawn;
            Context.CharacterIndex = 0;
            Context.Item = Item;
            Context.ItemDefinition = Definition;
            Context.InventorySlotIndex = Slot == EGridEquipmentSlot::None ? 0 : INDEX_NONE;
            Context.EquipmentSlot = Slot;
            FGridFacingTargetContext FacingTarget;
            return UGridItemContextActionLibrary::BuildItemContextActions(Context, FacingTarget, Out);
        }
    };

    TArray<const FGridItemContextAction*> GetEquipActions(const TArray<FGridItemContextAction>& Actions)
    {
        TArray<const FGridItemContextAction*> Found;
        for (const FGridItemContextAction& Action : Actions)
        {
            if (Action.ActionType == EGridItemActionType::Equip)
            {
                Found.Add(&Action);
            }
        }
        return Found;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0173TwoHandedContextActionTest,
    "Grimrock.RPG.ARMEMENT01.7.TwoHandsUI.SingleTwoHandEquipAction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0173TwoHandedContextActionTest::RunTest(const FString&)
{
    ARMEMENT0173::FWorldFixture Fixture;
    if (!TestTrue(TEXT("Pawn and inventory are available"), Fixture.Ready())) return false;

    UGridItemDefinitionAsset* Bow = Fixture.Define(TEXT("ARM0173_Bow"), EGridItemHandUsage::TwoHanded);
    if (!TestNotNull(TEXT("Two-handed definition registered"), Bow)) return false;

    const FGridItemInstance BowItem = Fixture.MakeItem(Bow->ItemDefinitionId);
    if (!TestTrue(TEXT("Bow can enter inventory"), Fixture.Inventory->AddItemToCharacterInventory(0, BowItem))) return false;
    const FGridItemInstance InBag = Fixture.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[0].Item;

    TArray<FGridItemContextAction> Actions;
    if (!TestTrue(TEXT("Right-click bag context builds"), Fixture.ActionsFor(InBag, Bow, EGridEquipmentSlot::None, Actions))) return false;
    const auto EquipActions = ARMEMENT0173::GetEquipActions(Actions);
    if (!TestEqual(TEXT("Only one equip action for a two-handed item"), EquipActions.Num(), 1)) return false;
    TestEqual(TEXT("Two-hand action targets canonical MainHand"), EquipActions[0]->EquipmentSlot, EGridEquipmentSlot::MainHand);
    TestEqual(TEXT("Two-hand action has an explicit label"), EquipActions[0]->Label.ToString(), FString(TEXT("S'équiper : deux mains")));

    if (!TestTrue(TEXT("The bow equips in one action"), Fixture.Inventory->EquipItemFromInventorySlot(0, 0, EquipActions[0]->EquipmentSlot))) return false;
    FGridItemInstance DisplayedOffHand;
    if (!TestTrue(TEXT("Virtual OffHand displays the bow"), Fixture.Inventory->GetEquippedItem(0, EGridEquipmentSlot::OffHand, DisplayedOffHand))) return false;
    TestEqual(TEXT("Same physical identity in both displayed hands"), DisplayedOffHand.RuntimeObjectId, BowItem.RuntimeObjectId);
    Actions.Reset();
    if (!TestTrue(TEXT("Right-click virtual OffHand context builds"),
        Fixture.ActionsFor(DisplayedOffHand, Bow, EGridEquipmentSlot::OffHand, Actions))) return false;
    const FGridItemContextAction* Unequip = Actions.FindByPredicate([](const FGridItemContextAction& Action)
    {
        return Action.ActionType == EGridItemActionType::Unequip;
    });
    if (!TestNotNull(TEXT("Virtual OffHand offers removal"), Unequip)) return false;
    TestEqual(TEXT("Remove targets clicked OffHand"), Unequip->EquipmentSlot, EGridEquipmentSlot::OffHand);
    TestEqual(TEXT("Equipped item offers no additional equip action"), ARMEMENT0173::GetEquipActions(Actions).Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0173OneHandContextActionTest,
    "Grimrock.RPG.ARMEMENT01.7.TwoHandsUI.OneHandedChoicePreserved",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0173OneHandContextActionTest::RunTest(const FString&)
{
    ARMEMENT0173::FWorldFixture Fixture;
    if (!TestTrue(TEXT("Pawn and inventory are available"), Fixture.Ready())) return false;
    UGridItemDefinitionAsset* Dagger = Fixture.Define(TEXT("ARM0173_Dagger"), EGridItemHandUsage::OneHanded);
    if (!TestNotNull(TEXT("One-handed definition registered"), Dagger)) return false;

    TArray<FGridItemContextAction> Actions;
    if (!TestTrue(TEXT("One-hand context builds"),
        Fixture.ActionsFor(Fixture.MakeItem(Dagger->ItemDefinitionId), Dagger, EGridEquipmentSlot::None, Actions))) return false;
    const auto EquipActions = ARMEMENT0173::GetEquipActions(Actions);
    if (!TestEqual(TEXT("One-handed equipment retains both slot choices"), EquipActions.Num(), 2)) return false;
    TestEqual(TEXT("MainHand remains offered first"), EquipActions[0]->EquipmentSlot, EGridEquipmentSlot::MainHand);
    TestEqual(TEXT("OffHand remains offered second"), EquipActions[1]->EquipmentSlot, EGridEquipmentSlot::OffHand);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
