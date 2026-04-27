#include "EditorTools/GridLevelEdMode.h"

#if WITH_EDITOR

#include "Editor.h"
#include "EditorViewportClient.h"
#include "EngineUtils.h"
#include "SceneManagement.h"

#include "EditorTools/GridLevelEditorActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Toolkits/ToolkitManager.h"
#include "EditorModeManager.h"
#include "EditorTools/GridLevelEdModeToolkit.h"

namespace
{
    FLinearColor GetDebugLinkColor (EGridLinkAction Action, bool bIncoming)
    {
        if (bIncoming)
        {
            return FLinearColor (1.f, 0.55f, 0.f, 1.f); // Orange
        }

        switch (Action)
        {
            case EGridLinkAction::Open:
            case EGridLinkAction::Activate:
                return FLinearColor (0.1f, 1.f, 0.25f, 1.f); // Vert

            case EGridLinkAction::Close:
            case EGridLinkAction::Deactivate:
                return FLinearColor (1.f, 0.1f, 0.1f, 1.f); // Rouge

            case EGridLinkAction::Toggle:
            default:
                return FLinearColor (0.f, 0.85f, 1.f, 1.f); // Cyan
        }
    }

    void DrawDebugArrowLine (
        FPrimitiveDrawInterface* PDI,
        const FVector& Start,
        const FVector& End,
        const FLinearColor& Color,
        float Thickness)
    {
        const FVector Delta = End - Start;
        const float Length = Delta.Size ();

        if (Length <= KINDA_SMALL_NUMBER)
        {
            return;
        }

        const FVector Dir = Delta / Length;

        const FVector LineStart = Start + FVector (0.f, 0.f, 24.f);
        const FVector LineEnd = End + FVector (0.f, 0.f, 24.f);

        PDI->DrawLine (LineStart, LineEnd, Color, SDPG_Foreground, Thickness);

        const FVector ArrowBase = FMath::Lerp (LineStart, LineEnd, 0.82f);

        FVector Right = FVector::CrossProduct (Dir, FVector::UpVector).GetSafeNormal ();
        if (Right.IsNearlyZero ())
        {
            Right = FVector::RightVector;
        }

        const float ArrowLength = 28.f;
        const float ArrowWidth = 14.f;

        const FVector ArrowTip = LineEnd;
        const FVector LeftWing = ArrowBase - Dir * ArrowLength + Right * ArrowWidth;
        const FVector RightWing = ArrowBase - Dir * ArrowLength - Right * ArrowWidth;

        PDI->DrawLine (ArrowTip, LeftWing, Color, SDPG_Foreground, Thickness);
        PDI->DrawLine (ArrowTip, RightWing, Color, SDPG_Foreground, Thickness);
    }

    void DrawDebugObjectBox (FPrimitiveDrawInterface* PDI, const FVector& Center, const FColor& Color, float Size)
    {
        DrawWireBox (PDI, FBox (Center - FVector (Size, Size, Size), Center + FVector (Size, Size, Size)), Color, SDPG_Foreground);
    }

    void DrawDebugVerticalMarker (FPrimitiveDrawInterface* PDI, const FVector& Center, const FLinearColor& Color, float Height, float Radius, float Thickness)
    {
        const FVector Bottom = Center + FVector (0.f, 0.f, 8.f);
        const FVector Top = Center + FVector (0.f, 0.f, Height);

        PDI->DrawLine (Bottom, Top, Color, SDPG_Foreground, Thickness);

        DrawWireBox (PDI, FBox (Top - FVector (Radius, Radius, Radius), Top + FVector (Radius, Radius, Radius)), Color.ToFColor (true), SDPG_Foreground);
    }
}

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
    const FVector RayEnd = RayOrigin + (RayDirection * EditorActor->ViewportPickTraceDistance);
    const FVector HitPoint = FMath::LinePlaneIntersection (RayOrigin, RayEnd, GridPlane);

    const float ForwardDot = FVector::DotProduct (RayDirection, FVector::UpVector);
    if (FMath::IsNearlyZero (ForwardDot))
    {
        return false;
    }

    const bool bGridHoverOk = EditorActor->ApplyGridHoverFromWorldPoint (HitPoint);

    if (EditorActor->ActiveTool == EGridEditorTool::Select ||
        EditorActor->ActiveTool == EGridEditorTool::Link)
    {
        EditorActor->UpdateHoveredObjectFromWorldPoint (HitPoint);
    }

    return bGridHoverOk;
}

void FGridLevelEdMode::ApplyPaint () const
{
    if (AGridLevelEditorActor* EditorActor = FindEditorActor ())
    {
        EditorActor->ApplyPrimaryToolAction ();

        if (Toolkit.IsValid ())
        {
            Toolkit->RefreshPalette ();
        }
    }
}

