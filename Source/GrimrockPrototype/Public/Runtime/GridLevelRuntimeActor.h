#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GridLevelAsset.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GridLevelRuntimeActor.generated.h"

class AGridEditorPreviewObjectActor;
class UGridObjectArchetypeAsset;
class AGridRuntimeObjectActor;

USTRUCT ()
struct FGridObjectEdgeKey
{
    GENERATED_BODY ()

    UPROPERTY ()
    int32 X = INDEX_NONE;

    UPROPERTY ()
    int32 Y = INDEX_NONE;

    UPROPERTY ()
    EGridEdge Edge = EGridEdge::None;

    FGridObjectEdgeKey () = default;

    FGridObjectEdgeKey (int32 InX, int32 InY, EGridEdge InEdge) : X (InX), Y (InY), Edge (InEdge)
    {}

    friend bool operator== (
        const FGridObjectEdgeKey& A,
        const FGridObjectEdgeKey& B)
    {
        return A.X == B.X && A.Y == B.Y && A.Edge == B.Edge;
    }
};

FORCEINLINE uint32 GetTypeHash (const FGridObjectEdgeKey& Key)
{
    uint32 Hash = GetTypeHash (Key.X);
    Hash = HashCombine (Hash, GetTypeHash (Key.Y));
    Hash = HashCombine (Hash, GetTypeHash (static_cast<uint8> (Key.Edge)));
    return Hash;
}

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
    UInstancedStaticMeshComponent* SecretWallISM;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UInstancedStaticMeshComponent* CeilingISM;
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

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Editor Preview")
    TSubclassOf<AGridEditorPreviewObjectActor> EditorPreviewObjectActorClass;

    UFUNCTION (BlueprintCallable, Category = "Editor Preview")
    void SetEditorHoveredObject (FGuid ObjectId);

    UFUNCTION (BlueprintCallable, Category = "Editor Preview")
    void SetEditorSelectedObject (FGuid ObjectId);

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Level")
    FVector GridOrigin = FVector::ZeroVector;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Debug")
    bool bRebuildInConstruction = true;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Object Archetypes")
    TArray<TObjectPtr<UGridObjectArchetypeAsset>> ObjectArchetypes;


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

    void RebuildRuntimeObjects ();
    void AddRuntimeObjectActor (const FGridLevelObjectData& ObjectData);
    bool IsRuntimeSpawnableObject (const FGridLevelObjectData& ObjectData) const;

protected:
    FVector CellToWorld (int32 X, int32 Y, float ZOffset = 0.f) const;

    void AddFloor (int32 X, int32 Y, float CellSize);
    void AddCeiling (int32 X, int32 Y, float CellSize);
    void AddEdgeInstance (UInstancedStaticMeshComponent* TargetISM, int32 X, int32 Y, EGridEdge Edge, float CellSize);

    virtual void BeginPlay () override;

