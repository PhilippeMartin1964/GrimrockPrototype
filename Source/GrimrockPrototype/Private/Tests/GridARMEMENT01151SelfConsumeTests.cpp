#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Save/GrimrockPartySaveGame.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01151SelfRestoration,
    "Grimrock.RPG.ARMEMENT01.15.SelfConsume.ExplorationPotionTransaction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01151SelfRestoration::RunTest(const FString&)
{
    URPGClassAsset* Priest = LoadObject<URPGClassAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Priest.DA_Class_Priest"));
    URPGRaceAsset* Human = LoadObject<URPGRaceAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Race_Human.DA_Race_Human"));
    if (!TestNotNull(TEXT("Production Priest available"), Priest) ||
        !TestNotNull(TEXT("Production Human available"), Human)) return false;

    UGridPartyInventoryComponent* Inventory = NewObject<UGridPartyInventoryComponent>();
    Inventory->InitializeDefaultPartyIfNeeded();
    FRPGCharacterCreationRequest Request;
    Request.DisplayName = FText::FromString(TEXT("Context Consume Test"));
    Request.ClassDefinition = Priest;
    Request.CombatActionSourceClassDefinition = Priest;
    Request.RaceDefinition = Human;
    Request.StartingEquipmentKitId = TEXT("Kit_Priest_Restoration");
    FText Error;
    if (!TestTrue(TEXT("Real approved kit can create a hero"),
        Inventory->CreateInitialCharacter(Request, Error)))
    {
        AddError(Error.ToString());
        return false;
    }

    FGridCharacterInventoryState& Hero = Inventory->PartyInventoryState.ActiveCharacters[0];
    const FName PotionId(TEXT("Item_Potion_Healing_01"));
    FGridInventorySlot* PotionSlot = Hero.InventorySlots.FindByPredicate(
        [PotionId](const FGridInventorySlot& Slot)
        {
            return !Slot.IsEmpty() && Slot.Item.ItemDefinitionId == PotionId;
        });
    if (!TestNotNull(TEXT("Starting potion is in the bag"), PotionSlot)) return false;
    const FGuid PotionGuid = PotionSlot->Item.RuntimeObjectId;
    const FGuid HeroGuid = Hero.CharacterId;
    TestTrue(TEXT("Potion runtime GUID valid"), PotionGuid.IsValid());
    TestEqual(TEXT("One starting potion"), Inventory->CountItemDefinitionInCharacterInventory(0, PotionId), 1);
    UGridItemDefinitionAsset* Definition = Inventory->FindItemDefinition(PotionId);
    if (!TestNotNull(TEXT("Production potion definition registered"), Definition)) return false;
    FGridCombatActionDefinition Action;
    if (!TestTrue(TEXT("Item has an executable QuickItem action"),
        Definition->BuildQuickItemCombatActionDefinition(Action))) return false;
    TestEqual(TEXT("Potion is self-targeted"), Action.TargetingPolicy, EGridCombatTargetingPolicy::Self);
    TestEqual(TEXT("Potion has health effect"), Action.EffectProfile.RestoreHealth, 20);

    FGridInventoryCharacterSummary Summary;
    if (!TestTrue(TEXT("Effective maximum health available"), Inventory->GetCharacterSummary(0, Summary))) return false;
    const int32 MaxHealth = Summary.DerivedStats.MaxHealth;
    const int32 ManaBefore = Hero.Resources.CurrentMana;

    // Full-health attempts must never consume or mutate resources.
    TestFalse(TEXT("Full health blocks self-consume"),
        Inventory->TryConsumeSelfRestorativeQuickItem(0, PotionGuid, Error));
    TestFalse(TEXT("Full-health rejection returns reason"), Error.IsEmpty());
    TestEqual(TEXT("Full-health rejection preserves potion"),
        Inventory->CountItemDefinitionInCharacterInventory(0, PotionId), 1);
    TestEqual(TEXT("Full-health rejection preserves HP"), Hero.Resources.CurrentHealth, MaxHealth);

    // A fabricated or unrelated runtime object cannot consume the potion.
    Error = FText::GetEmpty();
    TestFalse(TEXT("Stale GUID is rejected"),
        Inventory->TryConsumeSelfRestorativeQuickItem(0, FGuid::NewGuid(), Error));
    TestEqual(TEXT("Stale GUID leaves item quantity intact"),
        Inventory->CountItemDefinitionInCharacterInventory(0, PotionId), 1);

    const int32 InitialWounded = FMath::Max(1, MaxHealth - 7);
    Hero.Resources.CurrentHealth = InitialWounded;
    Error = FText::GetEmpty();
    if (!TestTrue(TEXT("Wounded character can consume self-targeted potion without a world target"),
        Inventory->TryConsumeSelfRestorativeQuickItem(0, PotionGuid, Error)))
    {
        AddError(Error.ToString());
        return false;
    }
    TestEqual(TEXT("Healing caps at effective maximum health"),
        Hero.Resources.CurrentHealth, FMath::Min(MaxHealth, InitialWounded + 20));
    TestEqual(TEXT("Mana is unchanged by health potion"), Hero.Resources.CurrentMana, ManaBefore);
    TestEqual(TEXT("Exactly one potion consumed"), Inventory->CountItemDefinitionInCharacterInventory(0, PotionId), 0);
    TestEqual(TEXT("Hero identity remains unchanged"), Hero.CharacterId, HeroGuid);

    // The original GUID can never be spent twice and a denied attempt is atomic.
    Hero.Resources.CurrentHealth = FMath::Max(1, MaxHealth - 5);
    const int32 HealthAfterDamage = Hero.Resources.CurrentHealth;
    TestFalse(TEXT("Spent potion cannot be consumed again"),
        Inventory->TryConsumeSelfRestorativeQuickItem(0, PotionGuid, Error));
    TestEqual(TEXT("Failed second attempt restores no HP"), Hero.Resources.CurrentHealth, HealthAfterDamage);
    TestEqual(TEXT("Failed second attempt leaves count zero"),
        Inventory->CountItemDefinitionInCharacterInventory(0, PotionId), 0);
    FString OwnershipError;
    TestTrue(TEXT("No ownership corruption"), Inventory->ValidateInventoryOwnership(OwnershipError));

    UGrimrockPartySaveGame* Save = NewObject<UGrimrockPartySaveGame>();
    Save->PartyInventoryState = Inventory->PartyInventoryState;
    TestEqual(TEXT("Save snapshot persists absence of consumed potion"),
        Save->PartyInventoryState.ActiveCharacters[0].StartingEquipmentKitId, FName(TEXT("Kit_Priest_Restoration")));
    TestEqual(TEXT("Saved inventory contains no consumed potion"),
        Save->PartyInventoryState.ActiveCharacters[0].InventorySlots.ContainsByPredicate(
            [PotionId](const FGridInventorySlot& Slot)
            {
                return !Slot.IsEmpty() && Slot.Item.ItemDefinitionId == PotionId;
            }), false);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
