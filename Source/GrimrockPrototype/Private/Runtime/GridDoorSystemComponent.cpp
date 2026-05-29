#include "Runtime/GridDoorSystemComponent.h"

#include "Core/GridLevelAsset.h"
#include "Runtime/GridDoorActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridRuntimeObjectActor.h"

static EGridEdge GetOppositeEdge (EGridEdge Edge)
{
    switch (Edge)
    {
        case EGridEdge::North: return EGridEdge::South;
        case EGridEdge::South: return EGridEdge::North;
        case EGridEdge::East:  return EGridEdge::West;
        case EGridEdge::West:  return EGridEdge::East;
        default:               return EGridEdge::None;
    }
}

UGridDoorSystemComponent::UGridDoorSystemComponent ()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UGridDoorSystemComponent::Initialize (AGridLevelRuntimeActor* InRuntimeActor)
{
    RuntimeActor = InRuntimeActor;
}

void UGridDoorSystemComponent::ResetRuntimeState ()
{
    RuntimeBlockedDoorEdges.Reset ();
}

void UGridDoorSystemComponent::RegisterDoorObject (const FGridLevelObjectData& ObjectData, AGridRuntimeObjectActor* RuntimeObjectActor)
{
    if (ObjectData.Type != EGridLevelObjectType::Door)
    {
        return;
    }
    const FGridEdgeKey Key (ObjectData.CellX, ObjectData.CellY, ObjectData.Edge);
    if (AGridDoorActor* DoorActor = Cast<AGridDoorActor> (RuntimeObjectActor))
    {
        DoorActorByEdge.Add (Key, DoorActor);
        DoorActor->OnDoorAnimationFinished.AddDynamic (this, &UGridDoorSystemComponent::HandleDoorAnimationFinished);
    }
    SetDoorPassageBlocked (ObjectData.CellX, ObjectData.CellY, ObjectData.Edge, !ObjectData.bInitiallyActive);
}

bool UGridDoorSystemComponent::HasDoorOnEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    return FindDoorObjectDataAtEdge (X, Y, Edge) != nullptr;
}

bool UGridDoorSystemComponent::IsDoorOpenOnEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    if (!HasDoorOnEdge (X, Y, Edge))
    {
        return false;
    }

    return !RuntimeBlockedDoorEdges.Contains (FGridEdgeKey (X, Y, Edge));
}

bool UGridDoorSystemComponent::ToggleDoorOnEdge (int32 X, int32 Y, EGridEdge Edge)
{
    return IsDoorOpenOnEdge (X, Y, Edge)
        ? CloseDoorOnEdge (X, Y, Edge)
        : OpenDoorOnEdge (X, Y, Edge);
}

bool UGridDoorSystemComponent::OpenDoorOnEdge (int32 X, int32 Y, EGridEdge Edge)
{
    AGridDoorActor* DoorActor = FindDoorActorAtEdge (X, Y, Edge);
    if (!DoorActor)
    {
        return false;
    }
    DoorActor->OpenDoor ();

    return true;
}

bool UGridDoorSystemComponent::CloseDoorOnEdge (int32 X, int32 Y, EGridEdge Edge)
{
    AGridDoorActor* DoorActor = FindDoorActorAtEdge (X, Y, Edge);
    if (!DoorActor)
    {
        return false;
    }
    SetDoorPassageBlocked (X, Y, Edge, true);
    DoorActor->CloseDoor ();
    return true;
}

bool UGridDoorSystemComponent::IsDoorPassageBlocked (int32 X, int32 Y, EGridEdge Edge) const
{
    const FGridEdgeKey DirectKey (X, Y, Edge);
    if (RuntimeBlockedDoorEdges.Contains (DirectKey))
    {
        return true;
    }
    if (!RuntimeActor)
    {
        return false;
    }
    int32 NeighborX = INDEX_NONE;
    int32 NeighborY = INDEX_NONE;
    if (!RuntimeActor->TryGetNeighborCell (X, Y, Edge, NeighborX, NeighborY))
    {
        return false;
    }
    const FGridEdgeKey ReverseKey (NeighborX, NeighborY, GetOppositeEdge (Edge));
    return RuntimeBlockedDoorEdges.Contains (ReverseKey);
}

void UGridDoorSystemComponent::SetDoorPassageBlocked (int32 X, int32 Y, EGridEdge Edge, bool bBlocked)
{
    const FGridEdgeKey Key (X, Y, Edge);

    if (bBlocked)
    {
        RuntimeBlockedDoorEdges.Add (Key);
    } else
    {
        RuntimeBlockedDoorEdges.Remove (Key);
    }
}

