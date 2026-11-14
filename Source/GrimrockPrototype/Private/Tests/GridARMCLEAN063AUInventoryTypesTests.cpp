#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UI/GridInventoryWidget.h"
#include "UI/GridInventorySlotWidget.h"
#include "UI/GridInventoryUiTypes.h"
#include "../UI/GridCanonicalInventoryItemTypeReadModel.h"

namespace ARMCLEAN063AU
{
    struct FFixture
    {
        UWorld* World = nullptr;
        UGameInstance* GI = nullptr;
        AGrimrockPartyPawn* Party = nullptr;
        UGridPartyInventoryComponent* Inventory = nullptr;
        UGridItemSubsystem* Registry = nullptr;

        FFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM063AU_%s"),
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
            Party = World->SpawnActor<AGrimrockPartyPawn>();
            if (!Party) return;
            Inventory = Party->PartyInventoryComponent.Get();
            if (Inventory)
            {
                Inventory->InitializeDefaultPartyIfNeeded();
                Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots.SetNum(4);
            }
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
            return Registry && Inventory && Inventory->IsValidCharacterIndex(0) &&
                Registry->HasCanonicalItem(TEXT("Weapon_StraightSword_01")) &&
                Registry->HasCanonicalItem(TEXT("Weapon_RoundShield_01")) &&
                Registry->HasCanonicalItem(TEXT("Item_Arrows_01"));
        }
        FGridItemInstance Make(FName Id, int32 Level) const
        {
            FGridItemInstance Item;
            Item.RuntimeObjectId = FGuid::NewGuid();
            Item.ItemDefinitionId = Id;
            Item.ItemLevel = Level;
            Item.Quantity = 1;
            Item.OwnerType = EGridItemOwnerType::CharacterInventory;
            Item.OwnerCharacterIndex = 0;
            Item.OwnerGuid = Inventory->PartyInventoryState.ActiveCharacters[0].CharacterId;
            Item.DisplayName = FText::FromString(TEXT("STALE HISTORICAL LABEL"));
            Item.Weight = 900.0f;
            return Item;
        }
        UGridItemDefinitionAsset* RegisterWrong(FName Id, EGridItemType Type) const
        {
            UGridItemDefinitionAsset* Definition = NewObject<UGridItemDefinitionAsset>(Inventory);
            Definition->ItemDefinitionId = Id;
            Definition->ItemType = Type;
            Definition->DisplayName = FText::FromString(TEXT("WRONG HISTORICAL CATEGORY"));
            Inventory->RegisterItemDefinition(Definition);
            return Definition;
        }
        void Put(int32 Slot, const FGridItemInstance& Item) const
        {
            FGridInventorySlot& Destination =
                Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[Slot];
            Destination.bOccupied = true;
            Destination.Item = Item;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean063AUInventoryItemTypes,
    "Grimrock.RPG.ARM.CLEAN06_3.CanonicalInventoryFilterSortAndTooltipTypes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean063AUInventoryItemTypes::RunTest(const FString&)
{
    ARMCLEAN063AU::FFixture Fixture;
    if (!TestTrue(TEXT("Real canonical registry in test world"), Fixture.Ready()))
        return false;
    const FName SwordId(TEXT("Weapon_StraightSword_01"));
    const FName ShieldId(TEXT("Weapon_RoundShield_01"));
    const FName AmmoId(TEXT("Item_Arrows_01"));
    const FName PotionId(TEXT("ARM063AU_UnregisteredPotion"));

    const FGridItemInstance Sword = Fixture.Make(SwordId, 1);
    const FGridItemInstance Shield = Fixture.Make(ShieldId, 20);
    const FGridItemInstance Arrows = Fixture.Make(AmmoId, 0);
    const FGridItemInstance Potion = Fixture.Make(PotionId, 0);
    UGridItemDefinitionAsset* WrongSword =
        Fixture.RegisterWrong(SwordId, EGridItemType::Potion);
    UGridItemDefinitionAsset* WrongShield =
        Fixture.RegisterWrong(ShieldId, EGridItemType::Book);
    UGridItemDefinitionAsset* WrongArrows =
        Fixture.RegisterWrong(AmmoId, EGridItemType::Weapon);
    UGridItemDefinitionAsset* RealLegacy =
        Fixture.RegisterWrong(PotionId, EGridItemType::Potion);

    for (const int32 Level : {1, 20})
    {
        FGridItemInstance ProbeSword = Fixture.Make(SwordId, Level);
        FGridItemInstance ProbeShield = Fixture.Make(ShieldId, Level);
        const auto SwordType =
            FGridCanonicalInventoryItemTypeReadModel::Resolve(
                Fixture.Registry, WrongSword, ProbeSword);
        const auto ShieldType =
            FGridCanonicalInventoryItemTypeReadModel::Resolve(
                Fixture.Registry, WrongShield, ProbeShield);
        TestTrue(TEXT("Sword type from canonical registry"),
            SwordType.bCanonicalIdentity && SwordType.bResolved);
        TestEqual(TEXT("Sword cannot inherit old Potion type"),
            SwordType.ItemType, EGridItemType::Weapon);
        TestTrue(TEXT("Shield type from canonical registry"),
            ShieldType.bCanonicalIdentity && ShieldType.bResolved);
        TestEqual(TEXT("Shield cannot inherit old Book type"),
            ShieldType.ItemType, EGridItemType::Shield);
        TestEqual(TEXT("Sword type works with no old asset"),
            FGridCanonicalInventoryItemTypeReadModel::Resolve(
                Fixture.Registry, nullptr, ProbeSword).ItemType,
            EGridItemType::Weapon);
    }

    const auto AmmoType = FGridCanonicalInventoryItemTypeReadModel::Resolve(
        Fixture.Registry, WrongArrows, Arrows);
    TestTrue(TEXT("Canonical N0 arrows resolve without old Weapon category"),
        AmmoType.bCanonicalIdentity && AmmoType.bResolved);
    TestEqual(TEXT("N0 ammo projects to Misc without invented enum"),
        AmmoType.ItemType, EGridItemType::Misc);
    const auto LegacyType = FGridCanonicalInventoryItemTypeReadModel::Resolve(
        Fixture.Registry, RealLegacy, Potion);
    TestFalse(TEXT("Nonregistered potion uses transitional type"),
        LegacyType.bCanonicalIdentity);
    TestEqual(TEXT("Potion retains old category"),
        LegacyType.ItemType, EGridItemType::Potion);
    TestEqual(TEXT("Mismatched old definition cannot classify unknown item"),
        FGridCanonicalInventoryItemTypeReadModel::Resolve(
            Fixture.Registry, WrongSword, Potion).ItemType, EGridItemType::None);
    for (const int32 InvalidLevel : {0, 21})
    {
        const auto Invalid = FGridCanonicalInventoryItemTypeReadModel::Resolve(
            Fixture.Registry, WrongSword, Fixture.Make(SwordId, InvalidLevel));
        TestTrue(TEXT("Invalid known ARM remains canonical identity"),
            Invalid.bCanonicalIdentity);
        TestFalse(TEXT("Invalid known ARM cannot inherit legacy type"),
            Invalid.bResolved);
        TestEqual(TEXT("Invalid known ARM has no invented category"),
            Invalid.ItemType, EGridItemType::None);
    }

    Fixture.Put(0, Sword);
    Fixture.Put(1, Shield);
    Fixture.Put(2, Arrows);
    Fixture.Put(3, Potion);
    UGridInventoryWidget* Widget = CreateWidget<UGridInventoryWidget>(
        Fixture.World, UGridInventoryWidget::StaticClass());
    if (!TestNotNull(TEXT("World-backed inventory widget"), Widget)) return false;
    Widget->InventoryComponent = Fixture.Inventory;
    TestEqual(TEXT("All keeps physical slot capacity"),
        Widget->ResolveInventorySlotWidgetCount(), 4);
    TestEqual(TEXT("All includes all four items"),
        Widget->GetVisibleInventoryItemCount(), 4);

    Widget->SetInventoryFilterCategory(EGridInventoryFilterCategory::Equipment);
    TestEqual(TEXT("Equipment counts sword and shield only"),
        Widget->GetVisibleInventoryItemCount(), 2);
    TArray<int32> Projection;
    Widget->GetInventoryProjectionSourceSlotIndices(Projection);
    TestEqual(TEXT("Equipment keeps all 4 display slots"), Projection.Num(), 4);
    if (Projection.Num() == 4)
    {
        TestTrue(TEXT("Both canonical equipment items project first"),
            (Projection[0] == 0 && Projection[1] == 1) ||
            (Projection[0] == 1 && Projection[1] == 0));
        TestEqual(TEXT("Filtered slots remain empty placeholders"),
            Projection[2], INDEX_NONE);
    }
    Widget->SetInventoryFilterCategory(EGridInventoryFilterCategory::Consumables);
    TestEqual(TEXT("Only real legacy potion is consumable"),
        Widget->GetVisibleInventoryItemCount(), 1);
    Widget->SetInventoryFilterCategory(EGridInventoryFilterCategory::BooksAndKeys);
    TestEqual(TEXT("Bogus legacy Book category cannot hijack shield"),
        Widget->GetVisibleInventoryItemCount(), 0);
    Widget->SetInventoryFilterCategory(EGridInventoryFilterCategory::Misc);
    TestEqual(TEXT("Canonical N0 arrows belong to Misc"),
        Widget->GetVisibleInventoryItemCount(), 1);

    Widget->SetInventoryFilterCategory(EGridInventoryFilterCategory::All);
    Widget->SetInventorySortMode(EGridInventorySortMode::TypeAscending);
    Widget->GetInventoryProjectionSourceSlotIndices(Projection);
    if (TestEqual(TEXT("All sort returns 4 slots"), Projection.Num(), 4))
    {
        TestEqual(TEXT("Ascending Weapon first"), Projection[0], 0);
        TestEqual(TEXT("Ascending Shield second"), Projection[1], 1);
        TestEqual(TEXT("Ascending Potion third"), Projection[2], 3);
        TestEqual(TEXT("Ascending Misc fourth"), Projection[3], 2);
    }
    Widget->SetInventorySortMode(EGridInventorySortMode::TypeDescending);
    Widget->GetInventoryProjectionSourceSlotIndices(Projection);
    if (TestEqual(TEXT("Descending sort returns 4 slots"), Projection.Num(), 4))
    {
        TestEqual(TEXT("Descending Misc first"), Projection[0], 2);
        TestEqual(TEXT("Descending Potion second"), Projection[1], 3);
        TestEqual(TEXT("Descending Shield third"), Projection[2], 1);
        TestEqual(TEXT("Descending Weapon fourth"), Projection[3], 0);
    }

    UGridInventorySlotWidget* Slot = CreateWidget<UGridInventorySlotWidget>(
        Fixture.World, UGridInventorySlotWidget::StaticClass());
    if (!TestNotNull(TEXT("Tooltip slot widget"), Slot)) return false;
    Slot->SetOwnerInventoryWidget(Widget);
    Slot->InitializeInventorySlot(EGridInventoryUiSlotType::Inventory, 0);
    Slot->SetItem(Sword);
    const FGridItemTooltipView SwordView = Slot->GetTooltipView();
    TestTrue(TEXT("Sword tooltip is valid"), SwordView.bValid);
    TestTrue(TEXT("Sword tooltip reuses the existing localized Weapon label"),
        SwordView.ItemType.EqualTo(
            NSLOCTEXT("GridInventoryTooltip", "ItemTypeWeapon", "Arme")));
    Slot->SetItem(Shield);
    const FGridItemTooltipView ShieldView = Slot->GetTooltipView();
    TestTrue(TEXT("Shield tooltip reuses the existing localized Shield label"),
        ShieldView.ItemType.EqualTo(
            NSLOCTEXT("GridInventoryTooltip", "ItemTypeShield", "Bouclier")));
    return true;
}

#endif
