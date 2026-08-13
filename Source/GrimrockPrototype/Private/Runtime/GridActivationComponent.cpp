#include "Runtime/GridActivationComponent.h"

#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridButtonActor.h"
#include "Runtime/GridLeverActor.h"
#include "Runtime/GridMechanismActor.h"
#include "Runtime/GridPressurePlateActor.h"
#include "Runtime/GridReceptacleActor.h"
#include "Runtime/GridWallLockActor.h"
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

    FString GridObjectConditionToString (EGridObjectCondition Condition)
    {
        if (const UEnum* Enum = StaticEnum<EGridObjectCondition> ())
        {
            return Enum->GetNameStringByValue (static_cast<int64> (Condition));
        }
        return FString::Printf (TEXT ("%d"), static_cast<int32> (Condition));
    }

    bool IsReceptacleCommand (EGridObjectCommand Command)
    {
        switch (Command)
        {
            case EGridObjectCommand::ReceptacleConsumeItem:
            case EGridObjectCommand::ReceptacleConsumeAllItems:
            case EGridObjectCommand::ReceptacleEnableRemoval:
            case EGridObjectCommand::ReceptacleDisableRemoval:
            return true;

            default:
            return false;
        }
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
    DispatchingSourceObjectIds.Reset ();
}

void UGridActivationComponent::SetActiveObjectIds (const TSet<FGuid>& InActiveObjectIds)
{
    ActiveObjectIds = InActiveObjectIds;
}

bool UGridActivationComponent::TryInteractAtEdge (int32 FromCellX, int32 FromCellY, EGridEdge Edge, AGrimrockPartyPawn* PartyPawn)
{
    const FGridLevelObjectData* ObjectData = FindInteractableObjectOnEdge (FromCellX, FromCellY, Edge);
    return ObjectData ? ActivateObject (*ObjectData, PartyPawn) : false;
}

AGridReceptacleActor* UGridActivationComponent::FindReceptacleAtEdge (int32 FromCellX, int32 FromCellY, EGridEdge Edge) const
{
    const FGridLevelObjectData* ObjectData = FindInteractableObjectOnEdge (FromCellX, FromCellY, Edge);
    if (!RuntimeActor || !ObjectData || ObjectData->Type != EGridLevelObjectType::Receptacle)
    {
        return nullptr;
    }

    return RuntimeActor->FindRuntimeObjectActor<AGridReceptacleActor> (ObjectData->ObjectId);
}

