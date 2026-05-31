#include "Runtime/GridLevelRuntimeActor.h"
#include "Core/GridTypes.h"
#include "Core/GridDirectionUtils.h"
#include "Core/GridObjectArchetypeAsset.h"
#include "Runtime/GridRuntimeObjectActor.h"
#include "Runtime/GridActivationComponent.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridEditorPreviewComponent.h"
#include "Runtime/GrimrockGameMode.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridLeverActor.h"
#include "Runtime/GridMechanismActor.h"
#include "Runtime/GridPressurePlateActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "UI/ReadableMessageWidget.h"
#include "Blueprint/UserWidget.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"

namespace
{
    const FName SingleLevelRuntimeStateId (TEXT ("SingleLevel"));

    FName ResolveRuntimeStateLevelId (const UGridDungeonAsset* DungeonAsset, FName CurrentDungeonLevelId)
    {
        if (DungeonAsset && !CurrentDungeonLevelId.IsNone ())
        {
            return CurrentDungeonLevelId;
        }

        return SingleLevelRuntimeStateId;
    }

    FString GetRuntimeWorldTypeText (const UWorld* World)
    {
        if (!World)
        {
            return TEXT ("None");
        }

        switch (World->WorldType)
        {
            case EWorldType::Game:
                return TEXT ("Game");

            case EWorldType::PIE:
                return TEXT ("PIE");

            case EWorldType::Editor:
                return TEXT ("Editor");

            case EWorldType::EditorPreview:
                return TEXT ("EditorPreview");

            case EWorldType::GamePreview:
                return TEXT ("GamePreview");

            case EWorldType::Inactive:
                return TEXT ("Inactive");

            default:
                return FString::Printf (TEXT ("Unknown(%d)"), static_cast<int32> (World->WorldType));
        }
    }

    FString GetRuntimeEdgeText (EGridEdge Edge)
    {
        if (const UEnum* EdgeEnum = StaticEnum<EGridEdge> ())
        {
            return EdgeEnum->GetNameStringByValue (static_cast<int64> (Edge));
        }

        return FString::Printf (TEXT ("%d"), static_cast<int32> (Edge));
    }

    FString GetRuntimeBoolText (bool bValue)
    {
        return bValue ? TEXT ("true") : TEXT ("false");
    }

    UGridItemDefinitionAsset* ResolveObjectItemDefinitionAsset (
        const FGridLevelObjectData& ObjectData,
        const UGridObjectArchetypeAsset* Archetype)
    {
        if (ObjectData.ItemDefinitionAsset)
        {
            return ObjectData.ItemDefinitionAsset;
        }
        if (Archetype && Archetype->DefaultBehavior.Item.ItemDefinitionAsset)
        {
            return Archetype->DefaultBehavior.Item.ItemDefinitionAsset;
        }
        return nullptr;
    }

    FName ResolveObjectItemDefinitionId (
        const FGridLevelObjectData& ObjectData,
        const UGridObjectArchetypeAsset* Archetype)
    {
        if (ObjectData.ItemDefinitionAsset && !ObjectData.ItemDefinitionAsset->ItemDefinitionId.IsNone ())
        {
            return ObjectData.ItemDefinitionAsset->ItemDefinitionId;
        }
        if (!ObjectData.ItemDefinitionId.IsNone ())
        {
            return ObjectData.ItemDefinitionId;
        }
        if (Archetype && Archetype->DefaultBehavior.Item.ItemDefinitionAsset &&
            !Archetype->DefaultBehavior.Item.ItemDefinitionAsset->ItemDefinitionId.IsNone ())
        {
            return Archetype->DefaultBehavior.Item.ItemDefinitionAsset->ItemDefinitionId;
        }
        if (Archetype && !Archetype->DefaultBehavior.Item.ItemDefinitionId.IsNone ())
        {
            return Archetype->DefaultBehavior.Item.ItemDefinitionId;
        }
        return ObjectData.ArchetypeId;
    }

    UGridItemDefinitionAsset* ResolveReceptacleInitialItemDefinition (const FGridReceptacleBehaviorParams& Receptacle)
    {
        return Receptacle.InitialContainedItemDefinition;
    }

    FName ResolveReceptacleInitialItemDefinitionId (const FGridReceptacleBehaviorParams& Receptacle)
    {
        if (Receptacle.InitialContainedItemDefinition && !Receptacle.InitialContainedItemDefinition->ItemDefinitionId.IsNone ())
        {
            return Receptacle.InitialContainedItemDefinition->ItemDefinitionId;
        }
        if (!Receptacle.InitialContainedItemDefinitionId.IsNone ())
        {
            return Receptacle.InitialContainedItemDefinitionId;
        }
        return Receptacle.InitialContainedItemArchetypeId;
    }

    FName ResolvePickupItemDefinitionId (const AGridItemActor* ItemActor, FName FallbackArchetypeId)
    {
        if (!ItemActor)
        {
            return FallbackArchetypeId;
        }
        if (const UGridItemDefinitionAsset* Definition = ItemActor->GetItemDefinitionAsset ())
        {
            if (!Definition->ItemDefinitionId.IsNone ())
            {
                return Definition->ItemDefinitionId;
            }
        }
        if (!ItemActor->GetItemDefinitionId ().IsNone ())
        {
            return ItemActor->GetItemDefinitionId ();
        }
        if (!FallbackArchetypeId.IsNone ())
        {
            return FallbackArchetypeId;
        }
        return ItemActor->GetItemArchetypeId ();
    }

    const FGridLevelObjectData* FindLevelObjectDataById (const UGridLevelAsset* LevelAsset, FGuid ObjectId)
    {
        if (!LevelAsset || !ObjectId.IsValid ())
        {
            return nullptr;
        }

        for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
        {
            if (ObjectData.ObjectId == ObjectId)
            {
                return &ObjectData;
            }
        }

        return nullptr;
    }

    int32 CountRuntimeTransitionObjects (const UGridLevelAsset* InLevelAsset)
    {
        if (!InLevelAsset)
        {
            return 0;
        }

        int32 TransitionCount = 0;
        for (const FGridLevelObjectData& ObjectData : InLevelAsset->Objects)
        {
            if (ObjectData.Behavior.Transition.bIsTransition)
            {
                ++TransitionCount;
            }
        }

        return TransitionCount;
    }

    int32 CountRemovedRuntimeObjects (const FGridLevelRuntimeState* RuntimeState)
    {
        if (!RuntimeState)
        {
            return 0;
        }

        int32 RemovedCount = 0;
        for (const TPair<FGuid, FGridRuntimeObjectPresenceState>& Pair : RuntimeState->ObjectPresence)
        {
            if (Pair.Value.bRemovedFromInitialPlacement)
            {
                ++RemovedCount;
            }
        }
        return RemovedCount;
    }

    int32 CountHiddenFloorCells (const UGridLevelAsset* InLevelAsset, const AGridLevelRuntimeActor* RuntimeActor)
    {
        if (!InLevelAsset || !RuntimeActor)
        {
            return 0;
        }

        int32 HiddenFloorCells = 0;
        for (int32 Y = 0; Y < InLevelAsset->Height; ++Y)
        {
            for (int32 X = 0; X < InLevelAsset->Width; ++X)
            {
                if (RuntimeActor->ShouldHideCellFloor (X, Y))
                {
                    ++HiddenFloorCells;
                }
            }
        }

        return HiddenFloorCells;
    }
}

bool AGridLevelRuntimeActor::IsSafeRuntimeRenderTransform (const FTransform& Transform)
{
    constexpr float MinAbsScale = 0.001f;
    const FVector Scale = Transform.GetScale3D ();

    return Transform.IsValid () &&
        !Transform.GetLocation ().ContainsNaN () &&
        !Scale.ContainsNaN () &&
        Transform.GetRotation ().IsNormalized () &&
        FMath::Abs (Scale.X) >= MinAbsScale &&
        FMath::Abs (Scale.Y) >= MinAbsScale &&
        FMath::Abs (Scale.Z) >= MinAbsScale;
}

void AGridLevelRuntimeActor::LogUnsafeInstanceTransform (const TCHAR* FunctionName, const UInstancedStaticMeshComponent* Component,
    int32 X, int32 Y, EGridEdge Edge, const FTransform& Transform) const
{
    UE_LOG (LogTemp, Error,
        TEXT ("Unsafe runtime render transform skipped: Function=%s Component=%s StaticMesh=%s Cell=(%d,%d) Edge=%d Location=%s Rotation=%s Scale=%s"),
        FunctionName,
        *GetNameSafe (Component),
        *GetNameSafe (Component ? Component->GetStaticMesh () : nullptr),
        X,
        Y,
        static_cast<int32> (Edge),
        *Transform.GetLocation ().ToCompactString (),
        *Transform.GetRotation ().ToString (),
        *Transform.GetScale3D ().ToCompactString ());
}

void AGridLevelRuntimeActor::LogUnsafeObjectTransform (const TCHAR* FunctionName, const FGridLevelObjectData& ObjectData,
    const UStaticMesh* StaticMesh, const FTransform& Transform) const
{
    UE_LOG (LogTemp, Error,
        TEXT ("Unsafe runtime render transform skipped: Function=%s ObjectId=%s ArchetypeId=%s Tag=%s Cell=(%d,%d) Edge=%d StaticMesh=%s Location=%s Rotation=%s Scale=%s"),
        FunctionName,
        *ObjectData.ObjectId.ToString (),
        *ObjectData.ArchetypeId.ToString (),
        *ObjectData.Tag.ToString (),
        ObjectData.CellX,
        ObjectData.CellY,
        static_cast<int32> (ObjectData.Edge),
        *GetNameSafe (StaticMesh),
        *Transform.GetLocation ().ToCompactString (),
        *Transform.GetRotation ().ToString (),
        *Transform.GetScale3D ().ToCompactString ());
}

void AGridLevelRuntimeActor::LogUnsafeItemTransform (const TCHAR* FunctionName, FName ArchetypeId, const AActor* OwnerActor,
    const USceneComponent* AttachParent, const UStaticMesh* StaticMesh, const FTransform& Transform) const
{
    UE_LOG (LogTemp, Error,
        TEXT ("Unsafe runtime item transform skipped: Function=%s ArchetypeId=%s Owner=%s AttachParent=%s StaticMesh=%s Location=%s Rotation=%s Scale=%s"),
        FunctionName,
        *ArchetypeId.ToString (),
        *GetNameSafe (OwnerActor),
        *GetNameSafe (AttachParent),
        *GetNameSafe (StaticMesh),
        *Transform.GetLocation ().ToCompactString (),
        *Transform.GetRotation ().ToString (),
        *Transform.GetScale3D ().ToCompactString ());
}

AGridLevelRuntimeActor::AGridLevelRuntimeActor ()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent> (TEXT ("Root"));
    SetRootComponent (SceneRoot);

    FloorISM = CreateDefaultSubobject<UInstancedStaticMeshComponent> (TEXT ("FloorISM"));
    FloorISM->SetupAttachment (SceneRoot);

    WallISM = CreateDefaultSubobject<UInstancedStaticMeshComponent> (TEXT ("WallISM"));
    WallISM->SetupAttachment (SceneRoot);

    CeilingISM = CreateDefaultSubobject<UInstancedStaticMeshComponent> (TEXT ("CeilingISM"));
    CeilingISM->SetupAttachment (SceneRoot);

    ActivationComponent = CreateDefaultSubobject<UGridActivationComponent> (TEXT ("ActivationComponent"));

    DoorSystemComponent = CreateDefaultSubobject<UGridDoorSystemComponent> (TEXT ("DoorSystemComponent"));

    EditorPreviewComponent = CreateDefaultSubobject<UGridEditorPreviewComponent> (TEXT ("EditorPreviewComponent"));
}

void AGridLevelRuntimeActor::ShowReadableMessage (const FText& MessageText)
{
    if (MessageText.IsEmpty ())
    {
        return;
    }
    UWorld* World = GetWorld ();
    if (!World || !ReadableMessageWidgetClass)
    {
        UE_LOG (LogTemp, Warning, TEXT ("ShowReadableMessage failed: missing world or widget class."));
        return;
    }
    APlayerController* PlayerController = World->GetFirstPlayerController ();
    if (!PlayerController)
    {
        UE_LOG (LogTemp, Warning, TEXT ("ShowReadableMessage failed: missing player controller."));
        return;
    }
    if (!ActiveReadableMessageWidget)
    {
        ActiveReadableMessageWidget = CreateWidget<UReadableMessageWidget> (PlayerController, ReadableMessageWidgetClass);

        if (!ActiveReadableMessageWidget)
        {
            UE_LOG (LogTemp, Warning, TEXT ("ShowReadableMessage failed: widget creation failed."));
            return;
        }
        ActiveReadableMessageWidget->AddToViewport (50);
    }
    ActiveReadableMessageWidget->SetReadableText (MessageText);
    GetWorldTimerManager ().ClearTimer (ReadableMessageTimerHandle);
    GetWorldTimerManager ().SetTimer (ReadableMessageTimerHandle, this, &AGridLevelRuntimeActor::HideReadableMessage, ReadableMessageDuration, false);
    if (!ActiveReadableMessageWidget->IsInViewport ())
    {
        ActiveReadableMessageWidget->AddToViewport (50);
    }
}

