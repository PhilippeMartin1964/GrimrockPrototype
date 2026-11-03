#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridItemVisualEffectsComponent.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace GridItemVFX012StackTests
{
    struct FTestWorld
    {
        UWorld* World = nullptr;
        FTestWorld()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false)
                .RequiresHitProxies(false)
                .CreatePhysicsScene(false)
                .CreateNavigation(false)
                .CreateAISystem(false)
                .ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ITEM_VFX012_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
                Context.SetCurrentWorld(World);
                World->InitializeActorsForPlay(FURL());
            }
        }

        ~FTestWorld()
        {
            if (World)
            {
                World->DestroyWorld(false);
                if (GEngine)
                {
                    GEngine->DestroyWorldContext(World);
                }
            }
        }
    };

    FGridItemInstance MakePotion(const UGridItemDefinitionAsset& Definition,
        int32 Quantity, bool bLightsEnabled)
    {
        FGridItemInstance Item;
        Item.RuntimeObjectId = FGuid::NewGuid();
        Item.ItemDefinitionId = Definition.ItemDefinitionId;
        Item.DisplayName = Definition.DisplayName;
        Item.Quantity = Quantity;
        Item.OwnerType = EGridItemOwnerType::World;
        Item.bLightsEnabled = bLightsEnabled;
        return Item;
    }

    const FGridItemInstance* FindPotion(const UGridPartyInventoryComponent& Inventory,
        int32 CharacterIndex, FName DefinitionId)
    {
        const FGridCharacterInventoryState& Character =
            Inventory.PartyInventoryState.ActiveCharacters[CharacterIndex];
        for (const FGridInventorySlot& Slot : Character.InventorySlots)
        {
            if (!Slot.IsEmpty() && Slot.Item.ItemDefinitionId == DefinitionId)
            {
                return &Slot.Item;
            }
        }
        return nullptr;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridItemVFX012StackLifecycleTest,
    "Grimrock.Items.VFX01_2.StackTransferAndWorldRehydration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridItemVFX012StackLifecycleTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UNiagaraSystem* Sparkle = LoadObject<UNiagaraSystem>(nullptr,
        TEXT("/Game/GrimrockPrototype/Effects/Alchimie/NS_LiquideSparkle.NS_LiquideSparkle"));
    if (!TestNotNull(TEXT("Production sparkle system available"), Sparkle))
    {
        return false;
    }

    GridItemVFX012StackTests::FTestWorld TestWorld;
    if (!TestNotNull(TEXT("Transient world available"), TestWorld.World))
    {
        return false;
    }

    AGrimrockPartyPawn* Party = TestWorld.World->SpawnActor<AGrimrockPartyPawn>();
    if (!TestNotNull(TEXT("Party exists"), Party) ||
        !TestNotNull(TEXT("Party inventory exists"), Party ? Party->PartyInventoryComponent.Get() : nullptr))
    {
        return false;
    }

    UGridPartyInventoryComponent* Inventory = Party->PartyInventoryComponent;
    Inventory->InitializeDefaultPartyIfNeeded();
    const int32 CharacterIndex = Inventory->GetSelectedCharacterIndex();

    UGridItemDefinitionAsset* Potion = NewObject<UGridItemDefinitionAsset>(Inventory);
    Potion->ItemDefinitionId = TEXT("ITEM_VFX012_Potion");
    Potion->DisplayName = FText::FromString(TEXT("Permanent bubbling potion"));
    Potion->ItemType = EGridItemType::Potion;
    Potion->bStackable = true;
    Potion->MaxStackSize = 10;

    FGridItemNiagaraEffectConfig Effect;
    Effect.EffectId = TEXT("LiquidSparkle");
    Effect.NiagaraSystem = Sparkle;
    Effect.ActivationMode = EGridItemVfxActivationMode::Always;
    Potion->VisualEffects.Effects.Add(Effect);

    TestTrue(TEXT("Potion has valid VFX but no emitter"),
        Potion->IsValidDefinition() && !Potion->HasLightEmitter());
    if (!TestTrue(TEXT("Potion definition registers"), Inventory->RegisterItemDefinition(Potion)))
    {
        return false;
    }

    // Old bug: an incoming lit source merges into an unlit target. The target
    // light flag remains false; permanent visual effects must still survive.
    if (!TestTrue(TEXT("Insert target potion stack"),
        Inventory->AddItemToCharacterInventory(
            CharacterIndex, GridItemVFX012StackTests::MakePotion(*Potion, 2, false))) ||
        !TestTrue(TEXT("Merge incoming stack with a different light state"),
        Inventory->AddItemToCharacterInventory(
            CharacterIndex, GridItemVFX012StackTests::MakePotion(*Potion, 3, true))))
    {
        return false;
    }

    const FGridItemInstance* Combined =
        GridItemVFX012StackTests::FindPotion(*Inventory, CharacterIndex, Potion->ItemDefinitionId);
    if (!TestNotNull(TEXT("Merged stack survives"), Combined))
    {
        return false;
    }
    TestEqual(TEXT("Stack quantity is five"), Combined->Quantity, 5);
    TestFalse(TEXT("Target instance keeps its own false light flag"), Combined->bLightsEnabled);
    const FGuid MergeGuid = Combined->RuntimeObjectId;

    AGridItemActor* Dropped = TestWorld.World->SpawnActor<AGridItemActor>();
    if (!TestNotNull(TEXT("Generic dropped actor exists"), Dropped))
    {
        return false;
    }
    Dropped->InitializeFromItemDefinition(Potion, MergeGuid);
    Dropped->SetItemLightsEnabled(Combined->bLightsEnabled);
    UNiagaraComponent* Bubble = Dropped->VisualEffectsComponent->GetEffectComponent(0);
    TestNotNull(TEXT("Dropped unlit potion creates permanent Niagara"), Bubble);
    TestTrue(TEXT("Potion bubbles when unlit"), Bubble && Bubble->IsVisible());
    TestFalse(TEXT("Niagara does not turn into a gameplay light"),
        Dropped->AreItemLightsEnabled());

    Dropped->ConfigureAsAttachedItem();
    TestTrue(TEXT("Receptacle-style attachment preserves permanent bubbles"),
        Bubble && Bubble->IsVisible());
    Dropped->SetItemLightsEnabled(true);
    Dropped->SetItemLightsEnabled(false);
    TestTrue(TEXT("Light-state toggles do not disable the permanent effect"),
        Bubble && Bubble->IsVisible());

    // Split through the real cursor/inventory transfer pipeline.
    int32 StackSlotIndex = INDEX_NONE;
    const TArray<FGridInventorySlot>& Slots =
        Inventory->PartyInventoryState.ActiveCharacters[CharacterIndex].InventorySlots;
    for (int32 Index = 0; Index < Slots.Num(); ++Index)
    {
        if (!Slots[Index].IsEmpty() && Slots[Index].Item.RuntimeObjectId == MergeGuid)
        {
            StackSlotIndex = Index;
            break;
        }
    }
    if (!TestTrue(TEXT("Locate merged potion stack"), StackSlotIndex != INDEX_NONE))
    {
        return false;
    }
    TestTrue(TEXT("Split three potions to cursor"),
        Inventory->TryTakeInventorySlotQuantityToCursor(CharacterIndex, StackSlotIndex, 3));
    TestTrue(TEXT("Split cursor exists"), Inventory->HasCursorItem());
    FGridItemInstance Split = Inventory->GetCursorItem();
    TestEqual(TEXT("Split quantity"), Split.Quantity, 3);
    TestFalse(TEXT("Split preserves the unlit item flag"), Split.bLightsEnabled);
    TestTrue(TEXT("Split gets a distinct identity"), Split.RuntimeObjectId != MergeGuid);

    AGridItemActor* SplitDrop = TestWorld.World->SpawnActor<AGridItemActor>();
    if (!TestNotNull(TEXT("Split world actor exists"), SplitDrop))
    {
        return false;
    }
    SplitDrop->InitializeFromItemDefinition(Potion, Split.RuntimeObjectId);
    SplitDrop->SetItemLightsEnabled(Split.bLightsEnabled);
    UNiagaraComponent* SplitBubble = SplitDrop->VisualEffectsComponent->GetEffectComponent(0);
    TestTrue(TEXT("Split and dropped unlit potion still bubbles"),
        SplitBubble && SplitBubble->IsVisible());

    const FGridItemInstance Snapshot = Split;
    AGridItemActor* Restored = TestWorld.World->SpawnActor<AGridItemActor>();
    if (!TestNotNull(TEXT("Recreated world actor exists"), Restored))
    {
        return false;
    }
    Restored->InitializeFromItemDefinition(Potion, Snapshot.RuntimeObjectId);
    Restored->OnPlacedInWorld();
    Restored->SetItemLightsEnabled(Snapshot.bLightsEnabled);
    TestTrue(TEXT("Re-created saved instance derives permanent VFX from item definition"),
        Restored->VisualEffectsComponent->GetEffectComponent(0) != nullptr &&
        Restored->VisualEffectsComponent->GetEffectComponent(0)->IsVisible());
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
