#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UObject/UnrealType.h"

namespace ARMEMENT0182
{
    struct FFixture
    {
        UWorld* World = nullptr;
        TStrongObjectPtr<UGameInstance> GI;
        UGridItemSubsystem* Registry = nullptr;
        AGridLevelRuntimeActor* Runtime = nullptr;
        UGridLevelAsset* Level = nullptr;

        explicit FFixture(EWorldType::Type WorldType = EWorldType::Game)
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
                .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(WorldType, false,
                FName(*FString::Printf(TEXT("ARM0182_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(WorldType);
                Context.SetCurrentWorld(World);
            }
            if (!World) return;
            if (WorldType == EWorldType::Game)
            {
                GI.Reset(NewObject<UGameInstance>(GetTransientPackage()));
                if (!GI.IsValid()) return;
                GI->Init();
                World->SetGameInstance(GI.Get());
                Registry = GI->GetSubsystem<UGridItemSubsystem>();
                if (!Registry || !Registry->IsConfigured()) return;
            }
            Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
            if (!Runtime) return;
            Level = NewObject<UGridLevelAsset>(Runtime);
            Level->Width = 3;
            Level->Height = 3;
            Level->EnsureCellCount();
            for (FGridLevelCellData& Cell : Level->Cells)
            {
                Cell.CellType = EGridCellType::Floor;
                Cell.bBlocksOccupancy = false;
            }
            Runtime->LevelAsset = Level;
        }

        ~FFixture()
        {
            if (!World) return;
            if (GI.IsValid())
            {
                World->SetGameInstance(nullptr);
                GI->Shutdown();
            }
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }

        bool Ready() const
        {
            return World && Runtime && Level &&
                (!Registry || Registry->ContainsItem(TEXT("Weapon_Dagger_01")));
        }

        FGuid AddSpawn(int32 X, int32 Y, int32 ItemLevel, bool bAtStart = true)
        {
            FGridItemSpawnInstance& Spawn = Level->ItemSpawns.AddDefaulted_GetRef();
            Spawn.SpawnId = FGuid::NewGuid();
            Spawn.CanonicalItemId = TEXT("Weapon_Dagger_01");
            Spawn.ItemLevel = ItemLevel;
            Spawn.Quantity = 1;
            Spawn.CellX = X;
            Spawn.CellY = Y;
            Spawn.bSpawnAtStart = bAtStart;
            return Spawn.SpawnId;
        }

        AGrimrockPartyPawn* MakeParty()
        {
            AGrimrockPartyPawn* Party = World ? World->SpawnActor<AGrimrockPartyPawn>() : nullptr;
            if (!Party || !Party->PartyInventoryComponent) return nullptr;
            Party->LevelRuntimeActor = Runtime;
            Party->CurrentCellX = 0;
            Party->CurrentCellY = 0;
            Party->Facing = EGridEdge::North;
            Party->SetActorLocation(Runtime->GetCellCenterWorld(0, 0, Party->EyeHeight));
            Party->PartyInventoryComponent->InitializeDefaultPartyIfNeeded();
            Runtime->WorldItemPickupReach = 600.f;
            return Party;
        }

        AGridItemActor* FindActor(FGuid Id) const
        {
            if (!World) return nullptr;
            for (TActorIterator<AGridItemActor> It(World); It; ++It)
            {
                AGridItemActor* Actor = *It;
                if (IsValid(Actor) && !Actor->IsActorBeingDestroyed() && Actor->GetRuntimeObjectId() == Id)
                {
                    return Actor;
                }
            }
            return nullptr;
        }

        int32 CountActors(FGuid Id) const
        {
            int32 Count = 0;
            if (!World) return Count;
            for (TActorIterator<AGridItemActor> It(World); It; ++It)
            {
                AGridItemActor* Actor = *It;
                if (IsValid(Actor) && !Actor->IsActorBeingDestroyed() && Actor->GetRuntimeObjectId() == Id) ++Count;
            }
            return Count;
        }

        const FGridItemInstance* FindInBag(const UGridPartyInventoryComponent* Inventory, FGuid Id) const
        {
            if (!Inventory || Inventory->PartyInventoryState.ActiveCharacters.IsEmpty()) return nullptr;
            for (const FGridInventorySlot& Slot : Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots)
            {
                if (!Slot.IsEmpty() && Slot.Item.RuntimeObjectId == Id) return &Slot.Item;
            }
            return nullptr;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0182AuthoringTest,
    "Grimrock.RPG.ARMEMENT01.8.ItemSpawns.AuthoredLevel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0182AuthoringTest::RunTest(const FString&)
{
    ARMEMENT0182::FFixture Fixture(EWorldType::EditorPreview);
    if (!TestTrue(TEXT("Editor fixture and production dagger exist"), Fixture.Ready())) return false;
    AGridLevelEditorActor* Editor = Fixture.World->SpawnActor<AGridLevelEditorActor>();
    if (!TestNotNull(TEXT("Editor actor exists"), Editor)) return false;
    Editor->LevelAsset = Fixture.Level;
    const FGuid SpawnId = Fixture.AddSpawn(1, 1, 1, false);
    Editor->LastSelectedObjectId = SpawnId;

    TestEqual(TEXT("Generator starts at explicit physical N1"), Fixture.Level->ItemSpawns[0].ItemLevel, 1);
    TestTrue(TEXT("Set generator to level 11"), Editor->SetSelectedObjectItemLevel(11));
    TestEqual(TEXT("Generator stores its own level"), Fixture.Level->ItemSpawns[0].ItemLevel, 11);
    TestFalse(TEXT("Level 21 is rejected"), Editor->SetSelectedObjectItemLevel(21));
    TestFalse(TEXT("Negative level is rejected"), Editor->SetSelectedObjectItemLevel(-1));
    TestEqual(TEXT("Rejections leave authored value intact"), Fixture.Level->ItemSpawns[0].ItemLevel, 11);
    TestFalse(TEXT("Editing level does not activate disabled generator"), Fixture.Level->ItemSpawns[0].bSpawnAtStart);
    TestFalse(TEXT("Registered generator cannot reset to historical N0"), Editor->SetSelectedObjectItemLevel(0));
    TestEqual(TEXT("Rejected zero preserves N11"), Fixture.Level->ItemSpawns[0].ItemLevel, 11);
    const FProperty* Field = FGridItemSpawnInstance::StaticStruct()->FindPropertyByName(TEXT("ItemLevel"));
    TestTrue(TEXT("ItemSpawn ItemLevel is serialized in LevelAsset"), Field && !Field->HasAnyPropertyFlags(CPF_Transient));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0182MaterializationTest,
    "Grimrock.RPG.ARMEMENT01.8.ItemSpawns.MaterializeAndPickup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0182MaterializationTest::RunTest(const FString&)
{
    ARMEMENT0182::FFixture Fixture;
    if (!TestTrue(TEXT("Runtime fixture and production dagger exist"), Fixture.Ready())) return false;
    const FGuid Fixed = Fixture.AddSpawn(0, 0, 9, true);
    const FGuid Default = Fixture.AddSpawn(1, 0, 1, true);
    const FGuid Invalid = Fixture.AddSpawn(2, 1, 0, true);
    const FGuid Disabled = Fixture.AddSpawn(2, 0, 14, false);
    Fixture.Runtime->RebuildRuntimeObjects();

    AGridItemActor* FixedActor = Fixture.FindActor(Fixed);
    if (!TestNotNull(TEXT("Enabled fixed-level generator produces a pickup"), FixedActor)) return false;
    TestNotNull(TEXT("Enabled explicit N1 generator produces a pickup"), Fixture.FindActor(Default));
    TestNull(TEXT("Invalid ARM N0 generator produces no pickup"), Fixture.FindActor(Invalid));
    TestNull(TEXT("Disabled generator produces no pickup"), Fixture.FindActor(Disabled));
    TestEqual(TEXT("Generator uses SpawnId for item identity"), FixedActor->GetRuntimeObjectId(), Fixed);
    TestEqual(TEXT("One world item per active generator"), Fixture.CountActors(Fixed), 1);
    Fixture.Runtime->RebuildRuntimeObjects();
    TestEqual(TEXT("Repeated object rebuild never duplicates item"), Fixture.CountActors(Fixed), 1);

    if (!TestTrue(TEXT("Capture initial generator state"), Fixture.Runtime->CaptureCurrentLevelRuntimeState())) return false;
    const FGridLevelRuntimeState* Snapshot = Fixture.Runtime->FindRuntimeStateForCurrentLevel();
    if (!TestNotNull(TEXT("Initial state exists"), Snapshot)) return false;
    const FGridRuntimeItemState* FixedState = Snapshot->Items.Find(Fixed);
    const FGridRuntimeItemState* DefaultState = Snapshot->Items.Find(Default);
    if (!TestNotNull(TEXT("Fixed generator item captured"), FixedState) ||
        !TestNotNull(TEXT("Explicit N1 item captured"), DefaultState)) return false;
    TestEqual(TEXT("Level 9 retained in runtime state"), FixedState->ItemLevel, 9);
    TestEqual(TEXT("Explicit N1 retained"), DefaultState->ItemLevel, 1);
    TestFalse(TEXT("No runtime item for invalid N0"), Snapshot->Items.Contains(Invalid));
    TestFalse(TEXT("No runtime item for a disabled generator"), Snapshot->Items.Contains(Disabled));

    AGrimrockPartyPawn* Party = Fixture.MakeParty();
    if (!TestNotNull(TEXT("Inventory pawn created"), Party)) return false;
    if (!TestTrue(TEXT("Click pickup of generated item"), Fixture.Runtime->TryPickupItemActor(FixedActor, Party))) return false;
    const FGridItemInstance* BagItem = Fixture.FindInBag(Party->PartyInventoryComponent, Fixed);
    if (!TestNotNull(TEXT("Picked-up generated dagger in bag"), BagItem)) return false;
    TestEqual(TEXT("Pickup preserves fixed level 9"), BagItem->ItemLevel, 9);
    TestEqual(TEXT("Pickup preserves SpawnId identity"), BagItem->RuntimeObjectId, Fixed);

    if (!TestTrue(TEXT("Capture collected-generator state"), Fixture.Runtime->CaptureCurrentLevelRuntimeState())) return false;
    const FGridLevelRuntimeState* Collected = Fixture.Runtime->FindRuntimeStateForCurrentLevel();
    if (!TestNotNull(TEXT("Collected state captured"), Collected)) return false;
    const FGridRuntimeObjectPresenceState* Presence = Collected->ObjectPresence.Find(Fixed);
    TestTrue(TEXT("Generator pickup is saved as removed"), Presence && Presence->bRemovedFromInitialPlacement);
    Fixture.Runtime->RebuildRuntimeObjects();
    TestNull(TEXT("Collected generator never respawns after rebuild"), Fixture.FindActor(Fixed));
    TestNotNull(TEXT("Uncollected generator stays available"), Fixture.FindActor(Default));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0182PersistenceTest,
    "Grimrock.RPG.ARMEMENT01.8.ItemSpawns.SaveRestoreFixedLevel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0182PersistenceTest::RunTest(const FString&)
{
    ARMEMENT0182::FFixture Fixture;
    if (!TestTrue(TEXT("Runtime fixture and production dagger exist"), Fixture.Ready())) return false;
    const FGuid SpawnId = Fixture.AddSpawn(1, 0, 7);
    Fixture.Runtime->RebuildRuntimeObjects();
    if (!TestNotNull(TEXT("Initial pickup generated"), Fixture.FindActor(SpawnId))) return false;
    if (!TestTrue(TEXT("Save current world"), Fixture.Runtime->CaptureCurrentLevelRuntimeState())) return false;
    FGridLevelRuntimeState* Saved = Fixture.Runtime->GetOrCreateRuntimeStateForCurrentLevel();
    if (!TestNotNull(TEXT("Saved runtime state exists"), Saved)) return false;
    FGridRuntimeItemState* Item = Saved->Items.Find(SpawnId);
    if (!TestNotNull(TEXT("Generated pickup has saved state"), Item)) return false;
    TestEqual(TEXT("Original fixed level is serialized"), Item->ItemLevel, 7);

    // Simulate loading a save with a different fixed level from the authored
    // generator. Persisted state is authoritative for the existing item.
    Item->ItemLevel = 12;
    if (!TestTrue(TEXT("Restore simulated save"), Fixture.Runtime->ApplyCurrentLevelRuntimeState())) return false;
    if (!TestTrue(TEXT("Recapture restored item"), Fixture.Runtime->CaptureCurrentLevelRuntimeState())) return false;
    const FGridLevelRuntimeState* Restored = Fixture.Runtime->FindRuntimeStateForCurrentLevel();
    const FGridRuntimeItemState* RestoredItem = Restored ? Restored->Items.Find(SpawnId) : nullptr;
    if (!TestNotNull(TEXT("Restored item remains present"), RestoredItem)) return false;
    TestEqual(TEXT("Restored item level comes from save, not generator"), RestoredItem->ItemLevel, 12);

    AGrimrockPartyPawn* Party = Fixture.MakeParty();
    if (!TestNotNull(TEXT("Inventory pawn created"), Party)) return false;
    if (!TestTrue(TEXT("Pickup of restored generated item"), Fixture.Runtime->TryPickupItemActor(Fixture.FindActor(SpawnId), Party))) return false;
    const FGridItemInstance* BagItem = Fixture.FindInBag(Party->PartyInventoryComponent, SpawnId);
    if (!TestNotNull(TEXT("Restored item arrives in inventory"), BagItem)) return false;
    TestEqual(TEXT("Restored fixed level survives inventory transfer"), BagItem->ItemLevel, 12);

    if (!TestTrue(TEXT("Save collected world state"), Fixture.Runtime->CaptureCurrentLevelRuntimeState())) return false;
    if (!TestTrue(TEXT("Apply collected state"), Fixture.Runtime->ApplyCurrentLevelRuntimeState())) return false;
    Fixture.Runtime->RebuildRuntimeObjects();
    TestNull(TEXT("Generator cannot reroll or respawn after collection"), Fixture.FindActor(SpawnId));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