bool UGridDoorSystemComponent::GetDoorState (FGuid ObjectId, bool& bOutOpen, bool& bOutMoving, bool& bOutBlocked) const
{
    bOutOpen = false;
    bOutMoving = false;
    bOutBlocked = true;

    if (!RuntimeActor || !RuntimeActor->LevelAsset || !ObjectId.IsValid ())
    {
        return false;
    }

    for (const FGridLevelObjectData& ObjectData : RuntimeActor->LevelAsset->Objects)
    {
        if (ObjectData.ObjectId != ObjectId || ObjectData.Type != EGridLevelObjectType::Door)
        {
            continue;
        }

        bOutBlocked = IsDoorPassageBlocked (ObjectData.CellX, ObjectData.CellY, ObjectData.Edge);
        bOutOpen = !bOutBlocked;
        if (const AGridDoorActor* DoorActor = FindDoorActorAtEdge (ObjectData.CellX, ObjectData.CellY, ObjectData.Edge))
        {
            bOutMoving = DoorActor->IsAnimating ();
            bOutOpen = DoorActor->IsFullyOpen () || (!bOutBlocked && !DoorActor->IsFullyClosed ());
        }
        return true;
    }

    return false;
}

bool UGridDoorSystemComponent::ApplyDoorState (FGuid ObjectId, bool bOpen, bool bBlocked)
{
    if (!RuntimeActor || !RuntimeActor->LevelAsset || !ObjectId.IsValid ())
    {
        return false;
    }

    for (const FGridLevelObjectData& ObjectData : RuntimeActor->LevelAsset->Objects)
    {
        if (ObjectData.ObjectId != ObjectId || ObjectData.Type != EGridLevelObjectType::Door)
        {
            continue;
        }

        if (AGridDoorActor* DoorActor = FindDoorActorAtEdge (ObjectData.CellX, ObjectData.CellY, ObjectData.Edge))
        {
            DoorActor->SnapDoorOpenState (bOpen);
        }
        SetDoorPassageBlocked (ObjectData.CellX, ObjectData.CellY, ObjectData.Edge, bBlocked);
        return true;
    }

    return false;
}

void UGridDoorSystemComponent::HandleDoorAnimationFinished (int32 X, int32 Y, EGridEdge Edge)
{
    AGridDoorActor* DoorActor = FindDoorActorAtEdge (X, Y, Edge);
    if (!DoorActor)
    {
        return;
    }
    SetDoorPassageBlocked (X, Y, Edge, !DoorActor->IsFullyOpen ());
}

AGridDoorActor* UGridDoorSystemComponent::FindDoorActorAtEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    if (const TWeakObjectPtr<AGridDoorActor>* DoorActorPtr = DoorActorByEdge.Find (FGridEdgeKey (X, Y, Edge)))
    {
        return DoorActorPtr->Get ();
    }
    return nullptr;
}

const FGridLevelObjectData* UGridDoorSystemComponent::FindDoorObjectDataAtEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    const int32* DoorIndex = DoorIndexByEdge.Find (FGridEdgeKey (X, Y, Edge));
    return DoorIndex ? GetDoorObjectByIndex (*DoorIndex) : nullptr;
}

void UGridDoorSystemComponent::RebuildIndexes ()
{
    DoorIndexByEdge.Reset ();
    DoorActorByEdge.Reset ();
    if (!RuntimeActor || !RuntimeActor->LevelAsset)
    {
        return;
    }
    const TArray<FGridLevelObjectData>& Objects = RuntimeActor->LevelAsset->Objects;
    for (int32 Index = 0; Index < Objects.Num (); ++Index)
    {
        const FGridLevelObjectData& ObjectData = Objects[Index];
        if (ObjectData.Type != EGridLevelObjectType::Door)
        {
            continue;
        }
        DoorIndexByEdge.Add (FGridEdgeKey (ObjectData.CellX, ObjectData.CellY, ObjectData.Edge), Index);
    }
}

const FGridLevelObjectData* UGridDoorSystemComponent::GetDoorObjectByIndex (int32 ObjectIndex) const
{
    if (!RuntimeActor || !RuntimeActor->LevelAsset)
    {
        return nullptr;
    }
    return RuntimeActor->LevelAsset->Objects.IsValidIndex (ObjectIndex) ? &RuntimeActor->LevelAsset->Objects[ObjectIndex] : nullptr;
}

FString UGridDoorSystemComponent::GetDebugSummary () const
{
    return FString::Printf (
        TEXT ("Doors | Indexed=%d Actors=%d Blocked=%d"),
        DoorIndexByEdge.Num (),
        DoorActorByEdge.Num (),
        RuntimeBlockedDoorEdges.Num ()
    );
}

void UGridDoorSystemComponent::LogDebugSummary () const
{
    UE_LOG (LogTemp, Log, TEXT ("%s"), *GetDebugSummary ());
}