void AGridLevelRuntimeActor::HideReadableMessage ()
{
    GetWorldTimerManager ().ClearTimer (ReadableMessageTimerHandle);

    if (ActiveReadableMessageWidget)
    {
        ActiveReadableMessageWidget->RemoveFromParent ();
        ActiveReadableMessageWidget = nullptr;
    }
}

void AGridLevelRuntimeActor::OnConstruction (const FTransform& Transform)
{
    Super::OnConstruction (Transform);
    if (!bRebuildInConstruction)
    {
        return;
    }
    UWorld* World = GetWorld ();
    if (!World || World->IsGameWorld ())
    {
        return;
    }
    RebuildLevel (EGridRuntimeRebuildMode::GeometryOnly);
}

void AGridLevelRuntimeActor::BeginPlay ()
{
    Super::BeginPlay ();

    if (DungeonAsset)
    {
        if (!CurrentDungeonLevelId.IsNone ())
        {
            if (UGridLevelAsset* ConfiguredDungeonLevel = DungeonAsset->GetLevelAssetById (CurrentDungeonLevelId))
            {
                LevelAsset = ConfiguredDungeonLevel;
            }
            else
            {
                UE_LOG (
                    LogTemp,
                    Warning,
                    TEXT ("GridLevelRuntimeActor: CurrentDungeonLevelId %s is not valid in DungeonAsset %s; keeping configured LevelAsset."),
                    *CurrentDungeonLevelId.ToString (),
                    *DungeonAsset->GetPathName ());
            }
        }

        if (CurrentDungeonLevelId.IsNone ())
        {
            for (const FGridDungeonLevelEntry& Entry : DungeonAsset->Levels)
            {
                if (Entry.bEnabled && Entry.LevelAsset == LevelAsset)
                {
                    CurrentDungeonLevelId = Entry.LevelId;
                    break;
                }
            }

            if (CurrentDungeonLevelId.IsNone ())
            {
                if (DungeonAsset->IsValidLevelId (DungeonAsset->DefaultLevelId))
                {
                    CurrentDungeonLevelId = DungeonAsset->DefaultLevelId;
                    LevelAsset = DungeonAsset->GetLevelAssetById (DungeonAsset->DefaultLevelId);
                    UE_LOG (
                        LogTemp,
                        Log,
                        TEXT ("GridLevelRuntimeActor: using DungeonAsset DefaultLevelId %s at BeginPlay."),
                        *CurrentDungeonLevelId.ToString ());
                }
                else
                {
                    UE_LOG (
                        LogTemp,
                        Warning,
                        TEXT ("GridLevelRuntimeActor: could not resolve CurrentDungeonLevelId from DungeonAsset %s; keeping configured LevelAsset %s."),
                        *DungeonAsset->GetPathName (),
                        LevelAsset ? *LevelAsset->GetPathName () : TEXT ("None"));
                }
            }
        }
    }

    if (ActivationComponent)
    {
        ActivationComponent->Initialize (this);
        ActivationComponent->ResetRuntimeState ();
    }
    if (DoorSystemComponent)
    {
        DoorSystemComponent->Initialize (this);
        DoorSystemComponent->ResetRuntimeState ();
    }
    if (EditorPreviewComponent)
    {
        EditorPreviewComponent->Initialize (this);
    }
    RebuildLevel ();
}

FGridLevelRuntimeState* AGridLevelRuntimeActor::GetOrCreateRuntimeStateForCurrentLevel ()
{
    const FName RuntimeLevelId = ResolveRuntimeStateLevelId (DungeonAsset, CurrentDungeonLevelId);
    FGridLevelRuntimeState& State = DungeonRuntimeState.LevelStates.FindOrAdd (RuntimeLevelId);
    State.LevelId = RuntimeLevelId;
    return &State;
}

const FGridLevelRuntimeState* AGridLevelRuntimeActor::FindRuntimeStateForCurrentLevel () const
{
    const FName RuntimeLevelId = ResolveRuntimeStateLevelId (DungeonAsset, CurrentDungeonLevelId);
    return DungeonRuntimeState.LevelStates.Find (RuntimeLevelId);
}

bool AGridLevelRuntimeActor::CaptureCurrentLevelRuntimeState ()
{
    if (!LevelAsset)
    {
        return false;
    }

    FGridLevelRuntimeState* State = GetOrCreateRuntimeStateForCurrentLevel ();
    if (!State)
    {
        return false;
    }

    State->Doors.Reset ();
    State->InteractiveObjects.Reset ();
    State->ObjectPresence.Reset ();
    State->Items.Reset ();
    State->Receptacles.Reset ();
    State->bHasBeenVisited = true;

    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (!ObjectData.ObjectId.IsValid ())
        {
            continue;
        }

        if (ObjectData.Type == EGridLevelObjectType::Door && DoorSystemComponent)
        {
            bool bDoorOpen = false;
            bool bDoorMoving = false;
            bool bDoorBlocked = true;
            if (DoorSystemComponent->GetDoorState (ObjectData.ObjectId, bDoorOpen, bDoorMoving, bDoorBlocked))
            {
                FGridRuntimeDoorState DoorState;
                DoorState.ObjectId = ObjectData.ObjectId;
                DoorState.bIsOpen = bDoorOpen;
                DoorState.bBlocksMovement = bDoorBlocked;
                State->Doors.Add (DoorState.ObjectId, DoorState);
            }
        }
    }

    TSet<FGuid> ActiveObjectIds;
    if (ActivationComponent)
    {
        ActiveObjectIds = ActivationComponent->GetActiveObjectIds ();
        for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
        {
            if (!ObjectData.ObjectId.IsValid ())
            {
                continue;
            }

            const bool bIsInteractiveObject =
                ObjectData.Type == EGridLevelObjectType::Button ||
                ObjectData.Type == EGridLevelObjectType::Lever ||
                ObjectData.Type == EGridLevelObjectType::PressurePlate ||
                ObjectData.Type == EGridLevelObjectType::Receptacle ||
                ObjectData.Type == EGridLevelObjectType::Trigger;
            if (!bIsInteractiveObject)
            {
                continue;
            }

            const bool bIsActive = ActiveObjectIds.Contains (ObjectData.ObjectId);
            FGridRuntimeInteractiveState InteractiveState;
            InteractiveState.ObjectId = ObjectData.ObjectId;
            InteractiveState.bIsActivated = bIsActive;
            InteractiveState.bIsPressed = bIsActive;
            InteractiveState.bIsOn = bIsActive;
            State->InteractiveObjects.Add (ObjectData.ObjectId, InteractiveState);
        }
    }

    TSet<FGuid> ExistingPlacedItemObjectIds;
    for (const FGridSpawnedItemRuntimeEntry& Entry : SpawnedItemEntries)
    {
        AGridItemActor* ItemActor = Entry.ItemActor.Get ();
        if (!IsValid (ItemActor))
        {
            continue;
        }
        ExistingPlacedItemObjectIds.Add (Entry.ObjectId);
        FGridRuntimeItemState ItemState;
        ItemState.ObjectId = Entry.ObjectId;
        ItemState.ArchetypeId = Entry.ItemArchetypeId;
        ItemState.ItemDefinitionId = !Entry.ItemDefinitionId.IsNone ()
            ? Entry.ItemDefinitionId
            : ItemActor->GetItemDefinitionId ();

        if (ItemState.ItemDefinitionId.IsNone ())
        {
            UE_LOG (LogTemp, Warning,
                TEXT ("GridRuntimeState Capture skipped item: ObjectId=%s Actor=%s no ItemDefinitionId or legacy ArchetypeId resolved."),
                *Entry.ObjectId.ToString (),
                *GetNameSafe (ItemActor));
            continue;
        }
        ItemState.Transform = ItemActor->GetActorTransform ();
        ItemState.bIsSimulatingPhysics = ItemActor->IsSimulatingPhysics ();
        ItemState.bIsContainedInReceptacle = false;
        ItemState.bLightsEnabled = ItemActor->AreItemLightsEnabled ();
        State->Items.Add (Entry.ObjectId, ItemState);
    }
    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (ObjectData.Type != EGridLevelObjectType::Item || !ObjectData.ObjectId.IsValid ())
        {
            continue;
        }

        FGridRuntimeObjectPresenceState PresenceState;
        PresenceState.ObjectId = ObjectData.ObjectId;
        PresenceState.bRemovedFromInitialPlacement = !ExistingPlacedItemObjectIds.Contains (ObjectData.ObjectId);
        State->ObjectPresence.Add (ObjectData.ObjectId, PresenceState);
    }

    for (const TPair<FGuid, TObjectPtr<AGridRuntimeObjectActor>>& Pair : SpawnedRuntimeObjectActors)
    {
        AGridReceptacleActor* ReceptacleActor = Cast<AGridReceptacleActor> (Pair.Value.Get ());
        if (!IsValid (ReceptacleActor))
        {
            continue;
        }
        FGridRuntimeReceptacleState ReceptacleState;
        ReceptacleActor->CaptureRuntimeReceptacleState (ReceptacleState);
        State->Receptacles.Add (Pair.Key, ReceptacleState);
    }

    UE_LOG (LogTemp, Log,
        TEXT ("GridRuntimeState Capture Level=%s Doors=%d RemovedObjects=%d Items=%d Receptacles=%d Interactives=%d"),
        *State->LevelId.ToString (),
        State->Doors.Num (),
        CountRemovedRuntimeObjects (State),
        State->Items.Num (),
        State->Receptacles.Num (),
        State->InteractiveObjects.Num ());

    return true;
}

