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
    FString GridObjectCommandToString (EGridObjectCommand Command)
    {
        if (const UEnum* Enum = StaticEnum<EGridObjectCommand> ())
        {
            return Enum->GetNameStringByValue (static_cast<int64> (Command));
        }
        return FString::Printf (TEXT ("%d"), static_cast<int32> (Command));
    }

    FString GridObjectTypeToString (EGridLevelObjectType Type)
    {
        if (const UEnum* Enum = StaticEnum<EGridLevelObjectType> ())
        {
            return Enum->GetNameStringByValue (static_cast<int64> (Type));
        }
        return FString::Printf (TEXT ("%d"), static_cast<int32> (Type));
    }

    FString GridObjectEventToString (EGridObjectEvent EventType)
    {
        if (const UEnum* Enum = StaticEnum<EGridObjectEvent> ())
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
            bAnyApplied |= ApplyLinkCommand (RuntimeActor->LevelAsset->Links[LinkIndex], bInvert);
        }
    }

    return bAnyApplied;
}

bool UGridActivationComponent::ApplyLinkCommand (const FGridObjectLink& LinkData, bool bInvert)
{
    if (!RuntimeActor)
    {
        LogLinkResult (LinkData, GetResolvedLinkCommand (LinkData.Command, bInvert), false, TEXT ("missing runtime actor"));
        return false;
    }

    const FGridLevelObjectData* TargetObject = FindObjectById (LinkData.TargetObjectId);
    if (!TargetObject)
    {
        LogLinkResult (LinkData, GetResolvedLinkCommand (LinkData.Command, bInvert), false, TEXT ("target object not found"));
        return false;
    }

    const EGridObjectCommand ResolvedCommand = GetResolvedLinkCommand (LinkData.Command, bInvert);
    bool bSuccess = false;
    const TCHAR* FailureReason = TEXT ("unsupported target type or command");

    switch (TargetObject->Type)
    {
        case EGridLevelObjectType::Door:
        {
            bSuccess = ApplyDoorLinkCommand (*TargetObject, ResolvedCommand);
            FailureReason = bSuccess ? nullptr : TEXT ("door command failed");
            break;
        }

        case EGridLevelObjectType::Button:
        case EGridLevelObjectType::PressurePlate:
        case EGridLevelObjectType::Lever:
        case EGridLevelObjectType::Decoration:
        case EGridLevelObjectType::MonsterSpawn:
        case EGridLevelObjectType::ItemSpawn:
        case EGridLevelObjectType::Item:
        case EGridLevelObjectType::Light:
        case EGridLevelObjectType::Teleporter:
        case EGridLevelObjectType::Trigger:
        case EGridLevelObjectType::Receptacle:
        {
            bSuccess = ApplyStatefulLinkCommand (*TargetObject, ResolvedCommand);
            FailureReason = bSuccess ? nullptr : TEXT ("stateful command failed");
            break;
        }

        case EGridLevelObjectType::None:
        default:
        break;
    }

    LogLinkResult (LinkData, ResolvedCommand, bSuccess, FailureReason);
    return bSuccess;
}

