#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GridCanonicalLootTestScope.h"
#include "../UI/GridInventoryItemMetadataReadModel.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridDungeonAsset.h"
#include "Core/GridObjectBehavior.h"
#include "Core/GridWorldObjectDefinitionAsset.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemTransferService.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UObject/UnrealType.h"

namespace ARMEMENT0183
{
    const FName DaggerId(TEXT("Weapon_Dagger_01"));

    struct FFixture
    {
        UWorld* World = nullptr;
        FGridCanonicalLootTestScope RegistryScope;
        AGridLevelRuntimeActor* Runtime = nullptr;
        UGridLevelAsset* Level = nullptr;
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

            Runtime->LevelAsset = Level;
            Runtime->WorldObjectDefinitions.Add(Definition);
        }

        ~FFixture()
        {
            RegistryScope.Detach();
            if (!World) return;
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }

        bool Ready() const { return World && Runtime && Level; }

        void AddInitial(int32 LevelNumber)
        {
            FGridReceptacleInitialItemConfig& Initial =
                Level->WorldObjectInstances[0].InstanceConfig.ReceptacleInitialContent.AddDefaulted_GetRef();
            Initial.CanonicalItemId = DaggerId;
            Initial.Quantity = 1;
            Initial.ItemLevel = LevelNumber;
        }

