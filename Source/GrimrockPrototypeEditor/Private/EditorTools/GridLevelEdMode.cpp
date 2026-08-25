#include "EditorTools/GridLevelEdMode.h"

#if WITH_EDITOR

#include "Editor.h"
#include "EditorViewportClient.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "SceneManagement.h"

#include "EditorTools/GridLevelEditorActor.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Toolkits/ToolkitManager.h"
#include "EditorModeManager.h"
#include "EditorTools/GridLevelEdModeToolkit.h"

namespace
{
	struct FConnectorDrawData
	{
		FVector SourceCenter = FVector::ZeroVector;
		FVector TargetCenter = FVector::ZeroVector;
		FVector ArrowEnd = FVector::ZeroVector;
		FVector Direction = FVector::ForwardVector;
	};

	FLinearColor GetDebugLinkColor(EGridObjectCommand /*Command*/, bool bIncoming)
	{
		if (bIncoming)
		{
			return FLinearColor(0.65f, 0.55f, 1.f, 0.85f);
		}

		return FLinearColor(0.25f, 0.85f, 1.f, 0.85f);
	}

	void DrawDashedLine(FPrimitiveDrawInterface* PDI, const FVector& Start, const FVector& End, const FLinearColor& Color, float DashLength, float GapLength,
		float Thickness, uint8 DepthPriority)
	{
		const FVector Delta = End - Start;
		const float Length = Delta.Size();
		if (Length <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		const FVector Dir = Delta / Length;
		const float StepLength = FMath::Max(DashLength + GapLength, 1.f);

		for (float Distance = 0.f; Distance < Length; Distance += StepLength)
		{
			const float SegmentEndDistance = FMath::Min(Distance + DashLength, Length);
			if (SegmentEndDistance <= Distance)
			{
				continue;
			}

			PDI->DrawLine(Start + Dir * Distance, Start + Dir * SegmentEndDistance, Color, DepthPriority, Thickness);
		}
	}

	void DrawArrowHead(
		FPrimitiveDrawInterface* PDI, const FVector& Tip, const FVector& Direction, const FLinearColor& Color, float Size, float Thickness, uint8 DepthPriority)
	{
		const FVector Dir = Direction.GetSafeNormal();
		if (Dir.IsNearlyZero())
		{
			return;
		}

		FVector Right = FVector::CrossProduct(Dir, FVector::UpVector).GetSafeNormal();
		if (Right.IsNearlyZero())
		{
			Right = FVector::RightVector;
		}

		const float WingWidth = Size * 0.45f;
		const FVector LeftWing = Tip - Dir * Size + Right * WingWidth;
		const FVector RightWing = Tip - Dir * Size - Right * WingWidth;

		PDI->DrawLine(Tip, LeftWing, Color, DepthPriority, Thickness);
		PDI->DrawLine(Tip, RightWing, Color, DepthPriority, Thickness);
	}

	void DrawDebugObjectBox(FPrimitiveDrawInterface* PDI, const FVector& Center, const FColor& Color, float Size)
	{
		DrawWireBox(PDI, FBox(Center - FVector(Size, Size, Size), Center + FVector(Size, Size, Size)), Color, SDPG_Foreground);
	}

	void DrawDebugVerticalMarker(FPrimitiveDrawInterface* PDI, const FVector& Center, const FLinearColor& Color, float Height, float Radius, float Thickness)
	{
		const FVector Bottom = Center + FVector(0.f, 0.f, 8.f);
		const FVector Top = Center + FVector(0.f, 0.f, Height);

		PDI->DrawLine(Bottom, Top, Color, SDPG_Foreground, Thickness);

		DrawWireBox(PDI, FBox(Top - FVector(Radius, Radius, Radius), Top + FVector(Radius, Radius, Radius)), Color.ToFColor(true), SDPG_Foreground);
	}

	bool BuildConnectorDrawData(
		AGridLevelEditorActor* EditorActor, const FGridLevelObjectData& SourceObject, const FGridLevelObjectData& TargetObject, FConnectorDrawData& OutDrawData)
	{
		FVector SourceCenter = FVector::ZeroVector;
		FVector TargetCenter = FVector::ZeroVector;
		if (!EditorActor->GetObjectEditorWorldCenter(SourceObject, SourceCenter) || !EditorActor->GetObjectEditorWorldCenter(TargetObject, TargetCenter))
		{
			return false;
		}

		const FVector Delta = TargetCenter - SourceCenter;
		const float Distance = Delta.Size();
		if (Distance < 10.f)
		{
			return false;
		}

		const FVector Direction = Delta / Distance;
		OutDrawData.SourceCenter = SourceCenter;
		OutDrawData.TargetCenter = TargetCenter;
		OutDrawData.Direction = Direction;
		OutDrawData.ArrowEnd = TargetCenter - Direction * 12.f;
		return true;
	}

	void DrawConnectorArrow(FPrimitiveDrawInterface* PDI, const FConnectorDrawData& DrawData, EGridObjectCommand Command, bool bIncoming)
	{
		const FLinearColor LinkColor = GetDebugLinkColor(Command, bIncoming);

		DrawDashedLine(PDI, DrawData.SourceCenter, DrawData.ArrowEnd, LinkColor, 16.f, 9.f, 1.f, SDPG_Foreground);
		DrawArrowHead(PDI, DrawData.ArrowEnd, DrawData.Direction, LinkColor, 22.f, 1.f, SDPG_Foreground);
	}

	FText GetConnectorLabelText(EGridObjectEvent SourceEvent, EGridObjectCommand Command)
	{
		const UEnum* EventEnum = StaticEnum<EGridObjectEvent>();
		const UEnum* CommandEnum = StaticEnum<EGridObjectCommand>();
		const FText EventText = EventEnum ? EventEnum->GetDisplayNameTextByValue(static_cast<int64>(SourceEvent)) : FText::FromString(TEXT("Event"));
		const FText CommandText = CommandEnum ? CommandEnum->GetDisplayNameTextByValue(static_cast<int64>(Command)) : FText::FromString(TEXT("Command"));

		return FText::Format(FText::FromString(TEXT("{0} / {1}")), EventText, CommandText);
	}

	bool ShouldDrawConnectorForSelection(
		const AGridLevelEditorActor* EditorActor, const FGridObjectLink& Link, const FGridLevelObjectData& SelectedObject, bool& bOutIncoming)
	{
		const bool bOutgoing = Link.SourceObjectId == SelectedObject.ObjectId;
		const bool bIncoming = Link.TargetObjectId == SelectedObject.ObjectId;

		if ((!EditorActor->bShowOutgoingConnectors || !bOutgoing) && (!EditorActor->bShowIncomingConnectors || !bIncoming))
		{
			return false;
		}

		bOutIncoming = bIncoming && !bOutgoing;
		return true;
	}

	FVector GetPatrolWaypointWorldCenter(const AGridLevelEditorActor* EditorActor, const FGridMonsterPatrolWaypoint& Waypoint, float ZOffset = 35.0f)
	{
		if (!EditorActor || !EditorActor->LevelAsset)
		{
			return FVector::ZeroVector;
		}

		const float CellSize = EditorActor->LevelAsset->CellSize;
		const float Half = CellSize * 0.5f;
		const FVector GridWorldOrigin = EditorActor->PreviewRuntimeActor
			? EditorActor->PreviewRuntimeActor->GetActorLocation() + EditorActor->PreviewRuntimeActor->GridOrigin
			: EditorActor->GetActorLocation();
		return GridWorldOrigin + FVector((Waypoint.Cell.X * CellSize) + Half, (Waypoint.Cell.Y * CellSize) + Half, ZOffset);
	}

	FVector GetPatrolFacingVector(EGridEdge Facing)
	{
		switch (Facing)
		{
			case EGridEdge::North:
				return FVector(0.f, 1.f, 0.f);
			case EGridEdge::East:
				return FVector(1.f, 0.f, 0.f);
			case EGridEdge::South:
				return FVector(0.f, -1.f, 0.f);
			case EGridEdge::West:
				return FVector(-1.f, 0.f, 0.f);
			default:
				return FVector::ZeroVector;
		}
	}

	FString GetPatrolModeText(EGridMonsterPatrolMode Mode)
	{
		if (const UEnum* PatrolEnum = StaticEnum<EGridMonsterPatrolMode>())
		{
			return PatrolEnum->GetNameStringByValue(static_cast<int64>(Mode));
		}
		return TEXT("None");
	}

	FString GetPatrolFacingText(EGridEdge Facing)
	{
		if (const UEnum* EdgeEnum = StaticEnum<EGridEdge>())
		{
			return EdgeEnum->GetNameStringByValue(static_cast<int64>(Facing));
		}
		return TEXT("None");
	}

	void DrawSelectedMonsterPatrolRoute(AGridLevelEditorActor* EditorActor, const FGridLevelObjectData& SelectedObject, FPrimitiveDrawInterface* PDI)
	{
		if (!EditorActor || !PDI || SelectedObject.Type != EGridLevelObjectType::MonsterSpawn || !EditorActor->bShowSelectedMonsterPatrolRoute)
		{
			return;
		}

		const TArray<FGridMonsterPatrolWaypoint>& Waypoints = SelectedObject.PatrolWaypoints;
		if (Waypoints.Num() == 0)
		{
			return;
		}

		const FLinearColor RouteColor(0.15f, 0.85f, 1.0f, 0.95f);
		const FLinearColor ClosingColor(0.55f, 0.75f, 1.0f, 0.85f);
		const float CellSize = EditorActor->LevelAsset ? EditorActor->LevelAsset->CellSize : 200.0f;

		for (int32 Index = 0; Index + 1 < Waypoints.Num(); ++Index)
		{
			const FVector Start = GetPatrolWaypointWorldCenter(EditorActor, Waypoints[Index]);
			const FVector End = GetPatrolWaypointWorldCenter(EditorActor, Waypoints[Index + 1]);
			PDI->DrawLine(Start, End, RouteColor, SDPG_Foreground, 3.0f);
			DrawArrowHead(PDI, End, End - Start, RouteColor, 20.0f, 2.0f, SDPG_Foreground);

			if (SelectedObject.PatrolMode == EGridMonsterPatrolMode::PingPong)
			{
				DrawArrowHead(PDI, Start, Start - End, RouteColor, 20.0f, 2.0f, SDPG_Foreground);
			}
		}

		if (SelectedObject.PatrolMode == EGridMonsterPatrolMode::Loop && Waypoints.Num() > 1)
		{
			const FVector Start = GetPatrolWaypointWorldCenter(EditorActor, Waypoints.Last());
			const FVector End = GetPatrolWaypointWorldCenter(EditorActor, Waypoints[0]);
			DrawDashedLine(PDI, Start, End, ClosingColor, 20.0f, 10.0f, 2.0f, SDPG_Foreground);
			DrawArrowHead(PDI, End, End - Start, ClosingColor, 20.0f, 2.0f, SDPG_Foreground);
		}

		for (int32 Index = 0; Index < Waypoints.Num(); ++Index)
		{
			const FGridMonsterPatrolWaypoint& Waypoint = Waypoints[Index];
			const FVector Center = GetPatrolWaypointWorldCenter(EditorActor, Waypoint);
			const bool bSelected = EditorActor->IsPatrolRouteEditModeActive() && EditorActor->SelectedPatrolWaypointIndex == Index;
			const FColor MarkerColor = bSelected ? FColor::Yellow : FColor::Cyan;
			const float Radius = bSelected ? 24.0f : 17.0f;
			DrawWireBox(PDI, FBox(Center - FVector(Radius, Radius, Radius), Center + FVector(Radius, Radius, Radius)), MarkerColor, SDPG_Foreground);

			const FVector FacingVector = GetPatrolFacingVector(Waypoint.Facing);
			if (!FacingVector.IsNearlyZero())
			{
				const FVector FacingEnd = Center + FacingVector * FMath::Max(40.0f, CellSize * 0.28f);
				const FLinearColor FacingColor = bSelected ? FLinearColor(1.0f, 0.8f, 0.1f, 1.0f) : FLinearColor(0.25f, 1.0f, 0.55f, 0.9f);
				PDI->DrawLine(Center, FacingEnd, FacingColor, SDPG_Foreground, 2.0f);
				DrawArrowHead(PDI, FacingEnd, FacingVector, FacingColor, 18.0f, 2.0f, SDPG_Foreground);
			}
		}
	}
}

const FEditorModeID FGridLevelEdMode::EM_GridLevelEdModeId = TEXT("EM_GrimrockGridLevelEdMode");

static bool IsRightMouseButtonDown(FViewport* Viewport)
{
	return Viewport && Viewport->KeyState(EKeys::RightMouseButton);
}

AGridLevelEditorActor* FGridLevelEdMode::FindEditorActor() const
{
	if (!GEditor)
	{
		return nullptr;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AGridLevelEditorActor> It(World); It; ++It)
	{
		return *It;
	}

	return nullptr;
}

bool FGridLevelEdMode::UpdateHoverFromMouse(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 MouseX, int32 MouseY) const
{
	AGridLevelEditorActor* EditorActor = FindEditorActor();
	if (!EditorActor || !EditorActor->LevelAsset)
	{
		return false;
	}

	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(Viewport, ViewportClient->GetScene(), ViewportClient->EngineShowFlags));

