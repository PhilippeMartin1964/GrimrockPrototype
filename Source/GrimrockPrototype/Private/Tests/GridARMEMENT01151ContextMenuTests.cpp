#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UI/GridInventoryWidget.h"

namespace ARMEMENT01151Context
{
    struct FWorldFixture
    {
        UWorld* World = nullptr;
        FWorldFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false)
                .CreatePhysicsScene(false).CreateNavigation(false)
                .CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM01151_Context_%s"),
                    *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
                Context.SetCurrentWorld(World);
            }
        }
        ~FWorldFixture()
        {
            if (!World) return;
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT01151ContextMenu,
    "Grimrock.RPG.ARMEMENT01.15.SelfConsume.ContextMenuByIndex",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT01151ContextMenu::RunTest(const FString&)
{
    ARMEMENT01151Context::FWorldFixture Fixture;
    if (!TestNotNull(TEXT("Transient world exists"), Fixture.World)) return false;
    AGrimrockPartyPawn* Pawn = Fixture.World->SpawnActor<AGrimrockPartyPawn>();
    if (!TestNotNull(TEXT("Party Pawn exists"), Pawn) ||
        !TestNotNull(TEXT("Party inventory exists"), Pawn ? Pawn->PartyInventoryComponent.Get() : nullptr))
        return false;
    UGridPartyInventoryComponent* Inventory = Pawn->PartyInventoryComponent.Get();
    Inventory->InitializeDefaultPartyIfNeeded();
    URPGClassAsset* Priest = LoadObject<URPGClassAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Class_Priest.DA_Class_Priest"));
    URPGRaceAsset* Human = LoadObject<URPGRaceAsset>(nullptr,
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_Race_Human.DA_Race_Human"));
    if (!TestNotNull(TEXT("Priest class exists"), Priest) ||
        !TestNotNull(TEXT("Human race exists"), Human)) return false;

    FRPGCharacterCreationRequest Creation;
    Creation.DisplayName = FText::FromString(TEXT("Context Potion"));
    Creation.ClassDefinition = Priest;
    Creation.CombatActionSourceClassDefinition = Priest;
    Creation.RaceDefinition = Human;
    Creation.StartingEquipmentKitId = TEXT("Kit_Priest_Restoration");
    FText Error;
    if (!TestTrue(TEXT("Real Restoration hero created"), Inventory->CreateInitialCharacter(Creation, Error)))
    {
        AddError(Error.ToString());
        return false;
    }
    UGridInventoryWidget* Widget = CreateWidget<UGridInventoryWidget>(
        Fixture.World, UGridInventoryWidget::StaticClass());
    if (!TestNotNull(TEXT("Inventory widget exists"), Widget)) return false;
    Widget->InitializeInventoryWidget(Pawn);

    const FName PotionId(TEXT("Item_Potion_Healing_01"));
    FGridCharacterInventoryState& Hero = Inventory->PartyInventoryState.ActiveCharacters[0];
    const int32 SlotIndex = Hero.InventorySlots.IndexOfByPredicate(
        [PotionId](const FGridInventorySlot& Slot)
        {
            return !Slot.IsEmpty() && Slot.Item.ItemDefinitionId == PotionId;
        });
    if (!TestTrue(TEXT("Potion physically placed in inventory"), SlotIndex != INDEX_NONE)) return false;

    FGridFacingTargetContext FacingTarget;
    TArray<FGridItemContextAction> Actions;
    if (!TestTrue(TEXT("Context menu builds while at full health"),
        Widget->BuildContextActionsForSlot(EGridInventoryUiSlotType::Inventory, SlotIndex,
            FacingTarget, Actions))) return false;
    int32 ConsumeIndex = Actions.IndexOfByPredicate(
        [](const FGridItemContextAction& Action) { return Action.ActionType == EGridItemActionType::Consume; });
    if (!TestTrue(TEXT("Context includes Consume"), ConsumeIndex != INDEX_NONE)) return false;
    TestFalse(TEXT("Menu disables Consume when already full"), Actions[ConsumeIndex].bEnabled);
    TestFalse(TEXT("Disabled menu entry gives a reason"), Actions[ConsumeIndex].DisabledReason.IsEmpty());
    TestEqual(TEXT("Full health leaves one potion"), Inventory->CountItemDefinitionInCharacterInventory(0, PotionId), 1);

    FGridInventoryCharacterSummary Summary;
    if (!TestTrue(TEXT("Character summary available"), Inventory->GetCharacterSummary(0, Summary))) return false;
    const int32 MaxHP = Summary.DerivedStats.MaxHealth;
    Hero.Resources.CurrentHealth = FMath::Max(1, MaxHP - 8);
    Actions.Reset();
    if (!TestTrue(TEXT("Context menu rebuilds for wounded character"),
        Widget->BuildContextActionsForSlot(EGridInventoryUiSlotType::Inventory, SlotIndex,
            FacingTarget, Actions))) return false;
    ConsumeIndex = Actions.IndexOfByPredicate(
        [](const FGridItemContextAction& Action) { return Action.ActionType == EGridItemActionType::Consume; });
    if (!TestTrue(TEXT("Wounded Consume entry exists"), ConsumeIndex != INDEX_NONE)) return false;
    TestTrue(TEXT("Wounded Consume entry is enabled"), Actions[ConsumeIndex].bEnabled);

    if (!TestTrue(TEXT("Clicked menu action executes exact self-targeted potion"),
        Widget->ExecuteInventoryContextActionByIndex(
            EGridInventoryUiSlotType::Inventory, SlotIndex, ConsumeIndex))) return false;
    TestEqual(TEXT("Self-health restored to cap"), Hero.Resources.CurrentHealth, MaxHP);
    TestEqual(TEXT("Exactly one potion consumed"), Inventory->CountItemDefinitionInCharacterInventory(0, PotionId), 0);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
