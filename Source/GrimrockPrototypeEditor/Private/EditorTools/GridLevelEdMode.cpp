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
    FLinearColor GetDebugLinkColor (EGridObjectCommand Command, bool bIncoming)
    {
        if (bIncoming)
        {
            return FLinearColor (1.f, 0.55f, 0.f, 1.f); // Orange
        }

        switch (Command)
        {
            case EGridObjectCommand::Open:
            case EGridObjectCommand::Activate:
                return FLinearColor (0.1f, 1.f, 0.25f, 1.f); // Vert

            case EGridObjectCommand::Close:
            case EGridObjectCommand::Deactivate:
                return FLinearColor (1.f, 0.1f, 0.1f, 1.f); // Rouge

            case EGridObjectCommand::Toggle:
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

static bool IsRightMouseButtonDown (FViewport* Viewport)
{
    return Viewport && Viewport->KeyState (EKeys::RightMouseButton);
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
    if (Key == EKeys::RightMouseButton)
    {
        bIsPainting = false;
        ResetPaintCache ();

        return FEdMode::InputKey (ViewportClient, Viewport, Key, Event);
    }
    if (IsRightMouseButtonDown (Viewport))
    {
        bIsPainting = false;
        ResetPaintCache ();

        return FEdMode::InputKey (ViewportClient, Viewport, Key, Event);
    }
    if (Key == EKeys::LeftMouseButton)
    {
        if (Event == IE_Pressed)
        {
            bIsPainting = true;
            ResetPaintCache ();

            FIntPoint MousePos;
            Viewport->GetMousePos (MousePos);

            if (UpdateHoverFromMouse (ViewportClient, Viewport, MousePos.X, MousePos.Y))
            {
                if (AGridLevelEditorActor* EditorActor = FindEditorActor ())
                {
                    if (CommitHoveredSelection (EditorActor) && ShouldApplyPaintForCurrentSelection (EditorActor))
                    {
                        ApplyPaint ();
                    }
                }
            }
            return true;
        }
        if (Event == IE_Released)
        {
            bIsPainting = false;
            ResetPaintCache ();
            return true;
        }
    }
    return FEdMode::InputKey (ViewportClient, Viewport, Key, Event);
}

bool FGridLevelEdMode::MouseMove (FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 X, int32 Y)
{
    UpdateHoverFromMouse (ViewportClient, Viewport, X, Y);
    return FEdMode::MouseMove (ViewportClient, Viewport, X, Y);
}

bool FGridLevelEdMode::ProcessCapturedMouseMoves (FEditorViewportClient* InViewportClient, FViewport* InViewport, const TArrayView<FIntPoint>& MouseMoves)
{
    if (IsRightMouseButtonDown (InViewport))
    {
        bIsPainting = false;
        ResetPaintCache ();

        return FEdMode::ProcessCapturedMouseMoves (InViewportClient, InViewport, MouseMoves);
    }

    if (!bIsPainting)
    {
        return FEdMode::ProcessCapturedMouseMoves (InViewportClient, InViewport, MouseMoves);
    }

    FIntPoint MousePos;
    InViewport->GetMousePos (MousePos);

    if (UpdateHoverFromMouse (InViewportClient, InViewport, MousePos.X, MousePos.Y))
    {
        if (AGridLevelEditorActor* EditorActor = FindEditorActor ())
        {
            if (CommitHoveredSelection (EditorActor) && ShouldApplyPaintForCurrentSelection (EditorActor))
            {
                ApplyPaint ();
            }
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
    DrawWireBox (PDI, FBox (Center - FVector (Half, Half, 4.f), Center + FVector (Half, Half, 300.f)), MainColor, SDPG_Foreground);

    const FVector GridWorldOrigin = EditorActor->PreviewRuntimeActor
        ? EditorActor->PreviewRuntimeActor->GetActorLocation () + EditorActor->PreviewRuntimeActor->GridOrigin
        : EditorActor->GetActorLocation ();

    FVector HoverEdgeCenter = Center;
    if (EditorActor->LevelAsset->IsValidCoord (EditorActor->HoveredCellX, EditorActor->HoveredCellY))
    {
        const float CellSize = EditorActor->LevelAsset->CellSize;
        const FVector HoverCenter = GridWorldOrigin + FVector (
            (EditorActor->HoveredCellX * CellSize) + Half,
            (EditorActor->HoveredCellY * CellSize) + Half,
            4.f);

        switch (EditorActor->HoveredEdge)
        {
            case EGridEdge::North: HoverEdgeCenter = HoverCenter + FVector (0.f, Half, 0.f); break;
            case EGridEdge::East:  HoverEdgeCenter = HoverCenter + FVector (Half, 0.f, 0.f); break;
            case EGridEdge::South: HoverEdgeCenter = HoverCenter + FVector (0.f, -Half, 0.f); break;
            case EGridEdge::West:  HoverEdgeCenter = HoverCenter + FVector (-Half, 0.f, 0.f); break;
            default:               HoverEdgeCenter = HoverCenter; break;
        }
    }

    DrawWireBox (PDI, FBox (HoverEdgeCenter - FVector (12.f, 12.f, 12.f), HoverEdgeCenter + FVector (12.f, 12.f, 12.f)), FColor::Orange, SDPG_Foreground);

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
            for (const FGridObjectLink& Link : EditorActor->LevelAsset->Links)
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

                const FLinearColor LinkColor = GetDebugLinkColor (Link.Command, bIncoming);

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

    LastObservedSelectedCellX = INDEX_NONE;
    LastObservedSelectedCellY = INDEX_NONE;
    LastObservedSelectedEdge = EGridEdge::None;
    LastObservedSelectedObjectId.Invalidate ();

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

void FGridLevelEdMode::ResetPaintCache () const
{
    LastPaintCellX = INDEX_NONE;
    LastPaintCellY = INDEX_NONE;
    LastPaintEdge = EGridEdge::None;
    LastPaintTool = EGridEditorTool::Select;
}

bool FGridLevelEdMode::ShouldApplyPaintForCurrentSelection (const AGridLevelEditorActor* EditorActor) const
{
    if (!EditorActor)
    {
        return false;
    }

    if (EditorActor->SelectedCellX == LastPaintCellX &&
        EditorActor->SelectedCellY == LastPaintCellY &&
        EditorActor->SelectedEdge == LastPaintEdge &&
        EditorActor->ActiveTool == LastPaintTool)
    {
        return false;
    }

    LastPaintCellX = EditorActor->SelectedCellX;
    LastPaintCellY = EditorActor->SelectedCellY;
    LastPaintEdge = EditorActor->SelectedEdge;
    LastPaintTool = EditorActor->ActiveTool;

    return true;
}

bool FGridLevelEdMode::CommitHoveredSelection (AGridLevelEditorActor* EditorActor) const
{
    if (!EditorActor)
    {
        return false;
    }

    const bool bCommitted = EditorActor->CommitHoveredCellSelection ();
    if (bCommitted)
    {
        RefreshToolkitIfObservedSelectionChanged (EditorActor);
    }

    return bCommitted;
}

void FGridLevelEdMode::RefreshToolkitIfObservedSelectionChanged (const AGridLevelEditorActor* EditorActor) const
{
    if (!EditorActor || !Toolkit.IsValid ())
    {
        return;
    }

    const bool bSelectionChanged =
        LastObservedSelectedCellX != EditorActor->SelectedCellX ||
        LastObservedSelectedCellY != EditorActor->SelectedCellY ||
        LastObservedSelectedEdge != EditorActor->SelectedEdge ||
        LastObservedSelectedObjectId != EditorActor->LastSelectedObjectId;

    if (!bSelectionChanged)
    {
        return;
    }

    LastObservedSelectedCellX = EditorActor->SelectedCellX;
    LastObservedSelectedCellY = EditorActor->SelectedCellY;
    LastObservedSelectedEdge = EditorActor->SelectedEdge;
    LastObservedSelectedObjectId = EditorActor->LastSelectedObjectId;

    Toolkit->RefreshPalette ();
}

#endif
// WITH_EDITOR
