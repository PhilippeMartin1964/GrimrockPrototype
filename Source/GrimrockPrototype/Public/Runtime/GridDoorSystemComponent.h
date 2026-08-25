#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/GridTypes.h"
#include "GridDoorSystemComponent.generated.h"

class AGridLevelRuntimeActor;
class AGridDoorActor;
class AGridRuntimeObjectActor;

UCLASS(ClassGroup = (Grid), meta = (BlueprintSpawnableComponent))
class GRIMROCKPROTOTYPE_API UGridDoorSystemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGridDoorSystemComponent();

	void Initialize(AGridLevelRuntimeActor* InRuntimeActor);
	void ResetRuntimeState();

	void RegisterDoorObject(const FGridLevelObjectData& ObjectData, AGridRuntimeObjectActor* RuntimeObjectActor);

	bool HasDoorOnEdge(int32 X, int32 Y, EGridEdge Edge) const;
	bool IsDoorOpenOnEdge(int32 X, int32 Y, EGridEdge Edge) const;

	bool ToggleDoorOnEdge(int32 X, int32 Y, EGridEdge Edge);
	bool OpenDoorOnEdge(int32 X, int32 Y, EGridEdge Edge);
	bool CloseDoorOnEdge(int32 X, int32 Y, EGridEdge Edge);

	bool IsDoorPassageBlocked(int32 X, int32 Y, EGridEdge Edge) const;
	void SetDoorPassageBlocked(int32 X, int32 Y, EGridEdge Edge, bool bBlocked);
	bool GetDoorState(FGuid ObjectId, bool& bOutOpen, bool& bOutMoving, bool& bOutBlocked) const;
	bool ApplyDoorState(FGuid ObjectId, bool bOpen, bool bBlocked);

	UFUNCTION()
	void HandleDoorAnimationFinished(int32 X, int32 Y, EGridEdge Edge);

	void RebuildIndexes();

	FString GetDebugSummary() const;
	void LogDebugSummary() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<AGridLevelRuntimeActor> RuntimeActor;

	const FGridLevelObjectData* FindDoorObjectDataAtEdge(int32 X, int32 Y, EGridEdge Edge) const;
	AGridDoorActor* FindDoorActorAtEdge(int32 X, int32 Y, EGridEdge Edge) const;

	const FGridLevelObjectData* GetDoorObjectByIndex(int32 ObjectIndex) const;

	TSet<FGridEdgeKey> RuntimeBlockedDoorEdges;
	TMap<FGridEdgeKey, int32> DoorIndexByEdge;
	TMap<FGridEdgeKey, TWeakObjectPtr<AGridDoorActor>> DoorActorByEdge;
};
