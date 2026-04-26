#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GridLevelAsset.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GridLevelRuntimeActor.generated.h"

class AGridDoorActor;
class AGridButtonActor;
class AGridLeverActor;
class AGridPressurePlateActor;

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

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UInstancedStaticMeshComponent* ButtonISM;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UInstancedStaticMeshComponent* LeverISM;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UInstancedStaticMeshComponent* PressurePlateISM;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UInstancedStaticMeshComponent* EditorSolidBlockISM;

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

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Meshes")
    TObjectPtr<UMaterialInterface> CeilingMaterial;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Editor Preview")
    TObjectPtr<UMaterialInterface> CeilingEditorMaterial;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Editor Preview")
    TObjectPtr<UStaticMesh> EditorSolidBlockMesh;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Editor Preview")
    TObjectPtr<UMaterialInterface> EditorSolidBlockMaterial;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Editor Preview")
    bool bShowEditorSolidBlocks = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Editor Preview")
    float EditorSolidBlockHeight = 300.f;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Meshes")
    TObjectPtr<UStaticMesh> ButtonMesh;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Level")
    FVector GridOrigin = FVector::ZeroVector;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Debug")
    bool bRebuildInConstruction = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Runtime|Doors")
    TSubclassOf<AGridDoorActor> DoorActorClass;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Runtime|Buttons")
    TSubclassOf<AGridButtonActor> ButtonActorClass;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Meshes")
    TObjectPtr<UMaterialInterface> ButtonMaterial;

    //Lever and PressurePlate 
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Runtime|Levers")
    TSubclassOf<AGridLeverActor> LeverActorClass;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Runtime|PressurePlates")
    TSubclassOf<AGridPressurePlateActor> PressurePlateActorClass;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Meshes")
    TObjectPtr<UStaticMesh> LeverMesh;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Meshes")
    TObjectPtr<UStaticMesh> PressurePlateMesh;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Meshes")
    TObjectPtr<UMaterialInterface> LeverMaterial;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Meshes")
    TObjectPtr<UMaterialInterface> PressurePlateMaterial;

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

    UFUNCTION (BlueprintCallable, Category = "Runtime|Interaction")
    bool TryInteractAtEdge (int32 FromCellX, int32 FromCellY, EGridEdge Edge);
    
    UFUNCTION (BlueprintCallable, Category = "Runtime|Interaction")
    void HandlePartyCellChanged (int32 OldCellX, int32 OldCellY, int32 NewCellX, int32 NewCellY);

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

    const FGridLevelObjectData* FindInteractableObjectOnEdge (int32 X, int32 Y, EGridEdge Edge) const;
    const FGridLevelObjectData* FindObjectById (FGuid ObjectId) const;

    bool ActivateObject (const FGridLevelObjectData& ObjectData);
    bool ApplyLinkAction (const FGridLevelLinkData& LinkData);
    bool ApplyLinkAction (const FGridLevelLinkData& LinkData, bool bInvert);

    UPROPERTY (Transient)
    TArray<TObjectPtr<AGridButtonActor>> SpawnedButtonActors;

    void ClearRuntimeButtons ();
    void RebuildRuntimeButtons ();
    void AddRuntimeButtonActor (const FGridLevelObjectData& ButtonObjectData);

    AGridButtonActor* FindRuntimeButtonActor (int32 X, int32 Y, EGridEdge Edge) const;

    // PressurePlate and Levers
    EGridLinkAction GetResolvedLinkAction (EGridLinkAction Action, bool bInvert) const;

    const FGridLevelObjectData* FindPressurePlateObjectAtCell (int32 X, int32 Y) const;

    bool ExecuteLinksFromObject (FGuid SourceObjectId, bool bInvert);
    bool ActivatePressurePlateAtCell (int32 X, int32 Y);
    bool DeactivatePressurePlateAtCell (int32 X, int32 Y);
    void AddEditorButtonInstance (const FGridLevelObjectData& ButtonObjectData);
    void AddEditorLeverInstance (const FGridLevelObjectData& LeverObjectData);
    void AddEditorPressurePlateInstance (const FGridLevelObjectData& PlateObjectData);
    void AddEditorDoorInstance (const FGridLevelObjectData& DoorObjectData);

    UPROPERTY (Transient)
    TArray<TObjectPtr<AGridLeverActor>> SpawnedLeverActors;

    UPROPERTY (Transient)
    TArray<TObjectPtr<AGridPressurePlateActor>> SpawnedPressurePlateActors;

    UPROPERTY (Transient)
    TSet<FGuid> ActiveObjectIds;

    void ClearRuntimeLevers ();
    void RebuildRuntimeLevers ();
    void AddRuntimeLeverActor (const FGridLevelObjectData& LeverObjectData);

    void ClearRuntimePressurePlates ();
    void RebuildRuntimePressurePlates ();
    void AddRuntimePressurePlateActor (const FGridLevelObjectData& PlateObjectData);

    AGridLeverActor* FindRuntimeLeverActor (int32 X, int32 Y, EGridEdge Edge) const;
    AGridPressurePlateActor* FindRuntimePressurePlateActor (int32 X, int32 Y) const;

    bool TryGetWallObjectPreviewTransform (
        int32 CellX,
        int32 CellY,
        EGridEdge Edge,
        float ZOffset,
        float WallInset,
        FTransform& OutTransform) const;

    bool TryGetCenteredCellPreviewTransform (
        int32 CellX,
        int32 CellY,
        float ZOffset,
        const FVector& Scale,
        FTransform& OutTransform) const;

    void AddEditorObjectInstance (
        UInstancedStaticMeshComponent* TargetISM,
        const FTransform& InstanceTransform);
};