bool AGridLevelRuntimeActor::ApplyCurrentLevelRuntimeState ()
{
    if (!LevelAsset)
    {
        return false;
    }

    const FGridLevelRuntimeState* State = FindRuntimeStateForCurrentLevel ();
    if (!State || !State->bHasBeenVisited)
    {
        return false;
    }

    if (DoorSystemComponent)
    {
        for (const TPair<FGuid, FGridRuntimeDoorState>& Pair : State->Doors)
        {
            DoorSystemComponent->ApplyDoorState (Pair.Key, Pair.Value.bIsOpen, Pair.Value.bBlocksMovement);
        }
    }

    if (ActivationComponent)
    {
        TSet<FGuid> ActiveObjectIds;
        for (const TPair<FGuid, FGridRuntimeInteractiveState>& Pair : State->InteractiveObjects)
        {
            if (Pair.Value.bIsActivated || Pair.Value.bIsOn || Pair.Value.bIsPressed)
            {
                ActiveObjectIds.Add (Pair.Key);
            }
        }
        ActivationComponent->SetActiveObjectIds (ActiveObjectIds);

        for (const TPair<FGuid, FGridRuntimeInteractiveState>& Pair : State->InteractiveObjects)
        {
            const bool bIsActive = Pair.Value.bIsActivated || Pair.Value.bIsOn || Pair.Value.bIsPressed;
            if (AGridLeverActor* LeverActor = FindRuntimeObjectActor<AGridLeverActor> (Pair.Key))
            {
                LeverActor->SetLeverState (bIsActive);
            }
            if (AGridPressurePlateActor* PlateActor = FindRuntimeObjectActor<AGridPressurePlateActor> (Pair.Key))
            {
                PlateActor->SetPressed (bIsActive);
            }
        }
    }

    auto RemoveSpawnedItemEntry = [this] (int32 EntryIndex)
    {
        if (!SpawnedItemEntries.IsValidIndex (EntryIndex))
        {
            return;
        }

        AGridItemActor* ItemActor = SpawnedItemEntries[EntryIndex].ItemActor.Get ();
        if (IsValid (ItemActor))
        {
            ItemActor->OnRemovedFromWorld ();
            ItemActor->Destroy ();
        }
        SpawnedItemActors.RemoveAllSwap ([ItemActor] (const TObjectPtr<AGridItemActor>& SpawnedItemActor)
        {
            return SpawnedItemActor.Get () == ItemActor;
        });
        SpawnedItemEntries.RemoveAtSwap (EntryIndex);
    };

    for (const TPair<FGuid, FGridRuntimeObjectPresenceState>& Pair : State->ObjectPresence)
    {
        const FGridRuntimeObjectPresenceState& PresenceState = Pair.Value;
        if (!PresenceState.bRemovedFromInitialPlacement)
        {
            continue;
        }

        UE_LOG (LogTemp, Log,
            TEXT ("GridRuntimeState Apply RemovedInitialObject ObjectId=%s"),
            *PresenceState.ObjectId.ToString ());

        for (int32 EntryIndex = SpawnedItemEntries.Num () - 1; EntryIndex >= 0; --EntryIndex)
        {
            if (SpawnedItemEntries[EntryIndex].ObjectId == PresenceState.ObjectId)
            {
                RemoveSpawnedItemEntry (EntryIndex);
            }
        }
    }

    for (const TPair<FGuid, FGridRuntimeItemState>& Pair : State->Items)
    {
        const FGridRuntimeItemState& ItemState = Pair.Value;
        if (ItemState.bIsContainedInReceptacle)
        {
            continue;
        }

        bool bFoundExistingItem = false;
        for (FGridSpawnedItemRuntimeEntry& Entry : SpawnedItemEntries)
        {
            if (Entry.ObjectId != Pair.Key)
            {
                continue;
            }

            if (AGridItemActor* ItemActor = Entry.ItemActor.Get ())
            {
                ItemActor->SetActorTransform (ItemState.Transform, false, nullptr, ETeleportType::TeleportPhysics);
                ItemActor->SetRuntimeObjectId (Pair.Key);
                ItemActor->SetItemLightsEnabled (ItemState.bLightsEnabled);
                bFoundExistingItem = true;
            }
            break;
        }

        const FName RuntimeItemDefinitionId = !ItemState.ItemDefinitionId.IsNone () ? ItemState.ItemDefinitionId : ItemState.ArchetypeId;
        if (!bFoundExistingItem && !RuntimeItemDefinitionId.IsNone ())
        {
            UGridItemDefinitionAsset* ItemDefinition = nullptr;
            AGridItemActor* ItemActor = SpawnItemActorForDefinition (ItemDefinition, RuntimeItemDefinitionId, this, nullptr);
            if (ItemActor)
            {
                ItemActor->SetActorTransform (ItemState.Transform, false, nullptr, ETeleportType::TeleportPhysics);
                ItemActor->SetRuntimeObjectId (Pair.Key);
                ItemActor->ConfigureAsWorldPickup ();
                ItemActor->SetItemLightsEnabled (ItemState.bLightsEnabled);
                SpawnedItemActors.Add (ItemActor);

                FGridSpawnedItemRuntimeEntry Entry;
                Entry.Cell = FIntPoint (ItemActor->RuntimeCellX, ItemActor->RuntimeCellY);
                Entry.ItemActor = ItemActor;
                Entry.ObjectId = Pair.Key;
                Entry.ItemArchetypeId = !ItemState.ArchetypeId.IsNone () ? ItemState.ArchetypeId : RuntimeItemDefinitionId;
                Entry.ItemDefinitionAsset = ItemDefinition;
                Entry.ItemDefinitionId = RuntimeItemDefinitionId;
                SpawnedItemEntries.Add (Entry);
            }
        }
    }
    for (const TPair<FGuid, FGridRuntimeReceptacleState>& Pair : State->Receptacles)
    {
        AGridReceptacleActor* ReceptacleActor = FindRuntimeObjectActor<AGridReceptacleActor> (Pair.Key);
        if (!ReceptacleActor)
        {
            continue;
        }
        const int32 ClearedItemCount = ReceptacleActor->ForceClearRuntimeContents (false);
        const FGridLevelObjectData* ReceptacleObjectData = FindLevelObjectDataById (LevelAsset, Pair.Key);
        const UGridObjectArchetypeAsset* ReceptacleArchetype = ReceptacleObjectData
            ? FindObjectArchetype (ReceptacleObjectData->ArchetypeId)
            : nullptr;
        const TSubclassOf<AGridItemActor> PreferredItemActorClass =
            ReceptacleActor->ContainedItemActorClass
            ? ReceptacleActor->ContainedItemActorClass
            : (ReceptacleArchetype ? ReceptacleArchetype->ItemActorClass : nullptr);
        for (const FGridRuntimeItemState& ItemState : Pair.Value.ContainedItems)
        {
            const FName RuntimeItemDefinitionId = !ItemState.ItemDefinitionId.IsNone ()
                ? ItemState.ItemDefinitionId
                : (!ItemState.ArchetypeId.IsNone ()
                    ? ItemState.ArchetypeId
                    : (ReceptacleObjectData ? ResolveReceptacleInitialItemDefinitionId (ReceptacleObjectData->Behavior.Receptacle) : NAME_None));
            if (RuntimeItemDefinitionId.IsNone ())
            {
                UE_LOG (LogTemp, Warning,
                    TEXT ("GridRuntimeState Apply skipped receptacle item: ReceptacleId=%s RuntimeId=%s no ItemDefinitionId or legacy ArchetypeId resolved."),
                    *Pair.Key.ToString (),
                    *ItemState.ObjectId.ToString ());
                continue;
            }
            UGridItemDefinitionAsset* ItemDefinition =
                (ReceptacleObjectData && ReceptacleObjectData->Behavior.Receptacle.InitialContainedItemDefinition &&
                    ReceptacleObjectData->Behavior.Receptacle.InitialContainedItemDefinition->ItemDefinitionId == RuntimeItemDefinitionId)
                ? ReceptacleObjectData->Behavior.Receptacle.InitialContainedItemDefinition.Get ()
                : nullptr;
            AGridItemActor* ItemActor = SpawnItemActorForDefinition (ItemDefinition, RuntimeItemDefinitionId, ReceptacleActor, ReceptacleActor->ItemAttachPoint.Get (), PreferredItemActorClass);
            if (ItemActor)
            {
                ItemActor->SetRuntimeObjectId (ItemState.ObjectId);

                if (ItemDefinition)
                {
                    ItemActor->InitializeFromItemDefinition (ItemDefinition, ItemState.ObjectId);
                } else
                {
                    ItemActor->InitializeFromItemDefinitionId (RuntimeItemDefinitionId, ItemState.ObjectId);
                }
                ItemActor->SetItemLightsEnabled (ItemState.bLightsEnabled);
            }
            FGridRuntimeItemState ResolvedItemState = ItemState;
            ResolvedItemState.ItemDefinitionId = RuntimeItemDefinitionId;
            if (ResolvedItemState.ArchetypeId.IsNone ())
            {
                ResolvedItemState.ArchetypeId = RuntimeItemDefinitionId;
            }
            ReceptacleActor->RestoreRuntimeContainedItem (ResolvedItemState, ItemActor);
        }
        UE_LOG (LogTemp, Verbose,
            TEXT ("GridRuntimeState Apply Receptacle Final ObjectId=%s HasItem=%s Count=%d"),
            *Pair.Key.ToString (),
            ReceptacleActor->HasItem () ? TEXT ("true") : TEXT ("false"),
            ReceptacleActor->GetContainedItemCount ());
    }
    UE_LOG (LogTemp, Log,
        TEXT ("GridRuntimeState Apply Level=%s Doors=%d RemovedObjects=%d Items=%d Receptacles=%d Interactives=%d"),
        *State->LevelId.ToString (),
        State->Doors.Num (),
        CountRemovedRuntimeObjects (State),
        State->Items.Num (),
        State->Receptacles.Num (),
        State->InteractiveObjects.Num ());

    return true;
}

void AGridLevelRuntimeActor::ClearVisuals (EGridRuntimeRebuildMode RebuildMode)
{
    if (FloorISM) FloorISM->ClearInstances ();
    if (WallISM) WallISM->ClearInstances ();
    if (CeilingISM)
    {
        CeilingISM->ClearInstances ();
        CeilingISM->EmptyOverrideMaterials ();
    }
    if (RebuildMode != EGridRuntimeRebuildMode::GeometryOnly)
    {
        ClearRuntimeObjectActors ();
        if (EditorPreviewComponent)
        {
            EditorPreviewComponent->ClearPreviewObjects ();
        }
    }
    if (RebuildMode != EGridRuntimeRebuildMode::GeometryOnly)
    {
        if (DoorSystemComponent) DoorSystemComponent->ResetRuntimeState ();
        if (ActivationComponent) ActivationComponent->ResetRuntimeState ();
    }
}

bool AGridLevelRuntimeActor::IsValidCell (int32 X, int32 Y) const
{
    return LevelAsset && LevelAsset->IsValidCoord (X, Y);
}

const FGridLevelCellData& AGridLevelRuntimeActor::GetCell (int32 X, int32 Y) const
{
    check (LevelAsset);
    return LevelAsset->GetCell (X, Y);
}

FVector AGridLevelRuntimeActor::CellToWorld (int32 X, int32 Y, float ZOffset) const
{
    const float Size = LevelAsset ? LevelAsset->CellSize : 200.f;
    return GridOrigin + FVector (X * Size, Y * Size, ZOffset);
}

FVector AGridLevelRuntimeActor::GetCellCenterWorld (int32 X, int32 Y, float ZOffset) const
{
    const float CellSize = LevelAsset ? LevelAsset->CellSize : 200.f;
    return GetActorLocation () + GridOrigin + FVector ((X * CellSize) + (CellSize * 0.5f), (Y * CellSize) + (CellSize * 0.5f), ZOffset);
}

void AGridLevelRuntimeActor::AddFloor (int32 X, int32 Y, float CellSize)
{
    const FVector Base = CellToWorld (X, Y, 0.f);
    const FVector CenterOffset (CellSize * 0.5f, CellSize * 0.5f, 0.f);
    const FTransform T (FRotator::ZeroRotator, Base + CenterOffset, FVector::OneVector);
    if (!IsSafeRuntimeRenderTransform (T))
    {
        LogUnsafeInstanceTransform (TEXT ("AddFloor"), FloorISM, X, Y, EGridEdge::None, T);
        return;
    }
    FloorISM->AddInstance (T);
}

void AGridLevelRuntimeActor::AddCeiling (int32 X, int32 Y, float CellSize)
{
    const FVector Base = CellToWorld (X, Y, 200.f);
    const FVector CenterOffset (CellSize * 0.5f, CellSize * 0.5f, 0.f);
    const FTransform T (FRotator::ZeroRotator, Base + CenterOffset, FVector (1.f, 1.f, 1.f));
    if (!IsSafeRuntimeRenderTransform (T))
    {
        LogUnsafeInstanceTransform (TEXT ("AddCeiling"), CeilingISM, X, Y, EGridEdge::None, T);
        return;
    }
    CeilingISM->AddInstance (T);
}

bool AGridLevelRuntimeActor::ShouldSuppressStandardWallForEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    if (!LevelAsset || Edge == EGridEdge::None)
    {
        return false;
    }

    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (ObjectData.CellX == X &&
            ObjectData.CellY == Y &&
            ObjectData.Edge == Edge)
        {
            const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
            if (Archetype && Archetype->bReplacesStandardWall)
            {
                return true;
            }
        }
    }

    return false;
}

bool AGridLevelRuntimeActor::ShouldHideCellFloor (int32 CellX, int32 CellY) const
{
    if (!LevelAsset)
    {
        return false;
    }

    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (ObjectData.CellX != CellX || ObjectData.CellY != CellY)
        {
            continue;
        }

        const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
        if (Archetype && Archetype->bHideCellFloor)
        {
            return true;
        }
    }

    return false;
}

void AGridLevelRuntimeActor::AddEdgeInstance (UInstancedStaticMeshComponent* TargetISM, int32 X, int32 Y, EGridEdge Edge, float CellSize)
{
    if (!TargetISM)
    {
        return;
    }
    const FVector Base = CellToWorld (X, Y, 0.f);
    FVector Pos = Base;
    FRotator Rot = FRotator::ZeroRotator;
    switch (Edge)
    {
        case EGridEdge::North:
        {
            Pos = Base + FVector (CellSize * 0.5f, CellSize, 0.f);
            Rot = FRotator (0.f, 0.f, 0.f);
            break;
        }
        case EGridEdge::South:
        {
            Pos = Base + FVector (CellSize * 0.5f, 0.f, 0.f);
            Rot = FRotator (0.f, 180.f, 0.f);
            break;
        }
        case EGridEdge::East:
        {
            Pos = Base + FVector (CellSize, CellSize * 0.5f, 0.f);
            Rot = FRotator (0.f, -90.f, 0.f);
            break;
        }
        case EGridEdge::West:
        {
            Pos = Base + FVector (0.f, CellSize * 0.5f, 0.f);
            Rot = FRotator (0.f, 90.f, 0.f);
            break;
        }
        default:
        {
            return;
        }
    }
    const FTransform T (Rot, Pos, FVector::OneVector); 
    if (!IsSafeRuntimeRenderTransform (T))
    {
        LogUnsafeInstanceTransform (TEXT ("AddEdgeInstance"), TargetISM, X, Y, Edge, T);
        return;
    }
    TargetISM->AddInstance (T);
}

