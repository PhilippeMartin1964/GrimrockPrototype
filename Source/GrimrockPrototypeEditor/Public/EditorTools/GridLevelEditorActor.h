#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Core/GridLevelAsset.h"
#include "Core/GridObjectBehavior.h"
#include "Components/TextRenderComponent.h"
#include "GridLevelEditorActor.generated.h"

class AGridLevelRuntimeActor;
class UGridObjectPaletteAsset;
class UGridObjectArchetypeAsset;

UENUM (BlueprintType)
enum class EGridEditorTool : uint8
{
    Select      UMETA (DisplayName = "Select"),
    PaintCell   UMETA (DisplayName = "Paint Cell"),
    PaintWall   UMETA (DisplayName = "Paint Wall"),
    PaintObject UMETA (DisplayName = "Paint Object"),
    Erase       UMETA (DisplayName = "Erase"),
    Link        UMETA (DisplayName = "Link")
};

UENUM (BlueprintType)
enum class EGridEditorObjectPlacementPolicy : uint8
{
    ReplaceSameSlotOnly UMETA (DisplayName = "Replace Same Slot Only"),
    RemoveAllAtSlot     UMETA (DisplayName = "Remove All At Slot")
};

UCLASS ()
class GRIMROCKPROTOTYPEEDITOR_API AGridLevelEditorActor : public AActor
{
    GENERATED_BODY ()

public:
    AGridLevelEditorActor ();

public:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Editor")
    TObjectPtr<UGridLevelAsset> LevelAsset;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Editor")
    TObjectPtr<AGridLevelRuntimeActor> PreviewRuntimeActor;

    UPROPERTY (VisibleAnywhere, Category = "Editor")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Selection")
    int32 SelectedCellX = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Selection")
    int32 SelectedCellY = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Selection")
    EGridEdge SelectedEdge = EGridEdge::North;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Selection")
    FGuid HoveredObjectId;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Selection", meta = (ClampMin = "1.0"))
    float ObjectHoverPickRadius = 48.f;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Viewport Picking", meta = (ClampMin = "100.0"))
    float ViewportPickTraceDistance = 50000.f;

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

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Tool")
    EGridEditorTool ActiveTool = EGridEditorTool::Select;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Link")
    EGridLinkAction LinkAction = EGridLinkAction::Toggle;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Link")
    FGuid PendingLinkSourceObjectId;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Link")
    bool bHasPendingLinkSource = false;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "Palette")
    TObjectPtr<UGridObjectPaletteAsset> ObjectPalette;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Palette")
    FName SelectedPaletteEntryId = NAME_None;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, Category = "Palette")
    FName SelectedArchetypeId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "Object Paint")
    FGridObjectBehaviorParams ObjectBehavior;

    UPROPERTY (VisibleAnywhere, Category = "Editor Grid")
    TObjectPtr<UStaticMeshComponent> CoordinateGridPlane;

    UPROPERTY (EditAnywhere, Category = "Editor Grid")
    TObjectPtr<UStaticMesh> CoordinateGridPlaneMesh;

    UPROPERTY (EditAnywhere, Category = "Editor Grid")
    TObjectPtr<UMaterialInterface> CoordinateGridMaterial;

    UPROPERTY (EditAnywhere, Category = "Editor Grid")
    bool bShowCoordinateGrid = false;

    UPROPERTY (EditAnywhere, Category = "Editor Grid")
    float CoordinateGridZOffset = 2.f;

    UPROPERTY (EditAnywhere, Category = "Editor Grid")
    bool bShowCoordinateLabels = false;

    UPROPERTY (EditAnywhere, Category = "Editor Grid")
    float CoordinateLabelWorldSize = 28.f;