bool UGridActivationComponent::ExecuteLinksFromObjectForEvent (
    FGuid SourceObjectId,
    EGridObjectEvent SourceEvent,
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
        SourceEvent != EGridObjectEvent::Activated &&
        MatchingEventLinkCount == 0;

    const EGridObjectEvent EffectiveEvent =
        bUseActivatedFallback ? EGridObjectEvent::Activated : SourceEvent;

    if (bUseActivatedFallback)
    {
        UE_LOG (LogTemp, Log, TEXT ("Grid object event %s: no %s links, falling back to Activated links."),
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

        const FGridObjectLink& LinkData = RuntimeActor->LevelAsset->Links[LinkIndex];
        if (LinkData.SourceEvent != EffectiveEvent)
        {
            continue;
        }

        ++ExecutedLinkCount;
        bAnyApplied |= ApplyLinkCommand (LinkData, bInvert);
    }

    UE_LOG (LogTemp, Log, TEXT ("Grid object event %s: Event=%s LinksExecuted=%d AnyApplied=%s"),
        *SourceObjectId.ToString (),
        *GridObjectEventToString (EffectiveEvent),
        ExecutedLinkCount,
        bAnyApplied ? TEXT ("true") : TEXT ("false"));

    return bAnyApplied;
}

int32 UGridActivationComponent::CountLinksFromObjectForEvent (FGuid SourceObjectId, EGridObjectEvent SourceEvent) const
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

EGridObjectCommand UGridActivationComponent::GetResolvedLinkCommand (EGridObjectCommand Command, bool bInvert) const
{
    if (!bInvert)
    {
        return Command;
    }

    switch (Command)
    {
        case EGridObjectCommand::Open:
        return EGridObjectCommand::Close;

        case EGridObjectCommand::Close:
        return EGridObjectCommand::Open;

        case EGridObjectCommand::Activate:
        return EGridObjectCommand::Deactivate;

        case EGridObjectCommand::Deactivate:
        return EGridObjectCommand::Activate;

        case EGridObjectCommand::Enable:
        return EGridObjectCommand::Disable;

        case EGridObjectCommand::Disable:
        return EGridObjectCommand::Enable;

        case EGridObjectCommand::Lock:
        return EGridObjectCommand::Unlock;

        case EGridObjectCommand::Unlock:
        return EGridObjectCommand::Lock;

        case EGridObjectCommand::Spawn:
        return EGridObjectCommand::Despawn;

        case EGridObjectCommand::Despawn:
        return EGridObjectCommand::Spawn;

        case EGridObjectCommand::Teleport:
        return EGridObjectCommand::Teleport;

        case EGridObjectCommand::ShowMessage:
        return EGridObjectCommand::ShowMessage;

        case EGridObjectCommand::Toggle:
        default:
        return EGridObjectCommand::Toggle;
    }
}

bool UGridActivationComponent::ApplyDoorLinkCommand (const FGridLevelObjectData& TargetObject, EGridObjectCommand Command)
{
    if (!RuntimeActor)
    {
        return false;
    }

    switch (Command)
    {
        case EGridObjectCommand::Toggle:
        return RuntimeActor->ToggleDoorOnEdge (TargetObject.CellX, TargetObject.CellY, TargetObject.Edge);

        case EGridObjectCommand::Open:
        case EGridObjectCommand::Activate:
        return RuntimeActor->OpenDoorOnEdge (TargetObject.CellX, TargetObject.CellY, TargetObject.Edge);

        case EGridObjectCommand::Close:
        case EGridObjectCommand::Deactivate:
        return RuntimeActor->CloseDoorOnEdge (TargetObject.CellX, TargetObject.CellY, TargetObject.Edge);

        default:
        return false;
    }
}

bool UGridActivationComponent::ApplyStatefulLinkCommand (const FGridLevelObjectData& TargetObject, EGridObjectCommand Command)
{
    switch (Command)
    {
        case EGridObjectCommand::Open:
        case EGridObjectCommand::Activate:
        return SetTargetActiveState (TargetObject, true);

        case EGridObjectCommand::Close:
        case EGridObjectCommand::Deactivate:
        return SetTargetActiveState (TargetObject, false);

        case EGridObjectCommand::Toggle:
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

void UGridActivationComponent::LogLinkResult (const FGridObjectLink& LinkData, EGridObjectCommand ResolvedCommand, bool bSuccess, const TCHAR* FailureReason) const
{
    if (bSuccess)
    {
        UE_LOG (LogTemp, Log, TEXT ("Grid link executed: Source=%s Target=%s Command=%s Success=true"),
            *LinkData.SourceObjectId.ToString (),
            *LinkData.TargetObjectId.ToString (),
            *GridObjectCommandToString (ResolvedCommand));
        return;
    }

    UE_LOG (LogTemp, Warning, TEXT ("Grid link failed: Source=%s Target=%s Command=%s Reason=%s"),
        *LinkData.SourceObjectId.ToString (),
        *LinkData.TargetObjectId.ToString (),
        *GridObjectCommandToString (ResolvedCommand),
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

    EGridObjectEvent SourceEvent = bEntering
        ? EGridObjectEvent::Activated
        : EGridObjectEvent::Deactivated;

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
                SourceEvent = EGridObjectEvent::Activated;
                bInvertLinks = Behavior.Activation.bInvertLinks;
            } else
            {
                ActiveObjectIds.Remove (TriggerData.ObjectId);
                SourceEvent = EGridObjectEvent::Deactivated;
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
        SourceEvent == EGridObjectEvent::Deactivated &&
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

    const TArray<FGridObjectLink>& Links = RuntimeActor->LevelAsset->Links;

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

bool UGridActivationComponent::ActivateReceptacle (
    const FGridLevelObjectData& ObjectData,
    AGrimrockPartyPawn* PartyPawn)
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
    const bool bHadItem = ReceptacleActor->HasItem ();
    if (!ReceptacleActor->TryInteractWithParty (PartyPawn))
    {
        return false;
    }
    const bool bHasItemNow = ReceptacleActor->HasItem ();
    if (bHasItemNow)
    {
        ActiveObjectIds.Add (ObjectData.ObjectId);
    } else
    {
        ActiveObjectIds.Remove (ObjectData.ObjectId);
    }

    if (!bHadItem && bHasItemNow)
    {
        ExecuteLinksFromObjectForEvent (ObjectData.ObjectId, EGridObjectEvent::ItemInserted, false, false);
        ExecuteLinksFromObjectForEvent (ObjectData.ObjectId, EGridObjectEvent::ItemChanged, false, false);
        return true;
    }
    if (bHadItem && !bHasItemNow)
    {
        ExecuteLinksFromObjectForEvent (ObjectData.ObjectId, EGridObjectEvent::ItemRemoved, false, false);
        ExecuteLinksFromObjectForEvent (ObjectData.ObjectId, EGridObjectEvent::ItemChanged, false, false);
        return true;
    }
    return false;
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
