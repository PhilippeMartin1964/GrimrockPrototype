#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GridDungeonAsset.h"
#include "Core/GridLevelAsset.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Runtime/GridGenericObjectActor.h"
#include "GridLevelRuntimeActor.generated.h"

class AGridEditorPreviewObjectActor;
class UGridObjectArchetypeAsset;
class AGridRuntimeObjectActor;
class AGridItemActor;
class UGridActivationComponent;
class UGridDoorSystemComponent;
class UGridEditorPreviewComponent;
class UReadableMessageWidget;
class UUserWidget;

UENUM ()
enum class EGridRuntimeRebuildMode : uint8
{
    Full,
    GeometryOnly,
    ObjectsOnly
};

USTRUCT ()
struct FGridSpawnedItemRuntimeEntry
{
    GENERATED_BODY ()

    UPROPERTY (Transient)
    FIntPoint Cell = FIntPoint::ZeroValue;

    UPROPERTY (Transient)
    TObjectPtr<AGridItemActor> ItemActor;

    UPROPERTY (Transient)
    FName ItemArchetypeId = NAME_None;
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

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Dungeon")
    TObjectPtr<UGridDungeonAsset> DungeonAsset;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Dungeon")
    FName CurrentDungeonLevelId = NAME_None;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Dungeon|Runtime")
    bool bIsExecutingDungeonTransition = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Meshes")
    TObjectPtr<UStaticMesh> FloorMesh;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Meshes")
    TObjectPtr<UStaticMesh> WallMesh;

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

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Runtime|Start")
    bool bApplyLevelStartOnBeginPlay = true;

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

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Level|Diagnostics")
    void LogLevelAssetDiagnostics () const;

    UFUNCTION (BlueprintCallable, Category = "Level|Diagnostics")
    FString GetLevelAssetDiagnostics () const;

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Runtime|Diagnostics")
    void LogPIEReadinessDiagnostics () const;

    UFUNCTION (BlueprintCallable, Category = "Runtime|Diagnostics")
    FString GetPIEReadinessDiagnostics () const;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Object Archetypes")
    TArray<TObjectPtr<UGridObjectArchetypeAsset>> ObjectArchetypes;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UReadableMessageWidget> ReadableMessageWidgetClass;

    UFUNCTION (BlueprintCallable, Category = "UI")
    void ShowReadableMessage (const FText& MessageText);

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "UI", meta = (ClampMin = "0.1"))
    float ReadableMessageDuration = 4.0f;

    UFUNCTION (BlueprintCallable, Category = "UI")
    void HideReadableMessage ();

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

    UFUNCTION (BlueprintCallable, Category = "Runtime|Interaction")
    bool TryPickupItemAtCell (int32 CellX, int32 CellY, AGrimrockPartyPawn* PartyPawn);

    UFUNCTION (BlueprintCallable, Category = "Runtime|Interaction")
    bool TryPickupItemActor (AGridItemActor* ItemActor, AGrimrockPartyPawn* PartyPawn);

    // Allows runtime objects such as Receptacles to trigger their outgoing links.
    UFUNCTION (BlueprintCallable, Category = "Runtime|Interaction")
    bool ExecuteLinksFromRuntimeObject (FGuid SourceObjectId, EGridObjectEvent SourceEvent);
    
    UFUNCTION (BlueprintCallable, Category = "Runtime|Interaction")
    void HandlePartyCellChanged (int32 OldCellX, int32 OldCellY, int32 NewCellX, int32 NewCellY);

    UFUNCTION (BlueprintCallable, Category = "Runtime|Interaction")
    void NotifyPawnEnteredCell (int32 CellX, int32 CellY);

    UFUNCTION (BlueprintCallable, Category = "Runtime|Interaction")
    void NotifyPawnExitedCell (int32 CellX, int32 CellY);

    UFUNCTION (BlueprintCallable, Category = "Dungeon|Runtime")
    bool TravelToDungeonLevel (FName TargetLevelId, int32 TargetCellX, int32 TargetCellY, EGridEdge TargetFacing, AGrimrockPartyPawn* PartyPawn);

    UFUNCTION (BlueprintCallable, Category = "Dungeon|Runtime")
    bool TryExecuteTransitionAtCell (int32 CellX, int32 CellY, AGrimrockPartyPawn* PartyPawn, bool bTriggeredByUseAction);

    UFUNCTION (BlueprintCallable, Category = "Dungeon|Runtime")
    bool FindTransitionAtCell (int32 CellX, int32 CellY, bool bTriggeredByUseAction, FGridObjectTransitionParams& OutTransition) const;

    void RebuildRuntimeObjects ();
    void AddRuntimeObjectActor (const FGridLevelObjectData& ObjectData);
    void AddPlacedItemActor (const FGridLevelObjectData& ObjectData);
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
    const UGridObjectArchetypeAsset* FindObjectArchetype (FName ArchetypeId) const;
    AGridItemActor* SpawnItemActorForArchetype (FName ItemArchetypeId, AActor* OwnerActor, USceneComponent* AttachParent) const;