void UGridActivationComponent::HandlePartyCellChanged (int32 OldCellX, int32 OldCellY, int32 NewCellX, int32 NewCellY)
{
    if (OldCellX == NewCellX && OldCellY == NewCellY)
    {
        RefreshPressurePlatesAtCell (NewCellX, NewCellY);
        NotifyPawnEnteredCell (NewCellX, NewCellY);
        return;
    }

    RefreshPressurePlatesAtCell (OldCellX, OldCellY);
    RefreshPressurePlatesAtCell (NewCellX, NewCellY);

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
                return ExecuteLinksFromObjectForEvent (ObjectData.ObjectId, EGridObjectEvent::Activated);
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

                const EGridObjectEvent LeverEvent =
                    bNewActive ? EGridObjectEvent::Activated : EGridObjectEvent::Deactivated;
                return ExecuteLinksFromObjectForEvent (ObjectData.ObjectId, LeverEvent);
            }
        case EGridLevelObjectType::Receptacle:
            {
                if (AGridWallLockActor* WallLockActor =
                    RuntimeActor->FindRuntimeObjectActor<AGridWallLockActor> (ObjectData.ObjectId))
                {
                    return WallLockActor->TryInteractWithParty (PartyPawn);
                }
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

bool UGridActivationComponent::RefreshPressurePlatesAtCell (int32 X, int32 Y)
{
    if (!RuntimeActor)
    {
        return false;
    }

    TArray<int32> PlateIndexes;
    PressurePlateIndexesByCell.MultiFind (FIntPoint (X, Y), PlateIndexes);
    bool bAnyStateChanged = false;

    for (const int32 PlateIndex : PlateIndexes)
    {
        const FGridLevelObjectData* PlateData = GetObjectByIndex (PlateIndex);
        if (!PlateData || !PlateData->ObjectId.IsValid ())
        {
            continue;
        }

        const FGridPressurePlateWeightParams& WeightParams = PlateData->Behavior.PressurePlateWeight;
        const float CurrentItemWeight = RuntimeActor->GetWorldItemWeightAtCell (
            X,
            Y,
            WeightParams.bCountEdgeItems);
        const bool bPartyActivates =
            WeightParams.bActivateWhenPartyPresent &&
            RuntimeActor->IsPartyOnCell (X, Y);
        const bool bWeightActivates =
            WeightParams.bUseItemWeight &&
            CurrentItemWeight >= FMath::Max (0.0f, WeightParams.RequiredItemWeight);
        const bool bShouldBePressed = bPartyActivates || bWeightActivates;
        const bool bWasPressed = ActiveObjectIds.Contains (PlateData->ObjectId);

        if (AGridPressurePlateActor* PlateActor =
            RuntimeActor->FindRuntimeObjectActor<AGridPressurePlateActor> (PlateData->ObjectId))
        {
            PlateActor->SetWeightState (
                CurrentItemWeight,
                WeightParams.RequiredItemWeight,
                WeightParams.bUseItemWeight,
                WeightParams.bActivateWhenPartyPresent);
        }

        if (bWasPressed == bShouldBePressed)
        {
            continue;
        }

        if (bShouldBePressed)
        {
            ActiveObjectIds.Add (PlateData->ObjectId);
        }
        else
        {
            ActiveObjectIds.Remove (PlateData->ObjectId);
        }

        if (AGridPressurePlateActor* PlateActor =
            RuntimeActor->FindRuntimeObjectActor<AGridPressurePlateActor> (PlateData->ObjectId))
        {
            PlateActor->SetPressed (bShouldBePressed);
        }

        const EGridObjectEvent StateEvent = bShouldBePressed
            ? EGridObjectEvent::Activated
            : EGridObjectEvent::Deactivated;
        UE_LOG (LogTemp, Log,
            TEXT ("GridPressurePlate StateChanged Id=%s Cell=(%d,%d) Party=%s ItemWeight=%.2f RequiredWeight=%.2f Pressed=%s"),
            *PlateData->ObjectId.ToString (),
            X,
            Y,
            bPartyActivates ? TEXT ("true") : TEXT ("false"),
            CurrentItemWeight,
            WeightParams.RequiredItemWeight,
            bShouldBePressed ? TEXT ("true") : TEXT ("false"));
        ExecuteLinksFromObjectForEvent (
            PlateData->ObjectId,
            StateEvent);
        bAnyStateChanged = true;
    }

    return bAnyStateChanged;
}

bool UGridActivationComponent::RefreshAllPressurePlates ()
{
    TArray<FIntPoint> PlateCells;
    PressurePlateIndexesByCell.GetKeys (PlateCells);

    TSet<FIntPoint> UniquePlateCells;
    UniquePlateCells.Append (PlateCells);

    bool bAnyStateChanged = false;
    for (const FIntPoint& PlateCell : UniquePlateCells)
    {
        bAnyStateChanged |= RefreshPressurePlatesAtCell (PlateCell.X, PlateCell.Y);
    }
    return bAnyStateChanged;
}

bool UGridActivationComponent::ApplyLinkCommand (const FGridObjectLink& LinkData)
{
    if (!RuntimeActor)
    {
        LogLinkResult (LinkData, LinkData.Command, false, TEXT ("missing runtime actor"));
        return false;
    }

    const FGridLevelObjectData* TargetObject = FindObjectById (LinkData.TargetObjectId);
    if (!TargetObject)
    {
        LogLinkResult (LinkData, LinkData.Command, false, TEXT ("target object not found"));
        return false;
    }

    AActor* SourceActor = RuntimeActor->FindRuntimeObjectActor<AActor> (LinkData.SourceObjectId);
    AActor* TargetActor = RuntimeActor->FindRuntimeObjectActor<AActor> (LinkData.TargetObjectId);
    if (!EvaluateGridObjectLinkCondition (LinkData, SourceActor, TargetActor))
    {
        return false;
    }

    const EGridObjectCommand ResolvedCommand = LinkData.Command;
    bool bSuccess = false;
    const TCHAR* FailureReason = TEXT ("unsupported target type or command");

    if (TargetObject->Type == EGridLevelObjectType::MonsterSpawn)
    {
        switch (ResolvedCommand)
        {
            case EGridObjectCommand::Spawn:
            case EGridObjectCommand::Despawn:
            case EGridObjectCommand::Teleport:
            case EGridObjectCommand::Activate:
            case EGridObjectCommand::Deactivate:
            case EGridObjectCommand::Enable:
            case EGridObjectCommand::Disable:
            case EGridObjectCommand::Toggle:
                bSuccess = RuntimeActor->ExecuteMonsterSpawnCommand (
                    TargetObject->ObjectId,
                    ResolvedCommand);
                break;

            default:
                break;
        }

        if (bSuccess)
        {
            if (RuntimeActor->FindSpawnedMonsterActor (
                    TargetObject->ObjectId))
            {
                ActiveObjectIds.Add (TargetObject->ObjectId);
            }
            else
            {
                ActiveObjectIds.Remove (TargetObject->ObjectId);
            }
        }
        LogLinkResult (
            LinkData,
            ResolvedCommand,
            bSuccess,
            bSuccess ? nullptr : TEXT ("monster lifecycle command failed"));
        return bSuccess;
    }

    if (IsReceptacleCommand (ResolvedCommand))
    {
        bSuccess = ApplyReceptacleLinkCommand (*TargetObject, ResolvedCommand);
        FailureReason = bSuccess ? nullptr : TEXT ("receptacle command failed");
        LogLinkResult (LinkData, ResolvedCommand, bSuccess, FailureReason);
        return bSuccess;
    }

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

bool UGridActivationComponent::EvaluateGridObjectLinkCondition (
    const FGridObjectLink& LinkData,
    AActor* SourceActor,
    AActor* TargetActor) const
{
    if (LinkData.Condition == EGridObjectCondition::None)
    {
        return true;
    }

    const AGridReceptacleActor* ReceptacleActor = Cast<AGridReceptacleActor> (TargetActor);
    if (!ReceptacleActor)
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("Grid link condition rejected: Source=%s SourceActor=%s Target=%s TargetActor=%s Condition=%s Reason=target is not a spawned receptacle"),
            *LinkData.SourceObjectId.ToString (),
            *GetNameSafe (SourceActor),
            *LinkData.TargetObjectId.ToString (),
            *GetNameSafe (TargetActor),
            *GridObjectConditionToString (LinkData.Condition));
        return false;
    }

    const auto RejectMissingConditionParameter = [&LinkData, SourceActor, TargetActor] (const TCHAR* ParameterName)
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("Grid link condition rejected: Source=%s SourceActor=%s Target=%s TargetActor=%s Condition=%s Reason=missing or invalid %s"),
            *LinkData.SourceObjectId.ToString (),
            *GetNameSafe (SourceActor),
            *LinkData.TargetObjectId.ToString (),
            *GetNameSafe (TargetActor),
            *GridObjectConditionToString (LinkData.Condition),
            ParameterName);
        return false;
    };

    bool bConditionResult = false;

    switch (LinkData.Condition)
    {
        case EGridObjectCondition::ReceptacleIsEmpty:
            bConditionResult = ReceptacleActor->IsEmpty ();
            break;

        case EGridObjectCondition::ReceptacleHasAnyItem:
            bConditionResult = ReceptacleActor->HasAnyItem ();
            break;

        case EGridObjectCondition::ReceptacleContainsItemDefinition:
            if (LinkData.ConditionItemDefinitionId.IsNone ())
            {
                return RejectMissingConditionParameter (TEXT ("ConditionItemDefinitionId"));
            }
            bConditionResult = ReceptacleActor->ContainsItemDefinition (LinkData.ConditionItemDefinitionId);
            break;

        case EGridObjectCondition::ReceptacleContainsItemTag:
            if (LinkData.ConditionItemTag.IsNone ())
            {
                return RejectMissingConditionParameter (TEXT ("ConditionItemTag"));
            }
            bConditionResult = ReceptacleActor->ContainsItemTag (LinkData.ConditionItemTag);
            break;

        case EGridObjectCondition::ReceptacleContainsItemType:
            if (LinkData.ConditionItemType == EGridItemType::None)
            {
                return RejectMissingConditionParameter (TEXT ("ConditionItemType"));
            }
            bConditionResult = ReceptacleActor->ContainsItemType (LinkData.ConditionItemType);
            break;

        case EGridObjectCondition::ReceptacleItemCountAtLeast:
            if (LinkData.ConditionCount <= 0)
            {
                return RejectMissingConditionParameter (TEXT ("ConditionCount"));
            }
            bConditionResult = ReceptacleActor->GetContainedItemCount () >= LinkData.ConditionCount;
            break;

        case EGridObjectCondition::ReceptacleWeightAtLeast:
            if (LinkData.ConditionWeight <= 0.0f)
            {
                return RejectMissingConditionParameter (TEXT ("ConditionWeight"));
            }
            bConditionResult = ReceptacleActor->GetContainedTotalWeight () >= LinkData.ConditionWeight;
            break;

        case EGridObjectCondition::None:
        default:
            return false;
    }

    const bool bFinalResult = LinkData.bInvertCondition ? !bConditionResult : bConditionResult;
    if (!bFinalResult)
    {
        UE_LOG (LogTemp, Verbose,
            TEXT ("Grid link condition failed: Source=%s SourceActor=%s Target=%s TargetActor=%s Condition=%s Inverted=%s"),
            *LinkData.SourceObjectId.ToString (),
            *GetNameSafe (SourceActor),
            *LinkData.TargetObjectId.ToString (),
            *GetNameSafe (TargetActor),
            *GridObjectConditionToString (LinkData.Condition),
            LinkData.bInvertCondition ? TEXT ("true") : TEXT ("false"));
    }
    return bFinalResult;
}

