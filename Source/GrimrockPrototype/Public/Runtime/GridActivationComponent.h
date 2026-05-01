#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/GridTypes.h"
#include "GridActivationComponent.generated.h"

class AGridLevelRuntimeActor;
class AGridButtonActor;
class AGridLeverActor;
class AGridPressurePlateActor;

UCLASS (ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridActivationComponent : public UActorComponent
{
    GENERATED_BODY ()

public:
    UGridActivationComponent ();

    void Initialize (AGridLevelRuntimeActor* InRuntime);
    void ResetRuntimeState ();

    bool TryInteractAtEdge (int32 FromCellX, int32 FromCellY, EGridEdge Edge);
    void HandlePartyCellChanged (int32 OldCellX, int32 OldCellY, int32 NewCellX, int32 NewCellY);

    void RegisterInitialObjectState (const FGridLevelObjectData& ObjectData);

private:
    UPROPERTY (Transient)
    TObjectPtr<AGridLevelRuntimeActor> RuntimeActor;

    UPROPERTY (Transient)
    TSet<FGuid> ActiveObjectIds;

private:
    const FGridLevelObjectData* FindObjectById (FGuid ObjectId) const;
    const FGridLevelObjectData* FindInteractableObjectOnEdge (int32 X, int32 Y, EGridEdge Edge) const;
    const FGridLevelObjectData* FindObjectDataAtCell (EGridLevelObjectType Type, int32 X, int32 Y) const;

    bool ActivateObject (const FGridLevelObjectData& ObjectData);

    bool ExecuteLinksFromObject (FGuid SourceObjectId, bool bInvert);
    bool ApplyLinkAction (const FGridLevelLinkData& LinkData, bool bInvert);
    EGridLinkAction GetResolvedLinkAction (EGridLinkAction Action, bool bInvert) const;

    bool ActivatePressurePlateAtCell (int32 X, int32 Y);
    bool DeactivatePressurePlateAtCell (int32 X, int32 Y);

    void ActivateTriggersAtCell (int32 X, int32 Y);
    void DeactivateTriggersAtCell (int32 X, int32 Y);
};