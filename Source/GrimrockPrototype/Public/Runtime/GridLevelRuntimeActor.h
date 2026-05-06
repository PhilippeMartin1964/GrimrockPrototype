#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GridLevelAsset.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GridLevelRuntimeActor.generated.h"

class AGridEditorPreviewObjectActor;
class UGridObjectArchetypeAsset;
class AGridRuntimeObjectActor;
class UGridActivationComponent;
class UGridDoorSystemComponent;
class UGridEditorPreviewComponent;

UENUM ()
enum class EGridRuntimeRebuildMode : uint8
{
    Full,
    GeometryOnly,
    ObjectsOnly
};

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
    TObjectPtr<UGridActivationComponent> ActivationComponent;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UGridDoorSystemComponent> DoorSystemComponent;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UGridEditorPreviewComponent> EditorPreviewComponent;

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

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Editor Preview")
    TObjectPtr<UMaterialInterface> CeilingEditorMaterial;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Editor Preview")
    TSubclassOf<AGridEditorPreviewObjectActor> EditorPreviewObjectActorClass;

    UFUNCTION (BlueprintCallable, Category = "Editor Preview")
    void SetEditorHoveredObject (FGuid ObjectId);

    UFUNCTION (BlueprintCallable, Category = "Editor Preview")
    void SetEditorSelectedObject (FGuid ObjectId);

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Editor Preview")
    void CleanupOrphanEditorPreviewObjects ();

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Level")
    FVector GridOrigin = FVector::ZeroVector;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Debug")
    bool bRebuildInConstruction = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Debug")
    bool bEnableRuntimeDebugLog = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Debug")
    bool bEnableRuntimeDebugScreen = false;

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Debug")
    void LogRuntimeDebugSummary () const;

    UFUNCTION (BlueprintCallable, Category = "Debug")
    FString GetRuntimeDebugSummary () const;

    UFUNCTION (BlueprintCallable, Category = "Debug")
    void ShowRuntimeDebugSummary (float Duration = 3.f) const;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Object Archetypes")
    TArray<TObjectPtr<UGridObjectArchetypeAsset>> ObjectArchetypes;

public:
    virtual void OnConstruction (const FTransform& Transform) override;

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Level")
    void RebuildLevel (EGridRuntimeRebuildMode RebuildMode = EGridRuntimeRebuildMode::Full);

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Level")
    void ClearVisuals (EGridRuntimeRebuildMode RebuildMode = EGridRuntimeRebuildMode::Full);

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
    bool TryInteractAtEdge (int32 FromCellX, int32 FromCellY, EGridEdge Edge, AGrimrockPartyPawn* PartyPawn);

    // Allows runtime objects such as Receptacles to trigger their outgoing links.
    UFUNCTION (BlueprintCallable, Category = "Runtime|Interaction")
    bool ExecuteLinksFromRuntimeObject (FGuid SourceObjectId, bool bInvert = false);
    
    UFUNCTION (BlueprintCallable, Category = "Runtime|Interaction")
    void HandlePartyCellChanged (int32 OldCellX, int32 OldCellY, int32 NewCellX, int32 NewCellY);

    void RebuildRuntimeObjects ();
    void AddRuntimeObjectActor (const FGridLevelObjectData& ObjectData);
    bool IsRuntimeSpawnableObject (const FGridLevelObjectData& ObjectData) const;

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

    UStaticMesh* GetObjectMesh (const FGridLevelObjectData& ObjectData) const;
    UMaterialInterface* GetObjectMaterial (const FGridLevelObjectData& ObjectData) const;
    bool GetObjectPlacementTransform (const FGridLevelObjectData& ObjectData, FTransform& OutTransform) const;

protected:
    FVector CellToWorld (int32 X, int32 Y, float ZOffset = 0.f) const;

    void AddFloor (int32 X, int32 Y, float CellSize);
    void AddCeiling (int32 X, int32 Y, float CellSize);
    void AddEdgeInstance (UInstancedStaticMeshComponent* TargetISM, int32 X, int32 Y, EGridEdge Edge, float CellSize);

    virtual void BeginPlay () override;

private:
    void GetEdgeTransform (int32 X, int32 Y, EGridEdge Edge, float CellSize, FVector& OutWorldLocation, FRotator& OutWorldRotation) const;

    TSubclassOf<AGridRuntimeObjectActor> GetObjectRuntimeActorClass (const FGridLevelObjectData& ObjectData) const;

    bool GetWallMountedObjectTransform (const FGridLevelObjectData& ObjectData, float ZOffset, float WallInset,
        float LocalOffsetAlongWall, float LocalOffsetVertical, FTransform& OutTransform) const;
    bool GetCenteredObjectTransform (const FGridLevelObjectData& ObjectData, float ZOffset, FTransform& OutTransform) const;

    const UGridObjectArchetypeAsset* FindObjectArchetype (FName ArchetypeId) const;

    UPROPERTY (Transient)
    TMap<FGuid, TObjectPtr<AGridRuntimeObjectActor>> SpawnedRuntimeObjectActors;

    void RegisterRuntimeObjectActor (const FGuid& ObjectId, AGridRuntimeObjectActor* Actor);
    void ClearRuntimeObjectActors ();

    template<typename TActor>TActor* SpawnRuntimeObjectActor (
        const FGridLevelObjectData& ObjectData, UStaticMesh*& OutMesh, UMaterialInterface*& OutMaterial, FTransform& OutTransform)
    {
        static_assert (TIsDerivedFrom<TActor, AGridRuntimeObjectActor>::IsDerived,  "TActor must derive from AGridRuntimeObjectActor");
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