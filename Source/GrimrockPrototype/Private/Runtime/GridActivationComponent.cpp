#include "Runtime/GridActivationComponent.h"

#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridButtonActor.h"
#include "Runtime/GridLeverActor.h"
#include "Runtime/GridMechanismActor.h"
#include "Runtime/GridPressurePlateActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Core/GridLevelAsset.h"

namespace
{
    FString GridLinkActionToString (EGridLinkAction Action)
    {
        if (const UEnum* Enum = StaticEnum<EGridLinkAction> ())
        {
            return Enum->GetNameStringByValue (static_cast<int64> (Action));
        }
        return FString::Printf (TEXT ("%d"), static_cast<int32> (Action));
    }

    FString GridObjectTypeToString (EGridLevelObjectType Type)
    {
        if (const UEnum* Enum = StaticEnum<EGridLevelObjectType> ())
        {
            return Enum->GetNameStringByValue (static_cast<int64> (Type));
        }
        return FString::Printf (TEXT ("%d"), static_cast<int32> (Type));
    }
}

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

bool UGridActivationComponent::TryInteractAtEdge (int32 FromCellX, int32 FromCellY, EGridEdge Edge, AGrimrockPartyPawn* PartyPawn)
{
    const FGridLevelObjectData* ObjectData = FindInteractableObjectOnEdge (FromCellX, FromCellY, Edge);
    return ObjectData ? ActivateObject (*ObjectData, PartyPawn) : false;
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

bool UGridActivationComponent::ActivateObject (const FGridLevelObjectData& ObjectData, AGrimrockPartyPawn* PartyPawn)
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
        case EGridLevelObjectType::Receptacle:
            {
				return ActivateReceptacle (ObjectData, PartyPawn);
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
        LogLinkResult (LinkData, GetResolvedLinkAction (LinkData.Action, bInvert), false, TEXT ("missing runtime actor"));
        return false;
    }

    const FGridLevelObjectData* TargetObject = FindObjectById (LinkData.TargetObjectId);
    if (!TargetObject)
    {
        LogLinkResult (LinkData, GetResolvedLinkAction (LinkData.Action, bInvert), false, TEXT ("target object not found"));
        return false;
    }

    const EGridLinkAction ResolvedAction = GetResolvedLinkAction (LinkData.Action, bInvert);
    bool bSuccess = false;
    const TCHAR* FailureReason = TEXT ("unsupported target type or action");

    switch (TargetObject->Type)
    {
        case EGridLevelObjectType::Door:
        {
            bSuccess = ApplyDoorLinkAction (*TargetObject, ResolvedAction);
            FailureReason = bSuccess ? nullptr : TEXT ("door action failed");
            break;
        }

        case EGridLevelObjectType::Button:
        case EGridLevelObjectType::PressurePlate:
        case EGridLevelObjectType::Lever:
        case EGridLevelObjectType::Decoration:
        case EGridLevelObjectType::MonsterSpawn:
        case EGridLevelObjectType::ItemSpawn:
        case EGridLevelObjectType::Light:
        case EGridLevelObjectType::Teleporter:
        case EGridLevelObjectType::Trigger:
        case EGridLevelObjectType::Receptacle:
        {
            bSuccess = ApplyStatefulLinkAction (*TargetObject, ResolvedAction);
            FailureReason = bSuccess ? nullptr : TEXT ("stateful action failed");
            break;
        }

        case EGridLevelObjectType::None:
        default:
        break;
    }

    LogLinkResult (LinkData, ResolvedAction, bSuccess, FailureReason);
    return bSuccess;
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

bool UGridActivationComponent::ApplyDoorLinkAction (const FGridLevelObjectData& TargetObject, EGridLinkAction Action)
{
    if (!RuntimeActor)
    {
        return false;
    }

    switch (Action)
    {
        case EGridLinkAction::Toggle:
        return RuntimeActor->ToggleDoorOnEdge (TargetObject.CellX, TargetObject.CellY, TargetObject.Edge);

        case EGridLinkAction::Open:
        case EGridLinkAction::Activate:
        return RuntimeActor->OpenDoorOnEdge (TargetObject.CellX, TargetObject.CellY, TargetObject.Edge);

        case EGridLinkAction::Close:
        case EGridLinkAction::Deactivate:
        return RuntimeActor->CloseDoorOnEdge (TargetObject.CellX, TargetObject.CellY, TargetObject.Edge);

        default:
        return false;
    }
}

bool UGridActivationComponent::ApplyStatefulLinkAction (const FGridLevelObjectData& TargetObject, EGridLinkAction Action)
{
    switch (Action)
    {
        case EGridLinkAction::Open:
        case EGridLinkAction::Activate:
        return SetTargetActiveState (TargetObject, true);

        case EGridLinkAction::Close:
        case EGridLinkAction::Deactivate:
        return SetTargetActiveState (TargetObject, false);

        case EGridLinkAction::Toggle:
        return SetTargetActiveState (TargetObject, !IsTargetActive (TargetObject.ObjectId));

        default:
        return false;
    }
}

bool UGridActivationComponent::SetTargetActiveState (const FGridLevelObjectData& TargetObject, bool bActive)
{
    if (!TargetObject.ObjectId.IsValid ())
    {
        return false;
    }

    if (bActive)
    {
        ActiveObjectIds.Add (TargetObject.ObjectId);
    } else
    {
        ActiveObjectIds.Remove (TargetObject.ObjectId);
    }

    if (RuntimeActor)
    {
        if (AGridLeverActor* LeverActor = RuntimeActor->FindRuntimeObjectActor<AGridLeverActor> (TargetObject.ObjectId))
        {
            LeverActor->SetLeverState (bActive);
            return true;
        }
        if (AGridPressurePlateActor* PlateActor = RuntimeActor->FindRuntimeObjectActor<AGridPressurePlateActor> (TargetObject.ObjectId))
        {
            PlateActor->SetPressed (bActive);
            return true;
        }
        if (bActive)
        {
            if (AGridButtonActor* ButtonActor = RuntimeActor->FindRuntimeObjectActor<AGridButtonActor> (TargetObject.ObjectId))
            {
                ButtonActor->TriggerPress ();
                return true;
            }
        }
        if (AGridMechanismActor* MechanismActor = RuntimeActor->FindRuntimeObjectActor<AGridMechanismActor> (TargetObject.ObjectId))
        {
            UE_LOG (LogTemp, Log, TEXT ("Grid link target %s is generic mechanism %s; state stored but no visual activation handler exists yet."),
                *TargetObject.ObjectId.ToString (), *MechanismActor->GetName ());
            return true;
        }
    }

    if (TargetObject.Type == EGridLevelObjectType::MonsterSpawn ||
        TargetObject.Type == EGridLevelObjectType::ItemSpawn)
    {
        UE_LOG (LogTemp, Log, TEXT ("Grid link target %s is %s; state stored, spawn behavior TODO."),
            *TargetObject.ObjectId.ToString (), *GridObjectTypeToString (TargetObject.Type));
    } else if (TargetObject.Type == EGridLevelObjectType::Teleporter)
    {
        UE_LOG (LogTemp, Log, TEXT ("Grid link target %s is Teleporter; state stored, teleport behavior TODO."),
            *TargetObject.ObjectId.ToString ());
    }

    return true;
}

bool UGridActivationComponent::IsTargetActive (FGuid ObjectId) const
{
    return ObjectId.IsValid () && ActiveObjectIds.Contains (ObjectId);
}

void UGridActivationComponent::LogLinkResult (const FGridLevelLinkData& LinkData, EGridLinkAction ResolvedAction, bool bSuccess, const TCHAR* FailureReason) const
{
    if (bSuccess)
    {
        UE_LOG (LogTemp, Log, TEXT ("Grid link executed: Source=%s Target=%s Action=%s Success=true"),
            *LinkData.SourceObjectId.ToString (),
            *LinkData.TargetObjectId.ToString (),
            *GridLinkActionToString (ResolvedAction));
        return;
    }

    UE_LOG (LogTemp, Warning, TEXT ("Grid link failed: Source=%s Target=%s Action=%s Reason=%s"),
        *LinkData.SourceObjectId.ToString (),
        *LinkData.TargetObjectId.ToString (),
        *GridLinkActionToString (ResolvedAction),
        FailureReason ? FailureReason : TEXT ("unknown"));
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
            ObjectData.Type == EGridLevelObjectType::Lever ||
            ObjectData.Type == EGridLevelObjectType::Receptacle)
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

bool UGridActivationComponent::ActivateReceptacle (const FGridLevelObjectData& ObjectData, AGrimrockPartyPawn* PartyPawn)
{
    if (!RuntimeActor || !PartyPawn)
    {
        return false;
    }
    AGridReceptacleActor* ReceptacleActor =
        RuntimeActor->FindRuntimeObjectActor<AGridReceptacleActor> (ObjectData.ObjectId);

    if (!ReceptacleActor)
    {
        return false;
    }
    // Cas 1 : le support contient déjà un item.
    // On le retire, on le rend au groupe, et on inverse les liens.
    if (ReceptacleActor->HasItem ())
    {
        FName RemovedItemId = NAME_None;

        if (!ReceptacleActor->TryRemoveItem (RemovedItemId))
        {
            return false;
        }
        PartyPawn->AddInventoryItem (RemovedItemId);
        ActiveObjectIds.Remove (ObjectData.ObjectId);
        UE_LOG (LogTemp, Log, TEXT ("Receptacle %s: removed item %s"), *ObjectData.ObjectId.ToString (), *RemovedItemId.ToString ());
        return ExecuteLinksFromObject (ObjectData.ObjectId, true);
    }
    // Cas 2 : le support est vide.
    // On tente d’insérer l’item accepté.
    FName ItemToInsert = ReceptacleActor->AcceptedItemId;

    if (ItemToInsert == NAME_None)
    {
        ItemToInsert = PartyPawn->DefaultInteractionItemId;
    }
    if (!PartyPawn->HasInventoryItem (ItemToInsert))
    {
        UE_LOG (LogTemp, Warning, TEXT ("Receptacle %s: party has no item %s"), *ObjectData.ObjectId.ToString (), *ItemToInsert.ToString ());
        return false;
    }
    if (!ReceptacleActor->TryInsertItem (ItemToInsert))
    {
        return false;
    }
    PartyPawn->RemoveInventoryItem (ItemToInsert);
    ActiveObjectIds.Add (ObjectData.ObjectId);
    UE_LOG (LogTemp, Log, TEXT ("Receptacle %s: inserted item %s"), *ObjectData.ObjectId.ToString (), *ItemToInsert.ToString ());
    return ExecuteLinksFromObject (ObjectData.ObjectId, false);
}

FString UGridActivationComponent::GetDebugSummary () const
{
    return FString::Printf (
        TEXT ("Activation | Objects=%d Links=%d Interactables=%d Plates=%d Triggers=%d Active=%d"),
        ObjectIndexById.Num (),
        LinkIndexesBySource.Num (),
        InteractableObjectIndexByEdge.Num (),
        PressurePlateIndexesByCell.Num (),
        TriggerIndexesByCell.Num (),
        ActiveObjectIds.Num ()
    );
}

void UGridActivationComponent::LogDebugSummary () const
{
    UE_LOG (LogTemp, Log, TEXT ("%s"), *GetDebugSummary ());
}