	FSceneView* SceneView = ViewportClient->CalcSceneView(&ViewFamily);
	if (!SceneView)
	{
		return false;
	}

	FVector RayOrigin = FVector::ZeroVector;
	FVector RayDirection = FVector::ForwardVector;
	SceneView->DeprojectFVector2D(FVector2D(MouseX, MouseY), RayOrigin, RayDirection);

	EditorActor->ResolvePreviewRuntimeActor();

	FVector GridOrigin = FVector::ZeroVector;
	if (EditorActor->PreviewRuntimeActor)
	{
		GridOrigin = EditorActor->PreviewRuntimeActor->GetActorLocation() + EditorActor->PreviewRuntimeActor->GridOrigin;
	}

	const FPlane GridPlane(GridOrigin, FVector::UpVector);
	const FVector RayEnd = RayOrigin + (RayDirection * EditorActor->ViewportPickTraceDistance);
	const FVector HitPoint = FMath::LinePlaneIntersection(RayOrigin, RayEnd, GridPlane);

	const float ForwardDot = FVector::DotProduct(RayDirection, FVector::UpVector);
	if (FMath::IsNearlyZero(ForwardDot))
	{
		return false;
	}

	const bool bGridHoverOk = EditorActor->ApplyGridHoverFromWorldPoint(HitPoint);

