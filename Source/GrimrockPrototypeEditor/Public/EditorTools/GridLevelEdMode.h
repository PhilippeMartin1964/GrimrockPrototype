#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
#include "EdMode.h"
#include "Core/GridTypes.h"
#include "EditorTools/GridLevelEditorActor.h"

class AGridLevelEditorActor;
class FGridLevelEdModeToolkit;

class FGridLevelEdMode : public FEdMode
{
public:
    static const FEditorModeID EM_GridLevelEdModeId;

    virtual bool UsesToolkits () const override { return true; }

    virtual void Enter () override;
    virtual void Exit () override;

    virtual bool InputKey (
        FEditorViewportClient* ViewportClient,
        FViewport* Viewport,
        FKey Key,
        EInputEvent Event) override;

    virtual bool MouseMove (
        FEditorViewportClient* ViewportClient,
        FViewport* Viewport,
        int32 X,
        int32 Y) override;

    virtual bool ProcessCapturedMouseMoves (
        FEditorViewportClient* InViewportClient,
        FViewport* InViewport,
        const TArrayView<FIntPoint>& MouseMoves) override;

    virtual void Render (
        const FSceneView* View,
        FViewport* Viewport,
        FPrimitiveDrawInterface* PDI) override;

private:
    AGridLevelEditorActor* FindEditorActor () const;
    bool UpdateHoverFromMouse (
        FEditorViewportClient* ViewportClient,
        FViewport* Viewport,
        int32 MouseX,
        int32 MouseY) const;

    void ApplyPaint () const;
    void ApplyErase () const;

private:
    bool bIsPainting = false;
    bool bIsErasing = false;
    TSharedPtr<FGridLevelEdModeToolkit> Toolkit;

    mutable int32 LastPaintCellX = INDEX_NONE;
    mutable int32 LastPaintCellY = INDEX_NONE;
    mutable EGridEdge LastPaintEdge = EGridEdge::None;
    mutable EGridEditorTool LastPaintTool = EGridEditorTool::Select;

    void ResetPaintCache () const;
    bool ShouldApplyPaintForCurrentSelection (const AGridLevelEditorActor* EditorActor) const;
    bool CommitHoveredSelection (AGridLevelEditorActor* EditorActor) const;
    void RefreshToolkitIfObservedSelectionChanged (const AGridLevelEditorActor* EditorActor) const;

    mutable int32 LastObservedSelectedCellX = INDEX_NONE;
    mutable int32 LastObservedSelectedCellY = INDEX_NONE;
    mutable EGridEdge LastObservedSelectedEdge = EGridEdge::None;
    mutable FGuid LastObservedSelectedObjectId;
};
#endif