void AGridLevelRuntimeActor::RebuildLevel (EGridRuntimeRebuildMode RebuildMode)
{
    ClearVisuals (RebuildMode);

    if (!LevelAsset || !FloorISM || !WallISM || !CeilingISM)
    {
        return;
    }

    LevelAsset->EnsureCellCount ();
#if WITH_EDITOR
    for (const UGridObjectArchetypeAsset* Archetype : ObjectArchetypes)
    {
        if (!Archetype)
        {
            continue;
        }

        TArray<FGridArchetypeValidationMessage> ValidationMessages;
        Archetype->ValidateArchetype (ValidationMessages);

        bool bHasWarningOrError = false;
        for (const FGridArchetypeValidationMessage& Message : ValidationMessages)
        {
            if (Message.Severity == EGridArchetypeValidationSeverity::Warning ||
                Message.Severity == EGridArchetypeValidationSeverity::Error)
            {
                bHasWarningOrError = true;
                break;
            }
        }

        if (bHasWarningOrError)
        {
            UE_LOG (LogTemp, Warning, TEXT ("%s"), *Archetype->GetValidationSummary ());
        }
    }
#endif
    if (ActivationComponent)
    {
        ActivationComponent->Initialize (this);
        ActivationComponent->RebuildIndexes ();
    }

    if (DoorSystemComponent)
    {
        DoorSystemComponent->Initialize (this);
        DoorSystemComponent->RebuildIndexes ();
    }
    if (EditorPreviewComponent)
    {
        EditorPreviewComponent->Initialize (this);
    }
    FloorISM->SetStaticMesh (FloorMesh);
    WallISM->SetStaticMesh (WallMesh);
    CeilingISM->SetStaticMesh (CeilingMesh);

    const bool bIsGameWorld = GetWorld () && GetWorld ()->IsGameWorld ();

    if (!bIsGameWorld && CeilingEditorMaterial && CeilingMesh)
    {
        const int32 MaterialCount = CeilingMesh->GetStaticMaterials ().Num ();

        for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
        {
            CeilingISM->SetMaterial (MaterialIndex, CeilingEditorMaterial);
        }
    }
    const float CellSize = LevelAsset->CellSize;
    for (int32 Y = 0; Y < LevelAsset->Height; ++Y)
    {
        for (int32 X = 0; X < LevelAsset->Width; ++X)
        {
            const FGridLevelCellData& Cell = LevelAsset->GetCell (X, Y);
            if (Cell.CellType == EGridCellType::Empty)
            {
                continue;
            }
            if (!ShouldHideCellFloor (X, Y))
            {
                AddFloor (X, Y, CellSize);
            }

            if (Cell.bHasCeiling)
            {
                AddCeiling (X, Y, CellSize);
            }
            auto DrawEdgeIfNeeded =
                [&] (EGridEdge Edge, EGridWallType WallType, bool bShouldDraw)
            {
                if (!bShouldDraw || ShouldSuppressStandardWallForEdge (X, Y, Edge))
                {
                    return;
                }
                switch (WallType)
                {
                    case EGridWallType::Solid:
                    AddEdgeInstance (WallISM, X, Y, Edge, CellSize);
                    break;

                    default:
                    break;
                }
            };
            DrawEdgeIfNeeded (EGridEdge::North, Cell.NorthWall, Cell.NorthWall != EGridWallType::None);
            DrawEdgeIfNeeded (EGridEdge::East, Cell.EastWall, Cell.EastWall != EGridWallType::None);
            DrawEdgeIfNeeded (EGridEdge::South, Cell.SouthWall, Cell.SouthWall != EGridWallType::None);
            DrawEdgeIfNeeded (EGridEdge::West, Cell.WestWall, Cell.WestWall != EGridWallType::None);
        }
    }
    if (RebuildMode != EGridRuntimeRebuildMode::GeometryOnly)
    {
        if (!GetWorld () || !GetWorld ()->IsGameWorld ())
        {
            if (EditorPreviewComponent)
            {
                EditorPreviewComponent->RebuildPreviewObjects ();
            }
        }
    }    
    if (RebuildMode != EGridRuntimeRebuildMode::GeometryOnly)
    {
        if (bIsGameWorld)
        {
            RebuildRuntimeObjects ();
        }
    }
    if (bEnableRuntimeDebugLog)
    {
        LogRuntimeDebugSummary ();
    }

    if (bEnableRuntimeDebugScreen)
    {
        ShowRuntimeDebugSummary ();
    }
}

bool AGridLevelRuntimeActor::IsWalkableCell (int32 X, int32 Y) const
{
    if (!LevelAsset || !LevelAsset->IsValidCoord (X, Y))
    {
        return false;
    }
    const FGridLevelCellData& Cell = LevelAsset->GetCell (X, Y);
    if (Cell.CellType == EGridCellType::Empty)
    {
        return false;
    }
    if (Cell.bBlocksOccupancy)
    {
        return false;
    }
    return true;
}

bool AGridLevelRuntimeActor::TryGetNeighborCell (int32 X, int32 Y, EGridEdge Direction, int32& OutX, int32& OutY) const
{
    OutX = X;
    OutY = Y;

    if (!LevelAsset)
    {
        return false;
    }

    switch (Direction)
    {
        case EGridEdge::North:
            OutY = Y + 1;
            break;

        case EGridEdge::East:
            OutX = X + 1;
            break;

        case EGridEdge::South:
            OutY = Y - 1;
            break;

        case EGridEdge::West:
            OutX = X - 1;
            break;

        default:
            return false;
    }

    return LevelAsset->IsValidCoord (OutX, OutY);
}

EGridWallType AGridLevelRuntimeActor::GetWallOnEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    if (!LevelAsset || !LevelAsset->IsValidCoord (X, Y))
    {
        return EGridWallType::Solid;
    }

    const FGridLevelCellData& Cell = LevelAsset->GetCell (X, Y);

    switch (Edge)
    {
        case EGridEdge::North:
            return Cell.NorthWall;

        case EGridEdge::East:
            return Cell.EastWall;

        case EGridEdge::South:
            return Cell.SouthWall;

        case EGridEdge::West:
            return Cell.WestWall;

        default:
            return EGridWallType::Solid;
    }
}

bool AGridLevelRuntimeActor::CanMove (int32 FromX, int32 FromY, EGridEdge Direction) const
{
    if (!IsWalkableCell (FromX, FromY))
    {
        return false;
    }

    int32 ToX = INDEX_NONE;
    int32 ToY = INDEX_NONE;

    if (!TryGetNeighborCell (FromX, FromY, Direction, ToX, ToY))
    {
        return false;
    }

    if (!IsWalkableCell (ToX, ToY))
    {
        return false;
    }
    if (DoorSystemComponent && DoorSystemComponent->IsDoorPassageBlocked (FromX, FromY, Direction))
    {
        return false;
    }
    const EGridWallType Wall = GetWallOnEdge (FromX, FromY, Direction);

    switch (Wall)
    {
        case EGridWallType::None:
            return true;

        case EGridWallType::Solid:
        default:
            return false;
    }
}

void AGridLevelRuntimeActor::GetEdgeTransform (int32 X, int32 Y, EGridEdge Edge, float CellSize, FVector& OutWorldLocation, FRotator& OutWorldRotation) const
{
    const FVector Base = GetActorLocation () + CellToWorld (X, Y, 0.f);

    switch (Edge)
    {
        case EGridEdge::North:
        OutWorldLocation = Base + FVector (CellSize * 0.5f, CellSize, 0.f);
        OutWorldRotation = FRotator (0.f, 0.f, 0.f);
        break;

        case EGridEdge::East:
        OutWorldLocation = Base + FVector (CellSize, CellSize * 0.5f, 0.f);
        OutWorldRotation = FRotator (0.f, -90.f, 0.f);
        break;

        case EGridEdge::South:
        OutWorldLocation = Base + FVector (CellSize * 0.5f, 0.f, 0.f);
        OutWorldRotation = FRotator (0.f, 180.f, 0.f);
        break;

        case EGridEdge::West:
        OutWorldLocation = Base + FVector (0.f, CellSize * 0.5f, 0.f);
        OutWorldRotation = FRotator (0.f, 90.f, 0.f);
        break;

        default:
        OutWorldLocation = Base;
        OutWorldRotation = FRotator::ZeroRotator;
        break;
    }
}

bool AGridLevelRuntimeActor::HasDoorOnEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    int32 ResolvedX = INDEX_NONE;
    int32 ResolvedY = INDEX_NONE;
    EGridEdge ResolvedEdge = EGridEdge::None;
    bool bResolvedOpposite = false;
    return TryResolveDoorEdge (X, Y, Edge, ResolvedX, ResolvedY, ResolvedEdge, bResolvedOpposite);
}

bool AGridLevelRuntimeActor::TryGetOppositeEdge (int32 X, int32 Y, EGridEdge Edge, int32& OutX, int32& OutY, EGridEdge& OutEdge) const
{
    OutX = X;
    OutY = Y;
    OutEdge = EGridEdge::None;

    if (Edge == EGridEdge::None)
    {
        return false;
    }
    if (!TryGetNeighborCell (X, Y, Edge, OutX, OutY))
    {
        return false;
    }
    OutEdge = GridDirectionUtils::GetBackward (Edge);
    return OutEdge != EGridEdge::None;
}

bool AGridLevelRuntimeActor::TryResolveDoorEdge (int32 X, int32 Y, EGridEdge Edge, int32& OutX, int32& OutY, EGridEdge& OutEdge, bool& bOutResolvedOpposite) const
{
    OutX = X;
    OutY = Y;
    OutEdge = Edge;
    bOutResolvedOpposite = false;

    if (!DoorSystemComponent)
    {
        return false;
    }
    if (DoorSystemComponent->HasDoorOnEdge (X, Y, Edge))
    {
        return true;
    }
    if (!TryGetOppositeEdge (X, Y, Edge, OutX, OutY, OutEdge))
    {
        return false;
    }
    bOutResolvedOpposite = DoorSystemComponent->HasDoorOnEdge (OutX, OutY, OutEdge);
    return bOutResolvedOpposite;
}

bool AGridLevelRuntimeActor::IsDoorOpenOnEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    int32 ResolvedX = INDEX_NONE;
    int32 ResolvedY = INDEX_NONE;
    EGridEdge ResolvedEdge = EGridEdge::None;
    bool bResolvedOpposite = false;
    return TryResolveDoorEdge (X, Y, Edge, ResolvedX, ResolvedY, ResolvedEdge, bResolvedOpposite) &&
        DoorSystemComponent->IsDoorOpenOnEdge (ResolvedX, ResolvedY, ResolvedEdge);
}

bool AGridLevelRuntimeActor::ToggleDoorOnEdge (int32 X, int32 Y, EGridEdge Edge)
{
    int32 ResolvedX = INDEX_NONE;
    int32 ResolvedY = INDEX_NONE;
    EGridEdge ResolvedEdge = EGridEdge::None;
    bool bResolvedOpposite = false;
    if (TryResolveDoorEdge (X, Y, Edge, ResolvedX, ResolvedY, ResolvedEdge, bResolvedOpposite) &&
        DoorSystemComponent->ToggleDoorOnEdge (ResolvedX, ResolvedY, ResolvedEdge))
    {
        if (bResolvedOpposite)
        {
            UE_LOG (LogTemp, Log, TEXT ("Grid Use: toggled door on opposite edge (%d,%d,%d)."),
                ResolvedX, ResolvedY, static_cast<int32> (ResolvedEdge));
        }
        return true;
    }
    return false;
}

bool AGridLevelRuntimeActor::OpenDoorOnEdge (int32 X, int32 Y, EGridEdge Edge)
{
    int32 ResolvedX = INDEX_NONE;
    int32 ResolvedY = INDEX_NONE;
    EGridEdge ResolvedEdge = EGridEdge::None;
    bool bResolvedOpposite = false;
    if (TryResolveDoorEdge (X, Y, Edge, ResolvedX, ResolvedY, ResolvedEdge, bResolvedOpposite) &&
        DoorSystemComponent->OpenDoorOnEdge (ResolvedX, ResolvedY, ResolvedEdge))
    {
        if (bResolvedOpposite)
        {
            UE_LOG (LogTemp, Log, TEXT ("Grid Use: opened door on opposite edge (%d,%d,%d)."),
                ResolvedX, ResolvedY, static_cast<int32> (ResolvedEdge));
        }
        return true;
    }
    return false;
}

bool AGridLevelRuntimeActor::CloseDoorOnEdge (int32 X, int32 Y, EGridEdge Edge)
{
    int32 ResolvedX = INDEX_NONE;
    int32 ResolvedY = INDEX_NONE;
    EGridEdge ResolvedEdge = EGridEdge::None;
    bool bResolvedOpposite = false;
    if (TryResolveDoorEdge (X, Y, Edge, ResolvedX, ResolvedY, ResolvedEdge, bResolvedOpposite) &&
        DoorSystemComponent->CloseDoorOnEdge (ResolvedX, ResolvedY, ResolvedEdge))
    {
        if (bResolvedOpposite)
        {
            UE_LOG (LogTemp, Log, TEXT ("Grid Use: closed door on opposite edge (%d,%d,%d)."),
                ResolvedX, ResolvedY, static_cast<int32> (ResolvedEdge));
        }
        return true;
    }
    return false;
}