bool UGridActivationComponent::ExecuteLinksFromObjectForEvent (
    FGuid SourceObjectId,
    EGridObjectEvent SourceEvent)
{
    return ExecuteLinksFromObjectForEventInternal (SourceObjectId, SourceEvent);
}

bool UGridActivationComponent::ExecuteLinksFromObjectForEventInternal (
    FGuid SourceObjectId,
    EGridObjectEvent SourceEvent)
{
    if (!RuntimeActor || !RuntimeActor->LevelAsset || !SourceObjectId.IsValid ())
    {
        return false;
    }
    if (!FindObjectById (SourceObjectId))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("Grid object event rejected: Source=%s Event=%s Reason=source object not found"),
            *SourceObjectId.ToString (),
            *GridObjectEventToString (SourceEvent));
        return false;
    }
    if (DispatchingSourceObjectIds.Contains (SourceObjectId))
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("Grid object event rejected: Source=%s Event=%s Reason=cyclic link dispatch"),
            *SourceObjectId.ToString (),
            *GridObjectEventToString (SourceEvent));
        return false;
    }

    DispatchingSourceObjectIds.Add (SourceObjectId);
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
        if (LinkData.SourceEvent != SourceEvent)
        {
            continue;
        }

        ++ExecutedLinkCount;
        bAnyApplied |= ApplyLinkCommand (LinkData);
    }

    UE_LOG (LogTemp, Log, TEXT ("Grid object event %s: Event=%s LinksExecuted=%d AnyApplied=%s"),
        *SourceObjectId.ToString (),
        *GridObjectEventToString (SourceEvent),
        ExecutedLinkCount,
        bAnyApplied ? TEXT ("true") : TEXT ("false"));

    DispatchingSourceObjectIds.Remove (SourceObjectId);
    return bAnyApplied;
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