public:
    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Editor")
    void EnsureLevelReady ();

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Editor")
    void RebuildPreview ();

    UFUNCTION (BlueprintCallable, Category = "Cell Paint")
    void ClearSelectedCell ();

    UFUNCTION (BlueprintCallable, Category = "Wall Paint")
    void PaintSelectedWall ();

    UFUNCTION (BlueprintCallable, Category = "Wall Paint")
    void ClearSelectedWall ();

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Object Paint")
    void PlaceSelectedObject ();

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Object Paint")
    void RemoveObjectsAtSelection ();

    UFUNCTION (BlueprintCallable, Category = "Object Paint")
    void SelectObjectAtSelection ();

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Object Paint")
    bool ApplyEditedSelectedObject ();

    UFUNCTION (BlueprintCallable, Category = "Viewport Picking")
    bool ApplyViewportHitSelection (const FVector& WorldHitLocation, const FVector& HitNormal);

    UFUNCTION (BlueprintCallable, Category = "Selection")
    bool IsSelectionValidForEditing () const;

    UFUNCTION (BlueprintCallable, Category = "Selection")
    bool ApplyGridHoverFromWorldPoint (const FVector& WorldPoint);

    UFUNCTION (BlueprintCallable, Category = "Selection")
    FVector GetSelectionPreviewCenter (float ZOffset = 4.f) const;

    UFUNCTION (BlueprintCallable, Category = "Selection")
    bool UpdateHoveredObjectFromWorldPoint (const FVector& WorldPoint);

    UFUNCTION (BlueprintCallable, Category = "Selection")
    bool SelectHoveredObject ();

    UFUNCTION (BlueprintCallable, Category = "Selection")
    bool TryGetHoveredObjectWorldLocation (FVector& OutWorldLocation) const;

    UFUNCTION (BlueprintCallable, Category = "Tool")
    void ApplyPrimaryToolAction ();

    UFUNCTION (BlueprintCallable, Category = "Tool")
    void ApplySecondaryToolAction ();

    UFUNCTION (BlueprintCallable, Category = "Tool")
    void EraseAtSelection ();

    UFUNCTION (BlueprintCallable, Category = "Link")
    bool BeginOrCompleteLinkAtSelection ();

    UFUNCTION (BlueprintCallable, Category = "Link")
    void ClearPendingLinkSource ();

    UFUNCTION (BlueprintCallable, Category = "Link")
    bool RemoveLinksAtSelection ();

    UFUNCTION (BlueprintCallable, Category = "Link")
    bool HasPendingLinkSource () const;

    UFUNCTION (BlueprintCallable, Category = "Link")
    bool TryGetPendingLinkSourceLocation (FVector& OutWorldLocation) const;

    UFUNCTION (BlueprintCallable, Category = "Link")
    bool RemoveLinkByIndexForSelectedObject (int32 LinkIndex);

    UFUNCTION (BlueprintCallable, Category = "Link")
    bool RemoveAllLinksForSelectedObject ();

    UFUNCTION (BlueprintCallable, Category = "Link")
    bool RemoveExactLink (FGuid SourceObjectId, FGuid TargetObjectId, EGridLinkAction Action);

    UFUNCTION (BlueprintCallable, Category = "Selection")
    bool TryGetSelectedObjectWorldLocation (FVector& OutWorldLocation) const;

    UFUNCTION (BlueprintCallable, Category = "Selection")
    bool SelectObjectById (FGuid ObjectId);

    void ResolvePreviewRuntimeActor ();

    UFUNCTION (BlueprintCallable, Category = "Palette")
    bool ApplyPaletteEntry (FName EntryId);

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Palette")
    void ApplySelectedPaletteEntry ();

    const FGridLevelObjectData* GetSelectedObjectData () const;

    UFUNCTION (BlueprintCallable, Category = "Selection")
    bool TryGetObjectWorldLocationById (FGuid ObjectId, FVector& OutWorldLocation) const;

    UFUNCTION (CallInEditor, BlueprintCallable, Category = "Selection")
    bool FocusSelectedObject ();

    UFUNCTION (BlueprintCallable, Category = "Object Paint")
    bool ApplyBehaviorToSelectedObject (const FGridObjectBehaviorParams& NewBehavior);

protected:
    virtual void OnConstruction (const FTransform& Transform) override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty (FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    bool HasValidLevelAsset () const;
    bool IsValidSelectedCell () const;
    bool RequiresEdge (EGridLevelObjectType ObjectType) const;
    bool IsCellCenteredObject (EGridLevelObjectType ObjectType) const;

    FGridLevelCellData* GetSelectedCellMutable ();
    EGridWallType* GetSelectedWallMutable (FGridLevelCellData& CellData);

    int32 RemoveObjectsAtSelectionInternal (bool bSameTypeOnly);

    FVector GetSelectedCellWorldCenter (float ZOffset = 0.f) const;
    bool TryConvertWorldHitToSelection (const FVector& WorldHitLocation, const FVector& HitNormal);

    EGridEdge GetEdgeFromPointInCell (const FVector2D& LocalInCell, float CellSize) const;

    const FGridLevelObjectData* FindObjectAtSelection () const;
    const FGridLevelObjectData* FindObjectById (const FGuid& ObjectId) const;
    bool TryGetObjectWorldLocation (const FGridLevelObjectData& ObjectData, FVector& OutWorldLocation) const;

    void ClearSelectedObjectState ();

    bool HasAnyObjectInSelectedCell () const;
    bool HasAnyWallInSelectedCell () const;

    void PaintSelectedCellAndWall ();

    void RebuildGeometryPreview ();

    UPROPERTY (Transient)
    TObjectPtr<UTextRenderComponent> CoordinateHoverLabel;

    void EnsureCoordinateHoverLabel ();
    void UpdateCoordinateHoverLabel ();
    void UpdateCoordinateGridPlane ();

};
