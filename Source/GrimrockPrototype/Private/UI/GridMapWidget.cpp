#include "UI/GridMapWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Core/GridDungeonAsset.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElementTypes.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Styling/CoreStyle.h"

float UGridMapWidget::ComputeDeterministicArtNoise(const FIntPoint& MapCell, EGridEdge Edge, int32 Salt)
{
	uint32 Hash = ::GetTypeHash(MapCell.X);
	Hash = HashCombine(Hash, ::GetTypeHash(MapCell.Y));
	Hash = HashCombine(Hash, ::GetTypeHash(static_cast<uint8>(Edge)));
	Hash = HashCombine(Hash, ::GetTypeHash(Salt));
	const float Unit = static_cast<float>(Hash & 0x00FFFFFFu) / 16777215.0f;
	return Unit * 2.0f - 1.0f;
}

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
		float ZoomScale,
		const FVector2D& PanOffsetPixels,
		bool bCenterOnParty,
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

		const float FitCellSize = FMath::Min(
			FMath::Min(AvailableWidth / static_cast<float>(CellCountX), AvailableHeight / static_cast<float>(CellCountY)),
			FMath::Max(4.0f, MaxCellPixels));
		OutMetrics.CellSize = FitCellSize * FMath::Max(0.01f, ZoomScale);
		if (OutMetrics.CellSize <= 0.0f)
		{
			return false;
		}

		const float MapWidth = static_cast<float>(CellCountX) * OutMetrics.CellSize;
		const float MapHeight = static_cast<float>(CellCountY) * OutMetrics.CellSize;
		OutMetrics.Origin = FVector2f(
			Padding.Left + (AvailableWidth - MapWidth) * 0.5f,
			Padding.Top + (AvailableHeight - MapHeight) * 0.5f);

		if (bCenterOnParty && View.bHasPartyMarker)
		{
			const FVector2f ViewportCenter(
				Padding.Left + AvailableWidth * 0.5f,
				Padding.Top + AvailableHeight * 0.5f);
			const FVector2f PartyCenterFromOrigin(
				(static_cast<float>(OutMetrics.MaxX - View.PartyMapCell.X) + 0.5f) * OutMetrics.CellSize,
				(static_cast<float>(OutMetrics.MaxY - View.PartyMapCell.Y) + 0.5f) * OutMetrics.CellSize);
			OutMetrics.Origin = ViewportCenter - PartyCenterFromOrigin;
		}

		OutMetrics.Origin += FVector2f(static_cast<float>(PanOffsetPixels.X), static_cast<float>(PanOffsetPixels.Y));
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

	void DrawHandDrawnLine(
		FSlateWindowElementList& OutDrawElements,
		int32 Layer,
		const FPaintGeometry& PaintGeometry,
		const FVector2f& Start,
		const FVector2f& End,
		const FLinearColor& Color,
		float Thickness,
		const FIntPoint& SeedCell,
		EGridEdge SeedEdge,
		int32 Salt,
		float JitterPixels,
		float SecondaryAlpha)
	{
		const FVector2f Delta = End - Start;
		if (Delta.SizeSquared() <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		FVector2f Perpendicular(-Delta.Y, Delta.X);
		Perpendicular.Normalize();

		auto DrawStroke = [&](int32 StrokeIndex, float AlphaScale, float ThicknessScale)
		{
			const int32 StrokeSalt = Salt + StrokeIndex * 17;
			const float StartJitter = UGridMapWidget::ComputeDeterministicArtNoise(SeedCell, SeedEdge, StrokeSalt + 1) * JitterPixels;
			const float MidJitter = UGridMapWidget::ComputeDeterministicArtNoise(SeedCell, SeedEdge, StrokeSalt + 2) * JitterPixels;
			const float EndJitter = UGridMapWidget::ComputeDeterministicArtNoise(SeedCell, SeedEdge, StrokeSalt + 3) * JitterPixels;
			const FVector2f Mid = (Start + End) * 0.5f;

			TArray<FVector2f> Points;
			Points.Reserve(3);
			Points.Add(Start + Perpendicular * StartJitter);
			Points.Add(Mid + Perpendicular * MidJitter);
			Points.Add(End + Perpendicular * EndJitter);

			FLinearColor StrokeColor = Color;
			StrokeColor.A *= AlphaScale;
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				static_cast<uint32>(Layer),
				PaintGeometry,
				MoveTemp(Points),
				ESlateDrawEffect::None,
				StrokeColor,
				true,
				FMath::Max(0.5f, Thickness * ThicknessScale));
		};

		DrawStroke(0, 1.0f, 1.0f);
		if (SecondaryAlpha > 0.0f)
		{
			DrawStroke(1, FMath::Clamp(SecondaryAlpha, 0.0f, 1.0f), 0.72f);
		}
	}

	FIntPoint GetNeighbourCell(const FIntPoint& Cell, EGridEdge Edge)
	{
		switch (Edge)
		{
			case EGridEdge::North: return Cell + FIntPoint(0, 1);
			case EGridEdge::East:  return Cell + FIntPoint(1, 0);
			case EGridEdge::South: return Cell + FIntPoint(0, -1);
			case EGridEdge::West:  return Cell + FIntPoint(-1, 0);
			default:               return Cell;
		}
	}

	void DrawParchmentBackground(
		FSlateWindowElementList& OutDrawElements,
		int32 Layer,
		const FGeometry& Geometry,
		const FPaintGeometry& PaintGeometry,
		const FMargin& Padding,
		const FSlateBrush* WhiteBrush,
		const FLinearColor& ParchmentColor,
		const FLinearColor& GrainColor,
		int32 GrainLineCount)
	{
		const FVector2D LocalSize = Geometry.GetLocalSize();
		const float Width = FMath::Max(0.0f, static_cast<float>(LocalSize.X) - Padding.Left - Padding.Right);
		const float Height = FMath::Max(0.0f, static_cast<float>(LocalSize.Y) - Padding.Top - Padding.Bottom);
		if (Width <= 0.0f || Height <= 0.0f)
		{
			return;
		}

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			static_cast<uint32>(Layer),
			Geometry.ToPaintGeometry(
				FVector2D(Width, Height),
				FSlateLayoutTransform(FVector2D(Padding.Left, Padding.Top))),
			WhiteBrush,
			ESlateDrawEffect::None,
			ParchmentColor);

		const int32 SafeCount = FMath::Clamp(GrainLineCount, 0, 96);
		for (int32 Index = 0; Index < SafeCount; ++Index)
		{
			const FIntPoint SeedCell(Index, 0);
			const float U = (UGridMapWidget::ComputeDeterministicArtNoise(SeedCell, EGridEdge::None, 101) + 1.0f) * 0.5f;
			const float V = (UGridMapWidget::ComputeDeterministicArtNoise(SeedCell, EGridEdge::None, 102) + 1.0f) * 0.5f;
			const float LengthFactor = 0.05f + 0.14f * ((UGridMapWidget::ComputeDeterministicArtNoise(SeedCell, EGridEdge::None, 103) + 1.0f) * 0.5f);
			const float Slope = UGridMapWidget::ComputeDeterministicArtNoise(SeedCell, EGridEdge::None, 104) * 5.0f;
			const FVector2f Start(
				Padding.Left + U * Width,
				Padding.Top + V * Height);
			const FVector2f End(
				FMath::Clamp(Start.X + Width * LengthFactor, Padding.Left, Padding.Left + Width),
				FMath::Clamp(Start.Y + Slope, Padding.Top, Padding.Top + Height));
			DrawLine(OutDrawElements, Layer + 1, PaintGeometry, Start, End, GrainColor, 1.0f);
		}
	}

	void DrawCellHatching(
		FSlateWindowElementList& OutDrawElements,
		int32 Layer,
		const FPaintGeometry& PaintGeometry,
		const FRenderMetrics& Metrics,
		const FIntPoint& MapCell,
		const FLinearColor& HatchColor,
		int32 HatchCount,
		float JitterPixels,
		float SecondaryAlpha)
	{
		if (Metrics.CellSize < 12.0f)
		{
			return;
		}

		const int32 SafeCount = FMath::Clamp(HatchCount, 0, 8);
		const FVector2f TopLeft = Metrics.CellTopLeft(MapCell);
		for (int32 Index = 0; Index < SafeCount; ++Index)
		{
			const float T = static_cast<float>(Index + 1) / static_cast<float>(SafeCount + 1);
			const float OffsetNoise = UGridMapWidget::ComputeDeterministicArtNoise(MapCell, EGridEdge::None, 200 + Index) * 0.07f;
			const float Y = FMath::Clamp(0.20f + T * 0.58f + OffsetNoise, 0.14f, 0.86f) * Metrics.CellSize;
			const FVector2f Start = TopLeft + FVector2f(Metrics.CellSize * 0.18f, Y);
			const FVector2f End = TopLeft + FVector2f(Metrics.CellSize * 0.78f, Y - Metrics.CellSize * 0.20f);
			DrawHandDrawnLine(
				OutDrawElements,
				Layer,
				PaintGeometry,
				Start,
				End,
				HatchColor,
				1.0f,
				MapCell,
				EGridEdge::None,
				220 + Index,
				JitterPixels * 0.35f,
				SecondaryAlpha * 0.35f);
		}
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
		bool bSecret,
		const FIntPoint& SeedCell,
		EGridEdge SeedEdge,
		float JitterPixels,
		float SecondaryAlpha)
	{
		const FVector2f Delta = B - A;
		const FVector2f P0 = A;
		const FVector2f P1 = A + Delta * 0.32f;
		const FVector2f P2 = A + Delta * 0.68f;
		const FVector2f P3 = B;

		DrawHandDrawnLine(OutDrawElements, Layer, PaintGeometry, P0, P1, Color, Thickness, SeedCell, SeedEdge, 310, JitterPixels, SecondaryAlpha);
		DrawHandDrawnLine(OutDrawElements, Layer, PaintGeometry, P2, P3, Color, Thickness, SeedCell, SeedEdge, 320, JitterPixels, SecondaryAlpha);

		if (!bOpen)
		{
			DrawHandDrawnLine(OutDrawElements, Layer, PaintGeometry, P1, P2, Color, Thickness * 0.75f, SeedCell, SeedEdge, 330, JitterPixels, SecondaryAlpha);
		}

		if (bSecret)
		{
			const FVector2f Center = (A + B) * 0.5f;
			FVector2f Perpendicular(-Delta.Y, Delta.X);
			if (Perpendicular.SizeSquared() > KINDA_SMALL_NUMBER)
			{
				Perpendicular.Normalize();
				Perpendicular *= FMath::Max(2.0f, Thickness * 1.5f);
				DrawHandDrawnLine(
					OutDrawElements,
					Layer + 1,
					PaintGeometry,
					Center - Perpendicular,
					Center + Perpendicular,
					Color,
					Thickness * 0.75f,
					SeedCell,
					SeedEdge,
					340,
					JitterPixels * 0.7f,
					SecondaryAlpha);
			}
		}
	}

	void DrawMapSymbol(
		FSlateWindowElementList& OutDrawElements,
		int32 Layer,
		const FPaintGeometry& PaintGeometry,
		const FRenderMetrics& Metrics,
		const FGridMapFloorSymbolView& Symbol,
		const FLinearColor& NavigationColor,
		const FLinearColor& HazardColor,
		float StrokeThickness,
		float SymbolScale,
		float MinCellPixels,
		float JitterPixels,
		float SecondaryAlpha)
	{
		if (Metrics.CellSize < MinCellPixels)
		{
			return;
		}

		const FVector2f Center = Metrics.CellCenter(Symbol.MapCell);
		const float R = Metrics.CellSize * 0.30f * FMath::Clamp(SymbolScale, 0.4f, 1.0f);
		const float T = FMath::Max(0.75f, StrokeThickness);
		const FLinearColor& Color = Symbol.Kind == EGridMapSymbolKind::Pit ? HazardColor : NavigationColor;

		auto Ink = [&](const FVector2f& A, const FVector2f& B, int32 Salt, float ThicknessScale = 1.0f)
		{
			DrawHandDrawnLine(
				OutDrawElements,
				Layer,
				PaintGeometry,
				A,
				B,
				Color,
				T * ThicknessScale,
				Symbol.MapCell,
				EGridEdge::None,
				Salt,
				JitterPixels * 0.45f,
				SecondaryAlpha * 0.65f);
		};

		switch (Symbol.Kind)
		{
			case EGridMapSymbolKind::StairsUp:
			case EGridMapSymbolKind::StairsDown:
			{
				const float Direction = Symbol.Kind == EGridMapSymbolKind::StairsUp ? -1.0f : 1.0f;
				const float Step = R * 0.52f;
				for (int32 Index = 0; Index < 3; ++Index)
				{
					const float Y = Center.Y + Direction * (static_cast<float>(Index) - 1.0f) * Step;
					const float HalfWidth = R * (0.46f + 0.20f * static_cast<float>(Index));
					Ink(
						FVector2f(Center.X - HalfWidth, Y),
						FVector2f(Center.X + HalfWidth, Y),
						510 + Index * 11);
				}

				const FVector2f ArrowBase(Center.X, Center.Y - Direction * R * 0.15f);
				const FVector2f ArrowTip(Center.X, Center.Y + Direction * R * 1.05f);
				Ink(ArrowBase, ArrowTip, 550, 0.85f);
				Ink(ArrowTip, ArrowTip + FVector2f(-R * 0.26f, -Direction * R * 0.28f), 551, 0.85f);
				Ink(ArrowTip, ArrowTip + FVector2f(R * 0.26f, -Direction * R * 0.28f), 552, 0.85f);
				break;
			}

			case EGridMapSymbolKind::Relocation:
			{
				const FVector2f Top = Center + FVector2f(0.0f, -R);
				const FVector2f Right = Center + FVector2f(R, 0.0f);
				const FVector2f Bottom = Center + FVector2f(0.0f, R);
				const FVector2f Left = Center + FVector2f(-R, 0.0f);
				Ink(Top, Right, 610);
				Ink(Right, Bottom, 611);
				Ink(Bottom, Left, 612);
				Ink(Left, Top, 613);

				const float Inner = R * 0.48f;
				Ink(Center + FVector2f(0.0f, -Inner), Center + FVector2f(Inner, 0.0f), 620, 0.80f);
				Ink(Center + FVector2f(Inner, 0.0f), Center + FVector2f(0.0f, Inner), 621, 0.80f);
				Ink(Center + FVector2f(0.0f, Inner), Center + FVector2f(-Inner, 0.0f), 622, 0.80f);
				Ink(Center + FVector2f(-Inner, 0.0f), Center + FVector2f(0.0f, -Inner), 623, 0.80f);
				break;
			}

			case EGridMapSymbolKind::Pit:
			{
				const float Half = R * 0.88f;
				const FVector2f TL = Center + FVector2f(-Half, -Half);
				const FVector2f TR = Center + FVector2f(Half, -Half);
				const FVector2f BR = Center + FVector2f(Half, Half);
				const FVector2f BL = Center + FVector2f(-Half, Half);
				Ink(TL, TR, 710);
				Ink(TR, BR, 711);
				Ink(BR, BL, 712);
				Ink(BL, TL, 713);
				Ink(TL + FVector2f(R * 0.18f, R * 0.18f), BR - FVector2f(R * 0.18f, R * 0.18f), 720, 0.85f);
				Ink(TR + FVector2f(-R * 0.18f, R * 0.18f), BL + FVector2f(R * 0.18f, -R * 0.18f), 721, 0.85f);
				break;
			}

			case EGridMapSymbolKind::PointOfInterest:
			{
				const float Inner = R * 0.42f;
				Ink(Center + FVector2f(0.0f, -R), Center + FVector2f(0.0f, R), 810, 0.90f);
				Ink(Center + FVector2f(-R, 0.0f), Center + FVector2f(R, 0.0f), 811, 0.90f);
				Ink(Center + FVector2f(-Inner, -Inner), Center + FVector2f(Inner, Inner), 812, 0.75f);
				Ink(Center + FVector2f(Inner, -Inner), Center + FVector2f(-Inner, Inner), 813, 0.75f);
				break;
			}

			default:
				break;
		}
	}

	void DrawPartyMarker(
		FSlateWindowElementList& OutDrawElements,
		int32 Layer,
		const FPaintGeometry& PaintGeometry,
		const FRenderMetrics& Metrics,
		const FGridMapFloorView& View,
		const FLinearColor& Color,
		float JitterPixels,
		float SecondaryAlpha)
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
			case EGridEdge::North: Forward = FVector2f(0.0f, -1.0f); break;
			case EGridEdge::East:  Forward = FVector2f(-1.0f, 0.0f); break;
			case EGridEdge::South: Forward = FVector2f(0.0f, 1.0f); break;
			case EGridEdge::West:  Forward = FVector2f(1.0f, 0.0f); break;
			default: break;
		}

		const FVector2f Right(-Forward.Y, Forward.X);
		const FVector2f Tip = Center + Forward * Radius;
		const FVector2f Left = Center - Forward * Radius * 0.65f - Right * Radius * 0.70f;
		const FVector2f RightPoint = Center - Forward * Radius * 0.65f + Right * Radius * 0.70f;
		const float MarkerThickness = FMath::Max(2.0f, Metrics.CellSize * 0.08f);

		DrawHandDrawnLine(OutDrawElements, Layer, PaintGeometry, Tip, Left, Color, MarkerThickness, View.PartyMapCell, View.PartyFacing, 410, JitterPixels * 0.55f, SecondaryAlpha);
		DrawHandDrawnLine(OutDrawElements, Layer, PaintGeometry, Left, RightPoint, Color, MarkerThickness, View.PartyMapCell, View.PartyFacing, 420, JitterPixels * 0.55f, SecondaryAlpha);
		DrawHandDrawnLine(OutDrawElements, Layer, PaintGeometry, RightPoint, Tip, Color, MarkerThickness, View.PartyMapCell, View.PartyFacing, 430, JitterPixels * 0.55f, SecondaryAlpha);
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
	if (Button_Recenter)
	{
		Button_Recenter->OnClicked.AddUniqueDynamic(this, &UGridMapWidget::HandleRecenterClicked);
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
	if (Button_Recenter)
	{
		Button_Recenter->OnClicked.RemoveDynamic(this, &UGridMapWidget::HandleRecenterClicked);
	}

	Super::NativeDestruct();
}

