#include "Runtime/GridDoorSystemComponent.h"

#include "Core/GridLevelAsset.h"
#include "Runtime/GridDoorActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridRuntimeObjectActor.h"

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
    const FGridDoorEdgeKey Key (ObjectData.CellX, ObjectData.CellY, ObjectData.Edge);
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

    return !RuntimeBlockedDoorEdges.Contains (FGridDoorEdgeKey (X, Y, Edge));
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

    SetDoorPassageBlocked (X, Y, Edge, false);
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
    return RuntimeBlockedDoorEdges.Contains (FGridDoorEdgeKey (X, Y, Edge));
}

void UGridDoorSystemComponent::SetDoorPassageBlocked (int32 X, int32 Y, EGridEdge Edge, bool bBlocked)
{
    const FGridDoorEdgeKey Key (X, Y, Edge);

    if (bBlocked)
    {
        RuntimeBlockedDoorEdges.Add (Key);
    } else
    {
        RuntimeBlockedDoorEdges.Remove (Key);
    }
}

void UGridDoorSystemComponent::HandleDoorAnimationFinished (int32 X, int32 Y, EGridEdge Edge)
{
    // Pour l’instant rien à faire ici si le blocage est déjà mis à jour
    // au début de OpenDoorOnEdge / CloseDoorOnEdge.
    //
    // Cette méthode reste utile si vous voulez plus tard bloquer/débloquer
    // seulement à la fin de l’animation.
}

AGridDoorActor* UGridDoorSystemComponent::FindDoorActorAtEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    if (const TWeakObjectPtr<AGridDoorActor>* DoorActorPtr = DoorActorByEdge.Find (FGridDoorEdgeKey (X, Y, Edge)))
    {
        return DoorActorPtr->Get ();
    }
    return nullptr;
}

const FGridLevelObjectData* UGridDoorSystemComponent::FindDoorObjectDataAtEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    const int32* DoorIndex = DoorIndexByEdge.Find (FGridDoorEdgeKey (X, Y, Edge));
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
        DoorIndexByEdge.Add (FGridDoorEdgeKey (ObjectData.CellX, ObjectData.CellY, ObjectData.Edge), Index);
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