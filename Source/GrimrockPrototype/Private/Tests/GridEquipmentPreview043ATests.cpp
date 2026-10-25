#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridEquipmentPreviewTypes.h"

namespace EquipmentPreview043A
{
    UGridPartyInventoryComponent* MakeInventory()
    {
        UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
        Inventory->InitializeDefaultPartyIfNeeded();
        return Inventory;
    }

    UGridItemDefinitionAsset* Definition(UGridPartyInventoryComponent* Inventory,
        FName Id, EGridItemHandUsage Hands, EGridEquipmentSlot Slot)
    {
        UGridItemDefinitionAsset* Def = NewObject<UGridItemDefinitionAsset>(Inventory);
        Def->ItemDefinitionId = Id;
        Def->DisplayName = FText::FromName(Id);
        Def->ItemType = EGridItemType::Weapon;
        Def->Weight = 1.f;
        Def->HandUsage = Hands;
        Def->CompatibleEquipmentSlots.Add(Slot);
        return Inventory->RegisterItemDefinition(Def) ? Def : nullptr;
    }

    FGridItemInstance Item(const UGridItemDefinitionAsset* Def)
    {
        FGridItemInstance Instance;
        Instance.ItemDefinitionId = Def->ItemDefinitionId;
        Instance.RuntimeObjectId = FGuid::NewGuid();
        Instance.DisplayName = Def->DisplayName;
        Instance.Weight = Def->Weight;
        Instance.Quantity = 1;
        return Instance;
    }

    int32 FindSlot(const UGridPartyInventoryComponent* Inventory, const FGuid& Id)
    {
        const auto& Slots = Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots;
        for (int32 I = 0; I < Slots.Num(); ++I)
        {
            if (!Slots[I].IsEmpty() && Slots[I].Item.RuntimeObjectId == Id)
            {
                return I;
            }
        }
        return INDEX_NONE;
    }

    bool Install(UGridPartyInventoryComponent* Inventory,
        const FGridItemInstance& Instance, EGridEquipmentSlot Slot)
    {
        return Inventory->AddItemToCharacterInventory(0, Instance) &&
            Inventory->EquipItemFromInventorySlot(
                0, FindSlot(Inventory, Instance.RuntimeObjectId), Slot);
    }