void UGridMapWidget::InitializeMapWidget(AGrimrockPartyPawn* InPartyPawn)
{
	OwningPartyPawn = InPartyPawn;
	bHasFloorSelection = false;
	ZoomScale = 1.0f;
	ResetViewTransform(false);
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
	ResetViewTransform(false);
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
	ResetViewTransform(false);
	Invalidate(EInvalidateWidgetReason::Paint);
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
	ResetViewTransform(false);
	Invalidate(EInvalidateWidgetReason::Paint);
	return true;
}

bool UGridMapWidget::RecenterMap()
{
	int32 PartyFloorZ = 0;
	if (!ResolvePartyFloorZ(PartyFloorZ))
	{
		return false;
	}

	SelectedFloorZ = PartyFloorZ;
	bHasFloorSelection = true;
	ResetViewTransform(true);
	return BuildSelectedFloorView();
}

bool UGridMapWidget::AdjustZoom(float WheelDelta)
{
	if (!bHasRenderableMap || FMath::IsNearlyZero(WheelDelta))
	{
		return false;
	}

	const float SafeMin = FMath::Min(MinZoomScale, MaxZoomScale);
	const float SafeMax = FMath::Max(MinZoomScale, MaxZoomScale);
	const float PreviousZoom = ZoomScale;
	ZoomScale = FMath::Clamp(ZoomScale + WheelDelta * ZoomStep, SafeMin, SafeMax);
	if (FMath::IsNearlyEqual(ZoomScale, PreviousZoom))
	{
		return false;
	}

	Invalidate(EInvalidateWidgetReason::Paint);
	return true;
}

