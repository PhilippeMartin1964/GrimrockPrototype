#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GridLevelAsset.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GridLevelRuntimeActor.generated.h"

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
    UInstancedStaticMeshComponent* CeilingISM;

public:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Level")
    TObjectPtr<UGridLevelAsset> LevelAsset;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Meshes")
    TObjectPtr<UStaticMesh> FloorMesh;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Meshes")
    TObjectPtr<UStaticMesh> WallMesh;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Meshes")
    TObjectPtr<UStaticMesh> CeilingMesh;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Level")
    FVector GridOrigin = FVector::ZeroVector;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Debug")
    bool bRebuildInConstruction = true;

public:
    virtual void OnConstruction (const FTransform& Transform) override;
    virtual void BeginPlay () override;

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Level")
    void RebuildLevel ();

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Level")
    void ClearVisuals ();

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

    UFUNCTION (BlueprintCallable, Category = "Runtime")
    FVector GetCellCenterWorld (int32 X, int32 Y, float ZOffset = 0.f) const;

protected:
    FVector CellToWorld (int32 X, int32 Y, float ZOffset = 0.f) const;

    void AddFloor (int32 X, int32 Y, float CellSize);
    void AddCeiling (int32 X, int32 Y, float CellSize);
    void AddEdgeInstance (UInstancedStaticMeshComponent* TargetISM, int32 X, int32 Y, EGridEdge Edge, float CellSize);
};