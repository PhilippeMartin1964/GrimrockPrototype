#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GridLevelAsset.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GridLevelRuntimeActor.generated.h"

class AGridDoorActor;

UCLASS ()
class GRIMROCKPROTOTYPE_API AGridLevelRuntimeActor : public AActor
{
    GENERATED_BODY ()

public:
    AGridLevelRuntimeActor ();

protected:
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USceneComponent* SceneRoot;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UInstancedStaticMeshComponent* FloorISM;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UInstancedStaticMeshComponent* WallISM;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UInstancedStaticMeshComponent* DoorISM;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UInstancedStaticMeshComponent* SecretWallISM;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UInstancedStaticMeshComponent* CeilingISM;

public:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Level")
    TObjectPtr<UGridLevelAsset> LevelAsset;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Meshes")
    TObjectPtr<UStaticMesh> FloorMesh;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Meshes")
    TObjectPtr<UStaticMesh> WallMesh;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Meshes")
    TObjectPtr<UStaticMesh> DoorMesh;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Meshes")
    TObjectPtr<UStaticMesh> SecretWallMesh;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Meshes")
    TObjectPtr<UStaticMesh> CeilingMesh;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Level")
    FVector GridOrigin = FVector::ZeroVector;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Debug")
    bool bRebuildInConstruction = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Runtime|Doors")
    TSubclassOf<AGridDoorActor> DoorActorClass;

public:
    virtual void OnConstruction (const FTransform& Transform) override;

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Level")
    void RebuildLevel ();

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Level")
    void ClearVisuals ();

    UFUNCTION(BlueprintCallable, Category = "Runtime")
    FVector GetCellCenterWorld(int32 X, int32 Y, float ZOffset = 0.f) const;

    UFUNCTION (BlueprintCallable, Category = "Runtime")
    bool IsValidCell (int32 X, int32 Y) const;

    UFUNCTION (BlueprintCallable, Category = "Runtime")
    const FGridLevelCellData& GetCell (int32 X, int32 Y) const;

    UFUNCTION (BlueprintCallable, Category = "Runtime")
    bool IsWalkableCell (int32 X, int32 Y) const;

    UFUNCTION (BlueprintCallable, Category = "Runtime")
    bool TryGetNeighborCell (int32 X, int32 Y, EGridEdge Direction, int32& OutX, int32& OutY) const;

    UFUNCTION (BlueprintCallable, Category = "Runtime")
    EGridWallType GetWallOnEdge (int32 X, int32 Y, EGridEdge Edge) const;

    UFUNCTION (BlueprintCallable, Category = "Runtime")
    bool CanMove (int32 FromX, int32 FromY, EGridEdge Direction) const;

    UFUNCTION (BlueprintCallable, Category = "Runtime|Interaction")
    bool HasDoorOnEdge (int32 X, int32 Y, EGridEdge Edge) const;

    UFUNCTION (BlueprintCallable, Category = "Runtime|Interaction")
    bool IsDoorOpenOnEdge (int32 X, int32 Y, EGridEdge Edge) const;

    UFUNCTION (BlueprintCallable, Category = "Runtime|Interaction")
    bool ToggleDoorOnEdge (int32 X, int32 Y, EGridEdge Edge);

    UFUNCTION (BlueprintCallable, Category = "Runtime|Interaction")
    bool OpenDoorOnEdge (int32 X, int32 Y, EGridEdge Edge);

    UFUNCTION (BlueprintCallable, Category = "Runtime|Interaction")
    bool CloseDoorOnEdge (int32 X, int32 Y, EGridEdge Edge);
	// Butttons and pressure plates could also use the same TryInteractAtEdge function, and then we can have different interaction types in the level data to distinguish them
    UFUNCTION (BlueprintCallable, Category = "Runtime|Interaction")
    bool TryInteractAtEdge (int32 FromCellX, int32 FromCellY, EGridEdge Edge);

protected:
    FVector CellToWorld (int32 X, int32 Y, float ZOffset = 0.f) const;

    void AddFloor (int32 X, int32 Y, float CellSize);
    void AddCeiling (int32 X, int32 Y, float CellSize);
    void AddEdgeInstance (UInstancedStaticMeshComponent* TargetISM, int32 X, int32 Y, EGridEdge Edge, float CellSize);

    virtual void BeginPlay () override;

private:
    UPROPERTY (Transient)
    TArray<TObjectPtr<AGridDoorActor>> SpawnedDoorActors;

    TSet<FString> RuntimeBlockedDoorEdges;

    void ClearRuntimeDoors ();
    void RebuildRuntimeDoors ();
    void AddRuntimeDoorActor (const FGridLevelObjectData& DoorObjectData);

    AGridDoorActor* FindRuntimeDoorActor (int32 X, int32 Y, EGridEdge Edge) const;
    const FGridLevelObjectData* FindDoorObjectData (int32 X, int32 Y, EGridEdge Edge) const;

    void GetEdgeTransform (
        int32 X,
        int32 Y,
        EGridEdge Edge,
        float CellSize,
        FVector& OutWorldLocation,
        FRotator& OutWorldRotation) const;

    FString MakeDoorEdgeKey (int32 X, int32 Y, EGridEdge Edge) const;
    void SetDoorPassageBlocked (int32 X, int32 Y, EGridEdge Edge, bool bBlocked);

    UFUNCTION ()
    void HandleDoorAnimationFinished (int32 X, int32 Y, EGridEdge Edge);
	// Buttons and pressure plates could also use the same HandleInteractionFinished function, and then we can have different interaction types in the level data to distinguish them
    const FGridLevelObjectData* FindInteractableObjectOnEdge (int32 X, int32 Y, EGridEdge Edge) const;
    const FGridLevelObjectData* FindObjectById (FGuid ObjectId) const;

    bool ActivateObject (const FGridLevelObjectData& ObjectData);
    bool ExecuteLinksFromObject (FGuid SourceObjectId);
    bool ApplyLinkAction (const FGridLevelLinkData& LinkData);
};