#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGStartingHealthPotionAuthoring.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GrimrockPartySaveGame.h"

namespace ARMEMENT0115Test
{
    const TCHAR* ItemPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Potion_Healing_01.DA_Item_Potion_Healing_01");

    URPGClassAsset* LoadClass(const TCHAR* ClassId)
    {
        const FString Path = FString::Printf(
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_%s.DA_Class_%s"),
            ClassId, ClassId);
        return LoadObject<URPGClassAsset>(nullptr, *Path);
    }

    FRPGCharacterCreationRequest Request(URPGClassAsset* Class, URPGRaceAsset* Human, FName Kit)
    {
        FRPGCharacterCreationRequest Result;
        Result.DisplayName = FText::FromString(TEXT("N1 Potion Test"));
        Result.RaceDefinition = Human;
        Result.ClassDefinition = Class;
        Result.CombatActionSourceClassDefinition = Class;
        Result.StartingEquipmentKitId = Kit;
        return Result;
    }

    const FGridItemInstance* FindBagPotion(const FGridCharacterInventoryState& Character)
    {
        for (const FGridInventorySlot& Slot : Character.InventorySlots)
        {
            if (!Slot.IsEmpty() && Slot.Item.ItemDefinitionId == FName(TEXT("Item_Potion_Healing_01")))
            {
                return &Slot.Item;
            }
        }
        return nullptr;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0115AuthoringContract,
    "Grimrock.RPG.ARMEMENT01.15.HealthPotion.AuthoringAndQuickItemContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0115AuthoringContract::RunTest(const FString&)
{
    UGridItemDefinitionAsset* Potion = NewObject<UGridItemDefinitionAsset>(GetTransientPackage());
    FRPGStartingHealthPotionAuthoring::Configure(*Potion);
    if (!TestTrue(TEXT("Approved transient potion definition is valid"),
        FRPGStartingHealthPotionAuthoring::Matches(*Potion))) return false;

    TestEqual(TEXT("Potion restores 20 health"), Potion->QuickItemCombatAction.EffectProfile.RestoreHealth, 20);
    TestEqual(TEXT("Potion costs one AP"), Potion->QuickItemCombatAction.ActionPointCost, 1);
    TestTrue(TEXT("Potion contains positive potion tag"), Potion->ItemTags.Contains(TEXT("QuickItem.Potion.Positive")));
    FGridCombatActionDefinition Action;
    if (!TestTrue(TEXT("QuickItem is executable by canonical builder"),
        Potion->BuildInventoryCombatActionDefinition(Action))) return false;
    TestEqual(TEXT("Uses item-backed stable action identity"), Action.ActionId,
        FName(TEXT("Use_Item_Potion_Healing_01")));
    TestEqual(TEXT("Consumable quantity is one"), Action.ResourceCosts.SourceItemQuantityCost, 1);
    TestEqual(TEXT("No talent grant or skill bonus is baked into potion"),
        Action.QuickItemScaling.RestoreHealthSkillRankScale, 0);

    FGridCombatActionContribution Contribution;
    Contribution.Definition = Action;
    Contribution.SourceDefinitionId = Potion->ItemDefinitionId;
    Contribution.AvailableSourceQuantity = 1;

    FGridCombatActionCatalogContext Context;
    Context.CharacterIndex = 0;
    Context.CharacterId = FGuid::NewGuid();
    Context.bCombatActive = true;
    Context.bActiveCombatant = true;
    Context.bEnableQuickItemExecutors = true;
    Context.RemainingActionPoints = 4;
    Context.CurrentHealth = 5;
    Context.MaximumHealth = 30;
    TArray<FGridAvailableCombatAction> Actions;
    FGridCombatActionCatalog::Build(Context, { Contribution }, Actions);
    if (!TestEqual(TEXT("Healing action exists at low health"), Actions.Num(), 1)) return false;
    TestTrue(TEXT("Healing available while wounded"), Actions[0].bEnabled);

    Context.CurrentHealth = Context.MaximumHealth;
    FGridCombatActionCatalog::Build(Context, { Contribution }, Actions);
    if (!TestEqual(TEXT("Healing action still listed at full health"), Actions.Num(), 1)) return false;
    TestEqual(TEXT("No healing or consumption at maximum health"), Actions[0].AvailabilityReason,
        EGridCombatActionAvailabilityReason::NoApplicableEffect);
    Context.CurrentHealth = 5;
    Contribution.AvailableSourceQuantity = 0;
    FGridCombatActionCatalog::Build(Context, { Contribution }, Actions);
    if (!TestEqual(TEXT("Potion action still listed without a potion"), Actions.Num(), 1)) return false;
    TestFalse(TEXT("Cannot consume absent potion"), Actions[0].bEnabled);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0115ProductionKits,
    "Grimrock.RPG.ARMEMENT01.15.HealthPotion.PriestAndApothecaryNewGame",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0115ProductionKits::RunTest(const FString&)
{
    UGridItemDefinitionAsset* Potion = LoadObject<UGridItemDefinitionAsset>(
        nullptr, ARMEMENT0115Test::ItemPath);
    if (!TestNotNull(TEXT("The single N1 potion DataAsset has been authored"), Potion)) return false;
    if (!TestTrue(TEXT("Potion package matches approved definition"),
        FRPGStartingHealthPotionAuthoring::Matches(*Potion))) return false;

    URPGRaceAsset* Human = LoadObject<URPGRaceAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Race_Human.DA_Race_Human"));
    if (!TestNotNull(TEXT("Production race exists"), Human)) return false;
    struct FCase
    {
        const TCHAR* ClassId;
        const TCHAR* KitId;
        const TCHAR* WeaponId;
    };
    const FCase Cases[] = {
        {TEXT("Priest"), TEXT("Kit_Priest_Restoration"), TEXT("Weapon_HolyScepter_01")},
        {TEXT("Alchemist"), TEXT("Kit_Alchemist_Apothecary"), TEXT("Weapon_Dagger_01")}
    };
    for (const FCase& Case : Cases)
    {
        URPGClassAsset* Class = ARMEMENT0115Test::LoadClass(Case.ClassId);
        if (!TestNotNull(TEXT("Production class exists"), Class)) return false;
        const int32 PreviousKitCount = Class->StartingEquipmentKits.Num();
        const FName PreviousDefault = Class->DefaultStartingEquipmentKitId;
        UGridPartyInventoryComponent* Party = NewObject<UGridPartyInventoryComponent>();
        Party->InitializeDefaultPartyIfNeeded();
        FText Error;
        if (!Party->CreateInitialCharacter(
            ARMEMENT0115Test::Request(Class, Human, FName(Case.KitId)), Error))
        {
            AddError(FString::Printf(TEXT("%s: %s"), Case.KitId, *Error.ToString()));
            return false;
        }
        const FGridCharacterInventoryState& Character = Party->PartyInventoryState.ActiveCharacters[0];
        const FGridCharacterEquipmentState& Equipment = Party->PartyInventoryState.ActiveEquipment[0];
        TestEqual(TEXT("Exact class orientation remains recorded"), Character.StartingEquipmentKitId,
            FName(Case.KitId));
        TestEqual(TEXT("N1 class weapon remains equipped"), Equipment.MainHand.ItemDefinitionId,
            FName(Case.WeaponId));
        const FGridItemInstance* Item = ARMEMENT0115Test::FindBagPotion(Character);
        if (!TestNotNull(TEXT("Health potion is actually granted to the bag"), Item)) return false;
        TestEqual(TEXT("One potion granted"), Item->Quantity, 1);
        TestEqual(TEXT("Potion uses level zero like other non-catalogue items"), Item->ItemLevel, 0);
        TestTrue(TEXT("Potion instance has valid GUID"), Item->RuntimeObjectId.IsValid());
        TestEqual(TEXT("Potion owned by newly created hero"), Item->OwnerGuid, Character.CharacterId);
        TestEqual(TEXT("Runtime registers canonical potion definition"),
            Party->FindItemDefinition(Potion->ItemDefinitionId), Potion);
        TestEqual(TEXT("Production class not modified"), Class->StartingEquipmentKits.Num(), PreviousKitCount);
        TestEqual(TEXT("Production class default not changed"),
            Class->DefaultStartingEquipmentKitId, PreviousDefault);
        FString OwnershipError;
        TestTrue(TEXT("No duplication or invalid ownership"), Party->ValidateInventoryOwnership(OwnershipError));

        UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
        Save->PartyInventoryState = Party->PartyInventoryState;
        const FGridItemInstance* Saved = ARMEMENT0115Test::FindBagPotion(
            Save->PartyInventoryState.ActiveCharacters[0]);
        if (!TestNotNull(TEXT("Potion persists in the party SaveGame snapshot"), Saved)) return false;
        TestEqual(TEXT("Potion runtime object identity persists"), Saved->RuntimeObjectId, Item->RuntimeObjectId);
        TestEqual(TEXT("Kit ID persists on the character"),
            Save->PartyInventoryState.ActiveCharacters[0].StartingEquipmentKitId, FName(Case.KitId));
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