	if (EditorActor->ActiveTool == EGridEditorTool::Select || EditorActor->ActiveTool == EGridEditorTool::Link)
	{
		EditorActor->UpdateHoveredObjectFromWorldPoint(HitPoint);
	}

	return bGridHoverOk;
}

void FGridLevelEdMode::ApplyPaint() const
{
	if (AGridLevelEditorActor* EditorActor = FindEditorActor())
	{
		EditorActor->ApplyPrimaryToolAction();

		if (Toolkit.IsValid())
		{
			Toolkit->RefreshPalette();
		}
	}
}

void FGridLevelEdMode::ApplyErase() const
{
	if (AGridLevelEditorActor* EditorActor = FindEditorActor())
	{
		EditorActor->ApplySecondaryToolAction();

		if (Toolkit.IsValid())
		{
			Toolkit->RefreshPalette();
		}
	}
}

bool FGridLevelEdMode::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	AGridLevelEditorActor* EditorActor = FindEditorActor();

	if (Event == IE_Pressed && Key == EKeys::P && EditorActor)
	{
		EditorActor->ToggleSelectedMonsterPatrolRouteEditing();
		bIsPainting = false;
		ResetPaintCache();
		if (Toolkit.IsValid())
		{
			Toolkit->RefreshPalette();
		}
		return true;
	}

	if (Key == EKeys::RightMouseButton)
	{
		bIsPainting = false;
		ResetPaintCache();

		return FEdMode::InputKey(ViewportClient, Viewport, Key, Event);
	}
	if (IsRightMouseButtonDown(Viewport))
	{
		bIsPainting = false;
		ResetPaintCache();

		return FEdMode::InputKey(ViewportClient, Viewport, Key, Event);
	}

	if (EditorActor && EditorActor->IsPatrolRouteEditModeActive())
	{
		if (Event == IE_Pressed)
		{
			bool bHandledPatrolCommand = true;
			if (Key == EKeys::M)
			{
				EditorActor->CycleSelectedMonsterPatrolMode();
			}
			else if (Key == EKeys::F)
			{
				EditorActor->CycleSelectedPatrolWaypointFacing();
			}
			else if (Key == EKeys::Delete || Key == EKeys::BackSpace)
			{
				EditorActor->RemoveSelectedPatrolWaypoint();
			}
			else if (Key == EKeys::PageUp)
			{
				EditorActor->MoveSelectedPatrolWaypointEarlier();
			}
			else if (Key == EKeys::PageDown)
			{
				EditorActor->MoveSelectedPatrolWaypointLater();
			}
			else if (Key == EKeys::Hyphen || Key == EKeys::Subtract)
			{
				EditorActor->DecreaseSelectedPatrolWaypointWait();
			}
			else if (Key == EKeys::Equals || Key == EKeys::Add)
			{
				EditorActor->IncreaseSelectedPatrolWaypointWait();
			}
			else
			{
				bHandledPatrolCommand = false;
			}

			if (bHandledPatrolCommand)
			{
				if (Toolkit.IsValid())
				{
					Toolkit->RefreshPalette();
				}
				return true;
			}
		}

		if (Key == EKeys::LeftMouseButton)
		{
			if (Event == IE_Pressed)
			{
				FIntPoint MousePos;
				Viewport->GetMousePos(MousePos);
				if (UpdateHoverFromMouse(ViewportClient, Viewport, MousePos.X, MousePos.Y))
				{
					EditorActor->AddOrSelectPatrolWaypointAtHoveredCell();
					if (Toolkit.IsValid())
					{
						Toolkit->RefreshPalette();
					}
				}
				return true;
			}
			if (Event == IE_Released)
			{
				return true;
			}
		}
	}

	if (Key == EKeys::LeftMouseButton)
	{
		if (Event == IE_Pressed)
		{
			bIsPainting = true;
			ResetPaintCache();

			FIntPoint MousePos;
			Viewport->GetMousePos(MousePos);

			if (UpdateHoverFromMouse(ViewportClient, Viewport, MousePos.X, MousePos.Y))
			{
				if (EditorActor || (EditorActor = FindEditorActor()))
				{
					if (CommitHoveredSelection(EditorActor) && ShouldApplyPaintForCurrentSelection(EditorActor))
					{
						ApplyPaint();
					}
				}
			}
			return true;
		}
		if (Event == IE_Released)
		{
			bIsPainting = false;
			ResetPaintCache();
			return true;
		}
	}
	return FEdMode::InputKey(ViewportClient, Viewport, Key, Event);
}

