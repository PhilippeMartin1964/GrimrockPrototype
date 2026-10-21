#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Core/GridLevelAsset.h"
#include "EditorTools/GridLevelEditorActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UObject/UnrealType.h"

namespace ARMEMENT0181
{
    constexpr const TCHAR* DaggerPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Dagger_01.DA_Weapon_Dagger_01");

    struct FWorldFixture
    {
        UWorld* World = nullptr;
        AGridLevelRuntimeActor* Runtime = nullptr;
        UGridLevelAsset* Level = nullptr;
        UGridItemDefinitionAsset* Dagger = nullptr;

        explicit FWorldFixture(EWorldType::Type WorldType = EWorldType::Game)
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
                .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(WorldType, false,
                FName(*FString::Printf(TEXT("ARM0181_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(WorldType);
                Context.SetCurrentWorld(World);
            }
            if (!World) return;
            Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
            if (!Runtime) return;
            Level = NewObject<UGridLevelAsset>(Runtime);
            Level->Width = 2;
            Level->Height = 2;
            Level->EnsureCellCount();
            for (FGridLevelCellData& Cell : Level->Cells)
            {
                Cell.CellType = EGridCellType::Floor;
                Cell.bBlocksOccupancy = false;
            }
            Runtime->LevelAsset = Level;
            Dagger = LoadObject<UGridItemDefinitionAsset>(nullptr, DaggerPath);
        }

        ~FWorldFixture()
        {
            if (!World) return;
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }

        bool Ready() const { return World && Runtime && Level && Dagger; }

        FGuid Place(int32 CellX, int32 CellY, int32 LevelNumber)
        {
            FGridLooseItemInstance& Item = Level->LooseItemInstances.AddDefaulted_GetRef();
            Item.InstanceId = FGuid::NewGuid();
            Item.ItemDefinition = Dagger;
            Item.ItemLevel = LevelNumber;
            Item.Quantity = 1;
            Item.CellX = CellX;
            Item.CellY = CellY;
            return Item.InstanceId;
        }

        AGrimrockPartyPawn* CreateParty()
        {
            AGrimrockPartyPawn* Party = World->SpawnActor<AGrimrockPartyPawn>();
            if (!Party || !Party->PartyInventoryComponent) return nullptr;
            Party->LevelRuntimeActor = Runtime;
            Party->CurrentCellX = 0;
            Party->CurrentCellY = 0;
            Party->Facing = EGridEdge::North;
            Party->SetActorLocation(Runtime->GetCellCenterWorld(0, 0, Party->EyeHeight));
            Party->PartyInventoryComponent->InitializeDefaultPartyIfNeeded();
            if (!Party->PartyInventoryComponent->RegisterItemDefinition(Dagger)) return nullptr;
            Runtime->WorldItemPickupReach = 500.0f;
            return Party;
        }
    };

    const FGridItemInstance* FindInBag(const UGridPartyInventoryComponent* Inventory, FGuid Id)
    {
        if (!Inventory || Inventory->PartyInventoryState.ActiveCharacters.IsEmpty()) return nullptr;
        for (const FGridInventorySlot& Slot : Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots)
        {
            if (!Slot.IsEmpty() && Slot.Item.RuntimeObjectId == Id)
            {
                return &Slot.Item;
            }
        }
        return nullptr;
    }

    const FGridSpawnedItemRuntimeEntry* FindWorldEntry(const AGridLevelRuntimeActor* Runtime, FGuid Id)
    {
        return Runtime->SpawnedItemEntries.FindByPredicate(
            [Id](const FGridSpawnedItemRuntimeEntry& Entry){ return Entry.ObjectId == Id; });
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0181EditorTest,
    "Grimrock.RPG.ARMEMENT01.8.PlacedItems.AuthoredLevelAndValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0181EditorTest::RunTest(const FString&)
{
    ARMEMENT0181::FWorldFixture Fixture(EWorldType::EditorPreview);
    if (!TestTrue(TEXT("Dagger and editor fixture loaded"), Fixture.Ready())) return false;
    AGridLevelEditorActor* Editor = Fixture.World->SpawnActor<AGridLevelEditorActor>();
    if (!TestNotNull(TEXT("Grid editor actor exists"), Editor)) return false;
    Editor->LevelAsset = Fixture.Level;

    const FGuid Id = Fixture.Place(0, 0, 0);
    Editor->LastSelectedObjectId = Id;
    TestEqual(TEXT("Legacy placement inherits definition when level is zero"),
        Fixture.Level->LooseItemInstances[0].ItemLevel, 0);
    TestTrue(TEXT("Set authored fixed level 9"), Editor->SetSelectedObjectItemLevel(9));
    TestEqual(TEXT("Authored placement saves level 9"), Fixture.Level->LooseItemInstances[0].ItemLevel, 9);
    TestFalse(TEXT("Level 21 rejected"), Editor->SetSelectedObjectItemLevel(21));
    TestFalse(TEXT("Negative level rejected"), Editor->SetSelectedObjectItemLevel(-1));
    TestEqual(TEXT("Rejected writes keep level 9"), Fixture.Level->LooseItemInstances[0].ItemLevel, 9);
    TestTrue(TEXT("Reset to asset default"), Editor->SetSelectedObjectItemLevel(0));
    TestEqual(TEXT("Zero reset persisted"), Fixture.Level->LooseItemInstances[0].ItemLevel, 0);

    const FProperty* LevelProperty = FGridLooseItemInstance::StaticStruct()->FindPropertyByName(TEXT("ItemLevel"));
    TestTrue(TEXT("LevelAsset stores an authored UPROPERTY ItemLevel"), LevelProperty &&
        !LevelProperty->HasAnyPropertyFlags(CPF_Transient));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0181PickupTest,
    "Grimrock.RPG.ARMEMENT01.8.PlacedItems.FixedLevelPickupBothPaths",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0181PickupTest::RunTest(const FString&)
{
    ARMEMENT0181::FWorldFixture Fixture;
    if (!TestTrue(TEXT("Runtime dagger fixture loaded"), Fixture.Ready())) return false;
    const FGuid DefaultId = Fixture.Place(0, 0, 0);
    const FGuid FixedId = Fixture.Place(1, 0, 9);
    Fixture.Runtime->RebuildRuntimeObjects();
    const FGridSpawnedItemRuntimeEntry* Default = ARMEMENT0181::FindWorldEntry(Fixture.Runtime, DefaultId);
    const FGridSpawnedItemRuntimeEntry* Fixed = ARMEMENT0181::FindWorldEntry(Fixture.Runtime, FixedId);
    if (!TestNotNull(TEXT("Default item spawned"), Default) || !TestNotNull(TEXT("Level 9 item spawned"), Fixed)) return false;
    TestEqual(TEXT("Default level materialized once"), Default->ItemLevel, Fixture.Dagger->DefaultWeaponItemLevel);
    TestEqual(TEXT("Level 9 materialized unchanged"), Fixed->ItemLevel, 9);
    AGridItemActor* FixedActor = Fixed->ItemActor.Get();

    if (!TestTrue(TEXT("Runtime state captured"), Fixture.Runtime->CaptureCurrentLevelRuntimeState())) return false;
    const FGridLevelRuntimeState* Saved = Fixture.Runtime->FindRuntimeStateForCurrentLevel();
    if (!TestNotNull(TEXT("World state available"), Saved)) return false;
    const FGridRuntimeItemState* Snapshot = Saved->Items.Find(FixedId);
    if (!TestNotNull(TEXT("Level 9 saved world item"), Snapshot)) return false;
    TestEqual(TEXT("Saved world item retains level 9"), Snapshot->ItemLevel, 9);

    AGrimrockPartyPawn* Party = Fixture.CreateParty();
    if (!TestNotNull(TEXT("Pickup pawn exists"), Party)) return false;
    if (!TestTrue(TEXT("Cell pickup of default-level dagger"), Fixture.Runtime->TryPickupItemAtCell(0, 0, Party))) return false;
    if (!TestTrue(TEXT("Clicked-actor pickup of level 9 dagger"), Fixture.Runtime->TryPickupItemActor(FixedActor, Party))) return false;

    const FGridItemInstance* DefaultItem = ARMEMENT0181::FindInBag(Party->PartyInventoryComponent, DefaultId);
    const FGridItemInstance* FixedItem = ARMEMENT0181::FindInBag(Party->PartyInventoryComponent, FixedId);
    if (!TestNotNull(TEXT("Default dagger entered the bag"), DefaultItem) ||
        !TestNotNull(TEXT("Level 9 dagger entered the bag"), FixedItem)) return false;
    TestEqual(TEXT("Default dagger level inherited correctly"), DefaultItem->ItemLevel, Fixture.Dagger->DefaultWeaponItemLevel);
    TestEqual(TEXT("Pickup retains exact authored level"), FixedItem->ItemLevel, 9);
    TestEqual(TEXT("Fixed dagger original identity unchanged"), FixedItem->RuntimeObjectId, FixedId);
    TestFalse(TEXT("Picked dagger no longer in world"), ARMEMENT0181::FindWorldEntry(Fixture.Runtime, FixedId) != nullptr);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0181DropTest,
    "Grimrock.RPG.ARMEMENT01.8.PlacedItems.WorldDropAndSaveRestore",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0181DropTest::RunTest(const FString&)
{
    ARMEMENT0181::FWorldFixture Fixture;
    if (!TestTrue(TEXT("Runtime dagger fixture loaded"), Fixture.Ready())) return false;
    FGridItemInstance WorldItem;
    WorldItem.RuntimeObjectId = FGuid::NewGuid();
    WorldItem.ItemDefinitionId = Fixture.Dagger->ItemDefinitionId;
    WorldItem.ItemLevel = 13;
    WorldItem.Quantity = 1;
    WorldItem.OwnerType = EGridItemOwnerType::World;

    if (!TestTrue(TEXT("Drop fixed level 13 item"), Fixture.Runtime->TryDropItemInstanceAtCell(
        WorldItem, Fixture.Dagger, 0, 0, EGridEdge::None, FVector::ZeroVector))) return false;
    const FGridSpawnedItemRuntimeEntry* Entry = ARMEMENT0181::FindWorldEntry(Fixture.Runtime, WorldItem.RuntimeObjectId);
    if (!TestNotNull(TEXT("Dropped item exists"), Entry)) return false;
    TestEqual(TEXT("World entry retains fixed level"), Entry->ItemLevel, 13);
    if (!TestTrue(TEXT("Capture dropped state"), Fixture.Runtime->CaptureCurrentLevelRuntimeState())) return false;
    const FGridLevelRuntimeState* Saved = Fixture.Runtime->FindRuntimeStateForCurrentLevel();
    if (!TestNotNull(TEXT("Dropped state exists"), Saved)) return false;
    const FGridRuntimeItemState* ItemState = Saved->Items.Find(WorldItem.RuntimeObjectId);
    if (!TestNotNull(TEXT("Dropped item is saved"), ItemState)) return false;
    TestEqual(TEXT("Snapshot saves level 13"), ItemState->ItemLevel, 13);
    const FProperty* LevelProperty = FGridRuntimeItemState::StaticStruct()->FindPropertyByName(TEXT("ItemLevel"));
    TestTrue(TEXT("ItemLevel is persisted with SaveGame"), LevelProperty && LevelProperty->HasAnyPropertyFlags(CPF_SaveGame));

    Fixture.Runtime->SpawnedItemEntries[0].ItemLevel = 1;
    if (!TestTrue(TEXT("Restore world state succeeds"), Fixture.Runtime->ApplyCurrentLevelRuntimeState())) return false;
    Entry = ARMEMENT0181::FindWorldEntry(Fixture.Runtime, WorldItem.RuntimeObjectId);
    if (!TestNotNull(TEXT("Restored world item exists"), Entry)) return false;
    TestEqual(TEXT("Restore reconstructs level 13, not default"), Entry->ItemLevel, 13);

    AGrimrockPartyPawn* Party = Fixture.CreateParty();
    if (!TestNotNull(TEXT("Pickup pawn exists"), Party)) return false;
    if (!TestTrue(TEXT("Pickup after restoring the save"), Fixture.Runtime->TryPickupItemActor(Entry->ItemActor.Get(), Party))) return false;
    const FGridItemInstance* InventoryItem = ARMEMENT0181::FindInBag(Party->PartyInventoryComponent, WorldItem.RuntimeObjectId);
    if (!TestNotNull(TEXT("Restored item is owned by inventory"), InventoryItem)) return false;
    TestEqual(TEXT("Item level preserved from drop to inventory"), InventoryItem->ItemLevel, 13);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
