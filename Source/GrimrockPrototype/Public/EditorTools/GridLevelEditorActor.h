#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GridLevelAsset.h"
#include "GridLevelEditorActor.generated.h"

class AGridLevelRuntimeActor;

UENUM (BlueprintType)
enum class EGridEditorObjectPlacementPolicy : uint8
{
    ReplaceSameSlotOnly UMETA (DisplayName = "Replace Same Slot Only"),
    RemoveAllAtSlot     UMETA (DisplayName = "Remove All At Slot")
};

UCLASS ()
class GRIMROCKPROTOTYPE_API AGridLevelEditorActor : public AActor
{
    GENERATED_BODY ()

public:
    AGridLevelEditorActor ();

public:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Editor")
    TObjectPtr<UGridLevelAsset> LevelAsset;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Editor")
    TObjectPtr<AGridLevelRuntimeActor> PreviewRuntimeActor;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Selection")
    int32 SelectedCellX = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Selection")
    int32 SelectedCellY = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Selection")
    EGridEdge SelectedEdge = EGridEdge::North;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Selection")
    bool bAutoSelectFromActorTransform = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "0.0"))
    float AutoSelectionZ = 10.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Selection")
    bool bAutoRebuildPreviewOnMove = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Viewport Picking", meta = (ClampMin = "100.0"))
    float ViewportPickTraceDistance = 50000.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Viewport Picking")
    bool bUseHitNormalForEdgeSelection = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Viewport Picking")
    bool bSnapAfterViewportPick = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Cell Paint")
    EGridCellType PaintCellType = EGridCellType::Floor;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Cell Paint")
    bool bPaintCellHasCeiling = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Cell Paint")
    bool bPaintCellBlocksOccupancy = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Wall Paint")
    EGridWallType PaintWallType = EGridWallType::Solid;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Object Paint")
    EGridLevelObjectType PaintObjectType = EGridLevelObjectType::Button;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Object Paint")
    bool bObjectInitiallyEnabled = true;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Object Paint")
    bool bObjectInitiallyActive = false;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Object Paint")
    FName ObjectArchetypeId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Object Paint")
    FName ObjectTag = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Object Paint", meta = (MultiLine = "true"))
    FString ObjectNotes;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Object Paint")
    EGridEditorObjectPlacementPolicy PlacementPolicy = EGridEditorObjectPlacementPolicy::ReplaceSameSlotOnly;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Selection")
    FGuid LastSelectedObjectId;

public:
    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Editor")
    void EnsureLevelReady ();

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Editor")
    void RebuildPreview ();

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Selection")
    void UpdateSelectionFromActorTransform ();

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Selection")
    void SnapActorToSelectedCell ();

    UFUNCTION (CallInEditor, Category = "Viewport Picking")
    void PickSelectionFromViewport ();

    UFUNCTION (CallInEditor, Category = "Viewport Picking")
    void PickSelectionAndPlaceObjectFromViewport ();

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Cell Paint")
    void PaintSelectedCell ();

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Cell Paint")
    void ClearSelectedCell ();

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Wall Paint")
    void PaintSelectedWall ();

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Wall Paint")
    void ClearSelectedWall ();

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Object Paint")
    void PlaceSelectedObject ();

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Object Paint")
    void RemoveObjectsAtSelection ();

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Object Paint")
    void SelectObjectAtSelection ();

    UFUNCTION (BlueprintCallable, Category = "Viewport Picking")
    bool ApplyViewportHitSelection (const FVector& WorldHitLocation, const FVector& HitNormal);

    UFUNCTION (BlueprintCallable, Category = "Selection")
    bool IsSelectionValidForEditing () const;

    UFUNCTION (BlueprintCallable, Category = "Selection")
    FVector GetSelectionPreviewCenter () const;

protected:
    virtual void OnConstruction (const FTransform& Transform) override;
    virtual void Tick (float DeltaTime) override;

#if WITH_EDITOR
    virtual void PostEditMove (bool bFinished) override;
    virtual void PostEditChangeProperty (FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    bool HasValidLevelAsset () const;
    bool IsValidSelectedCell () const;
    bool RequiresEdge (EGridLevelObjectType ObjectType) const;
    bool IsCellCenteredObject (EGridLevelObjectType ObjectType) const;

    FGridLevelCellData* GetSelectedCellMutable ();
    EGridWallType* GetSelectedWallMutable (FGridLevelCellData& CellData);

    void ResolvePreviewRuntimeActor ();
    int32 RemoveObjectsAtSelectionInternal (bool bSameTypeOnly);

    EGridEdge GetEdgeFromYaw (float YawDegrees) const;
    EGridEdge GetEdgeFromHitNormal (const FVector& HitNormal) const;
    FVector GetSelectedCellWorldCenter (float ZOffset = 0.f) const;
    bool TryConvertWorldHitToSelection (const FVector& WorldHitLocation, const FVector& HitNormal);

    bool bIsPainting = false;
};