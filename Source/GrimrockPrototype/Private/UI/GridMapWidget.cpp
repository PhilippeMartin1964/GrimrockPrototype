#include "UI/GridMapWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Core/GridDungeonAsset.h"
#include "Rendering/DrawElementTypes.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Styling/CoreStyle.h"

namespace GridMapWidgetPrivate
{
	struct FRenderMetrics
	{
		int32 MinX = 0;
		int32 MaxX = 0;
		int32 MinY = 0;
		int32 MaxY = 0;
		float CellSize = 0.0f;
		FVector2f Origin = FVector2f::ZeroVector;

		FVector2f CellTopLeft(const FIntPoint& MapCell) const
		{
			// Match the established editor overview convention: canonical X+ (East)
			// is displayed toward screen-left, while canonical Y+ (North) is screen-up.
			return FVector2f(
				Origin.X + static_cast<float>(MaxX - MapCell.X) * CellSize,
				Origin.Y + static_cast<float>(MaxY - MapCell.Y) * CellSize);
		}

		FVector2f CellCenter(const FIntPoint& MapCell) const
		{
			return CellTopLeft(MapCell) + FVector2f(CellSize * 0.5f, CellSize * 0.5f);
		}
	};

	bool BuildRenderMetrics(
		const FGridMapFloorView& View,
		const FGeometry& Geometry,
		const FMargin& Padding,
		float MaxCellPixels,
		FRenderMetrics& OutMetrics)
	{
		if (View.Cells.IsEmpty())
		{
			return false;
		}

		OutMetrics.MinX = View.Cells[0].MapCell.X;
		OutMetrics.MaxX = View.Cells[0].MapCell.X;
		OutMetrics.MinY = View.Cells[0].MapCell.Y;
		OutMetrics.MaxY = View.Cells[0].MapCell.Y;
		for (const FGridMapFloorCellView& Cell : View.Cells)
		{
			OutMetrics.MinX = FMath::Min(OutMetrics.MinX, Cell.MapCell.X);
			OutMetrics.MaxX = FMath::Max(OutMetrics.MaxX, Cell.MapCell.X);
			OutMetrics.MinY = FMath::Min(OutMetrics.MinY, Cell.MapCell.Y);
			OutMetrics.MaxY = FMath::Max(OutMetrics.MaxY, Cell.MapCell.Y);
		}
		if (View.bHasPartyMarker)
		{
			OutMetrics.MinX = FMath::Min(OutMetrics.MinX, View.PartyMapCell.X);
			OutMetrics.MaxX = FMath::Max(OutMetrics.MaxX, View.PartyMapCell.X);
			OutMetrics.MinY = FMath::Min(OutMetrics.MinY, View.PartyMapCell.Y);
			OutMetrics.MaxY = FMath::Max(OutMetrics.MaxY, View.PartyMapCell.Y);
		}

		const FVector2D LocalSize = Geometry.GetLocalSize();
		const float AvailableWidth = FMath::Max(0.0f, static_cast<float>(LocalSize.X) - Padding.Left - Padding.Right);
		const float AvailableHeight = FMath::Max(0.0f, static_cast<float>(LocalSize.Y) - Padding.Top - Padding.Bottom);
		const int32 CellCountX = OutMetrics.MaxX - OutMetrics.MinX + 1;
		const int32 CellCountY = OutMetrics.MaxY - OutMetrics.MinY + 1;
		if (AvailableWidth <= 0.0f || AvailableHeight <= 0.0f || CellCountX <= 0 || CellCountY <= 0)
		{
			return false;
		}

		OutMetrics.CellSize = FMath::Min(
			FMath::Min(AvailableWidth / static_cast<float>(CellCountX), AvailableHeight / static_cast<float>(CellCountY)),
			FMath::Max(4.0f, MaxCellPixels));
		if (OutMetrics.CellSize <= 0.0f)
		{
			return false;
		}

		const float MapWidth = static_cast<float>(CellCountX) * OutMetrics.CellSize;
		const float MapHeight = static_cast<float>(CellCountY) * OutMetrics.CellSize;
		OutMetrics.Origin = FVector2f(
			Padding.Left + (AvailableWidth - MapWidth) * 0.5f,
			Padding.Top + (AvailableHeight - MapHeight) * 0.5f);
		return true;
	}

