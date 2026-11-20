#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/Items/GridCanonicalInventoryCategoryTags.h"
#include "../Runtime/GridCanonicalItemInteractionReadModel.h"
#include "../Runtime/GridCanonicalEquipmentSlotsReadModel.h"
#include "../Runtime/GridCanonicalInventoryStackReadModel.h"
#include "Tests/GridCanonicalLootRegistryTestUtils.h"
#include "UObject/StrongObjectPtr.h"
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
#include "UI/GridQuickItemSurfaceTooltipText.h"
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
    TestFalse(TEXT("Unknown potion has no canonical category"),
        LegacyType.bCanonicalIdentity || LegacyType.bResolved);
    TestEqual(TEXT("Unknown cannot inherit Potion type"), LegacyType.ItemType, EGridItemType::None);
    TestEqual(TEXT("Isolated fixture retains Potion type"),
        FGridCanonicalInventoryItemTypeReadModel::Resolve(nullptr, RealLegacy, Potion).ItemType, EGridItemType::Potion);
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
    TestEqual(TEXT("Unknown identity cannot enter Consumables"),
        Widget->GetVisibleInventoryItemCount(), 0);
    Widget->SetInventoryFilterCategory(EGridInventoryFilterCategory::BooksAndKeys);
    TestEqual(TEXT("Bogus legacy Book category cannot hijack shield"),
        Widget->GetVisibleInventoryItemCount(), 0);
    Widget->SetInventoryFilterCategory(EGridInventoryFilterCategory::Misc);
    TestEqual(TEXT("Canonical arrows and uncategorized diagnostic slot belong to Misc"),
        Widget->GetVisibleInventoryItemCount(), 2);

    Widget->SetInventoryFilterCategory(EGridInventoryFilterCategory::All);
    Widget->SetInventorySortMode(EGridInventorySortMode::TypeAscending);
    Widget->GetInventoryProjectionSourceSlotIndices(Projection);
    if (TestEqual(TEXT("All sort returns 4 slots"), Projection.Num(), 4))
    {
        TestEqual(TEXT("Ascending uncategorized first"), Projection[0], 3);
        TestEqual(TEXT("Ascending Weapon second"), Projection[1], 0);
        TestEqual(TEXT("Ascending Shield third"), Projection[2], 1);
        TestEqual(TEXT("Ascending Misc fourth"), Projection[3], 2);
    }
    Widget->SetInventorySortMode(EGridInventorySortMode::TypeDescending);
    Widget->GetInventoryProjectionSourceSlotIndices(Projection);
    if (TestEqual(TEXT("Descending sort returns 4 slots"), Projection.Num(), 4))
    {
        TestEqual(TEXT("Descending Misc first"), Projection[0], 2);
        TestEqual(TEXT("Descending Shield second"), Projection[1], 1);
        TestEqual(TEXT("Descending Weapon third"), Projection[2], 0);
        TestEqual(TEXT("Descending uncategorized fourth"), Projection[3], 3);
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
    Slot->SetItem(Potion);
    const auto UnknownView = Slot->GetTooltipView();
    TestEqual(TEXT("Unknown tooltip only presents technical ID"), UnknownView.DisplayName.ToString(), PotionId.ToString());
    TestEqual(TEXT("Unknown tooltip does not expose historical mass"), UnknownView.UnitWeight, 0.0f);
    TestTrue(TEXT("Unknown tooltip grants no read/equip/hotbar"), !UnknownView.bReadable && !UnknownView.bEquippable && !UnknownView.bCanAssignToHotbar);
    Slot->SetItem(Shield);
    const FGridItemTooltipView ShieldView = Slot->GetTooltipView();
    TestTrue(TEXT("Shield tooltip reuses the existing localized Shield label"),
        ShieldView.ItemType.EqualTo(
            NSLOCTEXT("GridInventoryTooltip", "ItemTypeShield", "Bouclier")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X61LootPresentationCategory,
    "Grimrock.RPG.ARM.CLEAN06_4X61.LootPresentationCategories",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X61LootPresentationCategory::RunTest(const FString&)
{
    using namespace GridCanonicalInventoryCategoryTags;
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real root"), Root.Get()) ||
        !TestTrue(TEXT("Exact native registry shape"), GridCanonicalLootRegistryTestUtils::HasExactRegistryShape(*Root.Get())))
        return false;
    TStrongObjectPtr<UGridItemSystemAsset> Preview(NewObject<UGridItemSystemAsset>());
    Preview->WeaponCatalogue = Root->WeaponCatalogue;
    Preview->WeaponVisualCatalogue = Root->WeaponVisualCatalogue;
    Preview->ActionCatalogue = Root->ActionCatalogue;
    Preview->ActionPresentationCatalogue = Root->ActionPresentationCatalogue;
    Preview->ItemDefinitions = Root->ItemDefinitions;
    const FName Ids[] = {TEXT("GoblinKnife"), TEXT("EmptyVial"), TEXT("Item_RatMeat"), TEXT("Item_RatTooth")};
    const FName Tags[] = {Weapon(), Component(), Food(), Component()};
    const EGridItemType Types[] = {EGridItemType::Weapon, EGridItemType::Component, EGridItemType::Food, EGridItemType::Component};
    const int32 Capacities[] = {1,10,99,99};
    // Before the native transaction the same runtime projection is
    // exercised with an isolated preview. Afterwards test the real assets.
    if (Root->ItemDefinitions.Num() == 34)
    {
        for (int32 I = 0; I < 4; ++I)
        {
            UGridNonArmamentDefinitionAsset* D = nullptr;
            if (I == 0) D = NewObject<UGridNonArmamentDefinitionAsset>(Preview.Get());
            else if (I == 2)
            {
                auto* FoodItem = NewObject<UGridConsumableDefinitionAsset>(Preview.Get());
                FoodItem->MaxStack = Capacities[I];
                D = FoodItem;
            }
            else
            {
                auto* Stack = NewObject<UGridGenericStackableDefinitionAsset>(Preview.Get());
                Stack->MaxStack = Capacities[I];
                D = Stack;
            }
            D->ItemId = Ids[I];
            D->DisplayName = FText::FromName(Ids[I]);
            D->Tags = {Tags[I]};
            Preview->ItemDefinitions.Add(D);
        }
    }
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("38-item isolated resolver"), Registry->RegisterItemSystem(Preview.Get(), Errors))) return false;
    for (int32 I = 0; I < 4; ++I)
    {
        FGridItemInstance Item;
        Item.RuntimeObjectId = FGuid::NewGuid();
        Item.ItemDefinitionId = Ids[I];
        Item.ItemLevel = 0;
        Item.Quantity = 1;
        auto* Wrong = NewObject<UGridItemDefinitionAsset>(GI.Get());
        Wrong->ItemDefinitionId = Ids[I];
        Wrong->ItemType = EGridItemType::Book;
        Wrong->CompatibleEquipmentSlots = {EGridEquipmentSlot::MainHand};
        Wrong->bStackable = true;
        Wrong->MaxStackSize = 3;
        const auto Category = FGridCanonicalInventoryItemTypeReadModel::Resolve(Registry.Get(), Wrong, Item);
        TestTrue(TEXT("Registered N0 category resolved"), Category.bCanonicalIdentity && Category.bResolved);
        TestEqual(TEXT("Retain original inventory type without old definition"), Category.ItemType, Types[I]);
        TestEqual(TEXT("Same existing filter category"), ResolveGridInventoryFilterCategory(Category.ItemType),
            ResolveGridInventoryFilterCategory(Types[I]));
        const auto Slots = FGridCanonicalEquipmentSlotsReadModel::Resolve(Registry.Get(), Wrong, Item);
        TestTrue(TEXT("Category never grants a physical hand"), Slots.bResolved && Slots.AllowedSlots.IsEmpty());
        const auto Interaction = FGridCanonicalItemInteractionReadModel::Resolve(Registry.Get(), Wrong, Item);
        TestTrue(TEXT("No read/consume/hotbar/throw capability invented"),
            Interaction.bCanonicalIdentity && Interaction.bResolved && !Interaction.bCanRead &&
            !Interaction.bCanConsumeLegacy && !Interaction.bCanConsumeCanonical &&
            !Interaction.bCanAddToHotbar && !Interaction.bPhysicalThrowHotbar);
        const auto Stack = FGridCanonicalInventoryStackReadModel::Resolve(Registry.Get(), Wrong, Item);
        TestEqual(TEXT("Exact capacity wins over stale source"), Stack.MaxStackSize, Capacities[I]);
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean064X63PotionPresentationOnly,
    "Grimrock.RPG.ARM.CLEAN06_4X63.PotionPresentationDoesNotGrantUse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X63PotionPresentationOnly::RunTest(const FString&)
{
    namespace Tags = GridCanonicalInventoryCategoryTags;
    TStrongObjectPtr<UGridItemSystemAsset> Root(LoadObject<UGridItemSystemAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem")));
    if (!TestNotNull(TEXT("Real root"), Root.Get())) return false;
    TStrongObjectPtr<UGridItemSystemAsset> Preview(NewObject<UGridItemSystemAsset>());
    Preview->WeaponCatalogue = Root->WeaponCatalogue;
    Preview->WeaponVisualCatalogue = Root->WeaponVisualCatalogue;
    Preview->ActionCatalogue = Root->ActionCatalogue;
    Preview->ActionPresentationCatalogue = Root->ActionPresentationCatalogue;
    Preview->ItemDefinitions = Root->ItemDefinitions;
    auto* Definition = NewObject<UGridConsumableDefinitionAsset>(Preview.Get());
    Definition->ItemId = TEXT("Item_X63_PresentationOnly");
    Definition->DisplayName = FText::FromName(Definition->ItemId);
    Definition->Tags = {TEXT("QuickItem.Alchemy"), Tags::Potion()};
    Preview->ItemDefinitions.Add(Definition);
    TStrongObjectPtr<UGameInstance> GI(NewObject<UGameInstance>());
    TStrongObjectPtr<UGridItemSubsystem> Registry(NewObject<UGridItemSubsystem>(GI.Get()));
    TArray<FString> Errors;
    if (!TestTrue(TEXT("Presentation-only candidate registers"), Registry->RegisterItemSystem(Preview.Get(), Errors))) return false;
    FGridItemInstance Item;
    Item.RuntimeObjectId = FGuid::NewGuid();
    Item.ItemDefinitionId = Definition->ItemId;
    Item.Quantity = 1;
    Item.ItemLevel = 0;
    auto* Wrong = NewObject<UGridItemDefinitionAsset>(GI.Get());
    Wrong->ItemDefinitionId = Item.ItemDefinitionId;
    Wrong->ItemType = EGridItemType::Weapon;
    const auto View = FGridCanonicalInventoryItemTypeReadModel::Resolve(Registry.Get(), Wrong, Item);
    TestTrue(TEXT("Category resolves without historical definition authority"), View.bCanonicalIdentity && View.bResolved);
    TestEqual(TEXT("Explicit Potion category"), View.ItemType, EGridItemType::Potion);
    TestEqual(TEXT("Existing Consumables filter"), ResolveGridInventoryFilterCategory(View.ItemType),
        EGridInventoryFilterCategory::Consumables);
    const auto Interaction = FGridCanonicalItemInteractionReadModel::Resolve(Registry.Get(), Wrong, Item);
    TestTrue(TEXT("A category grants no unbound action"), Interaction.bResolved &&
        !Interaction.bCanConsumeLegacy && !Interaction.bCanConsumeCanonical && !Interaction.bCanAddToHotbar &&
        !Interaction.bPhysicalThrowHotbar);
    const auto Slots = FGridCanonicalEquipmentSlotsReadModel::Resolve(Registry.Get(), Wrong, Item);
    TestTrue(TEXT("Potion category grants no slot"), Slots.bResolved && Slots.AllowedSlots.IsEmpty());
    Definition->Tags.Add(Tags::Weapon());
    Errors.Reset();
    TestFalse(TEXT("Conflicting Potion/Weapon tags refused"), Preview->Validate(Errors));
    TestTrue(TEXT("Category conflict diagnostic"), !Errors.IsEmpty());
    return !HasAnyErrors();
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean065E1PointerlessTooltip,
    "Grimrock.RPG.ARM.CLEAN06_5E1.PointerlessInventoryTooltip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean065E1PointerlessTooltip::RunTest(const FString&)
{
    ARMCLEAN063AU::FFixture F;
    if (!TestTrue(TEXT("Live production registry"), F.Ready())) return false;
    auto* UI = CreateWidget<UGridInventoryWidget>(F.World, UGridInventoryWidget::StaticClass());
    auto* Slot = CreateWidget<UGridInventorySlotWidget>(F.World, UGridInventorySlotWidget::StaticClass());
    if (!TestNotNull(TEXT("Inventory UI"), UI) || !TestNotNull(TEXT("Slot UI"), Slot)) return false;
    UI->InventoryComponent = F.Inventory;
    Slot->SetOwnerInventoryWidget(UI);
    Slot->InitializeInventorySlot(EGridInventoryUiSlotType::Inventory, 0);
    const FName SwordId(TEXT("Weapon_StraightSword_01"));
    const FGridItemInstance Candidate = F.Make(SwordId, 20);
    F.Put(0, Candidate);
    FGridItemInstance Equipped = F.Make(SwordId, 1);
    Equipped.OwnerType = EGridItemOwnerType::EquipmentSlot;
    Equipped.EquipmentSlot = EGridEquipmentSlot::MainHand;
    F.Inventory->PartyInventoryState.ActiveEquipment[0].MainHand = Equipped;
    TestNull(TEXT("No historical sword definition"), F.Inventory->FindItemDefinition(SwordId));
    Slot->SetItem(Candidate);
    const auto SwordView = Slot->GetTooltipView();
    TestTrue(TEXT("Pointerless N20 weapon facts"), SwordView.WeaponProfile.bHasProfile &&
        SwordView.WeaponProfile.ItemLevel == 20 && !SwordView.WeaponProfile.Facts.IsEmpty());
    const auto* Comparison = SwordView.EquipmentComparisons.FindByPredicate(
        [](const auto& C) { return C.EquipmentSlot == EGridEquipmentSlot::MainHand; });
    if (TestNotNull(TEXT("Source-backed main-hand scenario"), Comparison))
        TestTrue(TEXT("Pointerless N20 versus N1 comparison"), Comparison->bGlobalEquipmentScenario &&
            !Comparison->WeaponFacts.IsEmpty());
    TestEqual(TEXT("Preview leaves equipped GUID alone"), F.Inventory->PartyInventoryState.ActiveEquipment[0].MainHand.RuntimeObjectId,
        Equipped.RuntimeObjectId);
    auto* Stale = F.RegisterWrong(SwordId, EGridItemType::Book);
    Stale->ReadText = FText::FromString(TEXT("Stale readable text"));
    Stale->EquipmentStatBonus.StrengthBonus = 999;
    Slot->SetItem(Candidate);
    const auto Contradictory = Slot->GetTooltipView();
    TestFalse(TEXT("Stale book cannot give sword read capability"), Contradictory.bReadable);
    TestEqual(TEXT("Same weapon facts with contradictory source"), Contradictory.WeaponProfile.Facts.Num(), SwordView.WeaponProfile.Facts.Num());

    auto Torch = F.Make(TEXT("Item_Torch"), 0);
    Torch.bLightsEnabled = true;
    TestNull(TEXT("No historical torch definition"), F.Inventory->FindItemDefinition(Torch.ItemDefinitionId));
    F.Put(0, Torch);
    Slot->SetItem(Torch);
    const auto TorchView = Slot->GetTooltipView();
    TestTrue(TEXT("Native torch emitter and enabled instance"), TorchView.bProvidesLight && TorchView.bLightEnabled);
    auto Oil = F.Make(TEXT("Item_Flask_Oil"), 0);
    F.Put(0, Oil);
    Slot->SetItem(Oil);
    const auto OilView = Slot->GetTooltipView();
    const auto Use = FGridCanonicalQuickItemUseReadModel::Resolve(F.Registry, Oil);
    if (TestTrue(TEXT("Native oil action resolves"), Use.bResolved))
    {
        const FText SurfaceText = FGridQuickItemSurfaceTooltipText::Build(Use.Action);
        TestTrue(TEXT("Oil description retains localized mechanical usage"),
            !SurfaceText.IsEmpty() && OilView.Description.ToString().Contains(SurfaceText.ToString()));
    }
    TestTrue(TEXT("Pointerless oil can bind hotbar"), OilView.bCanAssignToHotbar);
    Oil.ItemLevel = 1;
    F.Put(0, Oil);
    Slot->SetItem(Oil);
    TestFalse(TEXT("Invalid physical N1 non-ARM gives no shortcut"), Slot->GetTooltipView().bCanAssignToHotbar);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean065E4ContextEquip,
    "Grimrock.RPG.ARM.CLEAN06_5E4.PointerlessContextEquip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean065E4ContextEquip::RunTest(const FString&)
{
    ARMCLEAN063AU::FFixture F;
    if (!TestTrue(TEXT("Live production registry"), F.Ready())) return false;
    auto* UI = CreateWidget<UGridInventoryWidget>(F.World, UGridInventoryWidget::StaticClass());
    if (!TestNotNull(TEXT("Inventory UI"), UI)) return false;
    UI->OwningPartyPawn = F.Party;
    UI->InventoryComponent = F.Inventory;
    const auto Sword = F.Make(TEXT("Weapon_StraightSword_01"), 20);
    const auto Shield = F.Make(TEXT("Weapon_RoundShield_01"), 1);
    F.Put(0, Sword);
    F.Put(1, Shield);
    TestNull(TEXT("Sword needs no historical definition"), F.Inventory->FindItemDefinition(Sword.ItemDefinitionId));
    auto* WrongShield = F.RegisterWrong(Shield.ItemDefinitionId, EGridItemType::Book);
    WrongShield->CompatibleEquipmentSlots.Reset();
    const auto EquipViaMenu = [&](int32 SlotIndex, EGridEquipmentSlot Hand)
    {
        FGridFacingTargetContext Target;
        TArray<FGridItemContextAction> Actions;
        if (!TestTrue(TEXT("Real inventory context menu builds"),
            UI->BuildContextActionsForSlot(EGridInventoryUiSlotType::Inventory, SlotIndex, Target, Actions)))
            return false;
        const int32 Index = Actions.IndexOfByPredicate([Hand](const FGridItemContextAction& Action)
        {
            return Action.ActionType == EGridItemActionType::Equip && Action.EquipmentSlot == Hand && Action.bEnabled;
        });
        if (!TestTrue(TEXT("Canonical hand action enabled"), Index != INDEX_NONE)) return false;
        return TestTrue(TEXT("Actual context action by index executes"),
            UI->ExecuteInventoryContextActionByIndex(EGridInventoryUiSlotType::Inventory, SlotIndex, Index));
    };
    if (!EquipViaMenu(0, EGridEquipmentSlot::MainHand)) return false;
    if (!EquipViaMenu(1, EGridEquipmentSlot::OffHand)) return false;
    FGridItemInstance Actual;
    TestTrue(TEXT("Sword now physically equipped"), F.Inventory->GetEquippedItem(0, EGridEquipmentSlot::MainHand, Actual));
    TestEqual(TEXT("Sword GUID preserved"), Actual.RuntimeObjectId, Sword.RuntimeObjectId);
    TestEqual(TEXT("Sword level preserved"), Actual.ItemLevel, 20);
    TestTrue(TEXT("Shield now physically equipped despite stale Book"), F.Inventory->GetEquippedItem(0, EGridEquipmentSlot::OffHand, Actual));
    TestEqual(TEXT("Shield GUID preserved"), Actual.RuntimeObjectId, Shield.RuntimeObjectId);
    TestEqual(TEXT("Shield level preserved"), Actual.ItemLevel, 1);
    TestTrue(TEXT("Source inventory slots cleared"),
        F.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[0].IsEmpty() &&
        F.Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots[1].IsEmpty());
    // The exact same real menu route must still refuse invalid level data.
    const auto Invalid = F.Make(Sword.ItemDefinitionId, 0);
    F.Put(2, Invalid);
    FGridFacingTargetContext Target;
    TArray<FGridItemContextAction> Actions;
    TestTrue(TEXT("Invalid physical source can be inspected"),
        UI->BuildContextActionsForSlot(EGridInventoryUiSlotType::Inventory, 2, Target, Actions));
    TestFalse(TEXT("Invalid ARM does not offer equip"), Actions.ContainsByPredicate(
        [](const FGridItemContextAction& Action) { return Action.ActionType == EGridItemActionType::Equip; }));
    return !HasAnyErrors();
}
#endif