bool AGridLevelRuntimeActor::TryInteractAtEdge (int32 FromCellX, int32 FromCellY, EGridEdge Edge, AGrimrockPartyPawn* PartyPawn)
{
    if (!ActivationComponent)
    {
        return false;
    }
    if (ActivationComponent->TryInteractAtEdge (FromCellX, FromCellY, Edge, PartyPawn))
    {
        return true;
    }
    int32 OppositeX = INDEX_NONE;
    int32 OppositeY = INDEX_NONE;
    EGridEdge OppositeEdge = EGridEdge::None;
    if (TryGetOppositeEdge (FromCellX, FromCellY, Edge, OppositeX, OppositeY, OppositeEdge) &&
        ActivationComponent->TryInteractAtEdge (OppositeX, OppositeY, OppositeEdge, PartyPawn))
    {
        UE_LOG (LogTemp, Log, TEXT ("Grid Use: interacted with object on opposite edge (%d,%d,%d)."),
            OppositeX, OppositeY, static_cast<int32> (OppositeEdge));
        return true;
    }
    return false;
}

bool AGridLevelRuntimeActor::ExecuteLinksFromRuntimeObject (FGuid SourceObjectId, EGridObjectEvent SourceEvent)
{
    return ActivationComponent ? ActivationComponent->ExecuteLinksFromObjectForEvent (SourceObjectId, SourceEvent) : false;
}

void AGridLevelRuntimeActor::HandlePartyCellChanged (int32 OldCellX, int32 OldCellY, int32 NewCellX, int32 NewCellY)
{
    if (ActivationComponent)
    {
        ActivationComponent->HandlePartyCellChanged (OldCellX, OldCellY, NewCellX, NewCellY);
    }
}

void AGridLevelRuntimeActor::NotifyPawnEnteredCell (int32 CellX, int32 CellY)
{
    if (ActivationComponent)
    {
        ActivationComponent->NotifyPawnEnteredCell (CellX, CellY);
    }
}

void AGridLevelRuntimeActor::NotifyPawnExitedCell (int32 CellX, int32 CellY)
{
    if (ActivationComponent)
    {
        ActivationComponent->NotifyPawnExitedCell (CellX, CellY);
    }
}

bool AGridLevelRuntimeActor::FindTransitionAtCell (
    int32 CellX,
    int32 CellY,
    bool bTriggeredByUseAction,
    FGridObjectTransitionParams& OutTransition) const
{
    if (!LevelAsset)
    {
        UE_LOG (LogTemp, Warning, TEXT ("Dungeon transition lookup failed: LevelAsset is null."));
        return false;
    }

    int32 TransitionCountAtCell = 0;
    bool bFoundUsableTransition = false;

    for (const FGridLevelObjectData& Obj : LevelAsset->Objects)
    {
        const FGridObjectTransitionParams& Transition = Obj.Behavior.Transition;
        if (Obj.CellX != CellX || Obj.CellY != CellY || !Transition.bIsTransition)
        {
            continue;
        }

        ++TransitionCountAtCell;

        if (!bTriggeredByUseAction && Transition.bRequireUseAction)
        {
            UE_LOG (
                LogTemp,
                Log,
                TEXT ("Dungeon transition ignored at Cell=(%d,%d): object %s requires Use action."),
                CellX,
                CellY,
                *Obj.ObjectId.ToString ());
            continue;
        }

        if (!bFoundUsableTransition)
        {
            OutTransition = Transition;
            bFoundUsableTransition = true;
            UE_LOG (
                LogTemp,
                Log,
                TEXT ("Dungeon transition found at Cell=(%d,%d): TargetLevelId=%s TargetCell=(%d,%d) Facing=%s."),
                CellX,
                CellY,
                *Transition.TargetLevelId.ToString (),
                Transition.TargetCellX,
                Transition.TargetCellY,
                *GetRuntimeEdgeText (Transition.TargetFacing));
        }
    }

    if (TransitionCountAtCell > 1)
    {
        UE_LOG (
            LogTemp,
            Warning,
            TEXT ("Dungeon transition: multiple transition objects found at Cell=(%d,%d); using the first valid transition."),
            CellX,
            CellY);
    }

    return bFoundUsableTransition;
}

bool AGridLevelRuntimeActor::TryExecuteTransitionAtCell (
    int32 CellX,
    int32 CellY,
    AGrimrockPartyPawn* PartyPawn,
    bool bTriggeredByUseAction)
{
    FGridObjectTransitionParams Transition;
    if (!FindTransitionAtCell (CellX, CellY, bTriggeredByUseAction, Transition))
    {
        return false;
    }

    return TravelToDungeonLevel (
        Transition.TargetLevelId,
        Transition.TargetCellX,
        Transition.TargetCellY,
        Transition.TargetFacing,
        PartyPawn);
}

bool AGridLevelRuntimeActor::TravelToDungeonLevel (
    FName TargetLevelId,
    int32 TargetCellX,
    int32 TargetCellY,
    EGridEdge TargetFacing,
    AGrimrockPartyPawn* PartyPawn)
{
    if (bIsExecutingDungeonTransition)
    {
        UE_LOG (LogTemp, Warning, TEXT ("Dungeon transition ignored: another transition is already executing."));
        return false;
    }

    struct FScopedDungeonTransitionGuard
    {
        bool& bGuard;

        explicit FScopedDungeonTransitionGuard (bool& InGuard)
            : bGuard (InGuard)
        {
            bGuard = true;
        }

        ~FScopedDungeonTransitionGuard ()
        {
            bGuard = false;
        }
    };

    FScopedDungeonTransitionGuard TransitionGuard (bIsExecutingDungeonTransition);

    if (!DungeonAsset)
    {
        UE_LOG (LogTemp, Error, TEXT ("Dungeon transition failed: DungeonAsset is null."));
        return false;
    }

    if (TargetLevelId.IsNone ())
    {
        UE_LOG (LogTemp, Error, TEXT ("Dungeon transition failed: TargetLevelId is None."));
        return false;
    }

    const FGridDungeonLevelEntry* TargetEntry = DungeonAsset->FindLevelEntry (TargetLevelId);
    if (!TargetEntry)
    {
        UE_LOG (
            LogTemp,
            Error,
            TEXT ("Dungeon transition failed: TargetLevelId %s was not found in DungeonAsset %s."),
            *TargetLevelId.ToString (),
            *DungeonAsset->GetPathName ());
        return false;
    }

    if (!TargetEntry->bEnabled)
    {
        UE_LOG (
            LogTemp,
            Error,
            TEXT ("Dungeon transition failed: TargetLevelId %s is disabled."),
            *TargetLevelId.ToString ());
        return false;
    }

    UGridLevelAsset* TargetLevelAsset = TargetEntry->LevelAsset.Get ();
    if (!TargetLevelAsset)
    {
        UE_LOG (
            LogTemp,
            Error,
            TEXT ("Dungeon transition failed: TargetLevelId %s has no LevelAsset."),
            *TargetLevelId.ToString ());
        return false;
    }

    if (!TargetLevelAsset->IsValidCoord (TargetCellX, TargetCellY))
    {
        UE_LOG (
            LogTemp,
            Error,
            TEXT ("Dungeon transition failed: Target cell (%d,%d) is outside LevelAsset %s."),
            TargetCellX,
            TargetCellY,
            *TargetLevelAsset->GetPathName ());
        return false;
    }

    const FGridLevelCellData& TargetCell = TargetLevelAsset->GetCell (TargetCellX, TargetCellY);
    if (TargetCell.CellType == EGridCellType::Empty || TargetCell.bBlocksOccupancy)
    {
        UE_LOG (
            LogTemp,
            Error,
            TEXT ("Dungeon transition failed: Target cell (%d,%d) is not walkable in LevelAsset %s. CellType=%d BlocksOccupancy=%s."),
            TargetCellX,
            TargetCellY,
            *TargetLevelAsset->GetPathName (),
            static_cast<int32> (TargetCell.CellType),
            *GetRuntimeBoolText (TargetCell.bBlocksOccupancy));
        return false;
    }

    if (TargetFacing == EGridEdge::None)
    {
        UE_LOG (LogTemp, Error, TEXT ("Dungeon transition failed: TargetFacing is None."));
        return false;
    }

    if (!PartyPawn)
    {
        UE_LOG (LogTemp, Error, TEXT ("Dungeon transition failed: PartyPawn is null."));
        return false;
    }

    UE_LOG (
        LogTemp,
        Log,
        TEXT ("Dungeon transition: %s -> %s, Cell=(%d,%d), Facing=%s."),
        *CurrentDungeonLevelId.ToString (),
        *TargetLevelId.ToString (),
        TargetCellX,
        TargetCellY,
        *GetRuntimeEdgeText (TargetFacing));

    const FName OldLevelId = ResolveRuntimeStateLevelId (DungeonAsset, CurrentDungeonLevelId);
    CaptureCurrentLevelRuntimeState ();
    if (const FGridLevelRuntimeState* StoredState = DungeonRuntimeState.LevelStates.Find (OldLevelId))
    {
        UE_LOG (LogTemp, Log,
            TEXT ("GridRuntimeState Stored Level=%s Receptacles=%d Items=%d Doors=%d"),
            *OldLevelId.ToString (),
            StoredState->Receptacles.Num (),
            StoredState->Items.Num (),
            StoredState->Doors.Num ());
    }

    CurrentDungeonLevelId = TargetLevelId;
    LevelAsset = TargetLevelAsset;

    RebuildLevel ();
    ApplyCurrentLevelRuntimeState ();
    PartyPawn->SetGridStart (this, TargetCellX, TargetCellY, TargetFacing);

    UE_LOG (
        LogTemp,
        Log,
        TEXT ("Dungeon transition complete: CurrentDungeonLevelId=%s LevelAsset=%s PartyCell=(%d,%d) Facing=%s."),
        *CurrentDungeonLevelId.ToString (),
        LevelAsset ? *LevelAsset->GetPathName () : TEXT ("None"),
        TargetCellX,
        TargetCellY,
        *GetRuntimeEdgeText (TargetFacing));
    return true;
}

void AGridLevelRuntimeActor::SetEditorHoveredObject (FGuid ObjectId)
{
    if (EditorPreviewComponent)
    {
        EditorPreviewComponent->SetHoveredObject (ObjectId);
    }
}

void AGridLevelRuntimeActor::SetEditorSelectedObject (FGuid ObjectId)
{
    if (EditorPreviewComponent)
    {
        EditorPreviewComponent->SetSelectedObject (ObjectId);
    }
}

void AGridLevelRuntimeActor::CleanupOrphanEditorPreviewObjects ()
{
    if (EditorPreviewComponent)
    {
		EditorPreviewComponent->CleanupOrphanPreviewObjects ();
    }
}

const UGridObjectArchetypeAsset* AGridLevelRuntimeActor::FindObjectArchetype (FName ArchetypeId) const
{
    if (ArchetypeId.IsNone ())
    {
        return nullptr;
    }
    for (const UGridObjectArchetypeAsset* Archetype : ObjectArchetypes)
    {
        if (!Archetype)
        {
            continue;
        }
        if (Archetype->ArchetypeId == ArchetypeId)
        {
            return Archetype;
        }
    }
    return nullptr;
}

UGridItemDefinitionAsset* AGridLevelRuntimeActor::ResolveRuntimeItemDefinition (FName ItemDefinitionId) const
{
    if (ItemDefinitionId.IsNone ())
    {
        return nullptr;
    }

    if (LevelAsset)
    {
        for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
        {
            if (ObjectData.ItemDefinitionAsset &&
                ObjectData.ItemDefinitionAsset->ItemDefinitionId == ItemDefinitionId)
            {
                return ObjectData.ItemDefinitionAsset;
            }

            const FGridReceptacleBehaviorParams& Receptacle = ObjectData.Behavior.Receptacle;
            if (Receptacle.InitialContainedItemDefinition &&
                Receptacle.InitialContainedItemDefinition->ItemDefinitionId == ItemDefinitionId)
            {
                return Receptacle.InitialContainedItemDefinition;
            }
        }
    }

    for (const UGridObjectArchetypeAsset* Archetype : ObjectArchetypes)
    {
        if (!Archetype)
        {
            continue;
        }

        const auto& ItemParams = Archetype->DefaultBehavior.Item;
        if (ItemParams.ItemDefinitionAsset &&
            ItemParams.ItemDefinitionAsset->ItemDefinitionId == ItemDefinitionId)
        {
            return ItemParams.ItemDefinitionAsset;
        }

        const FGridReceptacleBehaviorParams& ReceptacleParams = Archetype->DefaultBehavior.Receptacle;
        if (ReceptacleParams.InitialContainedItemDefinition &&
            ReceptacleParams.InitialContainedItemDefinition->ItemDefinitionId == ItemDefinitionId)
        {
            return ReceptacleParams.InitialContainedItemDefinition;
        }
    }

    return nullptr;
}

