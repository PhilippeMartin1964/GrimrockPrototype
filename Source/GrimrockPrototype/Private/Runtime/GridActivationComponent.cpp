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

void UGridActivationComponent::HandlePartyCellChanged (
    int32 OldCellX,
    int32 OldCellY,
    int32 NewCellX,
    int32 NewCellY)
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
    if (!RuntimeActor || !RuntimeActor->LevelAsset || !ObjectId.IsValid ())
    {
        return nullptr;
    }

    for (const FGridLevelObjectData& ObjectData : RuntimeActor->LevelAsset->Objects)
    {
        if (ObjectData.ObjectId == ObjectId)
        {
            return &ObjectData;
        }
    }

    return nullptr;
}

const FGridLevelObjectData* UGridActivationComponent::FindObjectDataAtCell (
    EGridLevelObjectType Type,
    int32 X,
    int32 Y) const
{
    if (!RuntimeActor || !RuntimeActor->LevelAsset)
    {
        return nullptr;
    }

    for (const FGridLevelObjectData& ObjectData : RuntimeActor->LevelAsset->Objects)
    {
        if (ObjectData.Type == Type && ObjectData.CellX == X && ObjectData.CellY == Y)
        {
            return &ObjectData;
        }
    }

    return nullptr;
}

const FGridLevelObjectData* UGridActivationComponent::FindInteractableObjectOnEdge (
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
        if (ObjectData.CellX != X || ObjectData.CellY != Y || ObjectData.Edge != Edge)
        {
            continue;
        }

        if (ObjectData.Type == EGridLevelObjectType::Button ||
            ObjectData.Type == EGridLevelObjectType::Lever)
        {
            return &ObjectData;
        }
    }

    return nullptr;
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
            if (AGridButtonActor* ButtonActor =
                RuntimeActor->FindRuntimeObjectActor<AGridButtonActor> (ObjectData.ObjectId))
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

            if (AGridLeverActor* LeverActor =
                RuntimeActor->FindRuntimeObjectActor<AGridLeverActor> (ObjectData.ObjectId))
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
    const FGridLevelObjectData* PlateData =
        FindObjectDataAtCell (EGridLevelObjectType::PressurePlate, X, Y);

    if (!PlateData || ActiveObjectIds.Contains (PlateData->ObjectId))
    {
        return false;
    }

    ActiveObjectIds.Add (PlateData->ObjectId);

    if (RuntimeActor)
    {
        if (AGridPressurePlateActor* PlateActor =
            RuntimeActor->FindRuntimeObjectActor<AGridPressurePlateActor> (PlateData->ObjectId))
        {
            PlateActor->SetPressed (true);
        }
    }

    return ExecuteLinksFromObject (PlateData->ObjectId, false);
}

bool UGridActivationComponent::DeactivatePressurePlateAtCell (int32 X, int32 Y)
{
    const FGridLevelObjectData* PlateData =
        FindObjectDataAtCell (EGridLevelObjectType::PressurePlate, X, Y);

    if (!PlateData || !ActiveObjectIds.Contains (PlateData->ObjectId))
    {
        return false;
    }

    ActiveObjectIds.Remove (PlateData->ObjectId);

    if (RuntimeActor)
    {
        if (AGridPressurePlateActor* PlateActor =
            RuntimeActor->FindRuntimeObjectActor<AGridPressurePlateActor> (PlateData->ObjectId))
        {
            PlateActor->SetPressed (false);
        }
    }

    return ExecuteLinksFromObject (PlateData->ObjectId, true);
}

bool UGridActivationComponent::ExecuteLinksFromObject (FGuid SourceObjectId, bool bInvert)
{
    if (!RuntimeActor || !RuntimeActor->LevelAsset || !SourceObjectId.IsValid ())
    {
        return false;
    }

    bool bAnyApplied = false;

    for (const FGridLevelLinkData& LinkData : RuntimeActor->LevelAsset->Links)
    {
        if (LinkData.SourceObjectId != SourceObjectId)
        {
            continue;
        }

        bAnyApplied |= ApplyLinkAction (LinkData, bInvert);
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
    const FGridLevelObjectData* TriggerData =
        FindObjectDataAtCell (EGridLevelObjectType::Trigger, X, Y);

    if (!TriggerData || ActiveObjectIds.Contains (TriggerData->ObjectId))
    {
        return;
    }

    ActiveObjectIds.Add (TriggerData->ObjectId);
    ExecuteLinksFromObject (TriggerData->ObjectId, false);
}

void UGridActivationComponent::DeactivateTriggersAtCell (int32 X, int32 Y)
{
    const FGridLevelObjectData* TriggerData =
        FindObjectDataAtCell (EGridLevelObjectType::Trigger, X, Y);

    if (!TriggerData || !ActiveObjectIds.Contains (TriggerData->ObjectId))
    {
        return;
    }

    ActiveObjectIds.Remove (TriggerData->ObjectId);
    ExecuteLinksFromObject (TriggerData->ObjectId, true);
}

void UGridActivationComponent::RegisterInitialObjectState (const FGridLevelObjectData& ObjectData)
{
    if (ObjectData.bInitiallyActive)
    {
        ActiveObjectIds.Add (ObjectData.ObjectId);
    }
}