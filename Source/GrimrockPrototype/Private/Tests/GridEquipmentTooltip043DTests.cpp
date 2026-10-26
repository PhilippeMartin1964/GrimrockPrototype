#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventorySlotWidget.h"
#include "UI/GridInventoryWidget.h"
#include "UI/UILocalizationText.h"

namespace EquipmentTooltip043D
{
    FGridItemInstance MakeItem(const UGridItemDefinitionAsset* Definition)
    {
        FGridItemInstance Item;
        Item.RuntimeObjectId = FGuid::NewGuid();
        Item.ItemDefinitionId = Definition->ItemDefinitionId;
        Item.DisplayName = Definition->DisplayName;
        Item.ItemLevel = 1;
        Item.Quantity = 1;
        Item.OwnerType = EGridItemOwnerType::CharacterInventory;
        Item.OwnerCharacterIndex = 0;
        return Item;
    }

    FGridItemTooltipView BuildView(UGridPartyInventoryComponent* Inventory,
        const UGridItemDefinitionAsset* Definition)
    {
        UGridInventoryWidget* Parent = NewObject<UGridInventoryWidget>();
        Parent->InventoryComponent = Inventory;
        UGridInventorySlotWidget* Slot = NewObject<UGridInventorySlotWidget>();
        Slot->SetOwnerInventoryWidget(Parent);
        Slot->InitializeInventorySlot(EGridInventoryUiSlotType::Inventory, INDEX_NONE);
        Slot->SetItem(MakeItem(Definition));
        return Slot->GetTooltipView();
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEquipmentTooltip043DHandUsageTest,
    "Grimrock.UI.EquipmentTooltip043D.TwoHandsHeader",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEquipmentTooltip043DHandUsageTest::RunTest(const FString&)
{
    UGridItemDefinitionAsset* Axe = LoadObject<UGridItemDefinitionAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_GreatAxe_01.DA_Weapon_GreatAxe_01"));
    UGridItemDefinitionAsset* Pick = LoadObject<UGridItemDefinitionAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_WarPick_01.DA_Weapon_WarPick_01"));
    if (!TestNotNull(TEXT("Production ARM031 Great Axe"), Axe) ||
        !TestNotNull(TEXT("Production ARM025 War Pick"), Pick))
        return false;

    TestTrue(TEXT("Great Axe is genuinely two-handed"),
        Axe->GetEffectiveHandUsage() == EGridItemHandUsage::TwoHanded);
    TestTrue(TEXT("War Pick is genuinely one-handed"),
        Pick->GetEffectiveHandUsage() == EGridItemHandUsage::OneHanded);

    const bool AxeWasDirty = Axe->GetOutermost()->IsDirty();
    const bool PickWasDirty = Pick->GetOutermost()->IsDirty();
    UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
    Inventory->InitializeDefaultPartyIfNeeded();
    if (!TestTrue(TEXT("Great Axe definition registered"), Inventory->RegisterItemDefinition(Axe)) ||
        !TestTrue(TEXT("War Pick definition registered"), Inventory->RegisterItemDefinition(Pick)))
        return false;

    const FText TwoHands = FUILocalizationText::Get(TEXT("Inventory.Tooltip.Equipment.TwoHands"));
    if (!TestFalse(TEXT("Two hands key exists in the generated ST_UI"), TwoHands.IsEmpty()))
        return false;

    const FGridItemTooltipView AxeView = EquipmentTooltip043D::BuildView(Inventory, Axe);
    const FGridItemTooltipView PickView = EquipmentTooltip043D::BuildView(Inventory, Pick);
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
    TestEqual(TEXT("Great Axe DataAsset not modified"), Axe->GetOutermost()->IsDirty(), AxeWasDirty);
    TestEqual(TEXT("War Pick DataAsset not modified"), Pick->GetOutermost()->IsDirty(), PickWasDirty);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