bool FGridLevelEdMode::MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 X, int32 Y)
{
	UpdateHoverFromMouse(ViewportClient, Viewport, X, Y);
	return FEdMode::MouseMove(ViewportClient, Viewport, X, Y);
}

bool FGridLevelEdMode::ProcessCapturedMouseMoves(FEditorViewportClient* InViewportClient, FViewport* InViewport, const TArrayView<FIntPoint>& MouseMoves)
{
	if (IsRightMouseButtonDown(InViewport))
	{
		bIsPainting = false;
		ResetPaintCache();

		return FEdMode::ProcessCapturedMouseMoves(InViewportClient, InViewport, MouseMoves);
	}

	if (const AGridLevelEditorActor* EditorActor = FindEditorActor())
	{
		if (EditorActor->IsPatrolRouteEditModeActive())
		{
			bIsPainting = false;
			ResetPaintCache();
			return true;
		}
	}

	if (!bIsPainting)
	{
		return FEdMode::ProcessCapturedMouseMoves(InViewportClient, InViewport, MouseMoves);
	}

	FIntPoint MousePos;
	InViewport->GetMousePos(MousePos);

	if (UpdateHoverFromMouse(InViewportClient, InViewport, MousePos.X, MousePos.Y))
	{
		if (AGridLevelEditorActor* EditorActor = FindEditorActor())
		{
			if (CommitHoveredSelection(EditorActor) && ShouldApplyPaintForCurrentSelection(EditorActor))
			{
				ApplyPaint();
			}
		}
	}

	return true;
}

void FGridLevelEdMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	FEdMode::Render(View, Viewport, PDI);

	AGridLevelEditorActor* EditorActor = FindEditorActor();
	if (!EditorActor || !EditorActor->IsSelectionValidForEditing() || !EditorActor->LevelAsset)
	{
		return;
	}

	const FVector Center = EditorActor->GetSelectionPreviewCenter(4.f);
	const float Half = EditorActor->LevelAsset->CellSize * 0.5f;

	FColor MainColor = FColor::Yellow;

	switch (EditorActor->ActiveTool)
	{
		case EGridEditorTool::PaintCell:
			MainColor = FColor::Green;
			break;
		case EGridEditorTool::PaintWall:
			MainColor = FColor::Cyan;
			break;
		case EGridEditorTool::PaintObject:
			MainColor = FColor::Yellow;
			break;
		case EGridEditorTool::Erase:
			MainColor = FColor::Red;
			break;
		case EGridEditorTool::Select:
			MainColor = FColor::White;
			break;
		case EGridEditorTool::Link:
			MainColor = FColor::Cyan;
			break;
		default:
			break;
	}
	DrawWireBox(PDI, FBox(Center - FVector(Half, Half, 4.f), Center + FVector(Half, Half, 300.f)), MainColor, SDPG_Foreground);

	const FVector GridWorldOrigin = EditorActor->PreviewRuntimeActor
		? EditorActor->PreviewRuntimeActor->GetActorLocation() + EditorActor->PreviewRuntimeActor->GridOrigin
		: EditorActor->GetActorLocation();

	FVector HoverEdgeCenter = Center;
	if (EditorActor->LevelAsset->IsValidCoord(EditorActor->HoveredCellX, EditorActor->HoveredCellY))
	{
		const float CellSize = EditorActor->LevelAsset->CellSize;
		const FVector HoverCenter =
			GridWorldOrigin + FVector((EditorActor->HoveredCellX * CellSize) + Half, (EditorActor->HoveredCellY * CellSize) + Half, 4.f);

		switch (EditorActor->HoveredEdge)
		{
			case EGridEdge::North:
				HoverEdgeCenter = HoverCenter + FVector(0.f, Half, 0.f);
				break;
			case EGridEdge::East:
				HoverEdgeCenter = HoverCenter + FVector(Half, 0.f, 0.f);
				break;
			case EGridEdge::South:
				HoverEdgeCenter = HoverCenter + FVector(0.f, -Half, 0.f);
				break;
			case EGridEdge::West:
				HoverEdgeCenter = HoverCenter + FVector(-Half, 0.f, 0.f);
				break;
			default:
				HoverEdgeCenter = HoverCenter;
				break;
		}
	}

	DrawWireBox(PDI, FBox(HoverEdgeCenter - FVector(12.f, 12.f, 12.f), HoverEdgeCenter + FVector(12.f, 12.f, 12.f)), FColor::Orange, SDPG_Foreground);

	FVector SourceLocation = FVector::ZeroVector;
	if (EditorActor->HasPendingLinkSource() && EditorActor->TryGetPendingLinkSourceLocation(SourceLocation))
	{
		DrawWireBox(PDI, FBox(SourceLocation - FVector(16.f, 16.f, 16.f), SourceLocation + FVector(16.f, 16.f, 16.f)), FColor::Cyan, SDPG_Foreground);

		FVector HoverLocation = FVector::ZeroVector;
		if (EditorActor->TryGetSelectedObjectWorldLocation(HoverLocation))
		{
			PDI->DrawLine(SourceLocation, HoverLocation, FLinearColor{ 0.f, 1.f, 1.f, 1.f }, SDPG_Foreground, 2.0f);
		}
		else
		{
			PDI->DrawLine(SourceLocation, Center, FLinearColor{ 0.f, 1.f, 1.f, 1.f }, SDPG_Foreground, 1.5f);
		}
	}

	const FGridLevelObjectData* SelectedObject = EditorActor->GetSelectedObjectData();
	if (SelectedObject)
	{
		DrawSelectedMonsterPatrolRoute(EditorActor, *SelectedObject, PDI);

		for (const FGridObjectLink& Link : EditorActor->LevelAsset->Links)
		{
			bool bIncoming = false;
			if (!ShouldDrawConnectorForSelection(EditorActor, Link, *SelectedObject, bIncoming))
			{
				continue;
			}

			const FGridLevelObjectData* SourceObject = EditorActor->LevelAsset->Objects.FindByPredicate(
				[&Link](const FGridLevelObjectData& Obj)
				{
					return Obj.ObjectId == Link.SourceObjectId;
				});
			const FGridLevelObjectData* TargetObject = EditorActor->LevelAsset->Objects.FindByPredicate(
				[&Link](const FGridLevelObjectData& Obj)
				{
					return Obj.ObjectId == Link.TargetObjectId;
				});

			if (!SourceObject || !TargetObject)
			{
				continue;
			}

			FConnectorDrawData DrawData;
			if (BuildConnectorDrawData(EditorActor, *SourceObject, *TargetObject, DrawData))
			{
				DrawConnectorArrow(PDI, DrawData, Link.Command, bIncoming);
			}
		}
	}
}