	void DrawLine(
		FSlateWindowElementList& OutDrawElements,
		int32 Layer,
		const FPaintGeometry& PaintGeometry,
		const FVector2f& Start,
		const FVector2f& End,
		const FLinearColor& Color,
		float Thickness)
	{
		TArray<FVector2f> Points;
		Points.Reserve(2);
		Points.Add(Start);
		Points.Add(End);
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			static_cast<uint32>(Layer),
			PaintGeometry,
			MoveTemp(Points),
			ESlateDrawEffect::None,
			Color,
			true,
			Thickness);
	}

	void GetBoundaryEndpoints(
		const FRenderMetrics& Metrics,
		const FGridMapFloorBoundaryView& Boundary,
		FVector2f& OutA,
		FVector2f& OutB)
	{
		const FVector2f TopLeft = Metrics.CellTopLeft(Boundary.MapCell);
		const float S = Metrics.CellSize;
		switch (Boundary.Edge)
		{
			case EGridEdge::North:
				OutA = TopLeft;
				OutB = TopLeft + FVector2f(S, 0.0f);
				break;
			case EGridEdge::East:
				// X+ is screen-left in the established overview/map presentation.
				OutA = TopLeft;
				OutB = TopLeft + FVector2f(0.0f, S);
				break;
			case EGridEdge::South:
				OutA = TopLeft + FVector2f(0.0f, S);
				OutB = TopLeft + FVector2f(S, S);
				break;
			case EGridEdge::West:
				OutA = TopLeft + FVector2f(S, 0.0f);
				OutB = TopLeft + FVector2f(S, S);
				break;
			default:
				OutA = TopLeft;
				OutB = TopLeft;
				break;
		}
	}

	void DrawDoorBoundary(
		FSlateWindowElementList& OutDrawElements,
		int32 Layer,
		const FPaintGeometry& PaintGeometry,
		const FVector2f& A,
		const FVector2f& B,
		const FLinearColor& Color,
		float Thickness,
		bool bOpen,
		bool bSecret)
	{
		const FVector2f Delta = B - A;
		const FVector2f P0 = A;
		const FVector2f P1 = A + Delta * 0.32f;
		const FVector2f P2 = A + Delta * 0.68f;
		const FVector2f P3 = B;

		DrawLine(OutDrawElements, Layer, PaintGeometry, P0, P1, Color, Thickness);
		DrawLine(OutDrawElements, Layer, PaintGeometry, P2, P3, Color, Thickness);

		if (!bOpen)
		{
			DrawLine(OutDrawElements, Layer, PaintGeometry, P1, P2, Color, Thickness * 0.75f);
		}

		if (bSecret)
		{
			const FVector2f Center = (A + B) * 0.5f;
			FVector2f Perpendicular(-Delta.Y, Delta.X);
			if (Perpendicular.SizeSquared() > KINDA_SMALL_NUMBER)
			{
				Perpendicular.Normalize();
				Perpendicular *= FMath::Max(2.0f, Thickness * 1.5f);
				DrawLine(OutDrawElements, Layer + 1, PaintGeometry, Center - Perpendicular, Center + Perpendicular, Color, Thickness * 0.75f);
			}
		}
	}

	void DrawPartyMarker(
		FSlateWindowElementList& OutDrawElements,
		int32 Layer,
		const FPaintGeometry& PaintGeometry,
		const FRenderMetrics& Metrics,
		const FGridMapFloorView& View,
		const FLinearColor& Color)
	{
		if (!View.bHasPartyMarker)
		{
			return;
		}

		const FVector2f Center = Metrics.CellCenter(View.PartyMapCell);
		const float Radius = Metrics.CellSize * 0.28f;
		FVector2f Forward(0.0f, -1.0f);
		switch (View.PartyFacing)
		{
			case EGridEdge::North:
				Forward = FVector2f(0.0f, -1.0f);
				break;
			case EGridEdge::East:
				Forward = FVector2f(-1.0f, 0.0f);
				break;
			case EGridEdge::South:
				Forward = FVector2f(0.0f, 1.0f);
				break;
			case EGridEdge::West:
				Forward = FVector2f(1.0f, 0.0f);
				break;
			default:
				break;
		}
		const FVector2f Right(-Forward.Y, Forward.X);
		const FVector2f Tip = Center + Forward * Radius;
		const FVector2f Left = Center - Forward * Radius * 0.65f - Right * Radius * 0.70f;
		const FVector2f RightPoint = Center - Forward * Radius * 0.65f + Right * Radius * 0.70f;

		TArray<FVector2f> Triangle = { Tip, Left, RightPoint, Tip };
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			static_cast<uint32>(Layer),
			PaintGeometry,
			MoveTemp(Triangle),
			ESlateDrawEffect::None,
			Color,
			true,
			FMath::Max(2.0f, Metrics.CellSize * 0.08f));
	}
}

void UGridMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_LevelUp)
	{
		Button_LevelUp->OnClicked.AddUniqueDynamic(this, &UGridMapWidget::HandleLevelUpClicked);
	}
	if (Button_LevelDown)
	{
		Button_LevelDown->OnClicked.AddUniqueDynamic(this, &UGridMapWidget::HandleLevelDownClicked);
	}

	RefreshFloorNavigationControls();
}

