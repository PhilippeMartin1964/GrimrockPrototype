#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventorySlotWidget.h"
#include "UI/GridInventoryWidget.h"
#include "UI/GridItemTooltipWidget.h"
#include "UI/GridItemTooltipEquipmentComparisonWidget.h"
#include "UI/GridInventoryUiTypes.h"

namespace
{
    FGridItemInstance MakeInventoryItem(const UGridItemDefinitionAsset* Definition, int32 Level)
    {
        FGridItemInstance Item;
        Item.RuntimeObjectId = FGuid::NewGuid();
        Item.ItemDefinitionId = Definition->ItemDefinitionId;
        Item.ItemLevel = Level;
        Item.Quantity = 1;
        Item.OwnerType = EGridItemOwnerType::CharacterInventory;
        Item.OwnerCharacterIndex = 0;
        return Item;
    }

    FGridItemTooltipStatLine Stat(FName Id, int32 Delta)
    {
        FGridItemTooltipStatLine Line;
        Line.StatId = Id;
        Line.Label = FText::FromName(Id);
        Line.bHasComparison = true;
        Line.Delta = Delta;
        Line.DeltaState = Delta > 0
            ? EGridItemTooltipDeltaState::Positive :
            (Delta < 0 ? EGridItemTooltipDeltaState::Negative
                : EGridItemTooltipDeltaState::Neutral);
        Line.ValueText = FText::AsNumber(Delta);
        Line.DeltaDisplayText = FText::AsNumber(Delta);
        return Line;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridWeaponTooltip04NativeLegacyTest,
    "Grimrock.UI.WeaponTooltip04.NativeLegacyStatsPreserved",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWeaponTooltip04NativeLegacyTest::RunTest(const FString&)
{
    UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
    FGridCharacterInventoryState Character;
    Character.CharacterId = FGuid::NewGuid();
    Character.DisplayName = FText::FromString(TEXT("Native tooltip test"));
    Character.InventorySlots.SetNum(2);
    Inventory->PartyInventoryState.ActiveCharacters.Add(Character);
    Inventory->PartyInventoryState.ActiveEquipment.SetNum(1);
    Inventory->PartyInventoryState.SelectedCharacterIndex = 0;
    Inventory->PartyInventoryState.MaxActiveCharacters = 6;

    UGridItemDefinitionAsset* Legacy = NewObject<UGridItemDefinitionAsset>();
    Legacy->ItemDefinitionId = TEXT("INDUS04_2B_LegacyBelt");
    Legacy->DisplayName = FText::FromString(TEXT("Legacy Belt"));
    Legacy->ItemType = EGridItemType::Jewelry;
    Legacy->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::Belt);
    Legacy->EquipmentStatBonus.StrengthBonus = 3;
    Legacy->EquipmentResistanceBonus.FireResistance = 7;
    if (!TestTrue(TEXT("Legacy definition registered"),
        Inventory->RegisterItemDefinition(Legacy)))
        return false;

    UGridInventoryWidget* Owner = NewObject<UGridInventoryWidget>();
    Owner->InventoryComponent = Inventory;
    UGridInventorySlotWidget* Slot = NewObject<UGridInventorySlotWidget>();
    Slot->SetOwnerInventoryWidget(Owner);
    Slot->InitializeInventorySlot(EGridInventoryUiSlotType::Inventory, 0);
    Slot->SetItem(MakeInventoryItem(Legacy, 0));

    const FGridItemTooltipView View = Slot->GetTooltipView();
    TestTrue(TEXT("Existing tooltip view is valid"), View.bValid);
    TestFalse(TEXT("Legacy does not invent a weapon profile"), View.WeaponProfile.bHasProfile);
    const auto* Strength = View.StatLines.FindByPredicate(
        [](const FGridItemTooltipStatLine& F) { return F.StatId == TEXT("Strength"); });
    const auto* Fire = View.StatLines.FindByPredicate(
        [](const FGridItemTooltipStatLine& F) { return F.StatId == TEXT("FireResistance"); });
    TestNotNull(TEXT("Strength remains in native read model"), Strength);
    TestNotNull(TEXT("Fire resistance remains in native read model"), Fire);
    TestEqual(TEXT("Legacy compatibility comparison slot preserved"),
        View.EquipmentComparisons.Num(), 1);
    if (View.EquipmentComparisons.Num() != 1) return false;
    const FGridItemTooltipEquipmentComparison& Comparison = View.EquipmentComparisons[0];
    TestEqual(TEXT("Empty Belt slot preserved"), Comparison.EquipmentSlot,
        EGridEquipmentSlot::Belt);
    TestFalse(TEXT("Empty slot has no weapon attack comparison"),
        !Comparison.WeaponFacts.IsEmpty());
    const auto* CompStrength = Comparison.StatLines.FindByPredicate(
        [](const FGridItemTooltipStatLine& F) { return F.StatId == TEXT("Strength"); });
    if (!TestNotNull(TEXT("Legacy strength equipment delta preserved"), CompStrength))
        return false;
    TestEqual(TEXT("Strength candidate delta"), CompStrength->Delta, 3.0f);
    TestTrue(TEXT("Strength is positive"), CompStrength->DeltaState ==
        EGridItemTooltipDeltaState::Positive);
    TestFalse(TEXT("No suppression of legacy armor on a non-catalogue item"),
        UGridItemTooltipWidget::ShouldOmitLegacyArmor(
            View, Stat(TEXT("Armor"), 1)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridWeaponTooltip04ShieldNoDuplicateTest,
    "Grimrock.UI.WeaponTooltip04.NativeShieldArmorSingleAuthority",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridWeaponTooltip04ShieldNoDuplicateTest::RunTest(const FString&)
{
    const FGridItemTooltipStatLine Armor = Stat(TEXT("Armor"), 2);
    const FGridItemTooltipStatLine Strength = Stat(TEXT("Strength"), 1);
    FGridItemTooltipView Candidate;
    Candidate.bValid = true;
    Candidate.WeaponProfile.bHasProfile = true;
    Candidate.WeaponProfile.bPassive = true;
    Candidate.StatLines.Add(Armor);
    Candidate.StatLines.Add(Strength);
    FGridWeaponTooltipFact RealArmor;
    RealArmor.StatId = TEXT("WeaponPhysicalArmor");
    RealArmor.ValueText = FText::FromString(TEXT("15"));
    Candidate.WeaponProfile.Facts.Add(RealArmor);

    TestTrue(TEXT("Suppress old generic Armor when N12 shield armor is shown"),
        UGridItemTooltipWidget::ShouldOmitLegacyArmor(Candidate, Armor));
    TestFalse(TEXT("Never suppress other legacy bonuses"),
        UGridItemTooltipWidget::ShouldOmitLegacyArmor(Candidate, Strength));
    Candidate.WeaponProfile.bPassive = false;
    TestFalse(TEXT("Attack-capable items retain generic Armor bonus"),
        UGridItemTooltipWidget::ShouldOmitLegacyArmor(Candidate, Armor));
    Candidate.WeaponProfile.bPassive = true;
    Candidate.WeaponProfile.Facts.Reset();
    TestFalse(TEXT("No shield armor fact means no suppression"),
        UGridItemTooltipWidget::ShouldOmitLegacyArmor(Candidate, Armor));

    FGridItemTooltipEquipmentComparison Group;
    Group.EquipmentSlot = EGridEquipmentSlot::OffHand;
    Group.StatLines.Add(Armor);
    Group.StatLines.Add(Strength);
    TestFalse(TEXT("Legacy comparison Armor stays without real profile"),
        UGridItemTooltipEquipmentComparisonWidget::ShouldOmitLegacyArmor(
            Group, Armor));
    FGridWeaponTooltipComparisonFact ActualArmor;
    ActualArmor.StatId = TEXT("WeaponPhysicalArmor");
    ActualArmor.CandidateText = FText::FromString(TEXT("15"));
    ActualArmor.EquippedText = FText::FromString(TEXT("5"));
    Group.WeaponFacts.Add(ActualArmor);
    TestTrue(TEXT("Native comparison slot uses real shield Armor only once"),
        UGridItemTooltipEquipmentComparisonWidget::ShouldOmitLegacyArmor(
            Group, Armor));
    TestFalse(TEXT("Native comparison still shows Strength"),
        UGridItemTooltipEquipmentComparisonWidget::ShouldOmitLegacyArmor(
            Group, Strength));

    // A reparented WBP lacking its Designer children must never blank a valid tooltip.
    UGridItemTooltipWidget* Presenter = NewObject<UGridItemTooltipWidget>();
    // Intentionally missing Designer bindings must still emit an error in PIE.
    // The error is expected in THIS negative Automation fixture.
    AddExpectedError(TEXT("Tooltip presenter refused for GridItemTooltipWidget: missing"),
        EAutomationExpectedErrorFlags::Contains, 1);
    TestFalse(TEXT("Presenter refuses incomplete UMG rather than displaying partial stats"),
        Presenter->PresentTooltipView(Candidate));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
