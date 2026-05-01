#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/GridTypes.h"
#include "GridDoorSystemComponent.generated.h"

class AGridLevelRuntimeActor;
class AGridDoorActor;
class AGridRuntimeObjectActor;

USTRUCT ()
struct FGridDoorEdgeKey
{
    GENERATED_BODY ()

    UPROPERTY ()
    int32 X = INDEX_NONE;

    UPROPERTY ()
    int32 Y = INDEX_NONE;

    UPROPERTY ()
    EGridEdge Edge = EGridEdge::None;

    FGridDoorEdgeKey () = default;

    FGridDoorEdgeKey (int32 InX, int32 InY, EGridEdge InEdge)
        : X (InX)
        , Y (InY)
        , Edge (InEdge)
    {}

    friend bool operator==(const FGridDoorEdgeKey& A, const FGridDoorEdgeKey& B)
    {
        return A.X == B.X && A.Y == B.Y && A.Edge == B.Edge;
    }
};

FORCEINLINE uint32 GetTypeHash (const FGridDoorEdgeKey& Key)
{
    uint32 Hash = GetTypeHash (Key.X);
    Hash = HashCombine (Hash, GetTypeHash (Key.Y));
    Hash = HashCombine (Hash, GetTypeHash (static_cast<uint8>(Key.Edge)));
    return Hash;
}

UCLASS (ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridDoorSystemComponent : public UActorComponent
{
    GENERATED_BODY ()

public:
    UGridDoorSystemComponent ();

    void Initialize (AGridLevelRuntimeActor* InRuntimeActor);
    void ResetRuntimeState ();

    void RegisterDoorObject (const FGridLevelObjectData& ObjectData, AGridRuntimeObjectActor* RuntimeObjectActor);

    bool HasDoorOnEdge (int32 X, int32 Y, EGridEdge Edge) const;
    bool IsDoorOpenOnEdge (int32 X, int32 Y, EGridEdge Edge) const;

    bool ToggleDoorOnEdge (int32 X, int32 Y, EGridEdge Edge);
    bool OpenDoorOnEdge (int32 X, int32 Y, EGridEdge Edge);
    bool CloseDoorOnEdge (int32 X, int32 Y, EGridEdge Edge);

    bool IsDoorPassageBlocked (int32 X, int32 Y, EGridEdge Edge) const;
    void SetDoorPassageBlocked (int32 X, int32 Y, EGridEdge Edge, bool bBlocked);

    UFUNCTION ()
    void HandleDoorAnimationFinished (int32 X, int32 Y, EGridEdge Edge);

    void RebuildIndexes ();

private:
    UPROPERTY (Transient)
    TObjectPtr<AGridLevelRuntimeActor> RuntimeActor;

    UPROPERTY (Transient)
    TSet<FGridDoorEdgeKey> RuntimeBlockedDoorEdges;

private:
    const FGridLevelObjectData* FindDoorObjectDataAtEdge (int32 X, int32 Y, EGridEdge Edge) const;
    AGridDoorActor* FindDoorActorAtEdge (int32 X, int32 Y, EGridEdge Edge) const;

private:
    TMap<FGridDoorEdgeKey, int32> DoorIndexByEdge;
    TMap<FGridDoorEdgeKey, TWeakObjectPtr<AGridDoorActor>> DoorActorByEdge;

    const FGridLevelObjectData* GetDoorObjectByIndex (int32 ObjectIndex) const;
};