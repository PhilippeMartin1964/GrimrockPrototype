#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridInventorySlotWidget.h"
#include "UI/GridInventoryWidget.h"
#include "UI/GridWeaponTooltipProjection.h"

namespace TooltipClean01
{
    UGridItemDefinitionAsset* Shield()
    {
        return LoadObject<UGridItemDefinitionAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RoundShield_01.DA_Weapon_RoundShield_01"));
    }

    FGridItemInstance Item(UGridItemDefinitionAsset* Def, int32 Absorbed)
    {
        FGridItemInstance I;
        I.RuntimeObjectId = FGuid::NewGuid();
        I.ItemDefinitionId = Def->ItemDefinitionId;
        I.ItemLevel = 1;
        I.Quantity = 1;
        I.AbsorbedEquipmentPhysicalArmor = Absorbed;
        I.OwnerType = EGridItemOwnerType::EquipmentSlot;
        I.OwnerCharacterIndex = 0;
        I.EquipmentSlot = EGridEquipmentSlot::OffHand;
        return I;
    }

    FString Fact(const FGridWeaponTooltipPresentation& Presentation, FName Id)
    {
        const FGridWeaponTooltipFact* F = Presentation.Facts.FindByPredicate(
            [Id](const FGridWeaponTooltipFact& Entry) { return Entry.StatId == Id; });
        return F ? F->ValueText.ToString() : FString();
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUITooltipClean01NominalShieldTest,
    "Grimrock.UI.TooltipClean01.ShieldNominalFiveAndRemaining",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUITooltipClean01NominalShieldTest::RunTest(const FString&)
{
    UGridItemDefinitionAsset* Def = TooltipClean01::Shield();
    if (!TestNotNull(TEXT("Production ARM075 round shield"), Def)) return false;
    const bool WasDirty = Def->GetOutermost()->IsDirty();

    for (int32 Spent : {0, 3, 5, 20})
    {
        FGridWeaponTooltipPresentation Presentation;
        if (!TestTrue(TEXT("Passive catalogue read model built"),
                FGridWeaponTooltipProjection::Build(
                    Def, TooltipClean01::Item(Def, Spent), Presentation)))
            return false;
        TestTrue(TEXT("Shield is passive"), Presentation.bPassive);
        TestEqual(TEXT("Nominal armor never diminishes with use"),
            TooltipClean01::Fact(Presentation, TEXT("WeaponPhysicalArmor")), FString(TEXT("5")));
        TestEqual(TEXT("Remaining/max reflects instance depletion and caps at zero"),
            TooltipClean01::Fact(Presentation, TEXT("WeaponPhysicalArmorRemaining")),
            FString::Printf(TEXT("%d/5"), FMath::Max(0, 5-Spent)));
        TestEqual(TEXT("Exactly one current/max row"),
            Presentation.Facts.FilterByPredicate([](const FGridWeaponTooltipFact& Fact) {
                return Fact.StatId == TEXT("WeaponPhysicalArmorRemaining");
            }).Num(), 1);
    }
    TestEqual(TEXT("Real DataAsset remains read-only"), Def->GetOutermost()->IsDirty(), WasDirty);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUITooltipClean01RecoveryAndPreviewTest,
    "Grimrock.UI.TooltipClean01.ShieldAfterVictoryAndGlobalComparison",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUITooltipClean01RecoveryAndPreviewTest::RunTest(const FString&)
{
    UGridItemDefinitionAsset* Def = TooltipClean01::Shield();
    UGridItemDefinitionAsset* Axe = LoadObject<UGridItemDefinitionAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_GreatAxe_01.DA_Weapon_GreatAxe_01"));
    if (!TestNotNull(TEXT("Production shield"), Def) ||
        !TestNotNull(TEXT("Production two-handed axe"), Axe)) return false;

    UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
    URPGClassAsset* Class = NewObject<URPGClassAsset>(Inventory);
    Class->ClassId = TEXT("TooltipClean01ShieldClass");
    Class->DisplayName = FText::FromString(TEXT("Shield class"));
    Class->HealthAtLevelOne = 20;
    Class->BasePhysicalArmor = 0;
    Class->BaseMagicalArmor = 3;
    if (!TestTrue(TEXT("Valid synthetic class"), Class->IsValidDefinition()) ||
        !TestTrue(TEXT("Register ARM075"), Inventory->RegisterItemDefinition(Def)) ||
        !TestTrue(TEXT("Register ARM031"), Inventory->RegisterItemDefinition(Axe))) return false;

    FGridCharacterInventoryState Character;
    Character.CharacterId = FGuid::NewGuid();
    Character.ClassId = Class->ClassId;
    Character.ClassDefinition = Class;
    Character.Level = 1;
    Character.Resources.CurrentHealth = 14;
    Character.Resources.CurrentPhysicalArmor = 0;
    Character.Resources.CurrentMagicalArmor = 1;
    Character.InventorySlots.SetNum(4);
    auto& State = Inventory->PartyInventoryState;
    State.ActiveCharacters.Add(Character);
    State.ActiveEquipment.SetNum(1);
    State.SelectedCharacterIndex = 0;

    State.ActiveEquipment[0].OffHand = TooltipClean01::Item(Def, 3);
    const FGuid Id = State.ActiveEquipment[0].OffHand.RuntimeObjectId;
    FGridItemInstance AxeItem;
    AxeItem.RuntimeObjectId = FGuid::NewGuid();
    AxeItem.ItemDefinitionId = Axe->ItemDefinitionId;
    AxeItem.ItemLevel = 1;
    AxeItem.Quantity = 1;
    AxeItem.OwnerType = EGridItemOwnerType::CharacterInventory;
    AxeItem.OwnerCharacterIndex = 0;
    State.ActiveCharacters[0].InventorySlots[0].bOccupied = true;
    State.ActiveCharacters[0].InventorySlots[0].Item = AxeItem;

    auto* Ui = NewObject<UGridInventoryWidget>();
    Ui->InventoryComponent = Inventory;
    auto* Slot = NewObject<UGridInventorySlotWidget>();
    Slot->SetOwnerInventoryWidget(Ui);
    Slot->InitializeInventorySlot(EGridInventoryUiSlotType::Inventory, 0);
    Slot->SetItem(AxeItem);

    const FGridItemTooltipView Before = Slot->GetTooltipView();
    if (!TestEqual(TEXT("One two-hand comparison"), Before.EquipmentComparisons.Num(), 1)) return false;
    const auto& BeforeFacts = Before.EquipmentComparisons[0].StatLines;
    const FGridItemTooltipStatLine* ArmorBefore = BeforeFacts.FindByPredicate(
        [](const auto& Fact) { return Fact.StatId == TEXT("CurrentPhysicalArmor"); });
    if (!TestNotNull(TEXT("Actual remaining physical row before victory"), ArmorBefore)) return false;
    TestEqual(TEXT("Displacing spent shield loses only 2 current points"), ArmorBefore->Delta, -2.f);
    TestEqual(TEXT("Equipment preview doesn't refill shield"),
        State.ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, 3);

    TestEqual(TEXT("One character refilled after normal victory"), Inventory->RefillArmorReservesAfterVictory(), 1);
    TestEqual(TEXT("Shield depletion reset"), State.ActiveEquipment[0].OffHand.AbsorbedEquipmentPhysicalArmor, 0);
    TestEqual(TEXT("Max shield remains 5"), Inventory->GetEffectiveCurrentPhysicalArmor(0), 5);
    TestEqual(TEXT("HP not healed by victory"), State.ActiveCharacters[0].Resources.CurrentHealth, 14);
    TestEqual(TEXT("Shield GUID unchanged"), State.ActiveEquipment[0].OffHand.RuntimeObjectId, Id);

    const FGridItemTooltipView After = Slot->GetTooltipView();
    if (!TestEqual(TEXT("After victory still one two-hand scenario"), After.EquipmentComparisons.Num(), 1)) return false;
    const FGridItemTooltipStatLine* ArmorAfter = After.EquipmentComparisons[0].StatLines.FindByPredicate(
        [](const auto& Fact) { return Fact.StatId == TEXT("CurrentPhysicalArmor"); });
    if (!TestNotNull(TEXT("Restored comparison physical row"), ArmorAfter)) return false;
    TestEqual(TEXT("Displacing full shield now loses its full 5 armor"), ArmorAfter->Delta, -5.f);

    FGridWeaponTooltipPresentation RestoredShield;
    TestTrue(TEXT("Shield fact built after victory"),
        FGridWeaponTooltipProjection::Build(Def, State.ActiveEquipment[0].OffHand, RestoredShield));
    TestEqual(TEXT("Restored shield displays full 5/5"),
        TooltipClean01::Fact(RestoredShield, TEXT("WeaponPhysicalArmorRemaining")), FString(TEXT("5/5")));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