UStaticMesh* AGridLevelRuntimeActor::GetObjectMesh (const FGridLevelObjectData& ObjectData) const
{
    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    if (!Archetype)
    {
        return nullptr;
    }

    if (Archetype->PreviewMesh)
    {
        return Archetype->PreviewMesh.Get ();
    }

    if (Archetype->MovingMesh)
    {
        return Archetype->MovingMesh.Get ();
    }

    if (Archetype->FixedMesh)
    {
        return Archetype->FixedMesh.Get ();
    }

    return nullptr;
}

UMaterialInterface* AGridLevelRuntimeActor::GetObjectMaterial (const FGridLevelObjectData& ObjectData) const
{
    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    if (!Archetype)
    {
        return nullptr;
    }

    if (Archetype->PreviewMaterial)
    {
        return Archetype->PreviewMaterial.Get ();
    }

    if (Archetype->MovingMaterial)
    {
        return Archetype->MovingMaterial.Get ();
    }

    if (Archetype->FixedMaterial)
    {
        return Archetype->FixedMaterial.Get ();
    }

    return nullptr;
}

AGridItemActor* AGridLevelRuntimeActor::SpawnItemActorForDefinition (UGridItemDefinitionAsset* ItemDefinition, FName ItemDefinitionId,
    AActor* OwnerActor, USceneComponent* AttachParent, TSubclassOf<AGridItemActor> PreferredItemActorClass) const
{
    ItemDefinitionId = ItemDefinition && !ItemDefinition->ItemDefinitionId.IsNone ()
        ? ItemDefinition->ItemDefinitionId : ItemDefinitionId;
    if (!ItemDefinition && ItemDefinitionId.IsNone ())
    {
        UE_LOG (LogTemp, Warning, TEXT ("Grid item spawn failed: missing ItemDefinition and ItemDefinitionId."));
        return nullptr;
    }
    UWorld* World = GetWorld ();
    if (!World)
    {
        return nullptr;
    }
    TSubclassOf<AGridItemActor> ItemClass = PreferredItemActorClass;
    if (!ItemClass)
    {
        ItemClass = AGridItemActor::StaticClass ();
    }
    UStaticMesh* ItemMesh = ItemDefinition ? ItemDefinition->WorldMesh.LoadSynchronous () : nullptr;
    const FTransform SpawnTransform (
        AttachParent ? AttachParent->GetComponentRotation () : FRotator::ZeroRotator,
        AttachParent ? AttachParent->GetComponentLocation () : GetActorLocation (),
        FVector::OneVector);
    if (!IsSafeRuntimeRenderTransform (SpawnTransform))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("Grid item spawn failed: unsafe transform Item=%s Owner=%s AttachParent=%s."),
            *ItemDefinitionId.ToString (),
            OwnerActor ? *OwnerActor->GetName () : TEXT ("None"),
            AttachParent ? *AttachParent->GetName () : TEXT ("None"));
        return nullptr;
    }
    FActorSpawnParameters Params;
    Params.Owner = OwnerActor ? OwnerActor : const_cast<AGridLevelRuntimeActor*>(this);
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AGridItemActor* ItemActor = World->SpawnActor<AGridItemActor> (ItemClass, SpawnTransform.GetLocation (), SpawnTransform.GetRotation ().Rotator (), Params);

    if (!ItemActor)
    {
        return nullptr;
    }
    // Optional mesh. InitializeItem must not overwrite BP components
    // si ItemMesh est nullptr.
    ItemActor->InitializeItem (ItemDefinitionId, TArray<FName> (), ItemMesh, nullptr);
    if (ItemDefinition)
    {
        ItemActor->InitializeFromItemDefinition (ItemDefinition, FGuid ());
    } else
    {
        ItemActor->InitializeFromItemDefinitionId (ItemDefinitionId, FGuid ());
    }
    if (AttachParent)
    {
        ItemActor->ConfigureAsAttachedItem ();
        ItemActor->AttachToComponent (AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        ItemActor->SetActorRelativeTransform (FTransform::Identity);
    }
    return ItemActor;
}

bool AGridLevelRuntimeActor::GetFloorEdgeObjectTransform (const FGridLevelObjectData& ObjectData, float ZOffset, float EdgeInset,
    FTransform& OutTransform) const
{
    if (!LevelAsset || ObjectData.Edge == EGridEdge::None)
    {
        return false;
    }

    const float CellSize = LevelAsset->CellSize;
    const FVector Base = GetActorLocation () + CellToWorld (ObjectData.CellX, ObjectData.CellY, ZOffset);
    FVector Pos = Base + FVector (CellSize * 0.5f, CellSize * 0.5f, 0.f);
    FRotator Rot = FRotator::ZeroRotator;

    switch (ObjectData.Edge)
    {
        case EGridEdge::North:
            Pos = Base + FVector (CellSize * 0.5f, CellSize - EdgeInset, 0.f);
            Rot = FRotator (0.f, 0.f, 0.f);
            break;

        case EGridEdge::South:
            Pos = Base + FVector (CellSize * 0.5f, EdgeInset, 0.f);
            Rot = FRotator (0.f, 180.f, 0.f);
            break;

        case EGridEdge::East:
            Pos = Base + FVector (CellSize - EdgeInset, CellSize * 0.5f, 0.f);
            Rot = FRotator (0.f, 90.f, 0.f);
            break;

        case EGridEdge::West:
            Pos = Base + FVector (EdgeInset, CellSize * 0.5f, 0.f);
            Rot = FRotator (0.f, -90.f, 0.f);
            break;

        default:
            return false;
    }

    Rot.Yaw += ObjectData.LocalYaw;
    OutTransform = FTransform (Rot, Pos, FVector::OneVector);
    return true;
}

bool AGridLevelRuntimeActor::GetWallMountedObjectTransform (const FGridLevelObjectData& ObjectData, float ZOffset, float WallInset,
    float LocalOffsetAlongWall, float LocalOffsetVertical, FTransform& OutTransform) const
{
    if (!LevelAsset || ObjectData.Edge == EGridEdge::None)
    {
        return false;
    }

    const float CellSize = LevelAsset->CellSize;
    const float FinalZ = ZOffset + LocalOffsetVertical;
    const FVector Base = GetActorLocation () + CellToWorld (ObjectData.CellX, ObjectData.CellY, FinalZ);
    FVector Pos = Base;
    FRotator Rot = FRotator::ZeroRotator;

    switch (ObjectData.Edge)
    {
        case EGridEdge::North:
            Pos = Base + FVector ((CellSize * 0.5f) + LocalOffsetAlongWall, CellSize - WallInset, 0.f);
            Rot = FRotator (0.f, 90.f, 0.f);
            break;

        case EGridEdge::South:
            Pos = Base + FVector ((CellSize * 0.5f) - LocalOffsetAlongWall, WallInset, 0.f);
            Rot = FRotator (0.f, -90.f, 0.f);
            break;

        case EGridEdge::East:
            Pos = Base + FVector (CellSize - WallInset, (CellSize * 0.5f) - LocalOffsetAlongWall, 0.f);
            Rot = FRotator (0.f, 0.f, 0.f);
            break;

        case EGridEdge::West:
            Pos = Base + FVector (WallInset, (CellSize * 0.5f) + LocalOffsetAlongWall, 0.f);
            Rot = FRotator (0.f, 180.f, 0.f);
            break;
    }
    OutTransform = FTransform (Rot, Pos, FVector::OneVector);
    return true;
}

bool AGridLevelRuntimeActor::GetCenteredObjectTransform (const FGridLevelObjectData& ObjectData, float ZOffset, FTransform& OutTransform) const
{
    if (!LevelAsset)
    {
        return false;
    }
    const FVector Pos = GetActorLocation () +
        CellToWorld (ObjectData.CellX, ObjectData.CellY, ZOffset) +
        FVector (LevelAsset->CellSize * 0.5f, LevelAsset->CellSize * 0.5f, 0.f);

    FRotator Rotation = FRotator::ZeroRotator;
    Rotation.Yaw = ObjectData.LocalYaw;

    OutTransform = FTransform (Rotation, Pos, FVector::OneVector);
    return true;
}

bool AGridLevelRuntimeActor::GetObjectPlacementTransform (const FGridLevelObjectData& ObjectData, FTransform& OutTransform) const
{
    if (!LevelAsset)
    {
        return false;
    }
    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    if (!Archetype)
    {
        if (ObjectData.Type == EGridLevelObjectType::Item)
        {
            if (ObjectData.Edge != EGridEdge::None)
            {
                return GetFloorEdgeObjectTransform (ObjectData, 12.f, 18.f, OutTransform);
            }
            return GetCenteredObjectTransform (ObjectData, 12.f, OutTransform);
        }
        return false;
    }
    if (ObjectData.Type == EGridLevelObjectType::Door)
    {
        FVector Pos = FVector::ZeroVector;
        FRotator Rot = FRotator::ZeroRotator;

        GetEdgeTransform (ObjectData.CellX, ObjectData.CellY, ObjectData.Edge, LevelAsset->CellSize, Pos, Rot);

        OutTransform = FTransform (Rot, Pos, FVector::OneVector);
        return true;
    }
    if (ObjectData.Type == EGridLevelObjectType::Item && ObjectData.Edge != EGridEdge::None)
    {
        const float EdgeInset = FMath::Max (Archetype->WallInset, 18.f);
        return GetFloorEdgeObjectTransform (ObjectData, Archetype->PlacementZOffset, EdgeInset, OutTransform);
    }
    if (Archetype->IsEdgePlaced ())
    {
        return GetWallMountedObjectTransform (ObjectData, Archetype->PlacementZOffset, Archetype->WallInset,
            Archetype->LocalOffsetAlongWall, Archetype->LocalOffsetVertical, OutTransform);
    }
    if (Archetype->IsCenterPlaced ())
    {
        return GetCenteredObjectTransform (ObjectData, Archetype->PlacementZOffset, OutTransform);
    }
    return false;
}

void AGridLevelRuntimeActor::RegisterRuntimeObjectActor (const FGuid& ObjectId, AGridRuntimeObjectActor* Actor)
{
    if (!ObjectId.IsValid () || !IsValid (Actor))
    {
        return;
    }
    SpawnedRuntimeObjectActors.Add (ObjectId, Actor);
}

void AGridLevelRuntimeActor::ClearRuntimeObjectActors ()
{
    for (AGridItemActor* ItemActor : SpawnedItemActors)
    {
        if (IsValid (ItemActor))
        {
            ItemActor->OnRemovedFromWorld ();
            ItemActor->Destroy ();
        }
    }
    SpawnedItemActors.Reset ();
    SpawnedItemEntries.Reset ();

    for (TPair<FGuid, TObjectPtr<AGridRuntimeObjectActor>>& Pair : SpawnedRuntimeObjectActors)
    {
        if (IsValid (Pair.Value))
        {
            if (AGridReceptacleActor* ReceptacleActor = Cast<AGridReceptacleActor> (Pair.Value.Get ()))
            {
                ReceptacleActor->ForceClearRuntimeContents (false);
            }
            Pair.Value->Destroy ();
        }
    }
    SpawnedRuntimeObjectActors.Empty ();
}

bool AGridLevelRuntimeActor::TryPickupItemAtCell (int32 CellX, int32 CellY, AGrimrockPartyPawn* PartyPawn)
{
    if (!PartyPawn)
    {
        return false;
    }

    const FIntPoint TargetCell (CellX, CellY);
    for (int32 EntryIndex = 0; EntryIndex < SpawnedItemEntries.Num (); ++EntryIndex)
    {
        FGridSpawnedItemRuntimeEntry& Entry = SpawnedItemEntries[EntryIndex];
        if (Entry.Cell != TargetCell)
        {
            continue;
        }

        AGridItemActor* ItemActor = Entry.ItemActor.Get ();
        if (!IsValid (ItemActor))
        {
            SpawnedItemEntries.RemoveAtSwap (EntryIndex);
            SpawnedItemActors.RemoveAllSwap ([] (const TObjectPtr<AGridItemActor>& SpawnedItemActor)
            {
                return !IsValid (SpawnedItemActor.Get ());
            });
            return false;
        }

        const FName ItemDefinitionId = ResolvePickupItemDefinitionId (ItemActor, Entry.ItemDefinitionId.IsNone () ? Entry.ItemArchetypeId : Entry.ItemDefinitionId);
        if (ItemDefinitionId.IsNone ())
        {
            UE_LOG (LogTemp, Warning, TEXT ("Item pickup failed at cell %d,%d: missing item definition id."), CellX, CellY);
            return false;
        }

        FGridItemInstance ItemInstance;
        ItemInstance.RuntimeObjectId = ItemActor->GetRuntimeObjectId ();
        if (!ItemInstance.RuntimeObjectId.IsValid ())
        {
            ItemInstance.RuntimeObjectId = FGuid::NewGuid ();
        }
        ItemInstance.ItemDefinitionId = ItemDefinitionId;
        ItemInstance.Quantity = 1;
        ItemInstance.Weight = 0.0f;
        ItemInstance.bLightsEnabled = ItemActor->AreItemLightsEnabled ();
        ItemInstance.LastWorldTransform = ItemActor->GetActorTransform ();

        if (!PartyPawn->AddItemInstanceToSelectedCharacterInventory (ItemInstance))
        {
            UE_LOG (LogTemp, Warning, TEXT ("GridInventory Pickup Failed InventoryFull Item=%s RuntimeId=%s"),
                *ItemDefinitionId.ToString (),
                *ItemInstance.RuntimeObjectId.ToString ());
            return false;
        }

        if (IsValid (ItemActor))
        {
            ItemActor->OnRemovedFromWorld ();
            ItemActor->Destroy ();
        }
        SpawnedItemActors.RemoveAllSwap ([ItemActor] (const TObjectPtr<AGridItemActor>& SpawnedItemActor)
        {
            return SpawnedItemActor.Get () == ItemActor;
        });

        SpawnedItemEntries.RemoveAtSwap (EntryIndex);

        UE_LOG (LogTemp, Log, TEXT ("Picked up item %s from cell %d,%d."), *ItemDefinitionId.ToString (), CellX, CellY);
        return true;
    }

    return false;
}

