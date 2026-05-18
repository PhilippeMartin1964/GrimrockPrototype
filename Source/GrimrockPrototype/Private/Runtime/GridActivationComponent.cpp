#include "Runtime/GridActivationComponent.h"

#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridButtonActor.h"
#include "Runtime/GridLeverActor.h"
#include "Runtime/GridMechanismActor.h"
#include "Runtime/GridPressurePlateActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/GridGenericObjectActor.h"
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

    FString GridObjectEventToString (EGridObjectEventType EventType)
    {
        if (const UEnum* Enum = StaticEnum<EGridObjectEventType> ())
        {
            return Enum->GetNameStringByValue (static_cast<int64> (EventType));
        }
        return FString::Printf (TEXT ("%d"), static_cast<int32> (EventType));
    }

    FString GridTriggerModeToString (EGridObjectTriggerMode TriggerMode)
    {
        if (const UEnum* Enum = StaticEnum<EGridObjectTriggerMode> ())
        {
            return Enum->GetNameStringByValue (static_cast<int64> (TriggerMode));
        }
        return FString::Printf (TEXT ("%d"), static_cast<int32> (TriggerMode));
    }

    bool IsReadableGenericObject (const FGridLevelObjectData& ObjectData)
    {
        return ObjectData.Type == EGridLevelObjectType::Decoration
            || ObjectData.Type == EGridLevelObjectType::Light;
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
    ConsumedOneShotTriggerIds.Reset ();
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
        NotifyPawnEnteredCell (NewCellX, NewCellY);
        return;
    }

    DeactivatePressurePlateAtCell (OldCellX, OldCellY);
    ActivatePressurePlateAtCell (NewCellX, NewCellY);

    NotifyPawnExitedCell (OldCellX, OldCellY);
    NotifyPawnEnteredCell (NewCellX, NewCellY);
}

void UGridActivationComponent::NotifyPawnEnteredCell (int32 CellX, int32 CellY)
{
    ProcessTriggersAtCell (CellX, CellY, true);
}

