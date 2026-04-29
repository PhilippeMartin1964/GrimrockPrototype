#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GridTypes.h"
#include "GridRuntimeObjectActor.generated.h"

class UStaticMeshComponent;

UCLASS ()
class GRIMROCKPROTOTYPE_API AGridRuntimeObjectActor : public AActor
{
    GENERATED_BODY ()

public:
    AGridRuntimeObjectActor ();

public:
    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> MeshComponent;

    UPROPERTY (BlueprintReadOnly, Category = "Grid")
    FGuid ObjectId;

    UPROPERTY (BlueprintReadOnly, Category = "Grid")
    EGridLevelObjectType ObjectType = EGridLevelObjectType::None;

    UPROPERTY (BlueprintReadOnly, Category = "Grid")
    int32 CellX = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Grid")
    int32 CellY = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Grid")
    EGridEdge Edge = EGridEdge::None;

public:
    UFUNCTION (BlueprintCallable, Category = "Grid")
    virtual void InitializeGridObjectBase (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh,
        UMaterialInterface* Material, const FVector& WorldLocation, const FRotator& WorldRotation);

    UFUNCTION (BlueprintCallable, Category = "Grid")
    virtual void InitializeGridObject (const FGridLevelObjectData& ObjectData, UStaticMesh* Mesh,
        UMaterialInterface* Material, const FTransform& WorldTransform);

    UFUNCTION (BlueprintCallable, Category = "Grid")
    bool MatchesObjectId (FGuid InObjectId) const;

    UFUNCTION (BlueprintCallable, Category = "Grid")
    bool MatchesCell (int32 InCellX, int32 InCellY) const;

    UFUNCTION (BlueprintCallable, Category = "Grid")
    bool MatchesEdge (int32 InCellX, int32 InCellY, EGridEdge InEdge) const;
};