private:
    UPROPERTY (Transient)
    TSet<FGridObjectEdgeKey> RuntimeBlockedDoorEdges;

    void GetEdgeTransform (int32 X, int32 Y, EGridEdge Edge, float CellSize, FVector& OutWorldLocation, FRotator& OutWorldRotation) const;

    void SetDoorPassageBlocked (int32 X, int32 Y, EGridEdge Edge, bool bBlocked);

    UFUNCTION ()
    void HandleDoorAnimationFinished (int32 X, int32 Y, EGridEdge Edge);
    
    TSubclassOf<AGridRuntimeObjectActor> GetObjectRuntimeActorClass (const FGridLevelObjectData& ObjectData) const;

    const FGridLevelObjectData* FindInteractableObjectOnEdge (int32 X, int32 Y, EGridEdge Edge) const;
    const FGridLevelObjectData* FindObjectById (FGuid ObjectId) const;

    bool ActivateObject (const FGridLevelObjectData& ObjectData);
    bool ApplyLinkAction (const FGridLevelLinkData& LinkData);
    bool ApplyLinkAction (const FGridLevelLinkData& LinkData, bool bInvert);

    EGridLinkAction GetResolvedLinkAction (EGridLinkAction Action, bool bInvert) const;

    bool ExecuteLinksFromObject (FGuid SourceObjectId, bool bInvert);
    bool ActivatePressurePlateAtCell (int32 X, int32 Y);
    bool DeactivatePressurePlateAtCell (int32 X, int32 Y);

    UPROPERTY (Transient)
    TSet<FGuid> ActiveObjectIds;

    bool GetObjectPlacementTransform (const FGridLevelObjectData& ObjectData, FTransform& OutTransform) const;
    bool GetWallMountedObjectTransform (const FGridLevelObjectData& ObjectData, float ZOffset, float WallInset, FTransform& OutTransform) const;
    bool GetCenteredObjectTransform (const FGridLevelObjectData& ObjectData, float ZOffset, FTransform& OutTransform) const;

    UPROPERTY (Transient)
    TArray<TObjectPtr<AGridEditorPreviewObjectActor>> SpawnedEditorPreviewObjects;

    FGuid CurrentHoveredEditorObjectId;
    FGuid CurrentSelectedEditorObjectId;

    void ClearEditorPreviewObjects ();
    void RebuildEditorPreviewObjects ();
    void AddEditorPreviewObject (const FGridLevelObjectData& ObjectData);

    const UGridObjectArchetypeAsset* FindObjectArchetype (FName ArchetypeId) const;
    UStaticMesh* GetObjectMesh (const FGridLevelObjectData& ObjectData) const;
    UMaterialInterface* GetObjectMaterial (const FGridLevelObjectData& ObjectData) const; 

    UPROPERTY (Transient)
    TMap<FGuid, TObjectPtr<AGridRuntimeObjectActor>> SpawnedRuntimeObjectActors;

    void RegisterRuntimeObjectActor (const FGuid& ObjectId, AGridRuntimeObjectActor* Actor);
    void ClearRuntimeObjectActors ();

    bool IsEditorPreviewableObject (const FGridLevelObjectData& ObjectData) const;

    template<typename T>
    T* FindRuntimeObjectActor (const FGuid& ObjectId) const
    {
        if (!ObjectId.IsValid ())
        {
            return nullptr;
        }

        if (const TObjectPtr<AGridRuntimeObjectActor>* ActorPtr = SpawnedRuntimeObjectActors.Find (ObjectId))
        {
            return Cast<T> (ActorPtr->Get ());
        }

        return nullptr;
    }

    template<typename TActor>
    TActor* FindRuntimeActorForObjectAtEdge (EGridLevelObjectType Type, int32 X, int32 Y, EGridEdge Edge) const
    {
        const FGridLevelObjectData* ObjectData = FindObjectDataAtEdge (Type, X, Y, Edge);

        return ObjectData ? FindRuntimeObjectActor<TActor> (ObjectData->ObjectId)
            : nullptr;
    }

    template<typename TActor>
    TActor* FindRuntimeActorForObjectAtCell (EGridLevelObjectType Type, int32 X, int32 Y) const
    {
        const FGridLevelObjectData* ObjectData = FindObjectDataAtCell (Type, X, Y);

        return ObjectData ? FindRuntimeObjectActor<TActor> (ObjectData->ObjectId)
            : nullptr;
    }

    const FGridLevelObjectData* FindObjectDataAtEdge (EGridLevelObjectType Type, int32 X, int32 Y, EGridEdge Edge) const;
    const FGridLevelObjectData* FindObjectDataAtCell (EGridLevelObjectType Type, int32 X, int32 Y) const;

    template<typename TActor>TActor* SpawnRuntimeObjectActor (
        const FGridLevelObjectData& ObjectData, UStaticMesh*& OutMesh, UMaterialInterface*& OutMaterial, FTransform& OutTransform)
    {
        static_assert (TIsDerivedFrom<TActor, AGridRuntimeObjectActor>::IsDerived,
                       "TActor must derive from AGridRuntimeObjectActor");
        OutMesh = nullptr;
        OutMaterial = nullptr;
        OutTransform = FTransform::Identity;

        if (!LevelAsset)
        {
            return nullptr;
        }
        OutMesh = GetObjectMesh (ObjectData);
        OutMaterial = GetObjectMaterial (ObjectData);
        TSubclassOf<AGridRuntimeObjectActor> ActorClass = GetObjectRuntimeActorClass (ObjectData);

        if (!ActorClass || !OutMesh)
        {
            return nullptr;
        }
        UWorld* World = GetWorld ();
        if (!World)
        {
            return nullptr;
        }
        if (!GetObjectPlacementTransform (ObjectData, OutTransform))
        {
            return nullptr;
        }
        FActorSpawnParameters Params;
        Params.Owner = this;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        TActor* Actor = World->SpawnActor<TActor> (
            ActorClass, OutTransform.GetLocation (), OutTransform.GetRotation ().Rotator (), Params);
        if (!Actor)
        {
            return nullptr;
        }
        RegisterRuntimeObjectActor (ObjectData.ObjectId, Actor);
        return Actor;
    }
};