void UGridActivationComponent::NotifyPawnExitedCell (int32 CellX, int32 CellY)
{
    ProcessTriggersAtCell (CellX, CellY, false);
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
        case EGridLevelObjectType::Decoration:
        case EGridLevelObjectType::Light:
            {
                return ActivateReadableObject (ObjectData);
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

bool UGridActivationComponent::ExecuteLinksFromObjectForEvent (
    FGuid SourceObjectId,
    EGridObjectEventType SourceEvent,
    bool bInvert,
    bool bAllowActivatedFallback)
{
    if (!RuntimeActor || !RuntimeActor->LevelAsset || !SourceObjectId.IsValid ())
    {
        return false;
    }

    const int32 MatchingEventLinkCount = CountLinksFromObjectForEvent (SourceObjectId, SourceEvent);
    const bool bUseActivatedFallback =
        bAllowActivatedFallback &&
        SourceEvent != EGridObjectEventType::Activated &&
        MatchingEventLinkCount == 0;

    const EGridObjectEventType EffectiveEvent =
        bUseActivatedFallback ? EGridObjectEventType::Activated : SourceEvent;

    if (bUseActivatedFallback)
    {
        UE_LOG (LogTemp, Log, TEXT ("Grid trigger %s: no %s links, falling back to Activated links."),
            *SourceObjectId.ToString (),
            *GridObjectEventToString (SourceEvent));
    }

    bool bAnyApplied = false;
    int32 ExecutedLinkCount = 0;

    TArray<int32> LinkIndexes;
    LinkIndexesBySource.MultiFind (SourceObjectId, LinkIndexes);

    for (const int32 LinkIndex : LinkIndexes)
    {
        if (!RuntimeActor->LevelAsset->Links.IsValidIndex (LinkIndex))
        {
            continue;
        }

        const FGridLevelLinkData& LinkData = RuntimeActor->LevelAsset->Links[LinkIndex];
        if (LinkData.SourceEvent != EffectiveEvent)
        {
            continue;
        }

        ++ExecutedLinkCount;
        bAnyApplied |= ApplyLinkAction (LinkData, bInvert);
    }

    UE_LOG (LogTemp, Log, TEXT ("Grid trigger %s: Event=%s LinksExecuted=%d AnyApplied=%s"),
        *SourceObjectId.ToString (),
        *GridObjectEventToString (EffectiveEvent),
        ExecutedLinkCount,
        bAnyApplied ? TEXT ("true") : TEXT ("false"));

    return bAnyApplied;
}

int32 UGridActivationComponent::CountLinksFromObjectForEvent (FGuid SourceObjectId, EGridObjectEventType SourceEvent) const
{
    if (!RuntimeActor || !RuntimeActor->LevelAsset || !SourceObjectId.IsValid ())
    {
        return 0;
    }

    int32 Result = 0;

    TArray<int32> LinkIndexes;
    LinkIndexesBySource.MultiFind (SourceObjectId, LinkIndexes);

    for (const int32 LinkIndex : LinkIndexes)
    {
        if (RuntimeActor->LevelAsset->Links.IsValidIndex (LinkIndex) &&
            RuntimeActor->LevelAsset->Links[LinkIndex].SourceEvent == SourceEvent)
        {
            ++Result;
        }
    }

    return Result;
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

bool UGridActivationComponent::ProcessTriggersAtCell (int32 X, int32 Y, bool bEntering)
{
    TArray<int32> TriggerIndexes;
    TriggerIndexesByCell.MultiFind (FIntPoint (X, Y), TriggerIndexes);
    bool bAnyTriggered = false;

    for (const int32 TriggerIndex : TriggerIndexes)
    {
        const FGridLevelObjectData* TriggerData = GetObjectByIndex (TriggerIndex);
        if (!TriggerData)
        {
            continue;
        }

        bAnyTriggered |= ProcessTriggerEvent (*TriggerData, bEntering);
    }

    return bAnyTriggered;
}

bool UGridActivationComponent::ProcessTriggerEvent (const FGridLevelObjectData& TriggerData, bool bEntering)
{
    if (TriggerData.Type != EGridLevelObjectType::Trigger || !TriggerData.ObjectId.IsValid ())
    {
        return false;
    }

    const FGridObjectBehaviorParams& Behavior = GetRuntimeBehavior (TriggerData);
    const bool bWantsEvent = bEntering ? Behavior.Trigger.bFireOnEnter : Behavior.Trigger.bFireOnExit;
    const TCHAR* EventLabel = bEntering ? TEXT ("Enter") : TEXT ("Exit");

    if (!bWantsEvent)
    {
        UE_LOG (LogTemp, Log, TEXT ("Grid trigger detected: Id=%s Cell=(%d,%d) Event=%s ignored by behavior."),
            *TriggerData.ObjectId.ToString (),
            TriggerData.CellX,
            TriggerData.CellY,
            EventLabel);
        return false;
    }

    const bool bOneShot = Behavior.Activation.TriggerMode == EGridObjectTriggerMode::OneShot;

    if (bOneShot && ConsumedOneShotTriggerIds.Contains (TriggerData.ObjectId))
    {
        UE_LOG (LogTemp, Log, TEXT ("Grid trigger detected: Id=%s Cell=(%d,%d) Event=%s one-shot already consumed."),
            *TriggerData.ObjectId.ToString (),
            TriggerData.CellX,
            TriggerData.CellY,
            EventLabel);
        return false;
    }

    EGridObjectEventType SourceEvent = bEntering
        ? EGridObjectEventType::Activated
        : EGridObjectEventType::Deactivated;

    bool bInvertLinks = Behavior.Activation.bInvertLinks;
    bool bAllowActivatedFallback = !bEntering;

    switch (Behavior.Activation.TriggerMode)
    {
        case EGridObjectTriggerMode::Hold:
        {
            if (bEntering)
            {
                ActiveObjectIds.Add (TriggerData.ObjectId);
            } else if (ActiveObjectIds.Contains (TriggerData.ObjectId))
            {
                ActiveObjectIds.Remove (TriggerData.ObjectId);
            } else
            {
                UE_LOG (LogTemp, Log, TEXT ("Grid trigger detected: Id=%s Cell=(%d,%d) Event=Exit ignored because hold trigger is not active."),
                    *TriggerData.ObjectId.ToString (),
                    TriggerData.CellX,
                    TriggerData.CellY);
                return false;
            }
            break;
        }

        case EGridObjectTriggerMode::Toggle:
        {
            const bool bWasActive = ActiveObjectIds.Contains (TriggerData.ObjectId);
            const bool bNewActive = !bWasActive;

            if (bNewActive)
            {
                ActiveObjectIds.Add (TriggerData.ObjectId);
                SourceEvent = EGridObjectEventType::Activated;
                bInvertLinks = Behavior.Activation.bInvertLinks;
            } else
            {
                ActiveObjectIds.Remove (TriggerData.ObjectId);
                SourceEvent = EGridObjectEventType::Deactivated;
                bInvertLinks = !Behavior.Activation.bInvertLinks;
                bAllowActivatedFallback = true;
            }
            break;
        }

        case EGridObjectTriggerMode::Instant:
        case EGridObjectTriggerMode::OneShot:
        default:
            break;
    }

    UE_LOG (LogTemp, Log, TEXT ("Grid trigger detected: Id=%s Cell=(%d,%d) Event=%s Mode=%s SourceEvent=%s"),
        *TriggerData.ObjectId.ToString (),
        TriggerData.CellX,
        TriggerData.CellY,
        EventLabel,
        *GridTriggerModeToString (Behavior.Activation.TriggerMode),
        *GridObjectEventToString (SourceEvent));

    if (!bEntering &&
        SourceEvent == EGridObjectEventType::Deactivated &&
        CountLinksFromObjectForEvent (TriggerData.ObjectId, SourceEvent) == 0)
    {
        bInvertLinks = !Behavior.Activation.bInvertLinks;
    }

    const bool bApplied = ExecuteLinksFromObjectForEvent (
        TriggerData.ObjectId,
        SourceEvent,
        bInvertLinks,
        bAllowActivatedFallback);

    if (bOneShot)
    {
        ConsumedOneShotTriggerIds.Add (TriggerData.ObjectId);
        UE_LOG (LogTemp, Log, TEXT ("Grid trigger %s: one-shot consumed after %s."),
            *TriggerData.ObjectId.ToString (),
            EventLabel);
    }

    return bApplied;
}

void UGridActivationComponent::RegisterInitialObjectState (const FGridLevelObjectData& ObjectData)
{
    if (ObjectData.ObjectId.IsValid ())
    {
        RuntimeBehaviorByObjectId.Add (ObjectData.ObjectId, ObjectData.Behavior);
    }

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
    RuntimeBehaviorByObjectId.Reset ();

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
            RuntimeBehaviorByObjectId.Add (ObjectData.ObjectId, ObjectData.Behavior);
        }
        if (ObjectData.Type == EGridLevelObjectType::Button ||
            ObjectData.Type == EGridLevelObjectType::Lever ||
            ObjectData.Type == EGridLevelObjectType::Receptacle ||
            IsReadableGenericObject (ObjectData))
        {
            if (ObjectData.Edge != EGridEdge::None)
            {
                InteractableObjectIndexByEdge.Add (FGridEdgeKey (ObjectData.CellX, ObjectData.CellY, ObjectData.Edge), Index);
            }
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

const FGridObjectBehaviorParams& UGridActivationComponent::GetRuntimeBehavior (const FGridLevelObjectData& ObjectData) const
{
    if (const FGridObjectBehaviorParams* RuntimeBehavior = RuntimeBehaviorByObjectId.Find (ObjectData.ObjectId))
    {
        return *RuntimeBehavior;
    }

    return ObjectData.Behavior;
}

bool UGridActivationComponent::ActivateReadableObject (const FGridLevelObjectData& ObjectData)
{
    if (!RuntimeActor)
    {
        return false;
    }
    AGridGenericObjectActor* GenericActor = RuntimeActor->FindRuntimeObjectActor<AGridGenericObjectActor> (ObjectData.ObjectId);
    if (!GenericActor || !GenericActor->HasReadableText ())
    {
        return false;
    }
    if (GenericActor->bRuntimeReadableOnlyOnce && GenericActor->bRuntimeHasBeenRead)
    {
        return true;
    }
    if (GEngine)
    {
        const FText ReadableText = GenericActor->GetReadableText ();
        UE_LOG (LogTemp, Log, TEXT ("Readable object %s: %s"), *ObjectData.ObjectId.ToString (), *ReadableText.ToString ());
        RuntimeActor->ShowReadableMessage (ReadableText);
        GenericActor->MarkAsRead ();
        return true;
    }
    GenericActor->MarkAsRead ();
    return true;
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

        if (!ReceptacleActor->TryTakeContainedItem (PartyPawn, RemovedItemId))
        {
            return false;
        }
        ActiveObjectIds.Remove (ObjectData.ObjectId);
        UE_LOG (LogTemp, Log, TEXT ("Receptacle %s: removed item %s"), *ObjectData.ObjectId.ToString (), *RemovedItemId.ToString ());
        return ExecuteLinksFromObject (ObjectData.ObjectId, true);
    }
    // Cas 2 : le support est vide.
    // On tente d’insérer l’item accepté.
    FName ItemToInsert = ReceptacleActor->AcceptedArchetypeIds.Num () > 0
        ? ReceptacleActor->AcceptedArchetypeIds[0]
        : NAME_None;

    if (ItemToInsert == NAME_None)
    {
        ItemToInsert = PartyPawn->DefaultInteractionItemId;
    }
    if (!PartyPawn->HasInventoryItem (ItemToInsert))
    {
        UE_LOG (LogTemp, Warning, TEXT ("Receptacle %s: cannot insert %s because the party does not have it"),
            *ObjectData.ObjectId.ToString (), *ItemToInsert.ToString ());
        return false;
    }
    if (!PartyPawn->RemoveInventoryItem (ItemToInsert))
    {
        UE_LOG (LogTemp, Warning, TEXT ("Receptacle %s: failed to remove item %s from party inventory"),
            *ObjectData.ObjectId.ToString (), *ItemToInsert.ToString ());
        return false;
    }
    if (PartyPawn->IsHoldingItem (ItemToInsert))
    {
        PartyPawn->ClearHeldItem ();
        UE_LOG (LogTemp, Log, TEXT ("Receptacle %s: cleared held item %s before insertion"),
            *ObjectData.ObjectId.ToString (), *ItemToInsert.ToString ());
    }
    if (!ReceptacleActor->TryInsertItem (ItemToInsert))
    {
        PartyPawn->AddInventoryItem (ItemToInsert);
        UE_LOG (LogTemp, Warning, TEXT ("Receptacle %s: item %s is not accepted or could not be inserted"),
            *ObjectData.ObjectId.ToString (), *ItemToInsert.ToString ());
        return false;
    }
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
