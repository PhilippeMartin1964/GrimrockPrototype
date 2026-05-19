#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/GridTypes.h"
#include "GridActivationComponent.generated.h"

class AGridLevelRuntimeActor;
class AGrimrockPartyPawn;

UCLASS (ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridActivationComponent : public UActorComponent
{
    GENERATED_BODY ()

public:
    UGridActivationComponent ();

    void Initialize (AGridLevelRuntimeActor* InRuntime);
    void ResetRuntimeState ();

    //bool TryInteractAtEdge (int32 FromCellX, int32 FromCellY, EGridEdge Edge);
    bool TryInteractAtEdge (int32 FromCellX, int32 FromCellY, EGridEdge Edge, AGrimrockPartyPawn* PartyPawn);
    void HandlePartyCellChanged (int32 OldCellX, int32 OldCellY, int32 NewCellX, int32 NewCellY);
    void NotifyPawnEnteredCell (int32 CellX, int32 CellY);
    void NotifyPawnExitedCell (int32 CellX, int32 CellY);
    bool ExecuteLinksFromObject (FGuid SourceObjectId, bool bInvert);

    void RegisterInitialObjectState (const FGridLevelObjectData& ObjectData);

    void RebuildIndexes ();

	// Debugging
    FString GetDebugSummary () const;
    void LogDebugSummary () const;

private:
    UPROPERTY (Transient)
    TObjectPtr<AGridLevelRuntimeActor> RuntimeActor;

    UPROPERTY (Transient)
    TSet<FGuid> ActiveObjectIds;

    UPROPERTY (Transient)
    TSet<FGuid> ConsumedOneShotTriggerIds;

private:
    const FGridLevelObjectData* FindObjectById (FGuid ObjectId) const;
    const FGridLevelObjectData* FindInteractableObjectOnEdge (int32 X, int32 Y, EGridEdge Edge) const;

    bool ActivateObject (const FGridLevelObjectData& ObjectData, AGrimrockPartyPawn* PartyPawn);

    bool ApplyLinkAction (const FGridObjectLink& LinkData, bool bInvert);
    bool ExecuteLinksFromObjectForEvent (FGuid SourceObjectId, EGridObjectEvent SourceEvent, bool bInvert, bool bAllowActivatedFallback);
    int32 CountLinksFromObjectForEvent (FGuid SourceObjectId, EGridObjectEvent SourceEvent) const;
    EGridObjectCommand GetResolvedLinkAction (EGridObjectCommand Action, bool bInvert) const;
    bool ApplyDoorLinkAction (const FGridLevelObjectData& TargetObject, EGridObjectCommand Action);
    bool ApplyStatefulLinkAction (const FGridLevelObjectData& TargetObject, EGridObjectCommand Action);
    bool SetTargetActiveState (const FGridLevelObjectData& TargetObject, bool bActive);
    bool IsTargetActive (FGuid ObjectId) const;
    void LogLinkResult (const FGridObjectLink& LinkData, EGridObjectCommand ResolvedAction, bool bSuccess, const TCHAR* FailureReason) const;

    bool ActivatePressurePlateAtCell (int32 X, int32 Y);
    bool DeactivatePressurePlateAtCell (int32 X, int32 Y);

    bool ProcessTriggersAtCell (int32 X, int32 Y, bool bEntering);
    bool ProcessTriggerEvent (const FGridLevelObjectData& TriggerData, bool bEntering);

private:
    TMap<FGuid, int32> ObjectIndexById;
    TMultiMap<FGuid, int32> LinkIndexesBySource;
    TMap<FGridEdgeKey, int32> InteractableObjectIndexByEdge;
    TMultiMap<FIntPoint, int32> PressurePlateIndexesByCell;
    TMultiMap<FIntPoint, int32> TriggerIndexesByCell;
    TMap<FGuid, FGridObjectBehaviorParams> RuntimeBehaviorByObjectId;

    const FGridLevelObjectData* GetObjectByIndex (int32 ObjectIndex) const;
    const FGridObjectBehaviorParams& GetRuntimeBehavior (const FGridLevelObjectData& ObjectData) const;

    int32 GetIndexedObjectCount () const
    {
        return ObjectIndexById.Num ();
    }
    int32 GetIndexedLinkCount () const
    {
        return LinkIndexesBySource.Num ();
    }
    int32 GetActiveObjectCount () const
    {
        return ActiveObjectIds.Num ();
    }
    bool ActivateReadableObject (const FGridLevelObjectData& ObjectData);
    bool ActivateReceptacle (const FGridLevelObjectData& ObjectData, AGrimrockPartyPawn* PartyPawn);
};
