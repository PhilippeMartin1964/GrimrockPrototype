#include "EditorTools/GridLevelEdMode.h"

#if WITH_EDITOR

#include "Editor.h"
#include "EditorViewportClient.h"
#include "EngineUtils.h"
#include "SceneManagement.h"

#include "EditorTools/GridLevelEditorActor.h"
#include "Runtime/GridLevelRuntimeActor.h"

const FEditorModeID FGridLevelEdMode::EM_GridLevelEdModeId = TEXT ("EM_GrimrockGridLevelEdMode");

FGridLevelEdMode::FGridLevelEdMode ()
{
}

FGridLevelEdMode::~FGridLevelEdMode ()
{
}

void FGridLevelEdMode::Enter ()
{
    FEdMode::Enter ();
}

void FGridLevelEdMode::Exit ()
{
    bIsPainting = false;
    bIsErasing = false;
    FEdMode::Exit ();
}

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

bool FGridLevelEdMode::UpdateSelectionFromMouseRay (
    FEditorViewportClient* ViewportClient,
    FViewport* Viewport,
    int32 X,
    int32 Y)
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
    SceneView->DeprojectFVector2D (FVector2D (X, Y), RayOrigin, RayDirection);

    AGridLevelRuntimeActor* Preview = EditorActor->PreviewRuntimeActor;
    const FVector GridOrigin = Preview
        ? (Preview->GetActorLocation () + Preview->GridOrigin)
        : FVector::ZeroVector;

    // plan horizontal de la grille
    const FPlane GridPlane (GridOrigin, FVector::UpVector);

    const float Denom = FVector::DotProduct (RayDirection, FVector::UpVector);
    if (FMath::IsNearlyZero (Denom))
    {
        return false;
    }

    const float T = FMath::LinePlaneIntersection (
        RayOrigin,
        RayOrigin + RayDirection * 100000.f,
        GridPlane).Equals (RayOrigin, KINDA_SMALL_NUMBER)
        ? -1.f
        : FVector::Dist (
            RayOrigin,
            FMath::LinePlaneIntersection (
                RayOrigin,
                RayOrigin + RayDirection * 100000.f,
                GridPlane));

    if (T < 0.f)
    {
        return false;
    }

    const FVector HitPoint = FMath::LinePlaneIntersection (
        RayOrigin,
        RayOrigin + RayDirection * 100000.f,
        GridPlane);

    // normale verticale : pour une cellule au sol, on garde l’edge courant
    return EditorActor->ApplyViewportHitSelection (HitPoint, FVector::UpVector);
}

bool FGridLevelEdMode::HandleClick (
    FEditorViewportClient* InViewportClient,
    HHitProxy* HitProxy,
    const FViewportClick& Click)
{
    if (!InViewportClient)
    {
        return FEdMode::HandleClick (InViewportClient, HitProxy, Click);
    }

    if (Click.GetKey () == EKeys::LeftMouseButton)
    {
        if (UpdateSelectionFromMouseRay (
            InViewportClient,
            InViewportClient->Viewport,
            Click.GetClickPos ().X,
            Click.GetClickPos ().Y))
        {
            PaintCurrentSelection ();
            return true;
        }
    }

    if (Click.GetKey () == EKeys::RightMouseButton)
    {
        if (UpdateSelectionFromMouseRay (
            InViewportClient,
            InViewportClient->Viewport,
            Click.GetClickPos ().X,
            Click.GetClickPos ().Y))
        {
            EraseCurrentSelection ();
            return true;
        }
    }

    return FEdMode::HandleClick (InViewportClient, HitProxy, Click);
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
            return true; // on consomme
        }

        if (Event == IE_Released)
        {
            bIsPainting = false;
            return true; // on consomme
        }
    }

    if (Key == EKeys::RightMouseButton)
    {
        if (Event == IE_Pressed)
        {
            bIsErasing = true;
            return true; // on consomme
        }

        if (Event == IE_Released)
        {
            bIsErasing = false;
            return true; // on consomme
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
    if (!ViewportClient || !Viewport)
    {
        return FEdMode::MouseMove (ViewportClient, Viewport, X, Y);
    }

    if (bIsPainting || bIsErasing)
    {
        if (UpdateSelectionFromMouseRay (ViewportClient, Viewport, X, Y))
        {
            if (bIsPainting)
            {
                PaintCurrentSelection ();
            } else if (bIsErasing)
            {
                EraseCurrentSelection ();
            }

            return true;
        }
    }

    return FEdMode::MouseMove (ViewportClient, Viewport, X, Y);
}

void FGridLevelEdMode::PaintCurrentSelection ()
{
    if (AGridLevelEditorActor* EditorActor = FindEditorActor ())
    {
        EditorActor->PlaceSelectedObject ();
    }
}

void FGridLevelEdMode::EraseCurrentSelection ()
{
    if (AGridLevelEditorActor* EditorActor = FindEditorActor ())
    {
        EditorActor->RemoveObjectsAtSelection ();
    }
}

void FGridLevelEdMode::Render (
    const FSceneView* View,
    FViewport* Viewport,
    FPrimitiveDrawInterface* PDI)
{
    FEdMode::Render (View, Viewport, PDI);

    AGridLevelEditorActor* EditorActor = FindEditorActor ();
    if (!EditorActor || !EditorActor->LevelAsset || !EditorActor->IsSelectionValidForEditing ())
    {
        return;
    }

    const FVector Center = EditorActor->GetSelectionPreviewCenter ();
    const float Half = EditorActor->LevelAsset->CellSize * 0.5f;

    DrawWireBox (
        PDI,
        FBox (Center - FVector (Half, Half, 4.f), Center + FVector (Half, Half, 4.f)),
        FColor::Yellow,
        SDPG_Foreground);
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

    if (!InViewportClient || !InViewport)
    {
        return true;
    }

    FIntPoint MousePos;
    InViewport->GetMousePos (MousePos);

    if (UpdateSelectionFromMouseRay (InViewportClient, InViewport, MousePos.X, MousePos.Y))
    {
        if (bIsPainting)
        {
            PaintCurrentSelection ();
        } else if (bIsErasing)
        {
            EraseCurrentSelection ();
        }
    }

    return true; // très important : bloque le comportement caméra du viewport
}

#endif