void FGridLevelEdMode::DrawHUD(FEditorViewportClient* ViewportClient, FViewport* Viewport, const FSceneView* View, FCanvas* Canvas)
{
	FEdMode::DrawHUD(ViewportClient, Viewport, View, Canvas);

	AGridLevelEditorActor* EditorActor = FindEditorActor();
	if (!EditorActor || !EditorActor->IsSelectionValidForEditing() || !EditorActor->LevelAsset || !View || !Canvas || !GEngine)
	{
		return;
	}

	const FGridLevelObjectData* SelectedObject = EditorActor->GetSelectedObjectData();
	if (!SelectedObject)
	{
		return;
	}

	if (SelectedObject->Type == EGridLevelObjectType::MonsterSpawn && EditorActor->bShowSelectedMonsterPatrolRoute)
	{
		for (int32 Index = 0; Index < SelectedObject->PatrolWaypoints.Num(); ++Index)
		{
			const FGridMonsterPatrolWaypoint& Waypoint = SelectedObject->PatrolWaypoints[Index];
			const FVector LabelWorldPosition = GetPatrolWaypointWorldCenter(EditorActor, Waypoint, 75.0f);
			FVector2D PixelLocation = FVector2D::ZeroVector;
			if (!View->WorldToPixel(LabelWorldPosition, PixelLocation))
			{
				continue;
			}

			const bool bSelected = EditorActor->IsPatrolRouteEditModeActive() && EditorActor->SelectedPatrolWaypointIndex == Index;
			const FString Label = FString::Printf(TEXT("#%d%s"), Index + 1, bSelected ? TEXT(" *") : TEXT(""));
			Canvas->DrawShadowedString(PixelLocation.X, PixelLocation.Y, *Label, GEngine->GetSmallFont(),
				bSelected ? FLinearColor(1.0f, 0.82f, 0.12f, 1.0f) : FLinearColor(0.25f, 0.95f, 1.0f, 0.95f));
		}

		if (EditorActor->IsPatrolRouteEditModeActive())
		{
			const FString Header = FString::Printf(
				TEXT("PATROL ROUTE EDIT  Mode=%s  Waypoints=%d"), *GetPatrolModeText(SelectedObject->PatrolMode), SelectedObject->PatrolWaypoints.Num());
			Canvas->DrawShadowedString(20.0f, 85.0f, *Header, GEngine->GetSmallFont(), FLinearColor(1.0f, 0.82f, 0.12f, 1.0f));

			Canvas->DrawShadowedString(20.0f, 103.0f, TEXT("Left click add/select | Delete remove | M mode | F facing | PgUp/PgDn reorder | -/+ wait | P exit"),
				GEngine->GetSmallFont(), FLinearColor(0.92f, 0.92f, 0.92f, 0.95f));

			if (SelectedObject->PatrolWaypoints.IsValidIndex(EditorActor->SelectedPatrolWaypointIndex))
			{
				const FGridMonsterPatrolWaypoint& Waypoint = SelectedObject->PatrolWaypoints[EditorActor->SelectedPatrolWaypointIndex];
				const FString Detail = FString::Printf(TEXT("Waypoint #%d Cell=(%d,%d) Facing=%s Wait=%.1fs"), EditorActor->SelectedPatrolWaypointIndex + 1,
					Waypoint.Cell.X, Waypoint.Cell.Y, *GetPatrolFacingText(Waypoint.Facing), Waypoint.WaitSeconds);
				Canvas->DrawShadowedString(20.0f, 121.0f, *Detail, GEngine->GetSmallFont(), FLinearColor(0.25f, 1.0f, 0.55f, 0.95f));
			}
		}
	}

	if (!EditorActor->bShowConnectorLabels)
	{
		return;
	}

	TMap<FString, int32> PairLabelCounts;
	for (const FGridObjectLink& Link : EditorActor->LevelAsset->Links)
	{
		bool bIncoming = false;
		if (!ShouldDrawConnectorForSelection(EditorActor, Link, *SelectedObject, bIncoming))
		{
			continue;
		}

		const FGridLevelObjectData* SourceObject = EditorActor->LevelAsset->Objects.FindByPredicate(
			[&Link](const FGridLevelObjectData& Obj)
			{
				return Obj.ObjectId == Link.SourceObjectId;
			});
		const FGridLevelObjectData* TargetObject = EditorActor->LevelAsset->Objects.FindByPredicate(
			[&Link](const FGridLevelObjectData& Obj)
			{
				return Obj.ObjectId == Link.TargetObjectId;
			});

		if (!SourceObject || !TargetObject)
		{
			continue;
		}

		FConnectorDrawData DrawData;
		if (!BuildConnectorDrawData(EditorActor, *SourceObject, *TargetObject, DrawData))
		{
			continue;
		}

		const FString PairKey =
			FString::Printf(TEXT("%s:%s"), *Link.SourceObjectId.ToString(EGuidFormats::Digits), *Link.TargetObjectId.ToString(EGuidFormats::Digits));
		int32& PairLabelIndex = PairLabelCounts.FindOrAdd(PairKey);

		const FVector LabelWorldPosition = ((DrawData.SourceCenter + DrawData.TargetCenter) * 0.5f) + FVector(0.f, 0.f, 20.f + (10.f * PairLabelIndex));
		PairLabelIndex++;

		FVector2D PixelLocation = FVector2D::ZeroVector;
		if (!View->WorldToPixel(LabelWorldPosition, PixelLocation))
		{
			continue;
		}

		const FText LabelText = GetConnectorLabelText(Link.SourceEvent, Link.Command);
		Canvas->DrawShadowedString(PixelLocation.X, PixelLocation.Y, *LabelText.ToString(), GEngine->GetSmallFont(), FLinearColor(0.86f, 0.88f, 0.9f, 0.9f));
	}
}