        bool Build()
        {
            if (!Ready()) return false;
            if (!RegistryScope.Attach(World)) return false;
            FGridResolvedItem Dagger;
            if (!RegistryScope.Registry ||
                !RegistryScope.Registry->ResolveItem(DaggerId, 1, Dagger) ||
                !Dagger.bIsArmament ||
                Dagger.ItemId != DaggerId)
                return false;
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
    if (!TestTrue(TEXT("Canonical runtime fixture available"), Fixture.Ready())) return false;
    Fixture.AddInitial(9);
    Fixture.AddInitial(1);
    Fixture.AddInitial(0); // Invalid persisted ARM level must not materialize.

    const FProperty* DefinitionProperty =
        FGridReceptacleInitialItemConfig::StaticStruct()->FindPropertyByName(TEXT("ItemLevel"));
    TestTrue(TEXT("Initial content ItemLevel persists in LevelAsset"),
        DefinitionProperty && !DefinitionProperty->HasAnyPropertyFlags(CPF_Transient));
    if (!TestTrue(TEXT("Receptacle materialized with initial contents"), Fixture.Build())) return false;
    if (!TestEqual(TEXT("Two independently authored items"), Fixture.Receptacle->ContainedItems.Num(), 2)) return false;
    TestEqual(TEXT("First item has fixed level 9"), Fixture.Receptacle->ContainedItems[0].ItemLevel, 9);
    TestEqual(TEXT("Second item has explicit N1"), Fixture.Receptacle->ContainedItems[1].ItemLevel, 1);
    TestNotEqual(TEXT("Distinct contents keep distinct RuntimeObjectIds"),
        Fixture.Receptacle->ContainedItems[0].RuntimeObjectId, Fixture.Receptacle->ContainedItems[1].RuntimeObjectId);

    FGridRuntimeReceptacleState State;
    Fixture.Receptacle->CaptureRuntimeReceptacleState(State);
    if (!TestEqual(TEXT("Two items are captured in receptacle state"), State.ContainedItems.Num(), 2)) return false;
    TestEqual(TEXT("Fixed item level captured"), State.ContainedItems[0].ItemLevel, 9);
    TestEqual(TEXT("Explicit N1 preserved when saving"), State.ContainedItems[1].ItemLevel, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0183TransferTest,
    "Grimrock.RPG.ARMEMENT01.8.Receptacles.InsertRestoreTake",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0183TransferTest::RunTest(const FString&)
{
    ARMEMENT0183::FFixture Fixture;
    if (!TestTrue(TEXT("Canonical runtime fixture available"), Fixture.Ready()) ||
        !TestTrue(TEXT("Empty receptacle materialized"), Fixture.Build())) return false;
    TestEqual(TEXT("Receptacle initially empty"), Fixture.Receptacle->GetContainedItemCount(), 0);

    FGridItemInstance Cursor;
    Cursor.RuntimeObjectId = FGuid::NewGuid();
    Cursor.ItemDefinitionId = ARMEMENT0183::DaggerId;
    Cursor.ItemLevel = 12;
    FGridResolvedItem Physical;
    if (!TestTrue(TEXT("Canonical transferred dagger resolves"), Fixture.RegistryScope.Registry->ResolveItem(Cursor.ItemDefinitionId, Cursor.ItemLevel, Physical))) return false;
    Cursor.ReadTitleOverride = FText::FromString(TEXT("Actual narrative title"));
    Cursor.ReadTextOverride = FText::FromString(TEXT("Actual narrative body"));
    Cursor.Quantity = 1;
    Cursor.OwnerType = EGridItemOwnerType::Cursor;

    FGridItemInstance Accepted;
    if (!TestTrue(TEXT("Cursor item inserted into receptacle"),
        Fixture.Receptacle->TryInsertItemInstanceFromCursor(Cursor, Accepted))) return false;
    TestEqual(TEXT("Transfer preserves identity"), Accepted.RuntimeObjectId, Cursor.RuntimeObjectId);
    TestEqual(TEXT("Transfer preserves item level in returned record"), Accepted.ItemLevel, 12);
    TestNull(TEXT("Contained mass cache removed from schema"), FGridContainedReceptacleItem::StaticStruct()->FindPropertyByName(TEXT("Weight")));
    TestNull(TEXT("Contained name cache removed from schema"), FGridContainedReceptacleItem::StaticStruct()->FindPropertyByName(TEXT("DisplayName")));
    TestNull(TEXT("Physical instance mass removed from schema"), FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("Weight")));
    TestNull(TEXT("Physical instance name removed from schema"), FGridItemInstance::StaticStruct()->FindPropertyByName(TEXT("DisplayName")));
    TestEqual(TEXT("Accepted physical quantity unchanged"), Accepted.Quantity, Cursor.Quantity);
    const auto AcceptedMetadata = FGridInventoryItemMetadataReadModel::Resolve(Fixture.RegistryScope.Registry, nullptr, Accepted);
    TestTrue(TEXT("Accepted item metadata resolves"), AcceptedMetadata.bResolved);
    TestEqual(TEXT("Accepted item projection uses canonical mass"), AcceptedMetadata.UnitWeightKg, Physical.WeightKg);
    TestTrue(TEXT("Accepted item projection uses canonical name"), AcceptedMetadata.DisplayName.EqualTo(Physical.DisplayName));
    TestTrue(TEXT("Narrative title retained independently"), Accepted.ReadTitleOverride.EqualTo(Cursor.ReadTitleOverride));
    TestTrue(TEXT("Narrative body retained independently"), Accepted.ReadTextOverride.EqualTo(Cursor.ReadTextOverride));
    TestTrue(TEXT("Total receptacle weight is canonical"), FMath::IsNearlyEqual(Fixture.Receptacle->GetContainedTotalWeight(), Physical.WeightKg));
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
    auto* Visual = Fixture.Receptacle->GetContainedItemActor(0);
    if (!TestNotNull(TEXT("Restored physical actor"), Visual)) return false;
    const FGuid RealGuid = Visual->GetRuntimeObjectId();
    Visual->SetRuntimeObjectId(FGuid::NewGuid());
    TestFalse(TEXT("Direct retrieval refuses conflicting physical GUID"),
        Fixture.Receptacle->TryTakeItemAtIndex(0, Party, RemovedId));
    TestEqual(TEXT("Direct rejection leaves original contained item"), Fixture.Receptacle->GetContainedItemCount(), 1);
    TestNull(TEXT("Direct rejection grants no item"), Fixture.FindBagItem(Party->PartyInventoryComponent, Cursor.RuntimeObjectId));
    Visual->SetRuntimeObjectId(RealGuid);
    if (!TestTrue(TEXT("Take restored item into inventory"),
        Fixture.Receptacle->TryTakeItemAtIndex(0, Party, RemovedId))) return false;
    TestEqual(TEXT("Retrieved canonical ItemId is correct"), RemovedId, ARMEMENT0183::DaggerId);
    const FGridItemInstance* Bag = Fixture.FindBagItem(Party->PartyInventoryComponent, Cursor.RuntimeObjectId);
    if (!TestNotNull(TEXT("Retrieved object exists in inventory"), Bag)) return false;
    TestEqual(TEXT("Fixed level preserved on retrieval"), Bag->ItemLevel, 12);
    TestEqual(TEXT("Restored physical quantity retained"), Bag->Quantity, Cursor.Quantity);
    TestEqual(TEXT("Restored physical GUID retained"), Bag->RuntimeObjectId, Cursor.RuntimeObjectId);
    const auto BagMetadata = FGridInventoryItemMetadataReadModel::Resolve(Fixture.RegistryScope.Registry, nullptr, *Bag);
    TestTrue(TEXT("Restored inventory canonical metadata resolves"), BagMetadata.bResolved);
    TestEqual(TEXT("Restored inventory projection canonical weight"), BagMetadata.UnitWeightKg, Physical.WeightKg);
    TestTrue(TEXT("Restored inventory projection canonical name"), BagMetadata.DisplayName.EqualTo(Physical.DisplayName));
    TestTrue(TEXT("Narrative title survives save and retrieval"), Bag->ReadTitleOverride.EqualTo(Cursor.ReadTitleOverride));
    TestTrue(TEXT("Narrative body survives save and retrieval"), Bag->ReadTextOverride.EqualTo(Cursor.ReadTextOverride));
    TestEqual(TEXT("Receptacle empty after removal"), Fixture.Receptacle->GetContainedItemCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FARMEMENT0183FullSaveTest,
    "Grimrock.RPG.ARMEMENT01.8.Receptacles.WorldSaveRestore",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FARMEMENT0183FullSaveTest::RunTest(const FString&)
{
    ARMEMENT0183::FFixture Fixture;
    if (!TestTrue(TEXT("Canonical runtime fixture available"), Fixture.Ready())) return false;
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean065G48ReceptacleService,
    "Grimrock.RPG.ARM.CLEAN06_5G48.ReceptacleServicePhysicalRestitution",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean065G48ReceptacleService::RunTest(const FString&)
{
    ARMEMENT0183::FFixture F;
    if (!TestTrue(TEXT("Fixture ready"), F.Ready())) return false;
    F.AddInitial(12);
    if (!TestTrue(TEXT("Real N12 initial item"), F.Build()) ||
        !TestEqual(TEXT("One contained item"), F.Receptacle->GetContainedItemCount(), 1)) return false;
    AGrimrockPartyPawn* Party = F.MakeParty();
    if (!TestNotNull(TEXT("Party"), Party)) return false;
    UGridPartyInventoryComponent* Inventory = Party->PartyInventoryComponent;
    auto& Slots = Inventory->PartyInventoryState.ActiveCharacters[0].InventorySlots;
    Slots.SetNum(1);
    const FGuid Id = F.Receptacle->ContainedItems[0].RuntimeObjectId;
    AGridItemActor* Visual = F.Receptacle->GetContainedItemActor(0);
    if (!TestNotNull(TEXT("Canonical visual"), Visual)) return false;
    const FName ReadId(TEXT("Fixture_5G48_Read"));
    const FText Title = FText::FromString(TEXT("Inscription"));
    const FText Body = FText::FromString(TEXT("Persistent physical text"));
    Visual->InitializeReadableContent(nullptr, ReadId, Title, Body);
    Visual->SetItemLightsEnabled(false);
    // Full bag must reject before deleting the source or its actor.
    Slots[0].bOccupied = true;
    Slots[0].Item = FGridItemInstance();
    Slots[0].Item.RuntimeObjectId = FGuid::NewGuid();
    Slots[0].Item.ItemDefinitionId = TEXT("Key_Copper");
    Slots[0].Item.Quantity = 1;
    TestFalse(TEXT("Full destination refuses"),
        UGridItemTransferService::TransferReceptacleItemToInventory(F.Receptacle, 0, Inventory, 0).bSuccess);
    if (!TestEqual(TEXT("Failed transfer preserves contained row"), F.Receptacle->GetContainedItemCount(), 1)) return false;
    TestEqual(TEXT("Failed transfer preserves GUID"), F.Receptacle->ContainedItems[0].RuntimeObjectId, Id);
    TestEqual(TEXT("Failed transfer preserves actual N12"), F.Receptacle->ContainedItems[0].ItemLevel, 12);
    TestTrue(TEXT("Failed transfer preserves physical actor"), F.Receptacle->GetContainedItemActor(0) == Visual);
    Slots[0] = FGridInventorySlot();
    const FName PhysicalId = Visual->ItemDefinitionId;
    Visual->ItemDefinitionId = TEXT("Key_Copper");
    TestFalse(TEXT("Conflicting actor cannot rename the stored N12"),
        UGridItemTransferService::TransferReceptacleItemToInventory(F.Receptacle, 0, Inventory, 0).bSuccess);
    TestEqual(TEXT("Conflicting actor rejection retains source"), F.Receptacle->GetContainedItemCount(), 1);
    TestTrue(TEXT("Conflicting actor rejection does not fill bag"), Slots[0].IsEmpty());
    Visual->ItemDefinitionId = PhysicalId;
    if (!TestTrue(TEXT("Service transfers real N12 into bag"),
        UGridItemTransferService::TransferReceptacleItemToInventory(F.Receptacle, 0, Inventory, 0).bSuccess)) return false;
    TestEqual(TEXT("Source is now empty"), F.Receptacle->GetContainedItemCount(), 0);
    const FGridItemInstance* Item = F.FindBagItem(Inventory, Id);
    if (!TestNotNull(TEXT("Same physical item in bag"), Item)) return false;
    TestEqual(TEXT("N12 retained across service transfer"), Item->ItemLevel, 12);
    TestEqual(TEXT("Exactly one unit"), Item->Quantity, 1);
    TestEqual(TEXT("Readable identity retained"), Item->ReadableContentId, ReadId);
    TestTrue(TEXT("Readable title retained"), Item->ReadTitleOverride.EqualTo(Title));
    TestTrue(TEXT("Readable body retained"), Item->ReadTextOverride.EqualTo(Body));
    TestFalse(TEXT("Current extinguished state retained"), Item->bLightsEnabled);
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGridClean065G52ReceptacleCapture,
    "Grimrock.RPG.ARM.CLEAN06_5G52.AtomicReceptacleCapture",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean065G52ReceptacleCapture::RunTest(const FString&)
{
    ARMEMENT0183::FFixture F;
    if (!TestTrue(TEXT("Fixture ready"), F.Ready())) return false;
    F.AddInitial(12);
    if (!TestTrue(TEXT("Materialize N12 receptacle"), F.Build())) return false;
    UGridDungeonAsset* Dungeon = NewObject<UGridDungeonAsset>(F.Runtime);
    FGridDungeonLevelEntry Source;
    Source.LevelId = TEXT("CaptureSource");
    Source.LevelAsset = F.Level;
    Source.bEnabled = true;
    UGridLevelAsset* DestinationLevel = NewObject<UGridLevelAsset>(F.Runtime);
    DestinationLevel->Width = 3;
    DestinationLevel->Height = 3;
    DestinationLevel->EnsureCellCount();
    for (FGridLevelCellData& Cell : DestinationLevel->Cells)
    {
        Cell.CellType = EGridCellType::Floor;
        Cell.bBlocksOccupancy = false;
    }
    FGridDungeonLevelEntry Destination;
    Destination.LevelId = TEXT("CaptureDestination");
    Destination.LevelAsset = DestinationLevel;
    Destination.bEnabled = true;
    Dungeon->Levels = { Source, Destination };
    Dungeon->DefaultLevelId = Source.LevelId;
    F.Runtime->DungeonAsset = Dungeon;
    F.Runtime->CurrentDungeonLevelId = Source.LevelId;
    AGrimrockPartyPawn* Party = F.MakeParty();
    if (!TestNotNull(TEXT("Party"), Party) ||
        !TestTrue(TEXT("Capture valid initial level"), F.Runtime->CaptureCurrentLevelRuntimeState())) return false;
    const FGridRuntimeReceptacleState* Previous = F.SavedReceptacle();
    if (!TestNotNull(TEXT("Existing snapshot"), Previous) ||
        !TestEqual(TEXT("Existing physical item"), Previous->ContainedItems.Num(), 1)) return false;
    const FGuid Id = Previous->ContainedItems[0].ObjectId;
    auto* Visual = F.Receptacle->GetContainedItemActor(0);
    if (!TestNotNull(TEXT("Canonical visual"), Visual)) return false;
    FGridRuntimeReceptacleState LastGood = *Previous;
    Visual->SetRuntimeObjectId(FGuid::NewGuid());
    TestFalse(TEXT("Receptacle capture refuses conflicting physical GUID"), F.Receptacle->CaptureRuntimeReceptacleState(LastGood));
    if (!TestEqual(TEXT("Failed local capture leaves last good row"), LastGood.ContainedItems.Num(), 1)) return false;
    TestEqual(TEXT("Failed local capture preserves identity"), LastGood.ContainedItems[0].ObjectId, Id);
    TestFalse(TEXT("Level capture refuses before resetting prior world state"), F.Runtime->CaptureCurrentLevelRuntimeState());
    Previous = F.SavedReceptacle();
    if (!TestNotNull(TEXT("Previous level snapshot still present"), Previous) ||
        !TestEqual(TEXT("No partial level capture"), Previous->ContainedItems.Num(), 1)) return false;
    TestEqual(TEXT("Previous physical identity not rewritten"), Previous->ContainedItems[0].ObjectId, Id);
    TestFalse(TEXT("Travel refuses invalid source capture"),
        F.Runtime->TravelToDungeonLevel(Destination.LevelId, 1, 1, EGridEdge::North, Party));
    if (!TestTrue(TEXT("Rejected travel preserves source level"), F.Runtime->LevelAsset == F.Level) ||
        !TestEqual(TEXT("Rejected travel preserves source id"), F.Runtime->CurrentDungeonLevelId, Source.LevelId) ||
        !TestTrue(TEXT("Rejected travel retains receptacle"), IsValid(F.Receptacle)) ||
        !TestTrue(TEXT("Rejected travel retains physical actor"), IsValid(Visual))) return false;
    Visual->SetRuntimeObjectId(Id);
    F.Receptacle->ContainedItems[0].Quantity = MAX_int32;
    TestFalse(TEXT("Malformed physical quantity cannot be clamped on save"), F.Runtime->CaptureCurrentLevelRuntimeState());
    F.Receptacle->ContainedItems[0].Quantity = 1;
    TestTrue(TEXT("Corrected source captures normally"), F.Runtime->CaptureCurrentLevelRuntimeState());
    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
