#include "Runtime/GridActivationComponent.h"

#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridButtonActor.h"
#include "Runtime/GridLeverActor.h"
#include "Runtime/GridPressurePlateActor.h"
#include "Core/GridLevelAsset.h"

UGridActivationComponent::UGridActivationComponent ()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UGridActivationComponent::Initialize (AGridLevelRuntimeActor* InRuntime)
{
    RuntimeActor = InRuntime;
}

void UGridActivationComponent::ResetRuntimeState ()
{
    ActiveObjectIds.Reset ();
}

bool UGridActivationComponent::TryInteractAtEdge (int32 FromCellX, int32 FromCellY, EGridEdge Edge)
{
    const FGridLevelObjectData* ObjectData = FindInteractableObjectOnEdge (FromCellX, FromCellY, Edge);
    return ObjectData ? ActivateObject (*ObjectData) : false;
}

void UGridActivationComponent::HandlePartyCellChanged (int32 OldCellX, int32 OldCellY, int32 NewCellX, int32 NewCellY)
{
    if (OldCellX == NewCellX && OldCellY == NewCellY)
    {
        ActivatePressurePlateAtCell (NewCellX, NewCellY);
        return;
    }

    DeactivatePressurePlateAtCell (OldCellX, OldCellY);
    ActivatePressurePlateAtCell (NewCellX, NewCellY);

    DeactivateTriggersAtCell (OldCellX, OldCellY);
    ActivateTriggersAtCell (NewCellX, NewCellY);
}

const FGridLevelObjectData* UGridActivationComponent::FindObjectById (FGuid ObjectId) const
{
    const int32* ObjectIndex = ObjectIndexById.Find (ObjectId);
    return ObjectIndex ? GetObjectByIndex (*ObjectIndex) : nullptr;
}

const FGridLevelObjectData* UGridActivationComponent::FindInteractableObjectOnEdge (int32 X, int32 Y, EGridEdge Edge) const
{
    const int32* ObjectIndex = InteractableObjectIndexByEdge.Find (FGridEdgeKey (X, Y, Edge));
    return ObjectIndex ? GetObjectByIndex (*ObjectIndex) : nullptr;
}

bool UGridActivationComponent::ActivateObject (const FGridLevelObjectData& ObjectData)
{
    if (!RuntimeActor)
    {
        return false;
    }

    switch (ObjectData.Type)
    {
        case EGridLevelObjectType::Button:
        {
            if (AGridButtonActor* ButtonActor = RuntimeActor->FindRuntimeObjectActor<AGridButtonActor> (ObjectData.ObjectId))
            {
                ButtonActor->TriggerPress ();
            }

            return ExecuteLinksFromObject (ObjectData.ObjectId, false);
        }

        case EGridLevelObjectType::Lever:
        {
            const bool bWasActive = ActiveObjectIds.Contains (ObjectData.ObjectId);
            const bool bNewActive = !bWasActive;

            if (bNewActive)
            {
                ActiveObjectIds.Add (ObjectData.ObjectId);
            } else
            {
                ActiveObjectIds.Remove (ObjectData.ObjectId);
            }

            if (AGridLeverActor* LeverActor = RuntimeActor->FindRuntimeObjectActor<AGridLeverActor> (ObjectData.ObjectId))
            {
                LeverActor->SetLeverState (bNewActive);
            }

            return ExecuteLinksFromObject (ObjectData.ObjectId, bWasActive);
        }

        default:
        return false;
    }
}

bool UGridActivationComponent::ActivatePressurePlateAtCell (int32 X, int32 Y)
{
    TArray<int32> PlateIndexes;
    PressurePlateIndexesByCell.MultiFind (FIntPoint (X, Y), PlateIndexes);
    bool bAnyActivated = false;
    for (const int32 PlateIndex : PlateIndexes)
    {
        const FGridLevelObjectData* PlateData = GetObjectByIndex (PlateIndex);
        if (!PlateData || ActiveObjectIds.Contains (PlateData->ObjectId))
        {
            continue;
        }
        ActiveObjectIds.Add (PlateData->ObjectId);
        if (AGridPressurePlateActor* PlateActor =
            RuntimeActor->FindRuntimeObjectActor<AGridPressurePlateActor> (PlateData->ObjectId))
        {
            PlateActor->SetPressed (true);
        }
        bAnyActivated |= ExecuteLinksFromObject (PlateData->ObjectId, false);
    }
    return bAnyActivated;
}

