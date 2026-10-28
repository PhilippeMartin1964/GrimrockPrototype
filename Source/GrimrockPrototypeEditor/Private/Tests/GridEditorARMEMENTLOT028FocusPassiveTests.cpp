#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "ARMEMENT/GridWeaponPipelineManifest.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENTLOT028Focus,
    "Grimrock.RPG.ARMEMENT.LOT02.FocusPassiveEquipmentAndPersistence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENTLOT028Focus::RunTest(const FString&)
{
    FGridWeaponPipelineRow Row;
    Row.BuildKind = TEXT("FocusPassive");
    Row.EquipmentSlots = TEXT("OffHand");
    TestTrue(TEXT("Focus production mode supported"), Row.IsSupportedBuildKind());
    TestTrue(TEXT("Focus is passive, not a shield"), Row.IsPassive() && Row.IsFocus() && !Row.IsShield());
    TestFalse(TEXT("Focus never uses spell or physical throw"), Row.UsesRangedAction() || Row.IsThrowable());

    UGridWeaponCatalogueAsset* Catalogue = LoadObject<UGridWeaponCatalogueAsset>(
        nullptr, TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponCatalogue.DA_WeaponCatalogue"));
    if (!TestNotNull(TEXT("Real catalogue"), Catalogue)) return false;
    const FGridWeaponLevelProfile* N1 = Catalogue->FindProfile(TEXT("ARM070"), 1);
    if (!TestNotNull(TEXT("ARM070 canonical N1"), N1)) return false;
    TestTrue(TEXT("N1 is passive"), N1->IsValidProfile() &&
        !N1->DamageDice.bEnabled && N1->ActionPointCost == 0 && N1->RangeCells == 0);
    UGridItemDefinitionAsset* Focus = NewObject<UGridItemDefinitionAsset>();
    Focus->ItemDefinitionId = TEXT("Test_ARM070_Focus");
    Focus->ItemType = EGridItemType::Weapon;
    Focus->HandUsage = EGridItemHandUsage::NotHandHeld;
    Focus->CompatibleEquipmentSlots.Add(EGridEquipmentSlot::OffHand);
    Focus->WeaponArchetypeId = TEXT("ARM070");
    Focus->DefaultWeaponItemLevel = 1;
    Focus->WeaponCatalogue = Catalogue;
    Focus->bUseWeaponCatalogue = true;
    Focus->Weight = N1->WeightKg;
    Focus->EquipmentStatBonus.ArmorBonus = N1->PhysicalArmor;
    Focus->EquipmentStatBonus.MagicalArmorBonus = N1->MagicalArmor;
    TestTrue(TEXT("Focus definition is valid; OffHand only"), Focus->IsValidDefinition() &&
        Focus->CanEquipToSlot(EGridEquipmentSlot::OffHand) &&
        !Focus->CanProvideAttackFromSlot(EGridEquipmentSlot::OffHand) &&
        !Focus->CanEquipToSlot(EGridEquipmentSlot::MainHand));

    UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
    Inventory->InitializeDefaultPartyIfNeeded();
    if (!TestTrue(TEXT("Register focus"), Inventory->RegisterItemDefinition(Focus))) return false;
    FGridItemInstance Item;
    Item.ItemDefinitionId = Focus->ItemDefinitionId;
    Item.RuntimeObjectId = FGuid::NewGuid();
    Item.Quantity = 1;
    if (!TestTrue(TEXT("Apply actual item definition"), Inventory->ApplyItemDefinitionToInstance(Item)) ||
        !TestTrue(TEXT("Add to inventory"), Inventory->AddItemToCharacterInventory(0, Item)))
        return false;
    int32 Index = INDEX_NONE;
    const auto& Bag = Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots;
    for (int32 I = 0; I < Bag.Num(); ++I)
        if (!Bag[I].IsEmpty() && Bag[I].Item.RuntimeObjectId == Item.RuntimeObjectId) { Index = I; break; }
    if (!TestTrue(TEXT("Focus found in bag"), Index != INDEX_NONE) ||
        !TestTrue(TEXT("Equip real OffHand"), Inventory->EquipItemFromInventorySlot(0, Index, EGridEquipmentSlot::OffHand)))
        return false;

    FGridInventoryCharacterSummary Summary;
    TestTrue(TEXT("Summary uses equipment"), Inventory->GetCharacterSummary(0, Summary));
    TestEqual(TEXT("Canonical magical protection projected"),
        Summary.EquipmentStatBonus.MagicalArmorBonus, N1->MagicalArmor);
    const int32 BaseMagic = Inventory->PartyInventoryState.ActiveCharacters[0].Resources.CurrentMagicalArmor;
    TestEqual(TEXT("Effective magic reserve is base plus focus"), Inventory->GetEffectiveCurrentMagicalArmor(0),
        BaseMagic + N1->MagicalArmor);

    if (N1->MagicalArmor > 0)
    {
        const int32 Hit = FMath::Min(2, N1->MagicalArmor);
        TestEqual(TEXT("Equipment absorbs magical damage first"), Inventory->ApplyMagicalArmorDamage(0, Hit), Hit);
        TestEqual(TEXT("Absorption persisted on the focus instance"),
            Inventory->PartyInventoryState.ActiveEquipment[0].OffHand.AbsorbedEquipmentMagicalArmor, Hit);
        TestTrue(TEXT("Unequip preserves absorbed magic"),
            Inventory->UnequipItemToInventory(0, EGridEquipmentSlot::OffHand));
        TestEqual(TEXT("Bag does not grant equipped reserve"),
            Inventory->GetEffectiveCurrentMagicalArmor(0), BaseMagic);
        // The saved party copy retains depleted equipment even after transfer.
        FGridPartyInventoryState Snapshot = Inventory->PartyInventoryState;
        // The fixture uses InitializeDefaultPartyIfNeeded; mark it finalized
        // before exercising the actual SaveGame restore guard.
        Snapshot.bInitialCharacterCreationCompleted = true;
        UGridPartyInventoryComponent* Copy = NewObject<UGridPartyInventoryComponent>();
        Copy->InitializeDefaultPartyIfNeeded();
        FText Error;
        if (!TestTrue(TEXT("Restore inventory snapshot"), Copy->RestorePartyInventoryState(Snapshot, Error)))
        {
            AddError(Error.ToString());
            return false;
        }
        const auto& RestoredBag = Copy->PartyInventoryState.ActiveCharacters[0].InventorySlots;
        bool bFoundDepleted = false;
        for (const FGridInventorySlot& Slot : RestoredBag)
            if (!Slot.IsEmpty() && Slot.Item.ItemDefinitionId == Focus->ItemDefinitionId)
                bFoundDepleted |= Slot.Item.AbsorbedEquipmentMagicalArmor == Hit;
        TestTrue(TEXT("Magic armor depletion survives SaveGame state"), bFoundDepleted);
    }
    return true;
}
#endif
