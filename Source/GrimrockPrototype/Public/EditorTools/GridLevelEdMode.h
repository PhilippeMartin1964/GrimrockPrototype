#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
#include "EdMode.h"

class AGridLevelEditorActor;

class FGridLevelEdMode : public FEdMode
{
public:
    static const FEditorModeID EM_GridLevelEdModeId;

    virtual bool UsesToolkits () const override { return false; }

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
};
#endif