bool UGridActivationComponent::DeactivatePressurePlateAtCell (int32 X, int32 Y)
{
    TArray<int32> PlateIndexes;
    PressurePlateIndexesByCell.MultiFind (FIntPoint (X, Y), PlateIndexes);
    bool bAnyDeactivated = false;
    for (const int32 PlateIndex : PlateIndexes)
    {
        const FGridLevelObjectData* PlateData = GetObjectByIndex (PlateIndex);
        if (!PlateData || !ActiveObjectIds.Contains (PlateData->ObjectId))
        {
            continue;
        }
        ActiveObjectIds.Remove (PlateData->ObjectId);
        if (AGridPressurePlateActor* PlateActor =
            RuntimeActor->FindRuntimeObjectActor<AGridPressurePlateActor> (PlateData->ObjectId))
        {
            PlateActor->SetPressed (false);
        }
        bAnyDeactivated |= ExecuteLinksFromObject (PlateData->ObjectId, true);
    }
    return bAnyDeactivated;
}

bool UGridActivationComponent::ExecuteLinksFromObject (FGuid SourceObjectId, bool bInvert)
{
    if (!RuntimeActor || !RuntimeActor->LevelAsset || !SourceObjectId.IsValid ())
    {
        return false;
    }

    bool bAnyApplied = false;

    TArray<int32> LinkIndexes;
    LinkIndexesBySource.MultiFind (SourceObjectId, LinkIndexes);

    for (const int32 LinkIndex : LinkIndexes)
    {
        if (RuntimeActor->LevelAsset->Links.IsValidIndex (LinkIndex))
        {
            bAnyApplied |= ApplyLinkAction (RuntimeActor->LevelAsset->Links[LinkIndex], bInvert);
        }
    }

    return bAnyApplied;
}

bool UGridActivationComponent::ApplyLinkAction (const FGridLevelLinkData& LinkData, bool bInvert)
{
    if (!RuntimeActor)
    {
        return false;
    }

    const FGridLevelObjectData* TargetObject = FindObjectById (LinkData.TargetObjectId);
    if (!TargetObject)
    {
        return false;
    }

    const EGridLinkAction ResolvedAction = GetResolvedLinkAction (LinkData.Action, bInvert);

    switch (TargetObject->Type)
    {
        case EGridLevelObjectType::Door:
        {
            switch (ResolvedAction)
            {
                case EGridLinkAction::Toggle:
                return RuntimeActor->ToggleDoorOnEdge (TargetObject->CellX, TargetObject->CellY, TargetObject->Edge);

                case EGridLinkAction::Open:
                case EGridLinkAction::Activate:
                return RuntimeActor->OpenDoorOnEdge (TargetObject->CellX, TargetObject->CellY, TargetObject->Edge);

                case EGridLinkAction::Close:
                case EGridLinkAction::Deactivate:
                return RuntimeActor->CloseDoorOnEdge (TargetObject->CellX, TargetObject->CellY, TargetObject->Edge);

                default:
                return false;
            }
        }

        default:
        return false;
    }
}

EGridLinkAction UGridActivationComponent::GetResolvedLinkAction (EGridLinkAction Action, bool bInvert) const
{
    if (!bInvert)
    {
        return Action;
    }

    switch (Action)
    {
        case EGridLinkAction::Open:
        return EGridLinkAction::Close;

        case EGridLinkAction::Close:
        return EGridLinkAction::Open;

        case EGridLinkAction::Activate:
        return EGridLinkAction::Deactivate;

        case EGridLinkAction::Deactivate:
        return EGridLinkAction::Activate;

        case EGridLinkAction::Toggle:
        default:
        return EGridLinkAction::Toggle;
    }
}

