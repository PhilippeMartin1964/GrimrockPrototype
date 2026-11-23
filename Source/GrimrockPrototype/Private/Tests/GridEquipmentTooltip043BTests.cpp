#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GridCanonicalWeightTestWorld.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridEquipmentTooltipGlobalProjection.h"
#include "UI/GridInventorySlotWidget.h"
#include "UI/GridInventoryWidget.h"
#include "UI/UILocalizationText.h"

namespace GlobalTooltip043B
{
    UGridOtherEquipmentDefinitionAsset* Define(UGridItemSystemAsset& Root,
        FName Id, EGridNonArmamentHandPolicy Hands, EGridEquipmentSlot Slot)
    {
        auto* Def = NewObject<UGridOtherEquipmentDefinitionAsset>(&Root);
        Def->ItemId = Id;
        Def->DisplayName = FText::FromName(Id);
        Def->WeightKg = 1.f;
        Def->HandPolicy = Hands;
        Def->EquipmentSlots.Add(Slot);
        Root.ItemDefinitions.Add(Def);
        return Def;
    }

    FGridItemInstance Item(FName Id)
    {
        FGridItemInstance It;
        It.RuntimeObjectId = FGuid::NewGuid();
        It.ItemDefinitionId = Id;
        It.Quantity = 1;
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
    const FName SwordId(TEXT("PreviewB_Sword"));
    const FName ShieldId(TEXT("PreviewB_Shield"));
    const FName AxeId(TEXT("PreviewB_GreatAxe"));
    FGridCanonicalWeightTestWorld World({}, [&](UGridItemSystemAsset& Root)
    {
        auto* Sword = Define(Root, SwordId, EGridNonArmamentHandPolicy::OneHanded, EGridEquipmentSlot::MainHand);
        auto* Shield = Define(Root, ShieldId, EGridNonArmamentHandPolicy::OneHanded, EGridEquipmentSlot::OffHand);
        auto* Axe = Define(Root, AxeId, EGridNonArmamentHandPolicy::TwoHanded, EGridEquipmentSlot::MainHand);
        Sword->Bonuses.StrengthBonus=1;
        Shield->Bonuses.ArmorBonus=5;
        Axe->Bonuses.StrengthBonus=4;
    });
    UGridPartyInventoryComponent* Inv = World.MakeInventory();
    if (!TestNotNull(TEXT("Inventory with registered equipment"), Inv)) return false;
    Inv->InitializeDefaultPartyIfNeeded();
    if (!Equip(Inv,Item(SwordId),EGridEquipmentSlot::MainHand) ||
        !Equip(Inv,Item(ShieldId),EGridEquipmentSlot::OffHand)) return false;
    const auto Candidate=Item(AxeId);
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
    const FName SwordId(TEXT("PreviewB_EquippedSword"));
    const FName ShieldId(TEXT("PreviewB_EquippedShield"));
    const FName PickId(TEXT("PreviewB_Pick"));
    FGridCanonicalWeightTestWorld World({}, [&](UGridItemSystemAsset& Root)
    {
        Define(Root, SwordId, EGridNonArmamentHandPolicy::OneHanded, EGridEquipmentSlot::MainHand);
        auto* Shield = Define(Root, ShieldId, EGridNonArmamentHandPolicy::OneHanded, EGridEquipmentSlot::OffHand);
        Define(Root, PickId, EGridNonArmamentHandPolicy::OneHanded, EGridEquipmentSlot::MainHand);
        Shield->Bonuses.ArmorBonus=5;
    });
    UGridPartyInventoryComponent* Inv = World.MakeInventory();
    if (!TestNotNull(TEXT("Inventory with registered equipment"), Inv)) return false;
    Inv->InitializeDefaultPartyIfNeeded();
    if (!Equip(Inv,Item(SwordId),EGridEquipmentSlot::MainHand) ||
        !Equip(Inv,Item(ShieldId),EGridEquipmentSlot::OffHand)) return false;
    const auto Candidate=Item(PickId);
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
    const FName SwordId(TEXT("PreviewB_FullSword"));
    const FName ShieldId(TEXT("PreviewB_FullShield"));
    const FName AxeId(TEXT("PreviewB_FullAxe"));
    FGridCanonicalWeightTestWorld World({}, [&](UGridItemSystemAsset& Root)
    {
        Define(Root, SwordId, EGridNonArmamentHandPolicy::OneHanded, EGridEquipmentSlot::MainHand);
        Define(Root, ShieldId, EGridNonArmamentHandPolicy::OneHanded, EGridEquipmentSlot::OffHand);
        Define(Root, AxeId, EGridNonArmamentHandPolicy::TwoHanded, EGridEquipmentSlot::MainHand);
    });
    UGridPartyInventoryComponent* Inv = World.MakeInventory();
    if (!TestNotNull(TEXT("Inventory with registered equipment"), Inv)) return false;
    Inv->InitializeDefaultPartyIfNeeded();
    if(!Equip(Inv,Item(SwordId),EGridEquipmentSlot::MainHand) ||
       !Equip(Inv,Item(ShieldId),EGridEquipmentSlot::OffHand))return false;
    const auto Candidate=Item(AxeId);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGlobalTooltip043BNeutralAbsorbedTest,
    "Grimrock.UI.EquipmentTooltip043B.NeutralAndAbsorbedShield",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGlobalTooltip043BNeutralAbsorbedTest::RunTest(const FString&)
{
    using namespace GlobalTooltip043B;
    const FName SwordId(TEXT("PreviewB_NeutralSword"));
    const FName ShieldId(TEXT("PreviewB_AbsorbedShield"));
    const FName AxeId(TEXT("PreviewB_NeutralGreatAxe"));
    FGridCanonicalWeightTestWorld World({}, [&](UGridItemSystemAsset& Root)
    {
        auto* Sword = Define(Root, SwordId, EGridNonArmamentHandPolicy::OneHanded, EGridEquipmentSlot::MainHand);
        auto* Shield = Define(Root, ShieldId, EGridNonArmamentHandPolicy::OneHanded, EGridEquipmentSlot::OffHand);
        auto* Axe = Define(Root, AxeId, EGridNonArmamentHandPolicy::TwoHanded, EGridEquipmentSlot::MainHand);
        Sword->Bonuses.StrengthBonus = 2;
        Axe->Bonuses.StrengthBonus = 2;
        Shield->Bonuses.ArmorBonus = 5;
        Shield->Resistances.FireResistance = 3;
    });
    UGridPartyInventoryComponent* Inv = World.MakeInventory();
    if (!TestNotNull(TEXT("Inventory with registered equipment"), Inv)) return false;
    Inv->InitializeDefaultPartyIfNeeded();
    if (!Equip(Inv, Item(SwordId), EGridEquipmentSlot::MainHand) ||
        !Equip(Inv, Item(ShieldId), EGridEquipmentSlot::OffHand)) return false;
    Inv->PartyInventoryState.ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor = 3;
    const FGridItemInstance Candidate = Item(AxeId);
    if (!Inv->AddItemToCharacterInventory(0, Candidate)) return false;

    const FGridItemTooltipView View = Hover(Inv, Candidate);
    if (!TestEqual(TEXT("One two-hand scenario"), View.EquipmentComparisons.Num(), 1))
        return false;
    const auto& Scenario = View.EquipmentComparisons[0];
    const auto* Strength = Stat(Scenario, TEXT("Strength"));
    const auto* Armor = Stat(Scenario, TEXT("CurrentPhysicalArmor"));
    const auto* Fire = Stat(Scenario, TEXT("FireResistance"));
    if (!TestNotNull(TEXT("Relevant neutral Strength row"), Strength) ||
        !TestNotNull(TEXT("Effective armor row"), Armor) ||
        !TestNotNull(TEXT("Fire resistance row"), Fire)) return false;
    TestEqual(TEXT("Strength +2 replaced by Strength +2"), Strength->Delta, 0.f);
    TestTrue(TEXT("Unchanged Strength uses neutral state"),
        Strength->DeltaState == EGridItemTooltipDeltaState::Neutral);
    TestEqual(TEXT("Neutral display is localized"),
        Strength->DeltaDisplayText.ToString(),
        FUILocalizationText::Get(TEXT("Inventory.Tooltip.Global.NeutralDelta")).ToString());
    TestEqual(TEXT("Shield contributes only 2 remaining armor"), Armor->Delta, -2.f);
    TestEqual(TEXT("Shield fire resistance is lost"), Fire->Delta, -3.f);
    TestTrue(TEXT("Irrelevant neutral Dexterity is omitted"),
        Stat(Scenario, TEXT("Dexterity")) == nullptr);
    TestEqual(TEXT("Shield absorbed armor unchanged in actual equipment"),
        Inv->PartyInventoryState.ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, 3);
    TestEqual(TEXT("Preview has not removed the shield"),
        Inv->PartyInventoryState.ActiveEquipment[0].OffHand.ItemDefinitionId,
        FName(TEXT("PreviewB_AbsorbedShield")));
    return true;
}
#endif // WITH_DEV_AUTOMATION_TESTS
