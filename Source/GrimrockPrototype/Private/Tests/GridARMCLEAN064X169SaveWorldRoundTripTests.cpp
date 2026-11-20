#if WITH_DEV_AUTOMATION_TESTS

#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Save/GrimrockPartySaveGame.h"
#include "UObject/UnrealType.h"

namespace ARMCLEAN064X169
{
    bool IsSavedProperty(const UStruct* Owner, FName PropertyName)
    {
        const FProperty* Property = Owner ? Owner->FindPropertyByName(PropertyName) : nullptr;
        return Property && Property->HasAnyPropertyFlags(CPF_SaveGame);
    }

    FGridRuntimeItemState MakeItem(FGuid ObjectId, FName ItemId, int32 Level, int32 Count)
    {
        FGridRuntimeItemState Item;
        Item.ObjectId = ObjectId;
        Item.ItemDefinitionId = ItemId;
        Item.ItemLevel = Level;
        Item.Quantity = Count;
        Item.CellX = 3;
        Item.CellY = 5;
        Item.Edge = EGridEdge::None;
        Item.Transform = FTransform(FRotator(0.0, 90.0, 0.0), FVector(600.0, 1000.0, 60.0));
        return Item;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGridClean064X169SaveSchemaGate,
    "Grimrock.RPG.ARM.CLEAN06_4X169.CurrentSaveSchemaGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X169SaveSchemaGate::RunTest(const FString&)
{
    // Tagged v27 archives retain every physical field; the two redundant
    // definition pointers have now been removed after the native preflight.
    TestTrue(TEXT("World item GUID is saved"),
        ARMCLEAN064X169::IsSavedProperty(FGridRuntimeItemState::StaticStruct(),
            GET_MEMBER_NAME_CHECKED(FGridRuntimeItemState, ObjectId)));
    TestTrue(TEXT("World item canonical ID is saved"),
        ARMCLEAN064X169::IsSavedProperty(FGridRuntimeItemState::StaticStruct(),
            GET_MEMBER_NAME_CHECKED(FGridRuntimeItemState, ItemDefinitionId)));
    TestTrue(TEXT("World item N0/N1-N20 level is saved"),
        ARMCLEAN064X169::IsSavedProperty(FGridRuntimeItemState::StaticStruct(),
            GET_MEMBER_NAME_CHECKED(FGridRuntimeItemState, ItemLevel)));
    TestTrue(TEXT("World item quantity is saved"),
        ARMCLEAN064X169::IsSavedProperty(FGridRuntimeItemState::StaticStruct(),
            GET_MEMBER_NAME_CHECKED(FGridRuntimeItemState, Quantity)));
    TestNull(TEXT("Historical projectile pointer is absent from reflection"),
        FGridRuntimeItemState::StaticStruct()->FindPropertyByName(TEXT("ProjectileDefinitionAsset")));
    TestTrue(TEXT("Pending pit transfer identity is saved"),
        ARMCLEAN064X169::IsSavedProperty(FGridPendingWorldItemState::StaticStruct(),
            GET_MEMBER_NAME_CHECKED(FGridPendingWorldItemState, ItemState)));
    TestNull(TEXT("Historical pending pointer is absent from reflection"),
        FGridPendingWorldItemState::StaticStruct()->FindPropertyByName(TEXT("ItemDefinitionAsset")));
    TestTrue(TEXT("Destination level pending items are saved"),
        ARMCLEAN064X169::IsSavedProperty(FGridLevelRuntimeState::StaticStruct(),
            GET_MEMBER_NAME_CHECKED(FGridLevelRuntimeState, PendingInboundItems)));
    TestTrue(TEXT("Root dungeon state is a current SaveGame property"),
        ARMCLEAN064X169::IsSavedProperty(UGrimrockPartySaveGame::StaticClass(),
            GET_MEMBER_NAME_CHECKED(UGrimrockPartySaveGame, DungeonRuntimeState)));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGridClean064X169SaveWorldRoundTrip,
    "Grimrock.RPG.ARM.CLEAN06_4X169.CurrentWorldMemoryRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGridClean064X169SaveWorldRoundTrip::RunTest(const FString&)
{
    using namespace ARMCLEAN064X169;

    // Entire fixture is transient. No SaveGame slot, .sav, content asset or map is opened.
    UGrimrockPartySaveGame* Source = NewObject<UGrimrockPartySaveGame>(GetTransientPackage());
    if (!TestNotNull(TEXT("Transient current-version SaveGame"), Source))
        return false;

    const FName UpperLevelId(TEXT("ARM_CLEAN06_Fixture_Upper"));
    const FName LowerLevelId(TEXT("ARM_CLEAN06_Fixture_Lower"));
    const FGuid GroundId(11, 12, 13, 14);
    const FGuid LevelledId(21, 22, 23, 24);
    const FGuid TorchId(31, 32, 33, 34);
    const FGuid ReceptacleId(41, 42, 43, 44);
    const FGuid LockId(51, 52, 53, 54);
    const FGuid DoorId(61, 62, 63, 64);
    const FGuid PitId(71, 72, 73, 74);
    const FGuid FallingId(81, 82, 83, 84);

    Source->CurrentDungeonLevelId = UpperLevelId;
    Source->PartyCellX = 7;
    Source->PartyCellY = 8;
    Source->GameplayElapsedSeconds = 63.5;

    FGridLevelRuntimeState& Upper = Source->DungeonRuntimeState.LevelStates.Add(UpperLevelId);
    Upper.LevelId = UpperLevelId;
    Upper.bHasBeenVisited = true;
    Upper.bLevelVariablesInitialized = true;
    Upper.BoolVariables.Add(TEXT("PuzzleSolved"), true);

    FGridRuntimeItemState Ground = MakeItem(GroundId, TEXT("Item_Arrows_01"), 0, 7);
    Ground.bIsSimulatingPhysics = true;
    Ground.bHasWorldMeshTransform = true;
    Ground.WorldMeshTransform = FTransform(FVector(620.0, 1020.0, 15.0));
    Ground.bLightsEnabled = false;
    Upper.Items.Add(GroundId, Ground);

    // Synthetic ItemId proves N20 serialization without requiring a real asset lookup.
    Upper.Items.Add(LevelledId,
        MakeItem(LevelledId, TEXT("ARM_CLEAN06_Fixture_Armament"), 20, 1));

    FGridRuntimeReceptacleState Receptacle;
    Receptacle.ObjectId = ReceptacleId;
    Receptacle.bCanInsertItems = false;
    Receptacle.bCanRemoveItem = true;
    FGridRuntimeItemState Torch = MakeItem(TorchId, TEXT("Item_Torch"), 0, 1);
    Torch.bIsContainedInReceptacle = true;
    Torch.ReceptacleObjectId = ReceptacleId;
    Torch.bLightsEnabled = true;
    Receptacle.ContainedItems.Add(Torch);
    Upper.Receptacles.Add(ReceptacleId, Receptacle);

    FGridRuntimeInteractiveState LockState;
    LockState.ObjectId = LockId;
    LockState.bIsActivated = true;
    LockState.bIsOn = true;
    Upper.InteractiveObjects.Add(LockId, LockState);

    FGridRuntimeDoorState Door;
    Door.ObjectId = DoorId;
    Door.bIsOpen = true;
    Door.bBlocksMovement = false;
    Upper.Doors.Add(DoorId, Door);

    FGridRuntimePitState Pit;
    Pit.ObjectId = PitId;
    Pit.bIsOpen = true;
    Upper.Pits.Add(PitId, Pit);

    FGridLevelRuntimeState& Lower = Source->DungeonRuntimeState.LevelStates.Add(LowerLevelId);
    Lower.LevelId = LowerLevelId;
    FGridPendingWorldItemState Falling;
    Falling.ItemState = MakeItem(FallingId, TEXT("Key_Iron"), 0, 1);
    Lower.PendingInboundItems.Add(FallingId, Falling);

    FText ValidationError;
    if (!TestTrue(TEXT("Transient current-schema snapshot validates before save"),
            Source->ValidateCurrentState(ValidationError)))
    {
        AddError(ValidationError.ToString());
        return false;
    }

    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("Current SaveGame serializes into RAM"),
            UGameplayStatics::SaveGameToMemory(Source, Bytes)))
        return false;
    if (!TestTrue(TEXT("Serialized snapshot is nonempty"), !Bytes.IsEmpty()))
        return false;

    UGrimrockPartySaveGame* Loaded =
        Cast<UGrimrockPartySaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Current SaveGame deserializes from RAM"), Loaded))
        return false;
    TestTrue(TEXT("Loaded SaveGame passes exact-version validation"),
        Loaded->IsLoadValid() && Loaded->IsCompatible());
    TestEqual(TEXT("SaveVersion remains current"),
        Loaded->SaveVersion, UGrimrockPartySaveGame::CurrentSaveVersion);
    TestEqual(TEXT("Current level identity"), Loaded->CurrentDungeonLevelId, UpperLevelId);
    TestEqual(TEXT("Gameplay clock"), Loaded->GameplayElapsedSeconds, 63.5);

    const FGridLevelRuntimeState* SavedUpper =
        Loaded->DungeonRuntimeState.LevelStates.Find(UpperLevelId);
    const FGridLevelRuntimeState* SavedLower =
        Loaded->DungeonRuntimeState.LevelStates.Find(LowerLevelId);
    if (!TestNotNull(TEXT("Upper level survived"), SavedUpper) ||
        !TestNotNull(TEXT("Lower level survived"), SavedLower))
        return false;

    TestTrue(TEXT("Upper level remains visited"), SavedUpper->bHasBeenVisited);
    TestTrue(TEXT("Puzzle variable stays initialized and true"),
        SavedUpper->bLevelVariablesInitialized &&
        SavedUpper->BoolVariables.FindRef(TEXT("PuzzleSolved")));

    const FGridRuntimeItemState* SavedGround = SavedUpper->Items.Find(GroundId);
    const FGridRuntimeItemState* SavedLevelled = SavedUpper->Items.Find(LevelledId);
    if (!TestNotNull(TEXT("N0 world stack survives"), SavedGround) ||
        !TestNotNull(TEXT("N20 world item survives"), SavedLevelled))
        return false;
    TestEqual(TEXT("Canonical N0 item ID survives"),
        SavedGround->ItemDefinitionId, FName(TEXT("Item_Arrows_01")));
    TestEqual(TEXT("N0 remains zero"), SavedGround->ItemLevel, 0);
    TestEqual(TEXT("Stack count remains seven"), SavedGround->Quantity, 7);
    TestEqual(TEXT("World GUID remains unchanged"), SavedGround->ObjectId, GroundId);
    TestTrue(TEXT("Physics pose and physics flag survive"),
        SavedGround->bIsSimulatingPhysics && SavedGround->bHasWorldMeshTransform &&
        SavedGround->WorldMeshTransform.GetLocation().Equals(FVector(620.0, 1020.0, 15.0), 0.01));
    TestFalse(TEXT("Darkened item remains unlit"), SavedGround->bLightsEnabled);
    TestEqual(TEXT("Levelled item ID remains stable"),
        SavedLevelled->ItemDefinitionId, FName(TEXT("ARM_CLEAN06_Fixture_Armament")));
    TestEqual(TEXT("Highest authored level survives"), SavedLevelled->ItemLevel, 20);

    const FGridRuntimeReceptacleState* SavedReceptacle =
        SavedUpper->Receptacles.Find(ReceptacleId);
    if (!TestNotNull(TEXT("Receptacle survives"), SavedReceptacle))
        return false;
    TestFalse(TEXT("Insertion lockout survives"), SavedReceptacle->bCanInsertItems);
    TestTrue(TEXT("Removal permission survives"), SavedReceptacle->bCanRemoveItem);
    if (!TestEqual(TEXT("One contained item survives"),
            SavedReceptacle->ContainedItems.Num(), 1))
        return false;
    const FGridRuntimeItemState& SavedTorch = SavedReceptacle->ContainedItems[0];
    TestEqual(TEXT("Contained torch GUID survives"), SavedTorch.ObjectId, TorchId);
    TestEqual(TEXT("Contained torch keeps canonical identity"),
        SavedTorch.ItemDefinitionId, FName(TEXT("Item_Torch")));
    TestEqual(TEXT("Contained torch points to the same receptacle"),
        SavedTorch.ReceptacleObjectId, ReceptacleId);
    TestTrue(TEXT("Contained torch remains lit"), SavedTorch.bLightsEnabled);

    const FGridRuntimeInteractiveState* SavedLock =
        SavedUpper->InteractiveObjects.Find(LockId);
    const FGridRuntimeDoorState* SavedDoor = SavedUpper->Doors.Find(DoorId);
    const FGridRuntimePitState* SavedPit = SavedUpper->Pits.Find(PitId);
    TestTrue(TEXT("Unlocked interaction state survives"),
        SavedLock && SavedLock->bIsActivated && SavedLock->bIsOn);
    TestTrue(TEXT("Door state remains open and traversable"),
        SavedDoor && SavedDoor->bIsOpen && !SavedDoor->bBlocksMovement);
    TestTrue(TEXT("Open pit state survives"), SavedPit && SavedPit->bIsOpen);

    const FGridPendingWorldItemState* SavedFalling =
        SavedLower->PendingInboundItems.Find(FallingId);
    if (!TestNotNull(TEXT("Pending inter-level item survives"), SavedFalling))
        return false;
    TestEqual(TEXT("Pending item GUID survives"),
        SavedFalling->ItemState.ObjectId, FallingId);
    TestEqual(TEXT("Pending key identity survives"),
        SavedFalling->ItemState.ItemDefinitionId, FName(TEXT("Key_Iron")));
    TestEqual(TEXT("Pending item preserves N0"), SavedFalling->ItemState.ItemLevel, 0);

    FText LoadedError;
    TestTrue(TEXT("Round-tripped snapshot remains valid without mutation"),
        Loaded->ValidateCurrentState(LoadedError));
    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
