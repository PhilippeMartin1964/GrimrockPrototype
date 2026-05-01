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

void UGridDoorSystemComponent::RegisterDoorObject (
    const FGridLevelObjectData& ObjectData,
    AGridRuntimeObjectActor* RuntimeObjectActor)
{
    if (ObjectData.Type != EGridLevelObjectType::Door)
    {
        return;
    }

    if (AGridDoorActor* DoorActor = Cast<AGridDoorActor> (RuntimeObjectActor))
    {
        DoorActor->OnDoorAnimationFinished.AddDynamic (
            this,
            &UGridDoorSystemComponent::HandleDoorAnimationFinished
        );
    }

    SetDoorPassageBlocked (
        ObjectData.CellX,
        ObjectData.CellY,
        ObjectData.Edge,
        !ObjectData.bInitiallyActive
    );
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

const FGridLevelObjectData* UGridDoorSystemComponent::FindDoorObjectDataAtEdge (
    int32 X,
    int32 Y,
    EGridEdge Edge) const
{
    if (!RuntimeActor || !RuntimeActor->LevelAsset)
    {
        return nullptr;
    }

    for (const FGridLevelObjectData& ObjectData : RuntimeActor->LevelAsset->Objects)
    {
        if (ObjectData.Type == EGridLevelObjectType::Door &&
            ObjectData.CellX == X &&
            ObjectData.CellY == Y &&
            ObjectData.Edge == Edge)
        {
            return &ObjectData;
        }
    }

    return nullptr;
}

AGridDoorActor* UGridDoorSystemComponent::FindDoorActorAtEdge (
    int32 X,
    int32 Y,
    EGridEdge Edge) const
{
    const FGridLevelObjectData* DoorData = FindDoorObjectDataAtEdge (X, Y, Edge);
    if (!DoorData || !RuntimeActor)
    {
        return nullptr;
    }

    return RuntimeActor->FindRuntimeObjectActor<AGridDoorActor> (DoorData->ObjectId);
}