void FGridLevelEdMode::ApplyErase () const
{
    if (AGridLevelEditorActor* EditorActor = FindEditorActor ())
    {
        EditorActor->ApplySecondaryToolAction ();

        if (Toolkit.IsValid ())
        {
            Toolkit->RefreshPalette ();
        }
    }
}

bool FGridLevelEdMode::InputKey (FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
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

    return FEdMode::InputKey (ViewportClient, Viewport, Key, Event);
}

bool FGridLevelEdMode::MouseMove (FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 X, int32 Y)
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
    if (!bIsPainting)
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
        }
    }

    return true;
}

void FGridLevelEdMode::Render (const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
    FEdMode::Render (View, Viewport, PDI);

    AGridLevelEditorActor* EditorActor = FindEditorActor ();
    if (!EditorActor || !EditorActor->IsSelectionValidForEditing () || !EditorActor->LevelAsset)
    {
        return;
    }

    const FVector Center = EditorActor->GetSelectionPreviewCenter (4.f);
    const float Half = EditorActor->LevelAsset->CellSize * 0.5f;

    FColor MainColor = FColor::Yellow;

    switch (EditorActor->ActiveTool)
    {
        case EGridEditorTool::PaintCell:   MainColor = FColor::Green; break;
        case EGridEditorTool::PaintWall:   MainColor = FColor::Cyan; break;
        case EGridEditorTool::PaintObject: MainColor = FColor::Yellow; break;
        case EGridEditorTool::Erase:       MainColor = FColor::Red; break;
        case EGridEditorTool::Select:      MainColor = FColor::White; break;
        case EGridEditorTool::Link:        MainColor = FColor::Cyan; break;
        default: break;
    }
    DrawWireBox (PDI, 
        FBox (Center - FVector (Half, Half, 4.f), Center + FVector (Half, Half, 300.f)), 
        MainColor, SDPG_Foreground);

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

    DrawWireBox (PDI, FBox (EdgeCenter - FVector (12.f, 12.f, 12.f), EdgeCenter + FVector (12.f, 12.f, 12.f)), FColor::Orange, SDPG_Foreground);

    FVector SourceLocation = FVector::ZeroVector;
    if (EditorActor->HasPendingLinkSource () && EditorActor->TryGetPendingLinkSourceLocation (SourceLocation))
    {
        DrawWireBox (PDI, FBox (SourceLocation - FVector (16.f, 16.f, 16.f), SourceLocation + FVector (16.f, 16.f, 16.f)), FColor::Cyan, SDPG_Foreground);

        FVector HoverLocation = FVector::ZeroVector;
        if (EditorActor->TryGetSelectedObjectWorldLocation (HoverLocation))
        {
            PDI->DrawLine (SourceLocation, HoverLocation, FLinearColor {0.f, 1.f, 1.f, 1.f}, SDPG_Foreground, 2.0f);
        } else
        {
            PDI->DrawLine (SourceLocation, Center, FLinearColor {0.f, 1.f, 1.f, 1.f}, SDPG_Foreground, 1.5f);
        }
    }
    const FGridLevelObjectData* SelectedObject = EditorActor->GetSelectedObjectData ();
    if (SelectedObject && EditorActor->LevelAsset)
    {
        FVector SelectedLocation = FVector::ZeroVector;

        if (EditorActor->TryGetObjectWorldLocationById (SelectedObject->ObjectId, SelectedLocation))
        {
            for (const FGridLevelLinkData& Link : EditorActor->LevelAsset->Links)
            {
                const bool bOutgoing = Link.SourceObjectId == SelectedObject->ObjectId;
                const bool bIncoming = Link.TargetObjectId == SelectedObject->ObjectId;

                if (!bOutgoing && !bIncoming)
                {
                    continue;
                }

                FVector OtherLocation = FVector::ZeroVector;

                const FGuid OtherId = bOutgoing
                    ? Link.TargetObjectId
                    : Link.SourceObjectId;

                if (!EditorActor->TryGetObjectWorldLocationById (OtherId, OtherLocation))
                {
                    continue;
                }

                const FLinearColor LinkColor = GetDebugLinkColor (Link.Action, bIncoming);

                if (bOutgoing)
                {
                    DrawDebugArrowLine (PDI, SelectedLocation, OtherLocation, LinkColor, 3.f);
                } else
                {
                    DrawDebugArrowLine (PDI, OtherLocation, SelectedLocation, LinkColor, 2.25f);
                }
            }
        }
    }
}

void FGridLevelEdMode::Enter ()
{
    FEdMode::Enter ();

    if (!Toolkit.IsValid () && UsesToolkits ())
    {
        Toolkit = MakeShareable (new FGridLevelEdModeToolkit);
        Toolkit->Init (Owner->GetToolkitHost ());
    }
}

void FGridLevelEdMode::Exit ()
{
    if (Toolkit.IsValid ())
    {
        FToolkitManager::Get ().CloseToolkit (Toolkit.ToSharedRef ());
        Toolkit.Reset ();
    }

    FEdMode::Exit ();
}

#endif
// WITH_EDITOR