bool AGridLevelRuntimeActor::TryPickupItemActor (AGridItemActor* ItemActor, AGrimrockPartyPawn* PartyPawn)
{
    if (!IsValid (ItemActor) || !PartyPawn)
    {
        return false;
    }

    for (int32 EntryIndex = 0; EntryIndex < SpawnedItemEntries.Num (); ++EntryIndex)
    {
        FGridSpawnedItemRuntimeEntry& Entry = SpawnedItemEntries[EntryIndex];
        if (Entry.ItemActor.Get () != ItemActor)
        {
            continue;
        }

        const FName ItemDefinitionId = ResolvePickupItemDefinitionId (ItemActor, Entry.ItemDefinitionId.IsNone () ? Entry.ItemArchetypeId : Entry.ItemDefinitionId);
        if (ItemDefinitionId.IsNone ())
        {
            UE_LOG (LogTemp, Warning, TEXT ("Item pickup failed for actor %s: missing item definition id."), *ItemActor->GetName ());
            return false;
        }

        FGridItemInstance ItemInstance;
        ItemInstance.RuntimeObjectId = ItemActor->GetRuntimeObjectId ();
        if (!ItemInstance.RuntimeObjectId.IsValid ())
        {
            ItemInstance.RuntimeObjectId = FGuid::NewGuid ();
        }
        ItemInstance.ItemDefinitionId = ItemDefinitionId;
        ItemInstance.Quantity = 1;
        ItemInstance.Weight = 0.0f;
        ItemInstance.bLightsEnabled = ItemActor->AreItemLightsEnabled ();
        ItemInstance.LastWorldTransform = ItemActor->GetActorTransform ();

        if (!PartyPawn->AddItemInstanceToSelectedCharacterInventory (ItemInstance))
        {
            UE_LOG (LogTemp, Warning, TEXT ("GridInventory Pickup Failed InventoryFull Item=%s RuntimeId=%s"),
                *ItemDefinitionId.ToString (),
                *ItemInstance.RuntimeObjectId.ToString ());
            return false;
        }

        ItemActor->OnRemovedFromWorld ();
        ItemActor->Destroy ();

        SpawnedItemActors.RemoveAllSwap ([ItemActor] (const TObjectPtr<AGridItemActor>& SpawnedItemActor)
        {
            return SpawnedItemActor.Get () == ItemActor;
        });

        const FIntPoint PickedCell = Entry.Cell;
        SpawnedItemEntries.RemoveAtSwap (EntryIndex);

        UE_LOG (LogTemp, Log, TEXT ("Picked up item %s from clicked actor at cell %d,%d."), *ItemDefinitionId.ToString (), PickedCell.X, PickedCell.Y);
        return true;
    }

    return false;
}

TSubclassOf<AGridRuntimeObjectActor> AGridLevelRuntimeActor::GetObjectRuntimeActorClass (const FGridLevelObjectData& ObjectData) const
{
    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    return Archetype ? Archetype->RuntimeActorClass : nullptr;
}

bool AGridLevelRuntimeActor::IsRuntimeSpawnableObject (const FGridLevelObjectData& ObjectData) const
{
    if (!LevelAsset)
    {
        return false;
    }

    if (!ObjectData.bInitiallyEnabled)
    {
        return false;
    }

    if (!LevelAsset->IsValidCoord (ObjectData.CellX, ObjectData.CellY))
    {
        return false;
    }

    if (ObjectData.Type == EGridLevelObjectType::Item)
    {
        const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
        return ObjectData.ItemDefinitionAsset ||
            !ObjectData.ItemDefinitionId.IsNone () ||
            (Archetype && (Archetype->DefaultBehavior.Item.ItemDefinitionAsset ||
                !Archetype->DefaultBehavior.Item.ItemDefinitionId.IsNone ())) ||
            !ObjectData.ArchetypeId.IsNone ();
    }

    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    if (Archetype && Archetype->RuntimeActorClass)
    {
        return !Archetype->IsEdgePlaced () || ObjectData.Edge != EGridEdge::None;
    }

    switch (ObjectData.Type)
    {
        case EGridLevelObjectType::Door:
        case EGridLevelObjectType::Button:
        case EGridLevelObjectType::Lever:
        case EGridLevelObjectType::Receptacle:
        return ObjectData.Edge != EGridEdge::None;

        case EGridLevelObjectType::PressurePlate:
        return true;

        case EGridLevelObjectType::Trigger:
        return true;

        default:
        return false;
    }
}

