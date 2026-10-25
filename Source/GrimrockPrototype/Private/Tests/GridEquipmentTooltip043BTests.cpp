#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "UI/GridEquipmentTooltipGlobalProjection.h"
#include "UI/GridInventorySlotWidget.h"
#include "UI/GridInventoryWidget.h"
#include "UI/UILocalizationText.h"

namespace GlobalTooltip043B
{
    UGridPartyInventoryComponent* Inventory()
    {
        auto* Result = NewObject<UGridPartyInventoryComponent>();
        Result->InitializeDefaultPartyIfNeeded();
        return Result;
    }

    UGridItemDefinitionAsset* Define(UGridPartyInventoryComponent* Owner,
        FName Id, EGridItemHandUsage Usage, EGridEquipmentSlot Slot)
    {
        auto* Def = NewObject<UGridItemDefinitionAsset>(Owner);
        Def->ItemDefinitionId = Id;
        Def->DisplayName = FText::FromName(Id);
        Def->ItemType = EGridItemType::Weapon;
        Def->HandUsage = Usage;
        Def->CompatibleEquipmentSlots.Add(Slot);
        Def->Weight = 1.f;
        return Owner->RegisterItemDefinition(Def) ? Def : nullptr;
    }

    FGridItemInstance Item(const UGridItemDefinitionAsset* Definition)
    {
        FGridItemInstance It;
        It.RuntimeObjectId = FGuid::NewGuid();
        It.ItemDefinitionId = Definition->ItemDefinitionId;
        It.DisplayName = Definition->DisplayName;
        It.Quantity = 1;
        It.Weight = Definition->Weight;
        return It;
    }

    int32 SlotOf(const UGridPartyInventoryComponent* Inventory, const FGuid& Id)
    {
        const auto& Slots = Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots;
        for (int32 I=0; I<Slots.Num(); ++I)
            if (!Slots[I].IsEmpty() && Slots[I].Item.RuntimeObjectId == Id) return I;
        return INDEX_NONE;
    }

    bool Equip(UGridPartyInventoryComponent* Inv, const FGridItemInstance& It, EGridEquipmentSlot Slot)
    {
        return Inv->AddItemToCharacterInventory(0, It) &&
            Inv->EquipItemFromInventorySlot(0, SlotOf(Inv, It.RuntimeObjectId), Slot);
    }

    FGridItemTooltipView Hover(UGridPartyInventoryComponent* Inv, const FGridItemInstance& It,
        EGridInventoryUiSlotType SlotType=EGridInventoryUiSlotType::Inventory)
    {
        auto* UI = NewObject<UGridInventoryWidget>();
        UI->InventoryComponent = Inv;
        auto* Slot = NewObject<UGridInventorySlotWidget>();
        Slot->SetOwnerInventoryWidget(UI);
        Slot->InitializeInventorySlot(SlotType, SlotOf(Inv, It.RuntimeObjectId));
        Slot->SetItem(It);
        return Slot->GetTooltipView();
    }