protected:
    FVector CellToWorld (int32 X, int32 Y, float ZOffset = 0.f) const;

    void AddFloor (int32 X, int32 Y, float CellSize);
    void AddCeiling (int32 X, int32 Y, float CellSize);
    void AddEdgeInstance (UInstancedStaticMeshComponent* TargetISM, int32 X, int32 Y, EGridEdge Edge, float CellSize);
    bool ShouldSuppressStandardWallForEdge (int32 X, int32 Y, EGridEdge Edge) const;

    virtual void BeginPlay () override;

private:
    void GetEdgeTransform (int32 X, int32 Y, EGridEdge Edge, float CellSize, FVector& OutWorldLocation, FRotator& OutWorldRotation) const;
    bool TryGetOppositeEdge (int32 X, int32 Y, EGridEdge Edge, int32& OutX, int32& OutY, EGridEdge& OutEdge) const;
    bool TryResolveDoorEdge (int32 X, int32 Y, EGridEdge Edge, int32& OutX, int32& OutY, EGridEdge& OutEdge, bool& bOutResolvedOpposite) const;

    TSubclassOf<AGridRuntimeObjectActor> GetObjectRuntimeActorClass (const FGridLevelObjectData& ObjectData) const;

    bool GetWallMountedObjectTransform (const FGridLevelObjectData& ObjectData, float ZOffset, float WallInset,
        float LocalOffsetAlongWall, float LocalOffsetVertical, FTransform& OutTransform) const;
    bool GetFloorEdgeObjectTransform (const FGridLevelObjectData& ObjectData, float ZOffset, float EdgeInset,
        FTransform& OutTransform) const;
    bool GetCenteredObjectTransform (const FGridLevelObjectData& ObjectData, float ZOffset, FTransform& OutTransform) const;

    UPROPERTY (Transient)
    TMap<FGuid, TObjectPtr<AGridRuntimeObjectActor>> SpawnedRuntimeObjectActors;

    UPROPERTY (Transient)
    TArray<TObjectPtr<AGridItemActor>> SpawnedItemActors;

    UPROPERTY (Transient)
    TArray<FGridSpawnedItemRuntimeEntry> SpawnedItemEntries;

    UPROPERTY (Transient)
    TObjectPtr<UReadableMessageWidget> ActiveReadableMessageWidget;

    FTimerHandle ReadableMessageTimerHandle;

    static bool IsSafeRuntimeRenderTransform (const FTransform& Transform);
    void LogUnsafeInstanceTransform (const TCHAR* FunctionName, const UInstancedStaticMeshComponent* Component,
        int32 X, int32 Y, EGridEdge Edge, const FTransform& Transform) const;
    void LogUnsafeObjectTransform (const TCHAR* FunctionName, const FGridLevelObjectData& ObjectData,
        const UStaticMesh* StaticMesh, const FTransform& Transform) const;
    void LogUnsafeItemTransform (const TCHAR* FunctionName, FName ArchetypeId, const AActor* OwnerActor,
        const USceneComponent* AttachParent, const UStaticMesh* StaticMesh, const FTransform& Transform) const;

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
        if (!IsSafeRuntimeRenderTransform (OutTransform))
        {
            LogUnsafeObjectTransform (TEXT ("SpawnRuntimeObjectActor"), ObjectData, OutMesh, OutTransform);
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
