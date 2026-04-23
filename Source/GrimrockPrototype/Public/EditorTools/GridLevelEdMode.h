#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR
#include "EdMode.h"

class AGridLevelEditorActor;

class FGridLevelEdMode : public FEdMode
{
public:
    static const FEditorModeID EM_GridLevelEdModeId;

public:
    FGridLevelEdMode ();
    virtual ~FGridLevelEdMode () override;

    virtual void Enter () override;
    virtual void Exit () override;

    virtual bool UsesToolkits () const override { return false; }

    virtual bool HandleClick (
        FEditorViewportClient* InViewportClient,
        HHitProxy* HitProxy,
        const FViewportClick& Click) override;

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

    virtual void Render (
        const FSceneView* View,
        FViewport* Viewport,
        FPrimitiveDrawInterface* PDI) override;

    virtual bool ProcessCapturedMouseMoves (
        FEditorViewportClient* InViewportClient,
        FViewport* InViewport,
        const TArrayView<FIntPoint>& MouseMoves) override;

private:
    AGridLevelEditorActor* FindEditorActor () const;
    bool UpdateSelectionFromMouseRay (FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 X, int32 Y);
    void PaintCurrentSelection ();
    void EraseCurrentSelection ();

private:
    bool bIsPainting = false;
    bool bIsErasing = false;
};
#endif