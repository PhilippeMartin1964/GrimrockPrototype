#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GridCanonicalWeightTestWorld.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventorySlotWidget.h"
#include "UI/GridInventoryWidget.h"
#include "UI/UILocalizationText.h"

namespace EquipmentTooltip043D
{
    FGridItemInstance MakeItem(FName ItemId)
    {
        FGridItemInstance Item;
        Item.RuntimeObjectId = FGuid::NewGuid();
        Item.ItemDefinitionId = ItemId;
        Item.ItemLevel = 1;
        Item.Quantity = 1;
        Item.OwnerType = EGridItemOwnerType::CharacterInventory;
        Item.OwnerCharacterIndex = 0;
        return Item;
    }

    FGridItemTooltipView BuildView(UGridPartyInventoryComponent* Inventory,
        FName ItemId)
    {
        UGridInventoryWidget* Parent = NewObject<UGridInventoryWidget>();
        Parent->InventoryComponent = Inventory;
        UGridInventorySlotWidget* Slot = NewObject<UGridInventorySlotWidget>();
        Slot->SetOwnerInventoryWidget(Parent);
        Slot->InitializeInventorySlot(EGridInventoryUiSlotType::Inventory, INDEX_NONE);
        Slot->SetItem(MakeItem(ItemId));
        return Slot->GetTooltipView();
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEquipmentTooltip043DHandUsageTest,
    "Grimrock.UI.EquipmentTooltip043D.TwoHandsHeader",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEquipmentTooltip043DHandUsageTest::RunTest(const FString&)
{
    FGridCanonicalWeightTestWorld Fixture({});
    const UGridItemSubsystem* Registry = Fixture.Scope.Registry;
    FGridResolvedItem Axe;
    FGridResolvedItem Pick;
    if (!TestNotNull(TEXT("Canonical tooltip test world"), Fixture.World) ||
        !TestNotNull(TEXT("Production ItemSystem registry"), Registry) ||
        !TestTrue(TEXT("Production ARM031 Great Axe N1 resolves"),
            Registry && Registry->ResolveItem(TEXT("Weapon_GreatAxe_01"), 1, Axe)) ||
        !TestTrue(TEXT("Production ARM025 War Pick N1 resolves"),
            Registry && Registry->ResolveItem(TEXT("Weapon_WarPick_01"), 1, Pick)))
        return false;

    TestTrue(TEXT("Great Axe is genuinely two-handed"),
        Axe.bIsArmament && Axe.Weapon.HandPolicy == EGridHandPolicy::TwoHanded);
    TestTrue(TEXT("War Pick is a one-handed weapon usable in both hands"),
        Pick.bIsArmament &&
        Pick.Weapon.HandPolicy != EGridHandPolicy::TwoHanded &&
        Pick.Weapon.CanEquipInHand(EGridEquipmentSlot::MainHand) &&
        Pick.Weapon.CanEquipInHand(EGridEquipmentSlot::OffHand));

    UGridItemSystemAsset* Root = Registry->LoadConfiguredItemSystemForPreview();
    if (!TestNotNull(TEXT("Read-only production root"), Root)) return false;
    const bool bRootDirty = Root->GetOutermost()->IsDirty();
    UGridPartyInventoryComponent* Inventory = Fixture.MakeInventory();
    if (!TestNotNull(TEXT("World-bound canonical inventory"), Inventory))
        return false;
    Inventory->InitializeDefaultPartyIfNeeded();
    if (!TestNull(TEXT("No historical Great Axe definition registered"),
            Inventory->FindItemDefinition(Axe.ItemId)) ||
        !TestNull(TEXT("No historical War Pick definition registered"),
            Inventory->FindItemDefinition(Pick.ItemId)))
        return false;

    const FText TwoHands = FUILocalizationText::Get(TEXT("Inventory.Tooltip.Equipment.TwoHands"));
    if (!TestFalse(TEXT("Two hands key exists in the generated ST_UI"), TwoHands.IsEmpty()))
        return false;

    const FGridItemTooltipView AxeView = EquipmentTooltip043D::BuildView(Inventory, Axe.ItemId);
    const FGridItemTooltipView PickView = EquipmentTooltip043D::BuildView(Inventory, Pick.ItemId);
    if (!TestTrue(TEXT("Great Axe tooltip valid"), AxeView.bValid) ||
        !TestTrue(TEXT("War Pick tooltip valid"), PickView.bValid))
        return false;

    TestTrue(TEXT("Great Axe header uses localized two-hands label"),
        AxeView.CompatibleSlotsText.EqualTo(TwoHands));
    TestFalse(TEXT("Great Axe header does not use MainHand technical label"),
        AxeView.CompatibleSlotsText.EqualTo(
            FUILocalizationText::Get(TEXT("Inventory.Tooltip.Global.MainHand"))));
    TestTrue(TEXT("War Pick keeps main and offhand usage labels"),
        PickView.CompatibleSlotsText.ToString().Contains(
            FUILocalizationText::Get(TEXT("Inventory.Tooltip.Global.MainHand")).ToString()) &&
        PickView.CompatibleSlotsText.ToString().Contains(
            FUILocalizationText::Get(TEXT("Inventory.Tooltip.Global.OffHand")).ToString()));
    TestFalse(TEXT("War Pick is not mislabeled two handed"),
        PickView.CompatibleSlotsText.EqualTo(TwoHands));
    TestEqual(TEXT("Canonical ItemSystem package remains read-only"),
        Root->GetOutermost()->IsDirty(), bRootDirty);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