    const FGridEquipmentPreviewDelta* Delta(
        const FGridEquipmentPreviewResult& Result, FName Id)
    {
        return Result.NumericDeltas.FindByPredicate(
            [Id](const FGridEquipmentPreviewDelta& Value)
            {
                return Value.StatId == Id;
            });
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEquipmentPreview043ATwoHandTest,
    "Grimrock.UI.EquipmentPreview043A.TwoHandsShieldArmorAndNoMutation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEquipmentPreview043ATwoHandTest::RunTest(const FString&)
{
    using namespace EquipmentPreview043A;
    UGridPartyInventoryComponent* Inventory = MakeInventory();
    UGridItemDefinitionAsset* Sword = Definition(Inventory, TEXT("PreviewSword"),
        EGridItemHandUsage::OneHanded, EGridEquipmentSlot::MainHand);
    UGridItemDefinitionAsset* Shield = Definition(Inventory, TEXT("PreviewShield"),
        EGridItemHandUsage::OneHanded, EGridEquipmentSlot::OffHand);
    UGridItemDefinitionAsset* GreatAxe = Definition(Inventory, TEXT("PreviewGreatAxe"),
        EGridItemHandUsage::TwoHanded, EGridEquipmentSlot::MainHand);
    if (!TestNotNull(TEXT("Sword registered"), Sword) ||
        !TestNotNull(TEXT("Shield registered"), Shield) ||
        !TestNotNull(TEXT("Great axe registered"), GreatAxe)) return false;

    Sword->EquipmentStatBonus.StrengthBonus = 2;
    Shield->EquipmentStatBonus.ArmorBonus = 5;
    Shield->EquipmentResistanceBonus.FireResistance = 3;
    GreatAxe->EquipmentStatBonus.StrengthBonus = 7;
    const FGridItemInstance SwordItem = Item(Sword);
    const FGridItemInstance ShieldItem = Item(Shield);
    const FGridItemInstance AxeItem = Item(GreatAxe);
    if (!TestTrue(TEXT("Equip initial sword"), Install(Inventory, SwordItem, EGridEquipmentSlot::MainHand)) ||
        !TestTrue(TEXT("Equip initial shield"), Install(Inventory, ShieldItem, EGridEquipmentSlot::OffHand)) ||
        !TestTrue(TEXT("Put great axe in bag"), Inventory->AddItemToCharacterInventory(0, AxeItem)))
        return false;

    auto& Equipment = Inventory->PartyInventoryState.ActiveEquipment[0];
    Equipment.OffHand.AbsorbedEquipmentPhysicalArmor = 3; // 5 nominal, 2 remaining
    const int32 Slot = FindSlot(Inventory, AxeItem.RuntimeObjectId);
    if (!TestTrue(TEXT("Axe source slot present"), Slot != INDEX_NONE)) return false;
    const FGuid BeforeSwordId = Equipment.MainHand.RuntimeObjectId;
    const FGuid BeforeShieldId = Equipment.OffHand.RuntimeObjectId;
    const int32 BeforeAbsorbed = Equipment.OffHand.AbsorbedEquipmentPhysicalArmor;
    const int32 BeforePhysicalArmor = Inventory->GetEffectiveCurrentPhysicalArmor(0);

    FGridEquipmentPreviewResult Preview;
    if (!TestTrue(TEXT("Read-only global preview created"),
        Inventory->PreviewEquipFromInventorySlot(0, Slot, EGridEquipmentSlot::OffHand, Preview)))
        return false;
    TestTrue(TEXT("Preview valid and equip possible"), Preview.bValidPreview && Preview.bCanEquipNow);
    TestTrue(TEXT("Two-hand candidate"), Preview.bCandidateUsesTwoHands);
    TestTrue(TEXT("Both hands released"), Preview.bWouldReleaseBothHands);
    TestEqual(TEXT("Two hand canonical target is main"), Preview.ActualTargetSlot,
        EGridEquipmentSlot::MainHand);
    if (!TestEqual(TEXT("Two distinct physical items displaced"), Preview.DisplacedItems.Num(), 2))
        return false;
    TestEqual(TEXT("Before shield has only 2 armor remaining"),
        Preview.DisplacedItems[1].RemainingPhysicalArmor, 2);
    TestEqual(TEXT("Shield loss reflects available not nominal physical armor"),
        Preview.EffectivePhysicalArmorAfter - Preview.EffectivePhysicalArmorBefore, -2);
    TestEqual(TEXT("Positive strength replaces sword bonus"),
        Preview.AfterBonuses.StrengthBonus - Preview.BeforeBonuses.StrengthBonus, 5);
    TestEqual(TEXT("Shield's fire resistance lost"),
        Preview.AfterResistances.FireResistance - Preview.BeforeResistances.FireResistance, -3);
    TestEqual(TEXT("All numeric fields have a projected delta"), Preview.NumericDeltas.Num(), 19);
    if (const auto* Armor = Delta(Preview, TEXT("CurrentPhysicalArmor")))
        TestEqual(TEXT("Effective armor delta row"), Armor->Delta, -2.f);
    else AddError(TEXT("Missing effective armor delta row"));
    if (const auto* Strength = Delta(Preview, TEXT("Strength")))
        TestEqual(TEXT("Character effective Strength increases"), Strength->Delta, 5.f);
    else AddError(TEXT("Missing Strength delta row"));

    // Preview MUST NOT change the current inventory, item absorbed armor or equipment.
    TestEqual(TEXT("Sword remains equipped during preview"), Equipment.MainHand.RuntimeObjectId, BeforeSwordId);
    TestEqual(TEXT("Shield remains equipped during preview"), Equipment.OffHand.RuntimeObjectId, BeforeShieldId);
    TestEqual(TEXT("Absorbed armor unchanged"), Equipment.OffHand.AbsorbedEquipmentPhysicalArmor, BeforeAbsorbed);
    TestEqual(TEXT("Effective armor unchanged"), Inventory->GetEffectiveCurrentPhysicalArmor(0), BeforePhysicalArmor);
    TestTrue(TEXT("Great axe remains in inventory"), FindSlot(Inventory, AxeItem.RuntimeObjectId) == Slot);

    // The authoritative equip transaction must match the predicted result.
    if (!TestTrue(TEXT("Commit actual two-hand equip"),
        Inventory->EquipItemFromInventorySlot(0, Slot, EGridEquipmentSlot::OffHand)))
        return false;
    TestEqual(TEXT("Real Strength equals prediction"),
        Inventory->ComputeCharacterEquipmentStatBonus(0).StrengthBonus, Preview.AfterBonuses.StrengthBonus);
    TestEqual(TEXT("Real fire resistance equals prediction"),
        Inventory->ComputeCharacterEquipmentResistances(0).FireResistance, Preview.AfterResistances.FireResistance);
    TestEqual(TEXT("Real effective armor equals prediction"),
        Inventory->GetEffectiveCurrentPhysicalArmor(0), Preview.EffectivePhysicalArmorAfter);
    TestEqual(TEXT("Real main hand has axe"), Equipment.MainHand.RuntimeObjectId, AxeItem.RuntimeObjectId);
    TestFalse(TEXT("Real offhand physically empty"), Equipment.OffHand.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEquipmentPreview043ACapacityTest,
    "Grimrock.UI.EquipmentPreview043A.FullBagReadOnlyBlock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEquipmentPreview043ACapacityTest::RunTest(const FString&)
{
    using namespace EquipmentPreview043A;
    UGridPartyInventoryComponent* Inventory = MakeInventory();
    auto* Sword = Definition(Inventory, TEXT("FullSword"), EGridItemHandUsage::OneHanded, EGridEquipmentSlot::MainHand);
    auto* Shield = Definition(Inventory, TEXT("FullShield"), EGridItemHandUsage::OneHanded, EGridEquipmentSlot::OffHand);
    auto* Axe = Definition(Inventory, TEXT("FullAxe"), EGridItemHandUsage::TwoHanded, EGridEquipmentSlot::MainHand);
    if (!Sword || !Shield || !Axe) return false;
    const auto SwordItem = Item(Sword);
    const auto ShieldItem = Item(Shield);
    const auto AxeItem = Item(Axe);
    if (!Install(Inventory, SwordItem, EGridEquipmentSlot::MainHand) ||
        !Install(Inventory, ShieldItem, EGridEquipmentSlot::OffHand)) return false;
    auto& Character = Inventory->PartyInventoryState.ActiveCharacters[0];
    Character.InventorySlots.SetNum(1);
    Character.InventorySlots[0].bOccupied = true;
    Character.InventorySlots[0].Item = AxeItem;

    FGridEquipmentPreviewResult Preview;
    TestTrue(TEXT("A full bag still yields an informational preview"),
        Inventory->PreviewEquipFromInventorySlot(0, 0, EGridEquipmentSlot::MainHand, Preview));
    TestTrue(TEXT("Projected state is still valid"), Preview.bValidPreview);
    TestFalse(TEXT("Full bag cannot commit two displaced items"), Preview.bCanEquipNow);
    TestTrue(TEXT("Explicit inventory-full status"),
        Preview.Status == EGridEquipmentPreviewStatus::InventoryFull);
    TestEqual(TEXT("Only source slot is freed"), Preview.FreeInventorySlotsAfterSourceRemoval, 1);
    TestEqual(TEXT("Both hand items need slots"), Preview.RequiredSlotsForDisplacedItems, 2);
    TestEqual(TEXT("Source candidate retained"), Character.InventorySlots[0].Item.RuntimeObjectId,
        AxeItem.RuntimeObjectId);
    TestEqual(TEXT("Main hand unchanged"), Inventory->PartyInventoryState.ActiveEquipment[0].MainHand.RuntimeObjectId,
        SwordItem.RuntimeObjectId);
    TestEqual(TEXT("Offhand unchanged"), Inventory->PartyInventoryState.ActiveEquipment[0].OffHand.RuntimeObjectId,
        ShieldItem.RuntimeObjectId);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEquipmentPreview043AHandRulesTest,
    "Grimrock.UI.EquipmentPreview043A.OneHandAndVirtualOffHand",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEquipmentPreview043AHandRulesTest::RunTest(const FString&)
{
    using namespace EquipmentPreview043A;
    UGridPartyInventoryComponent* Inventory = MakeInventory();
    auto* Sword = Definition(Inventory, TEXT("HandSword"), EGridItemHandUsage::OneHanded, EGridEquipmentSlot::MainHand);
    auto* Shield = Definition(Inventory, TEXT("HandShield"), EGridItemHandUsage::OneHanded, EGridEquipmentSlot::OffHand);
    auto* Pick = Definition(Inventory, TEXT("HandPick"), EGridItemHandUsage::OneHanded, EGridEquipmentSlot::MainHand);
    auto* Axe = Definition(Inventory, TEXT("HandAxe"), EGridItemHandUsage::TwoHanded, EGridEquipmentSlot::MainHand);
    if (!Sword || !Shield || !Pick || !Axe) return false;
    Shield->EquipmentStatBonus.ArmorBonus = 5;
    if (!Install(Inventory, Item(Sword), EGridEquipmentSlot::MainHand) ||
        !Install(Inventory, Item(Shield), EGridEquipmentSlot::OffHand)) return false;
    const FGridItemInstance PickItem = Item(Pick);
    if (!Inventory->AddItemToCharacterInventory(0, PickItem)) return false;
    FGridEquipmentPreviewResult OffhandPreview;
    if (!TestTrue(TEXT("One-hand pick may replace offhand shield per CanEquipToSlot"),
        Inventory->PreviewEquipFromInventorySlot(0, FindSlot(Inventory, PickItem.RuntimeObjectId),
            EGridEquipmentSlot::OffHand, OffhandPreview))) return false;
    TestFalse(TEXT("One-hand swap does not displace MainHand"), OffhandPreview.bWouldReleaseBothHands);
    TestEqual(TEXT("Only shield physically displaced"), OffhandPreview.DisplacedItems.Num(), 1);
    TestEqual(TEXT("Offhand target stays Offhand"), OffhandPreview.ActualTargetSlot, EGridEquipmentSlot::OffHand);
    TestEqual(TEXT("Offhand swap loses shield protection"),
        OffhandPreview.EffectivePhysicalArmorAfter - OffhandPreview.EffectivePhysicalArmorBefore, -5);

    // Now switch to a two-handed candidate, then preview a one-hander from OffHand:
    const FGridItemInstance AxeItem = Item(Axe);
    if (!Inventory->AddItemToCharacterInventory(0, AxeItem) ||
        !Inventory->EquipItemFromInventorySlot(0, FindSlot(Inventory, AxeItem.RuntimeObjectId),
            EGridEquipmentSlot::MainHand)) return false;
    FGridEquipmentPreviewResult ReplaceAxe;
    if (!TestTrue(TEXT("Preview replacing virtual OffHand of a two-hand axe"),
        Inventory->PreviewEquipFromInventorySlot(0, FindSlot(Inventory, PickItem.RuntimeObjectId),
            EGridEquipmentSlot::OffHand, ReplaceAxe))) return false;
    TestTrue(TEXT("Two-hand reservation is released"), ReplaceAxe.bWouldReleaseBothHands);
    TestEqual(TEXT("One physical axe displaced, not duplicated"), ReplaceAxe.DisplacedItems.Num(), 1);
    TestEqual(TEXT("One-hander canonical placement remains OffHand"),
        ReplaceAxe.ActualTargetSlot, EGridEquipmentSlot::OffHand);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEquipmentPreview043AProductionGreatAxeTest,
    "Grimrock.UI.EquipmentPreview043A.ProductionGreatAxeAndRoundShield",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEquipmentPreview043AProductionGreatAxeTest::RunTest(const FString&)
{
    using namespace EquipmentPreview043A;
    UGridItemDefinitionAsset* GreatAxe = LoadObject<UGridItemDefinitionAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_GreatAxe_01.DA_Weapon_GreatAxe_01"));
    UGridItemDefinitionAsset* Shield = LoadObject<UGridItemDefinitionAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_RoundShield_01.DA_Weapon_RoundShield_01"));
    if (!TestNotNull(TEXT("Production great axe asset"), GreatAxe) ||
        !TestNotNull(TEXT("Production round shield asset"), Shield)) return false;
    if (!TestTrue(TEXT("Great axe has effective TwoHanded hand usage"),
        GreatAxe->GetEffectiveHandUsage() == EGridItemHandUsage::TwoHanded)) return false;

    UGridPartyInventoryComponent* Inventory = MakeInventory();
    if (!TestTrue(TEXT("Register great axe"), Inventory->RegisterItemDefinition(GreatAxe)) ||
        !TestTrue(TEXT("Register shield"), Inventory->RegisterItemDefinition(Shield)))
        return false;
    FGridItemInstance ShieldItem = Item(Shield);
    ShieldItem.ItemLevel = 1;
    FGridItemInstance AxeItem = Item(GreatAxe);
    AxeItem.ItemLevel = 1;
    if (!TestTrue(TEXT("Equip real round shield"),
        Install(Inventory, ShieldItem, EGridEquipmentSlot::OffHand)) ||
        !TestTrue(TEXT("Add real great axe into bag"),
            Inventory->AddItemToCharacterInventory(0, AxeItem)))
        return false;
    FGridEquipmentPreviewResult Preview;
    if (!TestTrue(TEXT("Real catalogue great axe preview"),
        Inventory->PreviewEquipFromInventorySlot(0,
            FindSlot(Inventory, AxeItem.RuntimeObjectId),
            EGridEquipmentSlot::MainHand, Preview)))
        return false;
    TestTrue(TEXT("Great axe occupies both hands in preview"),
        Preview.bCandidateUsesTwoHands && Preview.bWouldReleaseBothHands);
    if (!TestEqual(TEXT("One physical shield displaced"), Preview.DisplacedItems.Num(), 1))
        return false;
    TestEqual(TEXT("Displaced real shield identity"),
        Preview.DisplacedItems[0].RuntimeObjectId, ShieldItem.RuntimeObjectId);
    TestEqual(TEXT("Shield ARM075 N1 expected nominal armor"),
        Shield->EquipmentStatBonus.ArmorBonus, 5);
    TestEqual(TEXT("Real shield armor loss is minus five when intact"),
        Preview.EffectivePhysicalArmorAfter - Preview.EffectivePhysicalArmorBefore, -5);
    TestTrue(TEXT("Great axe still in bag after preview"),
        FindSlot(Inventory, AxeItem.RuntimeObjectId) != INDEX_NONE);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