void UGridActivationComponent::ActivateTriggersAtCell (int32 X, int32 Y)
{
    TArray<int32> TriggerIndexes;
    TriggerIndexesByCell.MultiFind (FIntPoint (X, Y), TriggerIndexes);
    for (const int32 TriggerIndex : TriggerIndexes)
    {
        const FGridLevelObjectData* TriggerData = GetObjectByIndex (TriggerIndex);
        if (!TriggerData || ActiveObjectIds.Contains (TriggerData->ObjectId))
        {
            continue;
        }
        ActiveObjectIds.Add (TriggerData->ObjectId);
        ExecuteLinksFromObject (TriggerData->ObjectId, false);
    }
}

void UGridActivationComponent::DeactivateTriggersAtCell (int32 X, int32 Y)
{
    TArray<int32> TriggerIndexes;
    TriggerIndexesByCell.MultiFind (FIntPoint (X, Y), TriggerIndexes);
    for (const int32 TriggerIndex : TriggerIndexes)
    {
        const FGridLevelObjectData* TriggerData = GetObjectByIndex (TriggerIndex);
        if (!TriggerData || !ActiveObjectIds.Contains (TriggerData->ObjectId))
        {
            continue;
        }
        ActiveObjectIds.Remove (TriggerData->ObjectId);
        ExecuteLinksFromObject (TriggerData->ObjectId, true);
    }
}

void UGridActivationComponent::RegisterInitialObjectState (const FGridLevelObjectData& ObjectData)
{
    if (ObjectData.bInitiallyActive)
    {
        ActiveObjectIds.Add (ObjectData.ObjectId);
    }
}

void UGridActivationComponent::RebuildIndexes ()
{
    ObjectIndexById.Reset ();
    LinkIndexesBySource.Reset ();
    InteractableObjectIndexByEdge.Reset ();
    PressurePlateIndexesByCell.Reset ();
    TriggerIndexesByCell.Reset ();

    if (!RuntimeActor || !RuntimeActor->LevelAsset)
    {
        return;
    }

    const TArray<FGridLevelObjectData>& Objects = RuntimeActor->LevelAsset->Objects;

    for (int32 Index = 0; Index < Objects.Num (); ++Index)
    {
        const FGridLevelObjectData& ObjectData = Objects[Index];

        if (ObjectData.ObjectId.IsValid ())
        {
            ObjectIndexById.Add (ObjectData.ObjectId, Index);
        }

        if (ObjectData.Type == EGridLevelObjectType::Button ||
            ObjectData.Type == EGridLevelObjectType::Lever)
        {
            InteractableObjectIndexByEdge.Add (FGridEdgeKey (ObjectData.CellX, ObjectData.CellY, ObjectData.Edge), Index);
        } else if (ObjectData.Type == EGridLevelObjectType::PressurePlate)
        {
            PressurePlateIndexesByCell.Add (FIntPoint (ObjectData.CellX, ObjectData.CellY), Index);
        } else if (ObjectData.Type == EGridLevelObjectType::Trigger)
        {
            TriggerIndexesByCell.Add (FIntPoint (ObjectData.CellX, ObjectData.CellY), Index);
        }
    }

    const TArray<FGridLevelLinkData>& Links = RuntimeActor->LevelAsset->Links;

    for (int32 Index = 0; Index < Links.Num (); ++Index)
    {
        if (Links[Index].SourceObjectId.IsValid ())
        {
            LinkIndexesBySource.Add (Links[Index].SourceObjectId, Index);
        }
    }
}

const FGridLevelObjectData* UGridActivationComponent::GetObjectByIndex (int32 ObjectIndex) const
{
    if (!RuntimeActor || !RuntimeActor->LevelAsset)
    {
        return nullptr;
    }

    return RuntimeActor->LevelAsset->Objects.IsValidIndex (ObjectIndex)
        ? &RuntimeActor->LevelAsset->Objects[ObjectIndex]
        : nullptr;
}