void UGridMapWidget::PanMapByPixels(const FVector2D& DeltaPixels)
{
	if (!bHasRenderableMap || DeltaPixels.IsNearlyZero())
	{
		return;
	}

	PanOffsetPixels += DeltaPixels;
	Invalidate(EInvalidateWidgetReason::Paint);
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

void UGridMapWidget::ResetViewTransform(bool bCenterOnParty)
{
	PanOffsetPixels = FVector2D::ZeroVector;
	bCenterViewOnParty = bCenterOnParty;
	bIsPanning = false;
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
				? FText::Format(NSLOCTEXT("GridMap", "FloorLabel", "Niveau {0}"), FText::AsNumber(SelectedFloorZ))
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

void UGridMapWidget::HandleRecenterClicked()
{
	RecenterMap();
}

FReply UGridMapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bHasRenderableMap && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsPanning = true;
		FReply Reply = FReply::Handled();
		if (const TSharedPtr<SWidget> CachedWidget = GetCachedWidget())
		{
			Reply.CaptureMouse(CachedWidget.ToSharedRef());
		}
		return Reply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UGridMapWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsPanning && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsPanning = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UGridMapWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsPanning)
	{
		const FVector2D CurrentLocal = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		const FVector2D PreviousLocal = InGeometry.AbsoluteToLocal(InMouseEvent.GetLastScreenSpacePosition());
		PanMapByPixels(CurrentLocal - PreviousLocal);
		return FReply::Handled();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UGridMapWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bHasRenderableMap && !FMath::IsNearlyZero(InMouseEvent.GetWheelDelta()))
	{
		AdjustZoom(InMouseEvent.GetWheelDelta());
		return FReply::Handled();
	}

	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

void UGridMapWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	bIsPanning = false;
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
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
	if (!BuildRenderMetrics(
		FloorView,
		AllottedGeometry,
		MapDrawPadding,
		MaxCellPixels,
		ZoomScale,
		PanOffsetPixels,
		bCenterViewOnParty,
		Metrics))
	{
		return BaseLayer;
	}

	const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const int32 ParchmentLayer = BaseLayer + 1;
	const int32 CellLayer = ParchmentLayer + 2;
	const int32 HatchLayer = CellLayer + 1;
	const int32 FeatherLayer = HatchLayer + 1;
	const int32 BoundaryLayer = FeatherLayer + 1;
	const int32 SymbolLayer = BoundaryLayer + 2;
	const int32 MarkerLayer = SymbolLayer + 2;

	if (bEnableParchmentStyle)
	{
		DrawParchmentBackground(
			OutDrawElements,
			ParchmentLayer,
			AllottedGeometry,
			PaintGeometry,
			MapDrawPadding,
			WhiteBrush,
			ParchmentColor,
			ParchmentGrainColor,
			ParchmentGrainLineCount);
	}

	TSet<FIntPoint> VisibleCells;
	VisibleCells.Reserve(FloorView.Cells.Num());
	for (const FGridMapFloorCellView& Cell : FloorView.Cells)
	{
		VisibleCells.Add(Cell.MapCell);
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

		if (bEnableParchmentStyle)
		{
			DrawCellHatching(
				OutDrawElements,
				HatchLayer,
				PaintGeometry,
				Metrics,
				Cell.MapCell,
				CellHatchColor,
				CellHatchLineCount,
				HandDrawnJitterPixels,
				SecondaryStrokeAlpha);
		}
	}

	if (bEnableParchmentStyle && Metrics.CellSize >= 8.0f)
	{
		for (const FGridMapFloorCellView& Cell : FloorView.Cells)
		{
			for (const EGridEdge Edge : { EGridEdge::North, EGridEdge::East, EGridEdge::South, EGridEdge::West })
			{
				if (VisibleCells.Contains(GetNeighbourCell(Cell.MapCell, Edge)))
				{
					continue;
				}

				FGridMapFloorBoundaryView FeatherBoundary;
				FeatherBoundary.MapCell = Cell.MapCell;
				FeatherBoundary.Edge = Edge;
				FVector2f A;
				FVector2f B;
				GetBoundaryEndpoints(Metrics, FeatherBoundary, A, B);
				DrawLine(
					OutDrawElements,
					FeatherLayer,
					PaintGeometry,
					A,
					B,
					FogFeatherColor,
					FMath::Max(1.5f, Metrics.CellSize * 0.07f));
			}
		}
	}

	for (const FGridMapFloorBoundaryView& Boundary : FloorView.Boundaries)
	{
		FVector2f A;
		FVector2f B;
		GetBoundaryEndpoints(Metrics, Boundary, A, B);
		switch (Boundary.Kind)
		{
			case EGridMapBoundaryKind::Wall:
				if (bEnableParchmentStyle)
				{
					DrawHandDrawnLine(
						OutDrawElements,
						BoundaryLayer,
						PaintGeometry,
						A,
						B,
						WallColor,
						WallThickness,
						Boundary.MapCell,
						Boundary.Edge,
						300,
						HandDrawnJitterPixels,
						SecondaryStrokeAlpha);
				}
				else
				{
					DrawLine(OutDrawElements, BoundaryLayer, PaintGeometry, A, B, WallColor, WallThickness);
				}
				break;

			case EGridMapBoundaryKind::Door:
				DrawDoorBoundary(
					OutDrawElements,
					BoundaryLayer,
					PaintGeometry,
					A,
					B,
					DoorColor,
					DoorThickness,
					Boundary.bDoorOpen,
					false,
					Boundary.MapCell,
					Boundary.Edge,
					bEnableParchmentStyle ? HandDrawnJitterPixels : 0.0f,
					bEnableParchmentStyle ? SecondaryStrokeAlpha : 0.0f);
				break;

			case EGridMapBoundaryKind::SecretDoor:
				DrawDoorBoundary(
					OutDrawElements,
					BoundaryLayer,
					PaintGeometry,
					A,
					B,
					SecretDoorColor,
					DoorThickness,
					Boundary.bDoorOpen,
					true,
					Boundary.MapCell,
					Boundary.Edge,
					bEnableParchmentStyle ? HandDrawnJitterPixels : 0.0f,
					bEnableParchmentStyle ? SecondaryStrokeAlpha : 0.0f);
				break;

			default:
				break;
		}
	}

	for (const FGridMapFloorSymbolView& Symbol : FloorView.Symbols)
	{
		DrawMapSymbol(
			OutDrawElements,
			SymbolLayer,
			PaintGeometry,
			Metrics,
			Symbol,
			NavigationSymbolColor,
			HazardSymbolColor,
			SymbolStrokeThickness,
			SymbolScale,
			SymbolMinCellPixels,
			bEnableParchmentStyle ? HandDrawnJitterPixels : 0.0f,
			bEnableParchmentStyle ? SecondaryStrokeAlpha : 0.0f);
	}

	if (bEnableParchmentStyle)
	{
		DrawPartyMarker(
			OutDrawElements,
			MarkerLayer,
			PaintGeometry,
			Metrics,
			FloorView,
			PartyMarkerColor,
			HandDrawnJitterPixels,
			SecondaryStrokeAlpha);
	}
	else
	{
		DrawPartyMarker(
			OutDrawElements,
			MarkerLayer,
			PaintGeometry,
			Metrics,
			FloorView,
			PartyMarkerColor,
			0.0f,
			0.0f);
	}
	return MarkerLayer;
}
