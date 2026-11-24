#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GridEditorCanonicalStarterTestWorld.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/Combat/GridCombatActionCatalog.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GrimrockPartySaveGame.h"

namespace ARMEMENT0115Test
{
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
    FGridEditorCanonicalStarterTestWorld Fixture;
    if (!TestTrue(TEXT("Real ItemSystem configured for potion"), Fixture.Ready()))
        return false;
    FGridResolvedItem Potion;
    if (!TestTrue(TEXT("One canonical N0 healing potion"),
        Fixture.Registry->ResolveItem(TEXT("Item_Potion_Healing_01"), 0, Potion)))
        return false;
    TestTrue(TEXT("Canonical positive-potion tag"),
        Potion.Tags.Contains(TEXT("QuickItem.Potion.Positive")));
    if (!TestEqual(TEXT("One authored N0 use binding"),
        Potion.AvailableActions.Num(), 1))
        return false;
    FGridCombatActionDefinition Action;
    if (!TestTrue(TEXT("Detailed canonical heal action resolves"),
        Fixture.Registry->ResolveDetailedNonArmAction(
            Potion.ItemId, 0, Potion.AvailableActions[0].ActionId, Action)))
        return false;
    TestEqual(TEXT("Heal restores 20"), Action.EffectProfile.RestoreHealth, 20);
    TestEqual(TEXT("One AP"), Action.ActionPointCost, 1);
    TestEqual(TEXT("One physical potion consumed"),
        Action.ResourceCosts.SourceItemQuantityCost, 1);
    TestEqual(TEXT("No passive skill boost baked in"),
        Action.QuickItemScaling.RestoreHealthSkillRankScale, 0);

    FGridCombatActionContribution Source;
    Source.Definition = Action;
    Source.SourceDefinitionId = Potion.ItemId;
    Source.AvailableSourceQuantity = 1;
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
    FGridCombatActionCatalog::Build(Context, {Source}, Actions);
    if (!TestEqual(TEXT("Healing listed when wounded"), Actions.Num(), 1))
        return false;
    TestTrue(TEXT("Healing enabled while wounded"), Actions[0].bEnabled);
    Context.CurrentHealth = Context.MaximumHealth;
    FGridCombatActionCatalog::Build(Context, {Source}, Actions);
    if (!TestEqual(TEXT("Healing still displayed at full HP"), Actions.Num(), 1))
        return false;
    TestEqual(TEXT("Full HP prevents wasting potion"),
        Actions[0].AvailabilityReason, EGridCombatActionAvailabilityReason::NoApplicableEffect);
    Context.CurrentHealth = 5;
    Source.AvailableSourceQuantity = 0;
    FGridCombatActionCatalog::Build(Context, {Source}, Actions);
    if (!TestEqual(TEXT("Action remains listed with no physical potion"), Actions.Num(), 1))
        return false;
    TestFalse(TEXT("No quantity, no use"), Actions[0].bEnabled);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0115ProductionKits,
    "Grimrock.RPG.ARMEMENT01.15.HealthPotion.PriestAndApothecaryNewGame",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0115ProductionKits::RunTest(const FString&)
{
    FGridEditorCanonicalStarterTestWorld Fixture;
    if (!TestTrue(TEXT("Real game-world ItemSystem configured"), Fixture.Ready()))
        return false;
    FGridResolvedItem Potion;
    if (!TestTrue(TEXT("Canonical healing potion N0 resolves"),
        Fixture.Registry->ResolveItem(TEXT("Item_Potion_Healing_01"), 0, Potion)))
        return false;
    TestTrue(TEXT("Potion is the single non-armament N0 identity"),
        Potion.IsValid() && !Potion.bIsArmament && Potion.ItemLevel == 0);
    TestTrue(TEXT("Potion preserves its positive-consumable tag"),
        Potion.Tags.Contains(TEXT("QuickItem.Potion.Positive")));
    TestTrue(TEXT("Potion has a valid native stack capacity"), Potion.MaxStack >= 1);
    FGridCombatActionDefinition Action;
    if (!TestTrue(TEXT("Canonical potion action is bound"),
        Potion.AvailableActions.Num() == 1 &&
        Fixture.Registry->ResolveDetailedNonArmAction(
            Potion.ItemId, 0, Potion.AvailableActions[0].ActionId, Action)))
        return false;
    TestEqual(TEXT("Potion consumes one physical instance"), Action.ResourceCosts.SourceItemQuantityCost, 1);
    TestEqual(TEXT("Potion still restores 20 health"), Action.EffectProfile.RestoreHealth, 20);

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
        const int32 BeforeCount = Class->StartingEquipmentKits.Num();
        const FName BeforeDefault = Class->DefaultStartingEquipmentKitId;
        UGridPartyInventoryComponent* Party = Fixture.MakeInventory();
        if (!TestNotNull(TEXT("World-bound canonical inventory"), Party)) return false;
        FText Error;
        if (!TestTrue(TEXT("Real NewGame grants the canonical starter"),
            Party->CreateInitialCharacter(
                ARMEMENT0115Test::Request(Class, Human, FName(Case.KitId)), Error)))
        {
            AddError(Error.ToString());
            return false;
        }
        const FGridCharacterInventoryState& Character = Party->PartyInventoryState.ActiveCharacters[0];
        const FGridCharacterEquipmentState& Equipment = Party->PartyInventoryState.ActiveEquipment[0];
        TestEqual(TEXT("Correct starting orientation"), Character.StartingEquipmentKitId, FName(Case.KitId));
        TestEqual(TEXT("Canonical N1 weapon equipped"), Equipment.MainHand.ItemDefinitionId, FName(Case.WeaponId));
        const FGridItemInstance* Item = ARMEMENT0115Test::FindBagPotion(Character);
        if (!TestNotNull(TEXT("Canonical healing potion granted in bag"), Item)) return false;
        TestEqual(TEXT("One physical potion granted"), Item->Quantity, 1);
        TestEqual(TEXT("Non-armament physical level is N0"), Item->ItemLevel, 0);
        TestTrue(TEXT("Unique object identity persists"), Item->RuntimeObjectId.IsValid());
        TestEqual(TEXT("Owner GUID is correct"), Item->OwnerGuid, Character.CharacterId);
        TestEqual(TEXT("Production class remains untouched"), Class->StartingEquipmentKits.Num(), BeforeCount);
        TestEqual(TEXT("Default kit remains untouched"), Class->DefaultStartingEquipmentKitId, BeforeDefault);
        FString OwnershipError;
        TestTrue(TEXT("Ownership invariants"), Party->ValidateInventoryOwnership(OwnershipError));
        UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
        Save->PartyInventoryState = Party->PartyInventoryState;
        const FGridItemInstance* Saved = ARMEMENT0115Test::FindBagPotion(
            Save->PartyInventoryState.ActiveCharacters[0]);
        if (!TestNotNull(TEXT("N0 potion persists in SaveGame"), Saved)) return false;
        TestEqual(TEXT("Potion GUID survives save"), Saved->RuntimeObjectId, Item->RuntimeObjectId);
        TestEqual(TEXT("Kit selection survives save"),
            Save->PartyInventoryState.ActiveCharacters[0].StartingEquipmentKitId, FName(Case.KitId));
    }
    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