void UGridMapWidget::NativeDestruct()
{
	if (Button_LevelUp)
	{
		Button_LevelUp->OnClicked.RemoveDynamic(this, &UGridMapWidget::HandleLevelUpClicked);
	}
	if (Button_LevelDown)
	{
		Button_LevelDown->OnClicked.RemoveDynamic(this, &UGridMapWidget::HandleLevelDownClicked);
	}

	Super::NativeDestruct();
}

void UGridMapWidget::InitializeMapWidget(AGrimrockPartyPawn* InPartyPawn)
{
	OwningPartyPawn = InPartyPawn;
	bHasFloorSelection = false;
	SelectPartyFloor();
}

bool UGridMapWidget::RefreshMap()
{
	if (!bHasFloorSelection)
	{
		return SelectPartyFloor();
	}
	return BuildSelectedFloorView();
}

bool UGridMapWidget::SelectPartyFloor()
{
	int32 PartyFloorZ = 0;
	if (!ResolvePartyFloorZ(PartyFloorZ))
	{
		bHasFloorSelection = false;
		FloorView.Reset();
		bHasRenderableMap = false;
		RefreshFloorNavigationControls();
		Invalidate(EInvalidateWidgetReason::Paint);
		return false;
	}

	SelectedFloorZ = PartyFloorZ;
	bHasFloorSelection = true;
	return BuildSelectedFloorView();
}

bool UGridMapWidget::NavigateFloorUp()
{
	int32 TargetFloorZ = 0;
	if (!FindAdjacentFloorZ(true, TargetFloorZ))
	{
		RefreshFloorNavigationControls();
		return false;
	}

	const int32 PreviousFloorZ = SelectedFloorZ;
	SelectedFloorZ = TargetFloorZ;
	if (!BuildSelectedFloorView())
	{
		SelectedFloorZ = PreviousFloorZ;
		BuildSelectedFloorView();
		return false;
	}
	return true;
}

bool UGridMapWidget::NavigateFloorDown()
{
	int32 TargetFloorZ = 0;
	if (!FindAdjacentFloorZ(false, TargetFloorZ))
	{
		RefreshFloorNavigationControls();
		return false;
	}

	const int32 PreviousFloorZ = SelectedFloorZ;
	SelectedFloorZ = TargetFloorZ;
	if (!BuildSelectedFloorView())
	{
		SelectedFloorZ = PreviousFloorZ;
		BuildSelectedFloorView();
		return false;
	}
	return true;
}

bool UGridMapWidget::CanNavigateFloorUp() const
{
	int32 IgnoredFloorZ = 0;
	return FindAdjacentFloorZ(true, IgnoredFloorZ);
}

bool UGridMapWidget::CanNavigateFloorDown() const
{
	int32 IgnoredFloorZ = 0;
	return FindAdjacentFloorZ(false, IgnoredFloorZ);
}

bool UGridMapWidget::BuildSelectedFloorView()
{
	FloorView.Reset();
	bHasRenderableMap = false;

	if (!bHasFloorSelection || !OwningPartyPawn || !OwningPartyPawn->LevelRuntimeActor)
	{
		RefreshFloorNavigationControls();
		Invalidate(EInvalidateWidgetReason::Paint);
		return false;
	}

	AGridLevelRuntimeActor* Runtime = OwningPartyPawn->LevelRuntimeActor;
	if (!Runtime->DungeonAsset)
	{
		RefreshFloorNavigationControls();
		Invalidate(EInvalidateWidgetReason::Paint);
		return false;
	}

	const UGridDoorSystemComponent* DoorSystem = Runtime->FindComponentByClass<UGridDoorSystemComponent>();
	bHasRenderableMap = FGridMapReadModelBuilder::BuildFloorView(
		*Runtime->DungeonAsset,
		Runtime->DungeonRuntimeState,
		Runtime->WorldObjectDefinitions,
		Runtime->CurrentDungeonLevelId,
		FIntPoint(OwningPartyPawn->CurrentCellX, OwningPartyPawn->CurrentCellY),
		OwningPartyPawn->Facing,
		SelectedFloorZ,
		DoorSystem,
		FloorView);

	RefreshFloorNavigationControls();
	Invalidate(EInvalidateWidgetReason::Paint);
	return bHasRenderableMap;
}