void FGridLevelEdMode::Enter()
{
	FEdMode::Enter();

	LastObservedSelectedCellX = INDEX_NONE;
	LastObservedSelectedCellY = INDEX_NONE;
	LastObservedSelectedEdge = EGridEdge::None;
	LastObservedSelectedObjectId.Invalidate();

	if (!Toolkit.IsValid() && UsesToolkits())
	{
		Toolkit = MakeShareable(new FGridLevelEdModeToolkit);
		Toolkit->Init(Owner->GetToolkitHost());
	}
}

void FGridLevelEdMode::Exit()
{
	if (AGridLevelEditorActor* EditorActor = FindEditorActor())
	{
		if (EditorActor->IsPatrolRouteEditModeActive())
		{
			EditorActor->ToggleSelectedMonsterPatrolRouteEditing();
		}
	}

	if (Toolkit.IsValid())
	{
		FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
		Toolkit.Reset();
	}

	FEdMode::Exit();
}

void FGridLevelEdMode::ResetPaintCache() const
{
	LastPaintCellX = INDEX_NONE;
	LastPaintCellY = INDEX_NONE;
	LastPaintEdge = EGridEdge::None;
	LastPaintTool = EGridEditorTool::Select;
}

bool FGridLevelEdMode::ShouldApplyPaintForCurrentSelection(const AGridLevelEditorActor* EditorActor) const
{
	if (!EditorActor)
	{
		return false;
	}

	if (EditorActor->SelectedCellX == LastPaintCellX && EditorActor->SelectedCellY == LastPaintCellY && EditorActor->SelectedEdge == LastPaintEdge &&
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

bool FGridLevelEdMode::CommitHoveredSelection(AGridLevelEditorActor* EditorActor) const
{
	if (!EditorActor)
	{
		return false;
	}

	const bool bCommitted = EditorActor->CommitHoveredCellSelection();
	if (bCommitted)
	{
		RefreshToolkitIfObservedSelectionChanged(EditorActor);
	}

	return bCommitted;
}

void FGridLevelEdMode::RefreshToolkitIfObservedSelectionChanged(const AGridLevelEditorActor* EditorActor) const
{
	if (!EditorActor || !Toolkit.IsValid())
	{
		return;
	}

	const bool bSelectionChanged = LastObservedSelectedCellX != EditorActor->SelectedCellX || LastObservedSelectedCellY != EditorActor->SelectedCellY ||
		LastObservedSelectedEdge != EditorActor->SelectedEdge || LastObservedSelectedObjectId != EditorActor->LastSelectedObjectId;

	if (!bSelectionChanged)
	{
		return;
	}

	LastObservedSelectedCellX = EditorActor->SelectedCellX;
	LastObservedSelectedCellY = EditorActor->SelectedCellY;
	LastObservedSelectedEdge = EditorActor->SelectedEdge;
	LastObservedSelectedObjectId = EditorActor->LastSelectedObjectId;

	Toolkit->RefreshPalette();
}

#endif
// WITH_EDITOR
