#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectBehavior.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UObject/UnrealType.h"

namespace ARMEMENT0183
{
    static const TCHAR* DaggerPath =
        TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Dagger_01.DA_Weapon_Dagger_01");

    struct FFixture
    {
        UWorld* World = nullptr;
        AGridLevelRuntimeActor* Runtime = nullptr;
        UGridLevelAsset* Level = nullptr;
        UGridItemDefinitionAsset* Dagger = nullptr;
        AGridReceptacleActor* Receptacle = nullptr;
        FGuid ReceptacleId;

        FFixture()
        {
            const UWorld::InitializationValues Values = UWorld::InitializationValues()
                .AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(false)
                .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
                .SetTransactional(false);
            World = UWorld::CreateWorld(EWorldType::Game, false,
                FName(*FString::Printf(TEXT("ARM0183_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))),
                nullptr, true, ERHIFeatureLevel::Num, &Values);
            if (World && GEngine)
            {
                FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
                Context.SetCurrentWorld(World);
            }
            if (!World) return;
            Runtime = World->SpawnActor<AGridLevelRuntimeActor>();
            if (!Runtime) return;
            Dagger = LoadObject<UGridItemDefinitionAsset>(nullptr, DaggerPath);
            Level = NewObject<UGridLevelAsset>(Runtime);
            Level->Width = 3;
            Level->Height = 3;
            Level->EnsureCellCount();
            for (FGridLevelCellData& Cell : Level->Cells)
            {
                Cell.CellType = EGridCellType::Floor;
                Cell.bBlocksOccupancy = false;
            }
            ReceptacleId = FGuid::NewGuid();
            FGridWorldObjectInstance& Placed = Level->WorldObjectInstances.AddDefaulted_GetRef();
            Placed.InstanceId = ReceptacleId;
            Placed.Type = EGridLevelObjectType::Receptacle;
            Placed.WorldObjectDefinitionId = TEXT("ARM0183_Receptacle");
            Placed.CellX = 1;
            Placed.CellY = 1;
            Placed.WallSide = EGridEdge::North;

            UGridWorldObjectDefinitionAsset* Definition = NewObject<UGridWorldObjectDefinitionAsset>(Runtime);
            Definition->DefinitionId = Placed.WorldObjectDefinitionId;
            Definition->SupportedType = EGridLevelObjectType::Receptacle;
            Definition->DefaultBehavior.Receptacle.MaxContainedItems = 4;
            Definition->PlacementSurface = EGridObjectPlacementKind::Wall;
            Definition->bIsInteractable = true;
            Definition->RuntimeActorClass = AGridReceptacleActor::StaticClass();
            Definition->StaticPart.Mesh = NewObject<UStaticMesh>(Runtime);

            // Exercise runtime definition lookup when the receptacle starts empty.
            // A disabled item generator references the pilot without spawning loot.
            FGridItemSpawnInstance& CatalogReference = Level->ItemSpawns.AddDefaulted_GetRef();
            CatalogReference.SpawnId = FGuid::NewGuid();
            CatalogReference.ItemDefinition = Dagger;
            CatalogReference.bSpawnAtStart = false;
            CatalogReference.CellX = 0;
            CatalogReference.CellY = 0;

            Runtime->LevelAsset = Level;
            Runtime->WorldObjectDefinitions.Add(Definition);
        }

        ~FFixture()
        {
            if (!World) return;
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }

        bool Ready() const { return World && Runtime && Level && Dagger; }

        void AddInitial(int32 LevelNumber)
        {
            FGridReceptacleInitialItemConfig& Initial =
                Level->WorldObjectInstances[0].InstanceConfig.ReceptacleInitialContent.AddDefaulted_GetRef();
            Initial.ItemDefinition = Dagger;
            Initial.Quantity = 1;
            Initial.ItemLevel = LevelNumber;
        }

        bool Build()
        {
            if (!Ready()) return false;
            Runtime->RebuildLevel();
            Receptacle = Runtime->FindRuntimeObjectActor<AGridReceptacleActor>(ReceptacleId);
            return Receptacle != nullptr;
        }

        AGrimrockPartyPawn* MakeParty()
        {
            AGrimrockPartyPawn* Party = World ? World->SpawnActor<AGrimrockPartyPawn>() : nullptr;
            if (!Party || !Party->PartyInventoryComponent) return nullptr;
            Party->LevelRuntimeActor = Runtime;
            Party->CurrentCellX = 1;
            Party->CurrentCellY = 1;
            Party->Facing = EGridEdge::North;
            Party->SetActorLocation(Runtime->GetCellCenterWorld(1, 1, Party->EyeHeight));
            Party->PartyInventoryComponent->InitializeDefaultPartyIfNeeded();
            if (!Party->PartyInventoryComponent->RegisterItemDefinition(Dagger)) return nullptr;
            return Party;
        }

        const FGridItemInstance* FindBagItem(const UGridPartyInventoryComponent* Inventory, FGuid Id) const
        {
            if (!Inventory || Inventory->PartyInventoryState.ActiveCharacters.IsEmpty()) return nullptr;
            for (const FGridInventorySlot& Slot : Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots)
            {
                if (!Slot.IsEmpty() && Slot.Item.RuntimeObjectId == Id) return &Slot.Item;
            }
            return nullptr;
        }

        const FGridRuntimeReceptacleState* SavedReceptacle() const
        {
            const FGridLevelRuntimeState* LevelState = Runtime ? Runtime->FindRuntimeStateForCurrentLevel() : nullptr;
            return LevelState ? LevelState->Receptacles.Find(ReceptacleId) : nullptr;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0183InitialTest,
    "Grimrock.RPG.ARMEMENT01.8.Receptacles.InitialFixedLevel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0183InitialTest::RunTest(const FString&)
{
    ARMEMENT0183::FFixture Fixture;
    if (!TestTrue(TEXT("Pilot dagger and runtime fixture available"), Fixture.Ready())) return false;
    Fixture.AddInitial(9);
    Fixture.AddInitial(0);

    const FProperty* DefinitionProperty =
        FGridReceptacleInitialItemConfig::StaticStruct()->FindPropertyByName(TEXT("ItemLevel"));
    TestTrue(TEXT("Initial content ItemLevel persists in LevelAsset"),
        DefinitionProperty && !DefinitionProperty->HasAnyPropertyFlags(CPF_Transient));
    if (!TestTrue(TEXT("Receptacle materialized with initial contents"), Fixture.Build())) return false;
    if (!TestEqual(TEXT("Two independently authored items"), Fixture.Receptacle->ContainedItems.Num(), 2)) return false;
    TestEqual(TEXT("First item has fixed level 9"), Fixture.Receptacle->ContainedItems[0].ItemLevel, 9);
    TestEqual(TEXT("Second item has inherited default"), Fixture.Receptacle->ContainedItems[1].ItemLevel,
        Fixture.Dagger->DefaultWeaponItemLevel);
    TestNotEqual(TEXT("Distinct contents keep distinct RuntimeObjectIds"),
        Fixture.Receptacle->ContainedItems[0].RuntimeObjectId, Fixture.Receptacle->ContainedItems[1].RuntimeObjectId);

    FGridRuntimeReceptacleState State;
    Fixture.Receptacle->CaptureRuntimeReceptacleState(State);
    if (!TestEqual(TEXT("Two items are captured in receptacle state"), State.ContainedItems.Num(), 2)) return false;
    TestEqual(TEXT("Fixed item level captured"), State.ContainedItems[0].ItemLevel, 9);
    TestEqual(TEXT("Default level resolved once before saving"), State.ContainedItems[1].ItemLevel,
        Fixture.Dagger->DefaultWeaponItemLevel);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0183TransferTest,
    "Grimrock.RPG.ARMEMENT01.8.Receptacles.InsertRestoreTake",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0183TransferTest::RunTest(const FString&)
{
    ARMEMENT0183::FFixture Fixture;
    if (!TestTrue(TEXT("Pilot dagger and runtime fixture available"), Fixture.Ready()) ||
        !TestTrue(TEXT("Empty receptacle materialized"), Fixture.Build())) return false;
    TestEqual(TEXT("Receptacle initially empty"), Fixture.Receptacle->GetContainedItemCount(), 0);

    FGridItemInstance Cursor;
    Cursor.RuntimeObjectId = FGuid::NewGuid();
    Cursor.ItemDefinitionId = Fixture.Dagger->ItemDefinitionId;
    Cursor.ItemLevel = 12;
    Cursor.Weight = Fixture.Dagger->Weight;
    Cursor.Quantity = 1;
    Cursor.OwnerType = EGridItemOwnerType::Cursor;

    FGridItemInstance Accepted;
    if (!TestTrue(TEXT("Cursor item inserted into receptacle"),
        Fixture.Receptacle->TryInsertItemInstanceFromCursor(Cursor, Accepted))) return false;
    TestEqual(TEXT("Transfer preserves identity"), Accepted.RuntimeObjectId, Cursor.RuntimeObjectId);
    TestEqual(TEXT("Transfer preserves item level in returned record"), Accepted.ItemLevel, 12);
    TestEqual(TEXT("Receptacle owns one item"), Fixture.Receptacle->GetContainedItemCount(), 1);
    if (!TestEqual(TEXT("Contained item level is still 12"), Fixture.Receptacle->ContainedItems[0].ItemLevel, 12)) return false;

    FGridRuntimeReceptacleState Saved;
    Fixture.Receptacle->CaptureRuntimeReceptacleState(Saved);
    if (!TestEqual(TEXT("Captured one item"), Saved.ContainedItems.Num(), 1)) return false;
    TestEqual(TEXT("Saved identity is unchanged"), Saved.ContainedItems[0].ObjectId, Cursor.RuntimeObjectId);
    TestEqual(TEXT("Saved level is 12"), Saved.ContainedItems[0].ItemLevel, 12);
    const FProperty* SavedLevelField =
        FGridRuntimeItemState::StaticStruct()->FindPropertyByName(TEXT("ItemLevel"));
    TestTrue(TEXT("Shared runtime item level is SaveGame"), SavedLevelField &&
        SavedLevelField->HasAnyPropertyFlags(CPF_SaveGame));

    TestEqual(TEXT("Clearing receptacle removes one item"), Fixture.Receptacle->ForceClearRuntimeContents(false), 1);
    if (!TestTrue(TEXT("Recreate contained item from saved state"),
        Fixture.Receptacle->RestoreRuntimeContainedItem(Saved.ContainedItems[0], nullptr))) return false;
    TestEqual(TEXT("Restore retains fixed item level"), Fixture.Receptacle->ContainedItems[0].ItemLevel, 12);
    TestEqual(TEXT("Restore retains original item identity"),
        Fixture.Receptacle->ContainedItems[0].RuntimeObjectId, Cursor.RuntimeObjectId);

    AGrimrockPartyPawn* Party = Fixture.MakeParty();
    if (!TestNotNull(TEXT("Party and inventory available"), Party)) return false;
    FName RemovedId;
    if (!TestTrue(TEXT("Take restored item into inventory"),
        Fixture.Receptacle->TryTakeItemAtIndex(0, Party, RemovedId))) return false;
    TestEqual(TEXT("Retrieved item definition is correct"), RemovedId, Fixture.Dagger->ItemDefinitionId);
    const FGridItemInstance* Bag = Fixture.FindBagItem(Party->PartyInventoryComponent, Cursor.RuntimeObjectId);
    if (!TestNotNull(TEXT("Retrieved object exists in inventory"), Bag)) return false;
    TestEqual(TEXT("Fixed level preserved on retrieval"), Bag->ItemLevel, 12);
    TestEqual(TEXT("Receptacle empty after removal"), Fixture.Receptacle->GetContainedItemCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0183FullSaveTest,
    "Grimrock.RPG.ARMEMENT01.8.Receptacles.WorldSaveRestore",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0183FullSaveTest::RunTest(const FString&)
{
    ARMEMENT0183::FFixture Fixture;
    if (!TestTrue(TEXT("Pilot dagger and runtime fixture available"), Fixture.Ready())) return false;
    Fixture.AddInitial(7);
    if (!TestTrue(TEXT("Receptacle created"), Fixture.Build())) return false;
    if (!TestTrue(TEXT("Save initial world"), Fixture.Runtime->CaptureCurrentLevelRuntimeState())) return false;

    FGridLevelRuntimeState* LevelState = Fixture.Runtime->GetOrCreateRuntimeStateForCurrentLevel();
    FGridRuntimeReceptacleState* ReceptacleState = LevelState ? LevelState->Receptacles.Find(Fixture.ReceptacleId) : nullptr;
    if (!TestNotNull(TEXT("Receptacle saved under stable object id"), ReceptacleState) ||
        !TestEqual(TEXT("Saved item count"), ReceptacleState->ContainedItems.Num(), 1)) return false;
    const FGuid ItemId = ReceptacleState->ContainedItems[0].ObjectId;
    TestEqual(TEXT("Original level 7 saved"), ReceptacleState->ContainedItems[0].ItemLevel, 7);

    // A loaded save with another fixed value must override authored defaults.
    ReceptacleState->ContainedItems[0].ItemLevel = 14;
    if (!TestTrue(TEXT("Restore complete world"), Fixture.Runtime->ApplyCurrentLevelRuntimeState())) return false;
    if (!TestEqual(TEXT("One contained item after world restore"), Fixture.Receptacle->GetContainedItemCount(), 1)) return false;
    TestEqual(TEXT("Saved level 14 supersedes authored 7"), Fixture.Receptacle->ContainedItems[0].ItemLevel, 14);
    TestEqual(TEXT("Object identity survived save/load"), Fixture.Receptacle->ContainedItems[0].RuntimeObjectId, ItemId);
    if (!TestTrue(TEXT("Recapture restored world"), Fixture.Runtime->CaptureCurrentLevelRuntimeState())) return false;
    const FGridRuntimeReceptacleState* Restored = Fixture.SavedReceptacle();
    if (!TestNotNull(TEXT("Restored receptacle has a save entry"), Restored) ||
        !TestEqual(TEXT("Restored content has one item"), Restored->ContainedItems.Num(), 1)) return false;
    TestEqual(TEXT("Saved fixed level survives a second capture"), Restored->ContainedItems[0].ItemLevel, 14);
    TestEqual(TEXT("Only one instance remains in runtime"), Fixture.Receptacle->ContainedItems.Num(), 1);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