bool UGridMapWidget::ResolvePartyFloorZ(int32& OutFloorZ) const
{
	OutFloorZ = 0;
	if (!OwningPartyPawn || !OwningPartyPawn->LevelRuntimeActor)
	{
		return false;
	}

	const AGridLevelRuntimeActor* Runtime = OwningPartyPawn->LevelRuntimeActor;
	if (!Runtime->DungeonAsset)
	{
		return false;
	}

	const FGridDungeonLevelEntry* ActiveEntry = Runtime->DungeonAsset->FindLevelEntry(Runtime->CurrentDungeonLevelId);
	if (!ActiveEntry || !ActiveEntry->bEnabled || !ActiveEntry->LevelAsset)
	{
		return false;
	}

	OutFloorZ = ActiveEntry->LogicalPosition.Z;
	return true;
}

bool UGridMapWidget::FindAdjacentFloorZ(bool bUp, int32& OutFloorZ) const
{
	OutFloorZ = 0;
	if (!bHasFloorSelection || !OwningPartyPawn || !OwningPartyPawn->LevelRuntimeActor)
	{
		return false;
	}

	const AGridLevelRuntimeActor* Runtime = OwningPartyPawn->LevelRuntimeActor;
	if (!Runtime->DungeonAsset)
	{
		return false;
	}

	TArray<int32> AvailableFloorZs;
	FGridMapReadModelBuilder::GetAvailableFloorZs(*Runtime->DungeonAsset, AvailableFloorZs);
	if (bUp)
	{
		for (const int32 FloorZ : AvailableFloorZs)
		{
			if (FloorZ > SelectedFloorZ)
			{
				OutFloorZ = FloorZ;
				return true;
			}
		}
		return false;
	}

	for (int32 Index = AvailableFloorZs.Num() - 1; Index >= 0; --Index)
	{
		if (AvailableFloorZs[Index] < SelectedFloorZ)
		{
			OutFloorZ = AvailableFloorZs[Index];
			return true;
		}
	}
	return false;
}

void UGridMapWidget::RefreshFloorNavigationControls()
{
	if (Button_LevelUp)
	{
		Button_LevelUp->SetIsEnabled(CanNavigateFloorUp());
	}
	if (Button_LevelDown)
	{
		Button_LevelDown->SetIsEnabled(CanNavigateFloorDown());
	}
	if (Text_FloorLabel)
	{
		Text_FloorLabel->SetText(
			bHasFloorSelection
				? FText::Format(NSLOCTEXT("GridMap", "FloorLabel", "Étage {0}"), FText::AsNumber(SelectedFloorZ))
				: FText::GetEmpty());
	}
}

void UGridMapWidget::HandleLevelUpClicked()
{
	NavigateFloorUp();
}

void UGridMapWidget::HandleLevelDownClicked()
{
	NavigateFloorDown();
}

int32 UGridMapWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(
		Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	if (!bHasRenderableMap || FloorView.Cells.IsEmpty())
	{
		return BaseLayer;
	}

	using namespace GridMapWidgetPrivate;
	FRenderMetrics Metrics;
	if (!BuildRenderMetrics(FloorView, AllottedGeometry, MapDrawPadding, MaxCellPixels, Metrics))
	{
		return BaseLayer;
	}

	const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const int32 CellLayer = BaseLayer + 1;
	const int32 BoundaryLayer = CellLayer + 1;
	const int32 MarkerLayer = BoundaryLayer + 2;

	for (const FGridMapFloorCellView& Cell : FloorView.Cells)
	{
		const FVector2f TopLeft = Metrics.CellTopLeft(Cell.MapCell);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			static_cast<uint32>(CellLayer),
			AllottedGeometry.ToPaintGeometry(
				FVector2D(Metrics.CellSize, Metrics.CellSize),
				FSlateLayoutTransform(FVector2D(TopLeft.X, TopLeft.Y))),
			WhiteBrush,
			ESlateDrawEffect::None,
			ExploredCellColor);
	}

	for (const FGridMapFloorBoundaryView& Boundary : FloorView.Boundaries)
	{
		FVector2f A;
		FVector2f B;
		GetBoundaryEndpoints(Metrics, Boundary, A, B);
		switch (Boundary.Kind)
		{
			case EGridMapBoundaryKind::Wall:
				DrawLine(OutDrawElements, BoundaryLayer, PaintGeometry, A, B, WallColor, WallThickness);
				break;
			case EGridMapBoundaryKind::Door:
				DrawDoorBoundary(
					OutDrawElements, BoundaryLayer, PaintGeometry, A, B, DoorColor, DoorThickness, Boundary.bDoorOpen, false);
				break;
			case EGridMapBoundaryKind::SecretDoor:
				DrawDoorBoundary(
					OutDrawElements, BoundaryLayer, PaintGeometry, A, B, SecretDoorColor, DoorThickness, Boundary.bDoorOpen, true);
				break;
			default:
				break;
		}
	}

	DrawPartyMarker(OutDrawElements, MarkerLayer, PaintGeometry, Metrics, FloorView, PartyMarkerColor);
	return MarkerLayer;
}