void AGridLevelRuntimeActor::AddPlacedItemActor (const FGridLevelObjectData& ObjectData)
{
    FTransform Transform;
    if (!GetObjectPlacementTransform (ObjectData, Transform))
    {
        UE_LOG (LogTemp, Warning, TEXT ("Placed item skipped: could not compute placement transform for object %s."), *ObjectData.ObjectId.ToString ());
        return;
    }
    if (!IsSafeRuntimeRenderTransform (Transform))
    {
        LogUnsafeObjectTransform (TEXT ("AddPlacedItemActor"), ObjectData, GetObjectMesh (ObjectData), Transform);
        return;
    }

    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    UGridItemDefinitionAsset* ItemDefinition = ResolveObjectItemDefinitionAsset (ObjectData, Archetype);
    const FName ItemDefinitionId = ResolveObjectItemDefinitionId (ObjectData, Archetype);
    AGridItemActor* ItemActor = SpawnItemActorForDefinition (ItemDefinition, ItemDefinitionId, this, nullptr);
    if (!ItemActor)
    {
        UE_LOG (LogTemp, Warning, TEXT ("Placed item skipped: failed to spawn item definition %s."), *ItemDefinitionId.ToString ());
        return;
    }

    ItemActor->SetActorTransform (Transform);
    ItemActor->SetRuntimeObjectId (ObjectData.ObjectId);
    if (ItemDefinition)
    {
        ItemActor->InitializeFromItemDefinition (ItemDefinition, ObjectData.ObjectId);
    }
    else if (!ItemDefinitionId.IsNone ())
    {
        ItemActor->InitializeFromItemDefinitionId (ItemDefinitionId, ObjectData.ObjectId);
    }
    ItemActor->SetRuntimeCell (ObjectData.CellX, ObjectData.CellY);
    ItemActor->ConfigureAsWorldPickup ();
    ItemActor->OnRemovedFromWorld ();
    SpawnedItemActors.Add (ItemActor);

    FGridSpawnedItemRuntimeEntry Entry;
    Entry.Cell = FIntPoint (ObjectData.CellX, ObjectData.CellY);
    Entry.ItemActor = ItemActor;
    Entry.ObjectId = ObjectData.ObjectId;
    Entry.ItemArchetypeId = ObjectData.ArchetypeId;
    Entry.ItemDefinitionAsset = ItemDefinition;
    Entry.ItemDefinitionId = ItemDefinitionId;
    SpawnedItemEntries.Add (Entry);

    UE_LOG (LogTemp, Log, TEXT ("Placed item spawned: %s at object %s."),
        *ItemDefinitionId.ToString (),
        *ObjectData.ObjectId.ToString (),
}

void AGridLevelRuntimeActor::AddRuntimeObjectActor (const FGridLevelObjectData& ObjectData)
{
    UStaticMesh* Mesh = nullptr;
    UMaterialInterface* Material = nullptr;
    FTransform Transform;
    AGridRuntimeObjectActor* Actor = SpawnRuntimeObjectActor<AGridRuntimeObjectActor> (ObjectData, Mesh, Material, Transform);
    UE_LOG (LogTemp, Verbose, TEXT ("Runtime object: Type=%d Archetype=%s Tag=%s Id=%s"),
        static_cast<int32>(ObjectData.Type),
        *ObjectData.ArchetypeId.ToString (),
        *ObjectData.Tag.ToString (),
        *ObjectData.ObjectId.ToString ());
    if (!Actor) return;
    FGridLevelObjectData RuntimeObjectData = ObjectData;
    const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
    if (AGridReceptacleActor* ReceptacleActor = Cast<AGridReceptacleActor> (Actor))
    {
        ReceptacleActor->ContainedItemActorClass = Archetype ? Archetype->ItemActorClass : nullptr;
    }
    if (AGridMechanismActor* MechanismActor = Cast<AGridMechanismActor> (Actor))
    {
        MechanismActor->InitializeMechanismVisuals (RuntimeObjectData, Archetype, Transform);
        Actor->InitializeGridObject (RuntimeObjectData, Mesh, Material, Transform);
    } else if (AGridGenericObjectActor* GenericActor = Cast<AGridGenericObjectActor> (Actor))
    {
        GenericActor->InitializeGenericObject (RuntimeObjectData, Archetype, Mesh, Material, Transform);
    } else
    {
        Actor->InitializeGridObject (RuntimeObjectData, Mesh, Material, Transform);
    }
    if (ActivationComponent)
    {
        ActivationComponent->RegisterInitialObjectState (RuntimeObjectData);
    }

    if (ObjectData.Type == EGridLevelObjectType::Door && DoorSystemComponent)
    {
        DoorSystemComponent->RegisterDoorObject (ObjectData, Actor);
    }
}

void AGridLevelRuntimeActor::RebuildRuntimeObjects ()
{
    if (!LevelAsset)
    {
        return;
    }
    LevelAsset->EnsureCellCount ();
    for (const FGridLevelObjectData& ObjectData : LevelAsset->Objects)
    {
        if (ObjectData.Type == EGridLevelObjectType::Item)
        {
            if (ObjectData.bInitiallyEnabled)
            {
                AddPlacedItemActor (ObjectData);
            }
            continue;
        }
        if (!IsRuntimeSpawnableObject (ObjectData))
        {
            if (ObjectData.bInitiallyEnabled)
            {
                const UGridObjectArchetypeAsset* Archetype = FindObjectArchetype (ObjectData.ArchetypeId);
                if (Archetype && !Archetype->RuntimeActorClass)
                {
                    UE_LOG (
                        LogTemp,
                        Warning,
                        TEXT ("Runtime object skipped: archetype %s has no RuntimeActorClass."),
                        *ObjectData.ArchetypeId.ToString ());
                }
            }
            continue;
        }
        AddRuntimeObjectActor (ObjectData);
    }
}

FString AGridLevelRuntimeActor::GetRuntimeDebugSummary () const
{
    FString Result;

    Result += FString::Printf (TEXT ("Grid Runtime | Level=%s\n"), LevelAsset ? *LevelAsset->GetName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("Runtime Actors=%d\n"), SpawnedRuntimeObjectActors.Num ());
    Result += FString::Printf (TEXT ("Spawned Items=%d\n"), SpawnedItemActors.Num ());
    Result += ActivationComponent ? ActivationComponent->GetDebugSummary () : TEXT ("Activation | Missing");
    Result += TEXT ("\n");
    Result += DoorSystemComponent ? DoorSystemComponent->GetDebugSummary () : TEXT ("Doors | Missing");
    return Result;
}

void AGridLevelRuntimeActor::LogRuntimeDebugSummary () const
{
    const FString Summary = GetRuntimeDebugSummary ();
    UE_LOG (LogTemp, Log, TEXT ("%s"), *Summary);
    if (ActivationComponent)
    {
        ActivationComponent->LogDebugSummary ();
    }
    if (DoorSystemComponent)
    {
        DoorSystemComponent->LogDebugSummary ();
    }
}

FString AGridLevelRuntimeActor::GetLevelAssetDiagnostics () const
{
    const UWorld* World = GetWorld ();
    FString WorldType = TEXT ("None");
    FString MapName = TEXT ("None");

    if (World)
    {
        MapName = World->GetMapName ();
        switch (World->WorldType)
        {
            case EWorldType::Game:
                WorldType = TEXT ("Game");
                break;

            case EWorldType::PIE:
                WorldType = TEXT ("PIE");
                break;

            case EWorldType::Editor:
                WorldType = TEXT ("Editor");
                break;

            case EWorldType::EditorPreview:
                WorldType = TEXT ("EditorPreview");
                break;

            case EWorldType::GamePreview:
                WorldType = TEXT ("GamePreview");
                break;

            case EWorldType::Inactive:
                WorldType = TEXT ("Inactive");
                break;

            default:
                WorldType = FString::Printf (TEXT ("Unknown(%d)"), static_cast<int32> (World->WorldType));
                break;
        }
    }

    FString Result;
    Result += TEXT ("Grid LevelAsset Diagnostics\n");
    Result += FString::Printf (TEXT ("RuntimeActor=%s\n"), *GetPathName ());
#if WITH_EDITOR
    Result += FString::Printf (TEXT ("ActorLabel=%s\n"), *GetActorLabel ());
#else
    Result += FString::Printf (TEXT ("ActorName=%s\n"), *GetName ());
#endif
    Result += FString::Printf (TEXT ("World=%s\n"), *GetNameSafe (World));
    Result += FString::Printf (TEXT ("WorldType=%s\n"), *WorldType);
    Result += FString::Printf (TEXT ("Map=%s\n"), *MapName);
    Result += FString::Printf (TEXT ("DungeonAsset=%s\n"), DungeonAsset ? *DungeonAsset->GetPathName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("CurrentDungeonLevelId=%s\n"), *CurrentDungeonLevelId.ToString ());
    Result += FString::Printf (TEXT ("LevelAsset=%s\n"), LevelAsset ? *LevelAsset->GetPathName () : TEXT ("None"));

    const FGridLevelRuntimeState* RuntimeState = FindRuntimeStateForCurrentLevel ();
    Result += FString::Printf (TEXT ("HasRuntimeState=%s\n"), RuntimeState && RuntimeState->bHasBeenVisited ? TEXT ("true") : TEXT ("false"));
    Result += FString::Printf (TEXT ("RuntimeRemovedObjects=%d\n"), CountRemovedRuntimeObjects (RuntimeState));
    Result += FString::Printf (TEXT ("RuntimeDoors=%d\n"), RuntimeState ? RuntimeState->Doors.Num () : 0);
    Result += FString::Printf (TEXT ("RuntimeItems=%d\n"), RuntimeState ? RuntimeState->Items.Num () : 0);
    Result += FString::Printf (TEXT ("RuntimeReceptacles=%d\n"), RuntimeState ? RuntimeState->Receptacles.Num () : 0);

    if (!LevelAsset)
    {
        Result += TEXT ("Status=ERROR: missing LevelAsset reference.\n");
        return Result;
    }

    const int32 ExpectedCellCount = LevelAsset->Width * LevelAsset->Height;
    int32 NonEmptyCellCount = 0;
    int32 BlockingCellCount = 0;
    int32 CeilingCellCount = 0;
    const int32 TransitionObjectCount = CountRuntimeTransitionObjects (LevelAsset);
    const int32 HiddenFloorCellCount = CountHiddenFloorCells (LevelAsset, this);

    for (const FGridLevelCellData& Cell : LevelAsset->Cells)
    {
        if (Cell.CellType != EGridCellType::Empty)
        {
            ++NonEmptyCellCount;
        }
        if (Cell.bBlocksOccupancy)
        {
            ++BlockingCellCount;
        }
        if (Cell.bHasCeiling)
        {
            ++CeilingCellCount;
        }
    }

    Result += FString::Printf (TEXT ("AssetPackage=%s\n"), *GetNameSafe (LevelAsset->GetOutermost ()));
    Result += FString::Printf (TEXT ("GridSize=%dx%d\n"), LevelAsset->Width, LevelAsset->Height);
    Result += FString::Printf (TEXT ("CellSize=%.2f\n"), LevelAsset->CellSize);
    Result += FString::Printf (TEXT ("StartCell=(%d,%d) StartFacing=%s StartCellValid=%s\n"),
        LevelAsset->StartCellX,
        LevelAsset->StartCellY,
        StaticEnum<EGridEdge> () ? *StaticEnum<EGridEdge> ()->GetNameStringByValue (static_cast<int64> (LevelAsset->StartFacing)) : TEXT ("Unknown"),
        LevelAsset->IsStartCellValid () ? TEXT ("true") : TEXT ("false"));
    Result += FString::Printf (TEXT ("Cells=%d ExpectedCells=%d\n"), LevelAsset->Cells.Num (), ExpectedCellCount);
    Result += FString::Printf (TEXT ("NonEmptyCells=%d BlockingCells=%d CeilingCells=%d\n"),
        NonEmptyCellCount,
        BlockingCellCount,
        CeilingCellCount);
    Result += FString::Printf (
        TEXT ("Objects=%d Links=%d TransitionObjects=%d HiddenFloorCells=%d\n"),
        LevelAsset->Objects.Num (),
        LevelAsset->Links.Num (),
        TransitionObjectCount,
        HiddenFloorCellCount);
    Result += FString::Printf (TEXT ("ObjectArchetypesOnRuntimeActor=%d\n"), ObjectArchetypes.Num ());
    Result += FString::Printf (TEXT ("FloorMesh=%s WallMesh=%s CeilingMesh=%s\n"),
        *GetNameSafe (FloorMesh),
        *GetNameSafe (WallMesh),
        *GetNameSafe (CeilingMesh));

    if (LevelAsset->Cells.Num () != ExpectedCellCount)
    {
        Result += TEXT ("Status=WARNING: Cells.Num does not match Width*Height. Run EnsureLevelReady from the editor actor.\n");
    }
    else
    {
        Result += TEXT ("Status=OK\n");
    }

    return Result;
}

void AGridLevelRuntimeActor::LogLevelAssetDiagnostics () const
{
    const FString Diagnostics = GetLevelAssetDiagnostics ();
    UE_LOG (LogTemp, Log, TEXT ("%s"), *Diagnostics);
}

FString AGridLevelRuntimeActor::GetPIEReadinessDiagnostics () const
{
    const UWorld* World = GetWorld ();
    const bool bHasGameWorld = World && World->IsGameWorld ();
    const bool bHasRequiredMeshes = FloorMesh && WallMesh && CeilingMesh;
    const bool bHasValidStart = LevelAsset && LevelAsset->IsStartCellValid ();

    int32 NullArchetypeCount = 0;
    for (const TObjectPtr<UGridObjectArchetypeAsset>& Archetype : ObjectArchetypes)
    {
        if (!Archetype)
        {
            ++NullArchetypeCount;
        }
    }

    AGrimrockPartyPawn* FoundPartyPawn = nullptr;
    AGameModeBase* ActiveGameMode = nullptr;
    if (World)
    {
        ActiveGameMode = World->GetAuthGameMode ();
        for (TActorIterator<AGrimrockPartyPawn> It (World); It; ++It)
        {
            FoundPartyPawn = *It;
            break;
        }
    }

    FString Result;
    Result += TEXT ("GridLevelRuntimeActor PIE Readiness\n");
    Result += FString::Printf (TEXT ("RuntimeActor: %s\n"), *GetName ());
    Result += FString::Printf (TEXT ("World: %s\n"), World ? *World->GetMapName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("WorldType: %s\n"), *GetRuntimeWorldTypeText (World));
    Result += FString::Printf (TEXT ("IsGameWorld: %s\n"), *GetRuntimeBoolText (bHasGameWorld));
    Result += FString::Printf (TEXT ("DungeonAsset: %s\n"), DungeonAsset ? *DungeonAsset->GetPathName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("CurrentDungeonLevelId: %s\n"), *CurrentDungeonLevelId.ToString ());
    Result += FString::Printf (TEXT ("LevelAsset: %s\n"), LevelAsset ? *LevelAsset->GetPathName () : TEXT ("None"));
    Result += FString::Printf (TEXT ("ApplyLevelStartOnBeginPlay: %s\n"), *GetRuntimeBoolText (bApplyLevelStartOnBeginPlay));

    if (LevelAsset)
    {
        Result += FString::Printf (
            TEXT ("Start: Cell=(%d,%d) Facing=%s Valid=%s\n"),
            LevelAsset->StartCellX,
            LevelAsset->StartCellY,
            *GetRuntimeEdgeText (LevelAsset->StartFacing),
            *GetRuntimeBoolText (bHasValidStart));
        Result += FString::Printf (
            TEXT ("Asset Stats: Cells=%d Objects=%d Links=%d TransitionObjects=%d\n"),
            LevelAsset->Cells.Num (),
            LevelAsset->Objects.Num (),
            LevelAsset->Links.Num (),
            CountRuntimeTransitionObjects (LevelAsset));
    }
    else
    {
        Result += TEXT ("Start: Cell=None Facing=None Valid=false\n");
        Result += TEXT ("Asset Stats: Cells=0 Objects=0 Links=0 TransitionObjects=0\n");
    }

    Result += FString::Printf (
        TEXT ("Meshes: Floor=%s Wall=%s Ceiling=%s\n"),
        *GetNameSafe (FloorMesh),
        *GetNameSafe (WallMesh),
        *GetNameSafe (CeilingMesh));
    Result += FString::Printf (
        TEXT ("ObjectArchetypes: Count=%d NullEntries=%d\n"),
        ObjectArchetypes.Num (),
        NullArchetypeCount);
    Result += FString::Printf (
        TEXT ("Components: Activation=%s Doors=%s EditorPreview=%s\n"),
        *GetRuntimeBoolText (ActivationComponent != nullptr),
        *GetRuntimeBoolText (DoorSystemComponent != nullptr),
        *GetRuntimeBoolText (EditorPreviewComponent != nullptr));
    Result += FString::Printf (
        TEXT ("EditorPreviewInGameWorld: %s\n"),
        bHasGameWorld
            ? TEXT ("Runtime objects are used; editor preview rebuild is skipped.")
            : TEXT ("Editor preview may be used outside PIE/game world."));
    Result += FString::Printf (
        TEXT ("PartyPawnInCurrentWorld: %s\n"),
        FoundPartyPawn ? *FoundPartyPawn->GetName () : TEXT ("None"));
    Result += FString::Printf (
        TEXT ("ActiveGameMode: %s\n"),
        ActiveGameMode ? *ActiveGameMode->GetClass ()->GetName () : TEXT ("None"));
    Result += FString::Printf (
        TEXT ("GameModeDefaultPawnClass: %s\n"),
        ActiveGameMode && ActiveGameMode->DefaultPawnClass ? *ActiveGameMode->DefaultPawnClass->GetName () : TEXT ("None"));
    Result += FString::Printf (
        TEXT ("GameModePlayerControllerClass: %s\n"),
        ActiveGameMode && ActiveGameMode->PlayerControllerClass ? *ActiveGameMode->PlayerControllerClass->GetName () : TEXT ("None"));

    if (ActiveGameMode && ActiveGameMode->GetClass () == AGrimrockGameMode::StaticClass ())
    {
        Result += TEXT ("GameModeNote: Native AGrimrockGameMode spawns the native C++ pawn unless a Blueprint override is used.\n");
    }
    else if (ActiveGameMode && ActiveGameMode->GetClass ()->IsChildOf (AGrimrockGameMode::StaticClass ()))
    {
        Result += TEXT ("GameModeNote: Blueprint GameMode is active; its Default Pawn Class and Player Controller Class overrides are used.\n");
    }
    else if (ActiveGameMode)
    {
        Result += TEXT ("GameModeNote: Active GameMode is not derived from AGrimrockGameMode.\n");
    }
    else
    {
        Result += TEXT ("GameModeNote: No active GameMode in this world yet. Run PIE to inspect the spawned GameMode.\n");
    }

    if (!LevelAsset)
    {
        Result += TEXT ("Status: ERROR - LevelAsset is null.");
    }
    else if (!bHasValidStart)
    {
        Result += TEXT ("Status: ERROR - LevelAsset start cell is invalid.");
    }
    else if (!bHasRequiredMeshes)
    {
        Result += TEXT ("Status: ERROR - FloorMesh, WallMesh or CeilingMesh is missing.");
    }
    else if (!ActivationComponent || !DoorSystemComponent)
    {
        Result += TEXT ("Status: ERROR - Runtime activation or door component is missing.");
    }
    else if (!FoundPartyPawn)
    {
        Result += TEXT ("Status: WARNING - No AGrimrockPartyPawn exists in the current world. PIE can still work if GameMode spawns one.");
    }
    else
    {
        Result += TEXT ("Status: OK - Runtime actor is ready for PIE with this LevelAsset.");
    }

    return Result;
}

void AGridLevelRuntimeActor::LogPIEReadinessDiagnostics () const
{
    UE_LOG (LogTemp, Log, TEXT ("%s"), *GetPIEReadinessDiagnostics ());
}

void AGridLevelRuntimeActor::ShowRuntimeDebugSummary (float Duration) const
{
    if (!GEngine)
    {
        return;
    }
    GEngine->AddOnScreenDebugMessage (-1, Duration, FColor::Green, GetRuntimeDebugSummary ()
    );
}