    const FGridItemTooltipStatLine* Stat(const FGridItemTooltipEquipmentComparison& Group, FName Id)
    {
        return Group.StatLines.FindByPredicate([Id](const auto& Line) { return Line.StatId == Id; });
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlobalTooltip043BGreatAxeTest,
    "Grimrock.UI.EquipmentTooltip043B.GreatAxeShieldAndBeforeAfter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlobalTooltip043BGreatAxeTest::RunTest(const FString&)
{
    using namespace GlobalTooltip043B;
    auto* Inv=Inventory();
    auto* Sword=Define(Inv, TEXT("PreviewB_Sword"), EGridItemHandUsage::OneHanded, EGridEquipmentSlot::MainHand);
    auto* Shield=Define(Inv, TEXT("PreviewB_Shield"), EGridItemHandUsage::OneHanded, EGridEquipmentSlot::OffHand);
    auto* Axe=Define(Inv, TEXT("PreviewB_GreatAxe"), EGridItemHandUsage::TwoHanded, EGridEquipmentSlot::MainHand);
    if (!Sword || !Shield || !Axe) return false;
    Sword->EquipmentStatBonus.StrengthBonus=1;
    Shield->EquipmentStatBonus.ArmorBonus=5;
    Axe->EquipmentStatBonus.StrengthBonus=4;
    if (!Equip(Inv,Item(Sword),EGridEquipmentSlot::MainHand) ||
        !Equip(Inv,Item(Shield),EGridEquipmentSlot::OffHand)) return false;
    const auto Candidate=Item(Axe);
    if (!Inv->AddItemToCharacterInventory(0,Candidate)) return false;
    const auto View=Hover(Inv,Candidate);
    if (!TestEqual(TEXT("One global two-handed scenario"),View.EquipmentComparisons.Num(),1))
        return false;
    const auto& Scenario=View.EquipmentComparisons[0];
    TestTrue(TEXT("Single group represents full equipment"),Scenario.bGlobalEquipmentScenario);
    TestTrue(TEXT("Two-hand title is localized"),!Scenario.SlotLabel.IsEmpty());
    TestTrue(TEXT("Both displaced physical names included"),
        Scenario.EquippedItemDisplayText.ToString().Contains(TEXT("PreviewB_Sword")) &&
        Scenario.EquippedItemDisplayText.ToString().Contains(TEXT("PreviewB_Shield")));
    const auto* Armor=Stat(Scenario,TEXT("CurrentPhysicalArmor"));
    const auto* Strength=Stat(Scenario,TEXT("Strength"));
    if (!TestNotNull(TEXT("Effective shield armor loss"),Armor) ||
        !TestNotNull(TEXT("Global Strength gain"),Strength)) return false;
    TestEqual(TEXT("Shield loses five current armor"),Armor->Delta,-5.f);
    TestEqual(TEXT("Global armor change is five, independent of base armor"),
        Armor->EquippedValue - Armor->ItemValue, 5.f);
    TestTrue(TEXT("Loss is red"),Armor->DeltaState==EGridItemTooltipDeltaState::Negative);
    TestEqual(TEXT("Strength gain counts removal of previous sword"),Strength->Delta,3.f);
    TestTrue(TEXT("Strength gain is green"),Strength->DeltaState==EGridItemTooltipDeltaState::Positive);
    TestEqual(TEXT("No unexpected data mutation"),
        Inv->PartyInventoryState.ActiveEquipment[0].OffHand.ItemDefinitionId,
        FName(TEXT("PreviewB_Shield")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlobalTooltip043BOffHandTest,
    "Grimrock.UI.EquipmentTooltip043B.OneHandOffHandScenario",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlobalTooltip043BOffHandTest::RunTest(const FString&)
{
    using namespace GlobalTooltip043B;
    auto* Inv=Inventory();
    auto* Sword=Define(Inv,TEXT("PreviewB_EquippedSword"),EGridItemHandUsage::OneHanded,EGridEquipmentSlot::MainHand);
    auto* Shield=Define(Inv,TEXT("PreviewB_EquippedShield"),EGridItemHandUsage::OneHanded,EGridEquipmentSlot::OffHand);
    auto* Pick=Define(Inv,TEXT("PreviewB_Pick"),EGridItemHandUsage::OneHanded,EGridEquipmentSlot::MainHand);
    if (!Sword || !Shield || !Pick) return false;
    Shield->EquipmentStatBonus.ArmorBonus=5;
    if (!Equip(Inv,Item(Sword),EGridEquipmentSlot::MainHand) ||
        !Equip(Inv,Item(Shield),EGridEquipmentSlot::OffHand)) return false;
    const auto Candidate=Item(Pick);
    if (!Inv->AddItemToCharacterInventory(0,Candidate)) return false;
    const auto View=Hover(Inv,Candidate);
    if (!TestEqual(TEXT("A one-handed weapon has both legal scenarios"),
        View.EquipmentComparisons.Num(),2)) return false;
    TestEqual(TEXT("Main-hand target first"),View.EquipmentComparisons[0].EquipmentSlot,
        EGridEquipmentSlot::MainHand);
    TestEqual(TEXT("Offhand scenario second"),View.EquipmentComparisons[1].EquipmentSlot,
        EGridEquipmentSlot::OffHand);
    const auto* Armor=Stat(View.EquipmentComparisons[1],TEXT("CurrentPhysicalArmor"));
    if (!TestNotNull(TEXT("Offhand pick shows lost shield armor"),Armor))return false;
    TestEqual(TEXT("Replacing shield costs 5 armor"),Armor->Delta,-5.f);
    TestTrue(TEXT("Mainhand replacement doesn't displace shield"),
        !View.EquipmentComparisons[0].EquippedItemDisplayText.ToString().Contains(TEXT("PreviewB_EquippedShield")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlobalTooltip043BFullBagTest,
    "Grimrock.UI.EquipmentTooltip043B.FullBagWarning",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlobalTooltip043BFullBagTest::RunTest(const FString&)
{
    using namespace GlobalTooltip043B;
    auto* Inv=Inventory();
    auto* Sword=Define(Inv,TEXT("PreviewB_FullSword"),EGridItemHandUsage::OneHanded,EGridEquipmentSlot::MainHand);
    auto* Shield=Define(Inv,TEXT("PreviewB_FullShield"),EGridItemHandUsage::OneHanded,EGridEquipmentSlot::OffHand);
    auto* Axe=Define(Inv,TEXT("PreviewB_FullAxe"),EGridItemHandUsage::TwoHanded,EGridEquipmentSlot::MainHand);
    if(!Sword||!Shield||!Axe)return false;
    if(!Equip(Inv,Item(Sword),EGridEquipmentSlot::MainHand) ||
       !Equip(Inv,Item(Shield),EGridEquipmentSlot::OffHand))return false;
    const auto Candidate=Item(Axe);
    auto& Slots=Inv->PartyInventoryState.ActiveCharacters[0].InventorySlots;
    Slots.SetNum(1);
    Slots[0].bOccupied=true;
    Slots[0].Item=Candidate;
    const auto View=Hover(Inv,Candidate);
    if(!TestEqual(TEXT("A blocked two-hand preview remains visible"),
         View.EquipmentComparisons.Num(),1))return false;
    const auto& G=View.EquipmentComparisons[0];
    FFormatNamedArguments Args;
    Args.Add(TEXT("Required"), FText::AsNumber(2));
    Args.Add(TEXT("Available"), FText::AsNumber(1));
    const FText LocalizedWarning = FText::Format(
        FUILocalizationText::Get(TEXT("Inventory.Tooltip.Global.InventoryFull")), Args);
    TestTrue(TEXT("Inventory full warning appears in localized summary"),
        !LocalizedWarning.IsEmpty() &&
        G.EquippedItemDisplayText.ToString().Contains(LocalizedWarning.ToString()));
    TestTrue(TEXT("Preview does not equip axe"),
        Inv->PartyInventoryState.ActiveEquipment[0].OffHand.IsValid());
    return true;
}
#endif // WITH_DEV_AUTOMATION_TESTS