bool UGridActivationComponent::ApplyReceptacleLinkCommand (
    const FGridLevelObjectData& TargetObject,
    EGridObjectCommand Command)
{
    UE_LOG (LogTemp, Log,
        TEXT ("Grid receptacle command received: Command=%s Target=%s TargetType=%s"),
        *GridObjectCommandToString (Command),
        *TargetObject.ObjectId.ToString (),
        *GridObjectTypeToString (TargetObject.Type));

    AGridReceptacleActor* ReceptacleActor = RuntimeActor
        ? RuntimeActor->FindRuntimeObjectActor<AGridReceptacleActor> (TargetObject.ObjectId)
        : nullptr;
    if (!ReceptacleActor)
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("Grid receptacle command failed: Command=%s Target=%s Reason=target is not a receptacle"),
            *GridObjectCommandToString (Command),
            *TargetObject.ObjectId.ToString ());
        return false;
    }

    bool bSuccess = true;
    switch (Command)
    {
        case EGridObjectCommand::ReceptacleConsumeItem:
            if (!ReceptacleActor->HasItem ())
            {
                UE_LOG (LogTemp, Warning,
                    TEXT ("Grid receptacle command failed: Command=%s Target=%s Actor=%s Reason=no item present"),
                    *GridObjectCommandToString (Command),
                    *TargetObject.ObjectId.ToString (),
                    *GetNameSafe (ReceptacleActor));
                return false;
            }
            bSuccess = ReceptacleActor->ConsumeItemAtIndex (0);
            break;

        case EGridObjectCommand::ReceptacleConsumeAllItems:
            if (!ReceptacleActor->HasItem ())
            {
                UE_LOG (LogTemp, Warning,
                    TEXT ("Grid receptacle command failed: Command=%s Target=%s Actor=%s Reason=no item present"),
                    *GridObjectCommandToString (Command),
                    *TargetObject.ObjectId.ToString (),
                    *GetNameSafe (ReceptacleActor));
                return false;
            }
            bSuccess = ReceptacleActor->ConsumeAllItems ();
            break;

        case EGridObjectCommand::ReceptacleEnableRemoval:
            ReceptacleActor->SetCanRemoveItem (true);
            break;

        case EGridObjectCommand::ReceptacleDisableRemoval:
            ReceptacleActor->SetCanRemoveItem (false);
            break;

        default:
            bSuccess = false;
            break;
    }

    if (bSuccess)
    {
        UE_LOG (LogTemp, Log,
            TEXT ("Grid receptacle command result: Command=%s Target=%s Actor=%s Success=true"),
            *GridObjectCommandToString (Command),
            *TargetObject.ObjectId.ToString (),
            *GetNameSafe (ReceptacleActor));
    } else
    {
        UE_LOG (LogTemp, Warning,
            TEXT ("Grid receptacle command result: Command=%s Target=%s Actor=%s Success=false"),
            *GridObjectCommandToString (Command),
            *TargetObject.ObjectId.ToString (),
            *GetNameSafe (ReceptacleActor));
    }
    return bSuccess;
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

    const bool bWasActive = IsTargetActive (TargetObject.ObjectId);
    const bool bStateChanged = bWasActive != bActive;

    if (bActive)
    {
        ActiveObjectIds.Add (TargetObject.ObjectId);
    } else
    {
        ActiveObjectIds.Remove (TargetObject.ObjectId);
    }

    bool bHandledByRuntimeActor = false;
    if (RuntimeActor)
    {
        if (AGridLeverActor* LeverActor = RuntimeActor->FindRuntimeObjectActor<AGridLeverActor> (TargetObject.ObjectId))
        {
            LeverActor->SetLeverState (bActive);
            bHandledByRuntimeActor = true;
        }
        else if (AGridPressurePlateActor* PlateActor = RuntimeActor->FindRuntimeObjectActor<AGridPressurePlateActor> (TargetObject.ObjectId))
        {
            PlateActor->SetPressed (bActive);
            bHandledByRuntimeActor = true;
        }
        else if (bActive)
        {
            if (AGridButtonActor* ButtonActor = RuntimeActor->FindRuntimeObjectActor<AGridButtonActor> (TargetObject.ObjectId))
            {
                ButtonActor->TriggerPress ();
                bHandledByRuntimeActor = true;
            }
        }
        if (!bHandledByRuntimeActor)
        {
            if (AGridMechanismActor* MechanismActor = RuntimeActor->FindRuntimeObjectActor<AGridMechanismActor> (TargetObject.ObjectId))
            {
                UE_LOG (LogTemp, Log, TEXT ("Grid link target %s is generic mechanism %s; state stored but no visual activation handler exists yet."),
                    *TargetObject.ObjectId.ToString (), *MechanismActor->GetName ());
                bHandledByRuntimeActor = true;
            }
        }
    }

    if (!bHandledByRuntimeActor &&
        TargetObject.Type == EGridLevelObjectType::ItemSpawn)
    {
        UE_LOG (LogTemp, Log, TEXT ("Grid link target %s is %s; state stored, spawn behavior TODO."),
            *TargetObject.ObjectId.ToString (), *GridObjectTypeToString (TargetObject.Type));
    }
    else if (!bHandledByRuntimeActor && TargetObject.Type == EGridLevelObjectType::Teleporter)
    {
        UE_LOG (LogTemp, Log, TEXT ("Grid link target %s is Teleporter; state stored, teleport behavior TODO."),
            *TargetObject.ObjectId.ToString ());
    }

    if (bStateChanged &&
        (TargetObject.Type == EGridLevelObjectType::Lever ||
            TargetObject.Type == EGridLevelObjectType::PressurePlate))
    {
        const EGridObjectEvent StateEvent =
            bActive ? EGridObjectEvent::Activated : EGridObjectEvent::Deactivated;
        UE_LOG (LogTemp, Log,
            TEXT ("Grid mechanism state changed by link command: Target=%s Type=%s PreviousActive=%s NewActive=%s Event=%s"),
            *TargetObject.ObjectId.ToString (),
            *GridObjectTypeToString (TargetObject.Type),
            bWasActive ? TEXT ("true") : TEXT ("false"),
            bActive ? TEXT ("true") : TEXT ("false"),
            *GridObjectEventToString (StateEvent));
        ExecuteLinksFromObjectForEvent (TargetObject.ObjectId, StateEvent);
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

    const TCHAR* EventLabel = bEntering ? TEXT ("Enter") : TEXT ("Exit");
    const EGridObjectEvent SourceEvent = bEntering
        ? EGridObjectEvent::Activated
        : EGridObjectEvent::Deactivated;

    UE_LOG (LogTemp, Log, TEXT ("Grid trigger detected: Id=%s Cell=(%d,%d) Event=%s SourceEvent=%s"),
        *TriggerData.ObjectId.ToString (),
        TriggerData.CellX,
        TriggerData.CellY,
        EventLabel,
        *GridObjectEventToString (SourceEvent));

    return ExecuteLinksFromObjectForEvent (
        TriggerData.ObjectId,
        SourceEvent);
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
    const int32 PreviousItemCount = ReceptacleActor->GetContainedItemCount ();
    if (!ReceptacleActor->TryInteractWithParty (PartyPawn))
    {
        return false;
    }
    const int32 CurrentItemCount = ReceptacleActor->GetContainedItemCount ();
    if (CurrentItemCount > 0)
    {
        ActiveObjectIds.Add (ObjectData.ObjectId);
    } else
    {
        ActiveObjectIds.Remove (ObjectData.ObjectId);
    }

    // The receptacle actor emits insertion and removal events when the transfer succeeds.
    return CurrentItemCount != PreviousItemCount;
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
