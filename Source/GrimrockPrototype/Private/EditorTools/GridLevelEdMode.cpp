#include "EditorTools/GridLevelEdMode.h"

#if WITH_EDITOR

#include "Editor.h"
#include "EditorViewportClient.h"
#include "EngineUtils.h"
#include "SceneManagement.h"

#include "EditorTools/GridLevelEditorActor.h"
#include "Runtime/GridLevelRuntimeActor.h"

const FEditorModeID FGridLevelEdMode::EM_GridLevelEdModeId = TEXT ("EM_GrimrockGridLevelEdMode");

AGridLevelEditorActor* FGridLevelEdMode::FindEditorActor () const
{
    if (!GEditor)
    {
        return nullptr;
    }

    UWorld* World = GEditor->GetEditorWorldContext ().World ();
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<AGridLevelEditorActor> It (World); It; ++It)
    {
        return *It;
    }

    return nullptr;
}

bool FGridLevelEdMode::UpdateHoverFromMouse (
    FEditorViewportClient* ViewportClient,
    FViewport* Viewport,
    int32 MouseX,
    int32 MouseY) const
{
    AGridLevelEditorActor* EditorActor = FindEditorActor ();
    if (!EditorActor || !EditorActor->LevelAsset)
    {
        return false;
    }

    FSceneViewFamilyContext ViewFamily (
        FSceneViewFamily::ConstructionValues (
            Viewport,
            ViewportClient->GetScene (),
            ViewportClient->EngineShowFlags));

    FSceneView* SceneView = ViewportClient->CalcSceneView (&ViewFamily);
    if (!SceneView)
    {
        return false;
    }

    FVector RayOrigin = FVector::ZeroVector;
    FVector RayDirection = FVector::ForwardVector;
    SceneView->DeprojectFVector2D (FVector2D (MouseX, MouseY), RayOrigin, RayDirection);

    EditorActor->ResolvePreviewRuntimeActor ();

    FVector GridOrigin = FVector::ZeroVector;
    if (EditorActor->PreviewRuntimeActor)
    {
        GridOrigin = EditorActor->PreviewRuntimeActor->GetActorLocation () + EditorActor->PreviewRuntimeActor->GridOrigin;
    }

    const FPlane GridPlane (GridOrigin, FVector::UpVector);
    const FVector RayEnd = RayOrigin + (RayDirection * 100000.f);
    const FVector HitPoint = FMath::LinePlaneIntersection (RayOrigin, RayEnd, GridPlane);

    const float ForwardDot = FVector::DotProduct (RayDirection, FVector::UpVector);
    if (FMath::IsNearlyZero (ForwardDot))
    {
        return false;
    }

    return EditorActor->ApplyGridHoverFromWorldPoint (HitPoint);
}

void FGridLevelEdMode::ApplyPaint () const
{
    if (AGridLevelEditorActor* EditorActor = FindEditorActor ())
    {
        EditorActor->PlaceSelectedObject ();
    }
}

void FGridLevelEdMode::ApplyErase () const
{
    if (AGridLevelEditorActor* EditorActor = FindEditorActor ())
    {
        EditorActor->RemoveObjectsAtSelection ();
    }
}

bool FGridLevelEdMode::InputKey (
    FEditorViewportClient* ViewportClient,
    FViewport* Viewport,
    FKey Key,
    EInputEvent Event)
{
    if (Key == EKeys::LeftMouseButton)
    {
        if (Event == IE_Pressed)
        {
            bIsPainting = true;

            FIntPoint MousePos;
            Viewport->GetMousePos (MousePos);
            if (UpdateHoverFromMouse (ViewportClient, Viewport, MousePos.X, MousePos.Y))
            {
                ApplyPaint ();
            }

            return true;
        }

        if (Event == IE_Released)
        {
            bIsPainting = false;
            return true;
        }
    }

    if (Key == EKeys::RightMouseButton)
    {
        if (Event == IE_Pressed)
        {
            bIsErasing = true;

            FIntPoint MousePos;
            Viewport->GetMousePos (MousePos);
            if (UpdateHoverFromMouse (ViewportClient, Viewport, MousePos.X, MousePos.Y))
            {
                ApplyErase ();
            }

            return true;
        }

        if (Event == IE_Released)
        {
            bIsErasing = false;
            return true;
        }
    }

    return FEdMode::InputKey (ViewportClient, Viewport, Key, Event);
}

bool FGridLevelEdMode::MouseMove (
    FEditorViewportClient* ViewportClient,
    FViewport* Viewport,
    int32 X,
    int32 Y)
{
    if (UpdateHoverFromMouse (ViewportClient, Viewport, X, Y))
    {
        return true;
    }

    return FEdMode::MouseMove (ViewportClient, Viewport, X, Y);
}

bool FGridLevelEdMode::ProcessCapturedMouseMoves (
    FEditorViewportClient* InViewportClient,
    FViewport* InViewport,
    const TArrayView<FIntPoint>& MouseMoves)
{
    if (!bIsPainting && !bIsErasing)
    {
        return FEdMode::ProcessCapturedMouseMoves (InViewportClient, InViewport, MouseMoves);
    }

    FIntPoint MousePos;
    InViewport->GetMousePos (MousePos);

    if (UpdateHoverFromMouse (InViewportClient, InViewport, MousePos.X, MousePos.Y))
    {
        if (bIsPainting)
        {
            ApplyPaint ();
        } else if (bIsErasing)
        {
            ApplyErase ();
        }
    }

    return true;
}

void FGridLevelEdMode::Render (
    const FSceneView* View,
    FViewport* Viewport,
    FPrimitiveDrawInterface* PDI)
{
    FEdMode::Render (View, Viewport, PDI);

    AGridLevelEditorActor* EditorActor = FindEditorActor ();
    if (!EditorActor || !EditorActor->IsSelectionValidForEditing () || !EditorActor->LevelAsset)
    {
        return;
    }

    const FVector Center = EditorActor->GetSelectionPreviewCenter (4.f);
    const float Half = EditorActor->LevelAsset->CellSize * 0.5f;

    DrawWireBox (
        PDI,
        FBox (Center - FVector (Half, Half, 4.f), Center + FVector (Half, Half, 4.f)),
        FColor::Yellow,
        SDPG_Foreground);

    const FVector EdgeCenter = [&] ()
    {
        switch (EditorActor->SelectedEdge)
        {
            case EGridEdge::North: return Center + FVector (0.f, Half, 0.f);
            case EGridEdge::East:  return Center + FVector (Half, 0.f, 0.f);
            case EGridEdge::South: return Center + FVector (0.f, -Half, 0.f);
            case EGridEdge::West:  return Center + FVector (-Half, 0.f, 0.f);
            default:               return Center;
        }
    }();

    DrawWireBox (
        PDI,
        FBox (EdgeCenter - FVector (12.f, 12.f, 12.f), EdgeCenter + FVector (12.f, 12.f, 12.f)),
        FColor::Orange,
        SDPG_Foreground);
}
#endif