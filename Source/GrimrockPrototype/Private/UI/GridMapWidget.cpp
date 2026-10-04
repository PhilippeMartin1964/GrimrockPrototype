#include "UI/GridMapWidget.h"
#include "UI/GridMapSurfaceWidget.h"
#include "UI/GridMapVisualThemeAsset.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Core/GridDungeonAsset.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "Layout/Clipping.h"
#include "Rendering/DrawElementTypes.h"
#include "Runtime/GridDoorSystemComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"

float UGridMapWidget::ComputeAutoFitCellSize(
	const FVector2D& LocalSize,
	const FMargin& Padding,
	int32 CellCountX,
	int32 CellCountY,
	float AutoFitMarginCells,
	float MaxCellPixels,
	float ZoomScale)
{
	const float AvailableWidth = FMath::Max(0.0f, static_cast<float>(LocalSize.X) - Padding.Left - Padding.Right);
	const float AvailableHeight = FMath::Max(0.0f, static_cast<float>(LocalSize.Y) - Padding.Top - Padding.Bottom);
	if (AvailableWidth <= 0.0f || AvailableHeight <= 0.0f || CellCountX <= 0 || CellCountY <= 0)
	{
		return 0.0f;
	}

	const float SafeMarginCells = FMath::Max(0.0f, AutoFitMarginCells);
	const float FitSpanX = static_cast<float>(CellCountX) + SafeMarginCells * 2.0f;
	const float FitSpanY = static_cast<float>(CellCountY) + SafeMarginCells * 2.0f;
	float FitCellSize = FMath::Min(AvailableWidth / FitSpanX, AvailableHeight / FitSpanY);

	if (MaxCellPixels > 0.0f)
	{
		FitCellSize = FMath::Min(FitCellSize, MaxCellPixels);
	}

	return FMath::Max(0.0f, FitCellSize * FMath::Max(0.01f, ZoomScale));
}

float UGridMapWidget::ComputeDeterministicArtNoise(const FIntPoint& MapCell, EGridEdge Edge, int32 Salt)
{
	uint32 Hash = ::GetTypeHash(MapCell.X);
	Hash = HashCombine(Hash, ::GetTypeHash(MapCell.Y));
	Hash = HashCombine(Hash, ::GetTypeHash(static_cast<uint8>(Edge)));
	Hash = HashCombine(Hash, ::GetTypeHash(Salt));
	const float Unit = static_cast<float>(Hash & 0x00FFFFFFu) / 16777215.0f;
	return Unit * 2.0f - 1.0f;
}

int32 UGridMapWidget::ComputeDeterministicSymbolVariant(
	const FIntPoint& MapCell,
	EGridMapSymbolKind SymbolKind,
	int32 VariantCount)
{
	const int32 SafeCount = FMath::Clamp(VariantCount, 1, 5);
	uint32 Hash = ::GetTypeHash(MapCell.X);
	Hash = HashCombine(Hash, ::GetTypeHash(MapCell.Y));
	Hash = HashCombine(Hash, ::GetTypeHash(static_cast<uint8>(SymbolKind)));
	Hash = HashCombine(Hash, ::GetTypeHash(0x4D415001u));
	return static_cast<int32>(Hash % static_cast<uint32>(SafeCount));
}

namespace GridMapWidgetPrivate
{
	bool IsInsideMapViewport(const FVector2D& LocalPoint, const FVector2D& LocalSize, const FMargin& Padding)
	{
		const float Left = Padding.Left;
		const float Top = Padding.Top;
		const float Right = static_cast<float>(LocalSize.X) - Padding.Right;
		const float Bottom = static_cast<float>(LocalSize.Y) - Padding.Bottom;
		return Right > Left && Bottom > Top &&
			LocalPoint.X >= Left && LocalPoint.X <= Right &&
			LocalPoint.Y >= Top && LocalPoint.Y <= Bottom;
	}

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

	void ConfigureTextureBrush(FSlateBrush& OutBrush, UTexture2D* Texture)
	{
		if (!Texture)
		{
			return;
		}

		OutBrush.DrawAs = ESlateBrushDrawType::Image;
		OutBrush.SetResourceObject(Texture);
		OutBrush.ImageSize = FVector2D(
			static_cast<double>(Texture->GetSizeX()),
			static_cast<double>(Texture->GetSizeY()));
	}

	struct FThemeBrushSet
	{
		FSlateBrush Parchment;
		FSlateBrush Floor;
		FSlateBrush Wall;
		FSlateBrush WallPillar;
		FSlateBrush DoorClosed;
		FSlateBrush DoorOpen;
		FSlateBrush StairsUp;
		FSlateBrush StairsDown;
		FSlateBrush Relocation;
		FSlateBrush Pit;
		FSlateBrush PointOfInterest;
		FSlateBrush PartyMarker;

		explicit FThemeBrushSet(const UGridMapVisualThemeAsset& Theme)
		{
			ConfigureTextureBrush(Parchment, Theme.ParchmentTexture);
			ConfigureTextureBrush(Floor, Theme.FloorTexture);
			ConfigureTextureBrush(Wall, Theme.WallTexture);
			ConfigureTextureBrush(WallPillar, Theme.WallPillarTexture);
			ConfigureTextureBrush(DoorClosed, Theme.DoorClosedTexture);
			ConfigureTextureBrush(DoorOpen, Theme.DoorOpenTexture);
			ConfigureTextureBrush(StairsUp, Theme.StairsUpTexture);
			ConfigureTextureBrush(StairsDown, Theme.StairsDownTexture);
			ConfigureTextureBrush(Relocation, Theme.RelocationTexture);
			ConfigureTextureBrush(Pit, Theme.PitTexture);
			ConfigureTextureBrush(PointOfInterest, Theme.PointOfInterestTexture);
			ConfigureTextureBrush(PartyMarker, Theme.PartyMarkerTexture);
		}
	};

	const FSlateBrush* GetSymbolBrush(
		const UGridMapVisualThemeAsset& Theme,
		const FThemeBrushSet& Brushes,
		EGridMapSymbolKind Kind)
	{
		switch (Kind)
		{
			case EGridMapSymbolKind::StairsUp:
				return Theme.StairsUpTexture ? &Brushes.StairsUp : nullptr;
			case EGridMapSymbolKind::StairsDown:
				return Theme.StairsDownTexture ? &Brushes.StairsDown : nullptr;
			case EGridMapSymbolKind::Relocation:
				return Theme.RelocationTexture ? &Brushes.Relocation : nullptr;
			case EGridMapSymbolKind::Pit:
				return Theme.PitTexture ? &Brushes.Pit : nullptr;
			case EGridMapSymbolKind::PointOfInterest:
				return Theme.PointOfInterestTexture ? &Brushes.PointOfInterest : nullptr;
			default:
				return nullptr;
		}
	}

	void DrawTexturedSegment(
		FSlateWindowElementList& OutDrawElements,
		int32 Layer,
		const FGeometry& Geometry,
		const FVector2f& A,
		const FVector2f& B,
		const FSlateBrush* Brush,
		float Thickness)
	{
		if (!Brush)
		{
			return;
		}

		const FVector2f Delta = B - A;
		const float Length = Delta.Size();
		if (Length <= KINDA_SMALL_NUMBER || Thickness <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		const FVector2f Center = (A + B) * 0.5f;
		const FVector2D Size(static_cast<double>(Length), static_cast<double>(Thickness));
		const FVector2D TopLeft(
			static_cast<double>(Center.X - Length * 0.5f),
			static_cast<double>(Center.Y - Thickness * 0.5f));
		const float Angle = FMath::Atan2(Delta.Y, Delta.X);

		FSlateDrawElement::MakeRotatedBox(
			OutDrawElements,
			static_cast<uint32>(Layer),
			Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(TopLeft)),
			Brush,
			ESlateDrawEffect::None,
			Angle,
			TOptional<FVector2f>(FVector2f(Length * 0.5f, Thickness * 0.5f)),
			FSlateDrawElement::RelativeToElement,
			FLinearColor::White);
	}

	void DrawTexturedSquare(
		FSlateWindowElementList& OutDrawElements,
		int32 Layer,
		const FGeometry& Geometry,
		const FVector2f& Center,
		float Size,
		const FSlateBrush* Brush,
		float AngleRadians = 0.0f)
	{
		if (!Brush || Size <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		const FVector2D DrawSize(static_cast<double>(Size), static_cast<double>(Size));
		const FVector2D TopLeft(
			static_cast<double>(Center.X - Size * 0.5f),
			static_cast<double>(Center.Y - Size * 0.5f));
		const FPaintGeometry GeometryForTexture =
			Geometry.ToPaintGeometry(DrawSize, FSlateLayoutTransform(TopLeft));

		if (FMath::IsNearlyZero(AngleRadians))
		{
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				static_cast<uint32>(Layer),
				GeometryForTexture,
				Brush,
				ESlateDrawEffect::None,
				FLinearColor::White);
			return;
		}

		FSlateDrawElement::MakeRotatedBox(
			OutDrawElements,
			static_cast<uint32>(Layer),
			GeometryForTexture,
			Brush,
			ESlateDrawEffect::None,
			AngleRadians,
			TOptional<FVector2f>(FVector2f(Size * 0.5f, Size * 0.5f)),
			FSlateDrawElement::RelativeToElement,
			FLinearColor::White);
	}

	float GetMapFacingAngle(EGridEdge Facing)
	{
		// Directional map artwork is authored pointing to screen-up / canonical North.
		switch (Facing)
		{
			case EGridEdge::North: return 0.0f;
			case EGridEdge::East:  return -(PI * 0.5f);
			case EGridEdge::South: return PI;
			case EGridEdge::West:  return (PI * 0.5f);
			default:               return 0.0f;
		}
	}

	bool BuildRenderMetrics(
		const FGridMapFloorView& View,
		const FGeometry& Geometry,
		const FMargin& Padding,
		float AutoFitMarginCells,
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

		OutMetrics.CellSize = UGridMapWidget::ComputeAutoFitCellSize(
			LocalSize,
			Padding,
			CellCountX,
			CellCountY,
			AutoFitMarginCells,
			MaxCellPixels,
			ZoomScale);
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
		const FLinearColor& EdgeColor,
		float EdgeThickness,
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

		const float Left = Padding.Left;
		const float Top = Padding.Top;
		const float Right = Padding.Left + Width;
		const float Bottom = Padding.Top + Height;
		const FIntPoint FrameSeed(0, 0);
		DrawHandDrawnLine(OutDrawElements, Layer + 2, PaintGeometry, FVector2f(Left, Top), FVector2f(Right, Top), EdgeColor, EdgeThickness, FrameSeed, EGridEdge::North, 900, 0.75f, 0.18f);
		DrawHandDrawnLine(OutDrawElements, Layer + 2, PaintGeometry, FVector2f(Right, Top), FVector2f(Right, Bottom), EdgeColor, EdgeThickness, FrameSeed, EGridEdge::West, 910, 0.75f, 0.18f);
		DrawHandDrawnLine(OutDrawElements, Layer + 2, PaintGeometry, FVector2f(Right, Bottom), FVector2f(Left, Bottom), EdgeColor, EdgeThickness, FrameSeed, EGridEdge::South, 920, 0.75f, 0.18f);
		DrawHandDrawnLine(OutDrawElements, Layer + 2, PaintGeometry, FVector2f(Left, Bottom), FVector2f(Left, Top), EdgeColor, EdgeThickness, FrameSeed, EGridEdge::East, 930, 0.75f, 0.18f);
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

	bool GetWallLeftPillarPlacement(
		const FGridMapFloorBoundaryView& Boundary,
		const FVector2f& A,
		const FVector2f& B,
		FIntPoint& OutVertex,
		FVector2f& OutCenter)
	{
		// Match the modular wall convention used by SM_Wall_Stone_05:
		// when facing the wall from its owning cell, the pillar sits on the left end.
		switch (Boundary.Edge)
		{
			case EGridEdge::North:
				OutVertex = FIntPoint(Boundary.MapCell.X, Boundary.MapCell.Y + 1);
				OutCenter = B; // West end.
				return true;

			case EGridEdge::East:
				OutVertex = FIntPoint(Boundary.MapCell.X + 1, Boundary.MapCell.Y + 1);
				OutCenter = A; // North end.
				return true;

			case EGridEdge::South:
				OutVertex = FIntPoint(Boundary.MapCell.X + 1, Boundary.MapCell.Y);
				OutCenter = A; // East end.
				return true;

			case EGridEdge::West:
				OutVertex = FIntPoint(Boundary.MapCell.X, Boundary.MapCell.Y);
				OutCenter = B; // South end.
				return true;

			default:
				return false;
		}
	}

	void DrawWallBoundary(
		FSlateWindowElementList& OutDrawElements,
		int32 Layer,
		const FPaintGeometry& PaintGeometry,
		const FVector2f& A,
		const FVector2f& B,
		const FLinearColor& UnderlayColor,
		const FLinearColor& InkColor,
		float Thickness,
		float UnderlayThicknessScale,
		const FIntPoint& SeedCell,
		EGridEdge SeedEdge,
		float JitterPixels,
		float SecondaryAlpha,
		int32 StoneMarkCount)
	{
		DrawHandDrawnLine(
			OutDrawElements,
			Layer,
			PaintGeometry,
			A,
			B,
			UnderlayColor,
			Thickness * FMath::Max(1.0f, UnderlayThicknessScale),
			SeedCell,
			SeedEdge,
			280,
			JitterPixels * 0.55f,
			SecondaryAlpha * 0.45f);

		DrawHandDrawnLine(
			OutDrawElements,
			Layer + 1,
			PaintGeometry,
			A,
			B,
			InkColor,
			Thickness,
			SeedCell,
			SeedEdge,
			300,
			JitterPixels,
			SecondaryAlpha);

		const FVector2f Delta = B - A;
		const float Length = Delta.Size();
		const int32 SafeMarkCount = FMath::Clamp(StoneMarkCount, 0, 8);
		if (Length <= KINDA_SMALL_NUMBER || SafeMarkCount <= 0)
		{
			return;
		}

		const FVector2f Tangent = Delta / Length;
		const FVector2f Normal(-Tangent.Y, Tangent.X);
		for (int32 Index = 0; Index < SafeMarkCount; ++Index)
		{
			const float BaseT = static_cast<float>(Index + 1) / static_cast<float>(SafeMarkCount + 1);
			const float TJitter = UGridMapWidget::ComputeDeterministicArtNoise(SeedCell, SeedEdge, 350 + Index * 7) * 0.055f;
			const float Along = FMath::Clamp(BaseT + TJitter, 0.10f, 0.90f);
			const FVector2f Center = A + Delta * Along;
			const float LengthNoise = UGridMapWidget::ComputeDeterministicArtNoise(SeedCell, SeedEdge, 351 + Index * 7);
			const float HalfMark = FMath::Max(1.5f, Thickness * (0.85f + LengthNoise * 0.16f));

			FLinearColor MarkColor = InkColor;
			MarkColor.A *= 0.62f;
			DrawHandDrawnLine(
				OutDrawElements,
				Layer + 2,
				PaintGeometry,
				Center - Normal * HalfMark,
				Center + Normal * HalfMark,
				MarkColor,
				FMath::Max(0.75f, Thickness * 0.42f),
				SeedCell,
				SeedEdge,
				360 + Index * 13,
				JitterPixels * 0.48f,
				SecondaryAlpha * 0.45f);
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
		float SecondaryAlpha,
		float JambLengthScale,
		int32 PanelLineCount)
	{
		const FVector2f Delta = B - A;
		const FVector2f P0 = A;
		const FVector2f P1 = A + Delta * 0.32f;
		const FVector2f P2 = A + Delta * 0.68f;
		const FVector2f P3 = B;

		DrawHandDrawnLine(OutDrawElements, Layer, PaintGeometry, P0, P1, Color, Thickness, SeedCell, SeedEdge, 310, JitterPixels, SecondaryAlpha);
		DrawHandDrawnLine(OutDrawElements, Layer, PaintGeometry, P2, P3, Color, Thickness, SeedCell, SeedEdge, 320, JitterPixels, SecondaryAlpha);

		FVector2f Normal(-Delta.Y, Delta.X);
		if (Normal.SizeSquared() > KINDA_SMALL_NUMBER)
		{
			Normal.Normalize();
			const FVector2f JambAxis = Normal * FMath::Max(2.0f, Thickness * FMath::Max(1.0f, JambLengthScale));
			DrawHandDrawnLine(OutDrawElements, Layer + 1, PaintGeometry, P1 - JambAxis, P1 + JambAxis, Color, Thickness * 0.85f, SeedCell, SeedEdge, 325, JitterPixels * 0.6f, SecondaryAlpha);
			DrawHandDrawnLine(OutDrawElements, Layer + 1, PaintGeometry, P2 - JambAxis, P2 + JambAxis, Color, Thickness * 0.85f, SeedCell, SeedEdge, 326, JitterPixels * 0.6f, SecondaryAlpha);

			if (!bOpen)
			{
				DrawHandDrawnLine(OutDrawElements, Layer, PaintGeometry, P1, P2, Color, Thickness * 0.78f, SeedCell, SeedEdge, 330, JitterPixels, SecondaryAlpha);

				const int32 SafePanelCount = FMath::Clamp(PanelLineCount, 0, 6);
				for (int32 Index = 0; Index < SafePanelCount; ++Index)
				{
					const float Side = (Index % 2 == 0) ? -1.0f : 1.0f;
					const float Band = 1.0f + static_cast<float>(Index / 2) * 0.45f;
					const FVector2f Offset = Normal * Side * Thickness * (0.72f * Band);
					FLinearColor PanelColor = Color;
					PanelColor.A *= 0.56f;
					DrawHandDrawnLine(
						OutDrawElements,
						Layer + 1,
						PaintGeometry,
						P1 + Offset,
						P2 + Offset,
						PanelColor,
						FMath::Max(0.65f, Thickness * 0.32f),
						SeedCell,
						SeedEdge,
						335 + Index * 9,
						JitterPixels * 0.45f,
						SecondaryAlpha * 0.45f);
				}

				if (SafePanelCount > 0)
				{
					const float Brace = FMath::Max(1.5f, Thickness * 1.05f);
					FLinearColor BraceColor = Color;
					BraceColor.A *= 0.66f;
					DrawHandDrawnLine(
						OutDrawElements,
						Layer + 2,
						PaintGeometry,
						P1 - Normal * Brace,
						P2 + Normal * Brace,
						BraceColor,
						FMath::Max(0.7f, Thickness * 0.36f),
						SeedCell,
						SeedEdge,
						346,
						JitterPixels * 0.55f,
						SecondaryAlpha * 0.50f);
				}
			}
			else
			{
				FVector2f Tangent = Delta;
				if (Tangent.SizeSquared() > KINDA_SMALL_NUMBER)
				{
					Tangent.Normalize();
					const float LeafLength = (P2 - P1).Size() * 0.78f;
					FLinearColor LeafColor = Color;
					LeafColor.A *= 0.72f;
					DrawHandDrawnLine(
						OutDrawElements,
						Layer + 1,
						PaintGeometry,
						P1,
						P1 + Tangent * LeafLength * 0.48f + Normal * LeafLength * 0.52f,
						LeafColor,
						FMath::Max(0.8f, Thickness * 0.48f),
						SeedCell,
						SeedEdge,
						348,
						JitterPixels * 0.60f,
						SecondaryAlpha * 0.55f);
				}
			}
		}

		if (bSecret)
		{
			const FVector2f Center = (A + B) * 0.5f;
			FVector2f Perpendicular(-Delta.Y, Delta.X);
			if (Perpendicular.SizeSquared() > KINDA_SMALL_NUMBER)
			{
				Perpendicular.Normalize();
				const float RuneHalf = FMath::Max(2.0f, Thickness * 1.5f);
				const FVector2f RuneAxis = Perpendicular * RuneHalf;
				DrawHandDrawnLine(
					OutDrawElements,
					Layer + 2,
					PaintGeometry,
					Center - RuneAxis,
					Center + RuneAxis,
					Color,
					Thickness * 0.72f,
					SeedCell,
					SeedEdge,
					340,
					JitterPixels * 0.7f,
					SecondaryAlpha);
				DrawHandDrawnLine(
					OutDrawElements,
					Layer + 2,
					PaintGeometry,
					Center - RuneAxis * 0.45f - Delta * 0.06f,
					Center + RuneAxis * 0.45f + Delta * 0.06f,
					Color,
					Thickness * 0.48f,
					SeedCell,
					SeedEdge,
					342,
					JitterPixels * 0.55f,
					SecondaryAlpha * 0.65f);
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
		float SecondaryAlpha,
		int32 StairStepCount,
		int32 PitDepthLineCount,
		int32 SymbolVariantCount)
	{
		if (Metrics.CellSize < MinCellPixels)
		{
			return;
		}

		const FVector2f Center = Metrics.CellCenter(Symbol.MapCell);
		const float R = Metrics.CellSize * 0.30f * FMath::Clamp(SymbolScale, 0.4f, 1.0f);
		const float T = FMath::Max(0.75f, StrokeThickness);
		const bool bDirectionalStairs =
			Symbol.Kind == EGridMapSymbolKind::StairsUp || Symbol.Kind == EGridMapSymbolKind::StairsDown;
		const float SymbolAngle = bDirectionalStairs ? GetMapFacingAngle(Symbol.Facing) : 0.0f;
		const float SymbolCos = FMath::Cos(SymbolAngle);
		const float SymbolSin = FMath::Sin(SymbolAngle);
		auto RotateSymbolPoint = [&](const FVector2f& Point)
		{
			if (FMath::IsNearlyZero(SymbolAngle))
			{
				return Point;
			}
			const FVector2f Offset = Point - Center;
			return Center + FVector2f(
				Offset.X * SymbolCos - Offset.Y * SymbolSin,
				Offset.X * SymbolSin + Offset.Y * SymbolCos);
		};
		const FLinearColor& Color = Symbol.Kind == EGridMapSymbolKind::Pit ? HazardColor : NavigationColor;
		const int32 SafeVariantCount = FMath::Clamp(SymbolVariantCount, 1, 5);
		const int32 Variant = UGridMapWidget::ComputeDeterministicSymbolVariant(Symbol.MapCell, Symbol.Kind, SafeVariantCount);
		const float VariantBias = SafeVariantCount > 1
			? (2.0f * static_cast<float>(Variant) / static_cast<float>(SafeVariantCount - 1)) - 1.0f
			: 0.0f;

		auto Ink = [&](const FVector2f& A, const FVector2f& B, int32 Salt, float ThicknessScale = 1.0f, float AlphaScale = 1.0f)
		{
			FLinearColor InkColor = Color;
			InkColor.A *= AlphaScale;
			DrawHandDrawnLine(
				OutDrawElements,
				Layer,
				PaintGeometry,
				RotateSymbolPoint(A),
				RotateSymbolPoint(B),
				InkColor,
				T * ThicknessScale,
				Symbol.MapCell,
				EGridEdge::None,
				Salt + Variant * 101,
				JitterPixels * 0.52f,
				SecondaryAlpha * 0.70f);
		};

		switch (Symbol.Kind)
		{
			case EGridMapSymbolKind::StairsUp:
			case EGridMapSymbolKind::StairsDown:
			{
				const float Direction = Symbol.Kind == EGridMapSymbolKind::StairsUp ? -1.0f : 1.0f;
				const int32 SafeStepCount = FMath::Clamp(StairStepCount, 3, 7);
				const float Span = R * 1.45f;
				for (int32 Index = 0; Index < SafeStepCount; ++Index)
				{
					const float StepT = SafeStepCount > 1
						? static_cast<float>(Index) / static_cast<float>(SafeStepCount - 1)
						: 0.5f;
					const float Y = Center.Y + Direction * (StepT - 0.5f) * Span;
					const float HalfWidth = R * (0.36f + 0.52f * StepT);
					const float Skew = VariantBias * R * 0.08f * (StepT - 0.5f);
					Ink(
						FVector2f(Center.X - HalfWidth + Skew, Y),
						FVector2f(Center.X + HalfWidth + Skew, Y + VariantBias * R * 0.025f),
						510 + Index * 11);
				}

				const float FirstY = Center.Y + Direction * -0.5f * Span;
				const float LastY = Center.Y + Direction * 0.5f * Span;
				Ink(
					FVector2f(Center.X - R * 0.36f, FirstY),
					FVector2f(Center.X - R * 0.88f + VariantBias * R * 0.06f, LastY),
					545,
					0.62f,
					0.68f);
				Ink(
					FVector2f(Center.X + R * 0.36f, FirstY),
					FVector2f(Center.X + R * 0.88f + VariantBias * R * 0.06f, LastY),
					546,
					0.62f,
					0.68f);

				const FVector2f ArrowBase(Center.X + VariantBias * R * 0.06f, Center.Y - Direction * R * 0.12f);
				const FVector2f ArrowTip(Center.X + VariantBias * R * 0.06f, Center.Y + Direction * R * 1.02f);
				Ink(ArrowBase, ArrowTip, 550, 0.88f);
				Ink(ArrowTip, ArrowTip + FVector2f(-R * 0.27f, -Direction * R * 0.30f), 551, 0.88f);
				Ink(ArrowTip, ArrowTip + FVector2f(R * 0.27f, -Direction * R * 0.30f), 552, 0.88f);
				break;
			}

			case EGridMapSymbolKind::Relocation:
			{
				const float Warp = VariantBias * R * 0.08f;
				const FVector2f Top = Center + FVector2f(Warp, -R);
				const FVector2f Right = Center + FVector2f(R, -Warp);
				const FVector2f Bottom = Center + FVector2f(-Warp, R);
				const FVector2f Left = Center + FVector2f(-R, Warp);
				Ink(Top, Right, 610);
				Ink(Right, Bottom, 611);
				Ink(Bottom, Left, 612);
				Ink(Left, Top, 613);

				const float Inner = R * (0.43f + 0.04f * VariantBias);
				const FVector2f ITop = Center + FVector2f(-Warp * 0.35f, -Inner);
				const FVector2f IRight = Center + FVector2f(Inner, Warp * 0.35f);
				const FVector2f IBottom = Center + FVector2f(Warp * 0.35f, Inner);
				const FVector2f ILeft = Center + FVector2f(-Inner, -Warp * 0.35f);
				Ink(ITop, IRight, 620, 0.82f);
				Ink(IRight, IBottom, 621, 0.82f);
				Ink(IBottom, ILeft, 622, 0.82f);
				Ink(ILeft, ITop, 623, 0.82f);

				Ink(ITop, Top, 630, 0.55f, 0.72f);
				Ink(IRight, Right, 631, 0.55f, 0.72f);
				Ink(IBottom, Bottom, 632, 0.55f, 0.72f);
				Ink(ILeft, Left, 633, 0.55f, 0.72f);
				Ink(
					Center + FVector2f(-R * 0.18f, R * 0.04f),
					Center + FVector2f(R * 0.18f, -R * 0.04f),
					640,
					0.62f,
					0.78f);
				break;
			}

			case EGridMapSymbolKind::Pit:
			{
				const float Half = R * 0.92f;
				const float CornerJitter = R * 0.08f;
				auto CornerNoise = [&](int32 SaltX, int32 SaltY)
				{
					return FVector2f(
						UGridMapWidget::ComputeDeterministicArtNoise(Symbol.MapCell, EGridEdge::None, SaltX + Variant * 17) * CornerJitter,
						UGridMapWidget::ComputeDeterministicArtNoise(Symbol.MapCell, EGridEdge::None, SaltY + Variant * 17) * CornerJitter);
				};

				const FVector2f TL = Center + FVector2f(-Half, -Half) + CornerNoise(701, 702);
				const FVector2f TR = Center + FVector2f(Half, -Half) + CornerNoise(703, 704);
				const FVector2f BR = Center + FVector2f(Half, Half) + CornerNoise(705, 706);
				const FVector2f BL = Center + FVector2f(-Half, Half) + CornerNoise(707, 708);
				Ink(TL, TR, 710);
				Ink(TR, BR, 711);
				Ink(BR, BL, 712);
				Ink(BL, TL, 713);

				const float InnerHalf = Half * 0.52f;
				const FVector2f InnerOffset(VariantBias * R * 0.05f, -VariantBias * R * 0.035f);
				const FVector2f ITL = Center + InnerOffset + FVector2f(-InnerHalf, -InnerHalf);
				const FVector2f ITR = Center + InnerOffset + FVector2f(InnerHalf, -InnerHalf);
				const FVector2f IBR = Center + InnerOffset + FVector2f(InnerHalf, InnerHalf);
				const FVector2f IBL = Center + InnerOffset + FVector2f(-InnerHalf, InnerHalf);
				Ink(ITL, ITR, 720, 0.78f);
				Ink(ITR, IBR, 721, 0.78f);
				Ink(IBR, IBL, 722, 0.78f);
				Ink(IBL, ITL, 723, 0.78f);

				Ink(TL, ITL, 730, 0.54f, 0.72f);
				Ink(TR, ITR, 731, 0.54f, 0.72f);
				Ink(BR, IBR, 732, 0.54f, 0.72f);
				Ink(BL, IBL, 733, 0.54f, 0.72f);

				const int32 SafeDepthCount = FMath::Clamp(PitDepthLineCount, 1, 6);
				for (int32 Index = 0; Index < SafeDepthCount; ++Index)
				{
					const float DepthT = static_cast<float>(Index + 1) / static_cast<float>(SafeDepthCount + 1);
					const FVector2f Start = FMath::Lerp(ITL, IBL, DepthT);
					const FVector2f End = FMath::Lerp(ITR, IBR, FMath::Clamp(DepthT + VariantBias * 0.10f, 0.0f, 1.0f));
					Ink(Start, End, 740 + Index * 7, 0.42f, 0.48f);
				}
				break;
			}

			case EGridMapSymbolKind::PointOfInterest:
			{
				const float LongRay = R * (1.0f + VariantBias * 0.05f);
				const float ShortRay = R * (0.46f - VariantBias * 0.04f);
				const float Skew = VariantBias * R * 0.09f;
				Ink(Center + FVector2f(Skew, -LongRay), Center + FVector2f(-Skew, LongRay), 810, 0.92f);
				Ink(Center + FVector2f(-LongRay, -Skew), Center + FVector2f(LongRay, Skew), 811, 0.92f);
				Ink(Center + FVector2f(-ShortRay, -ShortRay), Center + FVector2f(ShortRay, ShortRay), 812, 0.72f);
				Ink(Center + FVector2f(ShortRay, -ShortRay), Center + FVector2f(-ShortRay, ShortRay), 813, 0.72f);

				const float Diamond = R * 0.62f;
				Ink(Center + FVector2f(0.0f, -Diamond), Center + FVector2f(Diamond, 0.0f), 820, 0.52f, 0.70f);
				Ink(Center + FVector2f(Diamond, 0.0f), Center + FVector2f(0.0f, Diamond), 821, 0.52f, 0.70f);
				Ink(Center + FVector2f(0.0f, Diamond), Center + FVector2f(-Diamond, 0.0f), 822, 0.52f, 0.70f);
				Ink(Center + FVector2f(-Diamond, 0.0f), Center + FVector2f(0.0f, -Diamond), 823, 0.52f, 0.70f);

				Ink(
					Center + FVector2f(-R * 0.14f, R * 0.05f),
					Center + FVector2f(R * 0.14f, -R * 0.05f),
					830,
					1.15f,
					0.92f);
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
		float SecondaryAlpha,
		float MarkerScale)
	{
		if (!View.bHasPartyMarker)
		{
			return;
		}

		const FVector2f Center = Metrics.CellCenter(View.PartyMapCell);
		const float Radius = Metrics.CellSize * 0.28f * FMath::Clamp(MarkerScale, 0.4f, 1.0f);
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

	if (MapSurface)
	{
		MapSurface->InitializeMapSurface(this);
	}

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
	if (MapSurface)
	{
		MapSurface->InitializeMapSurface(nullptr);
	}

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
		InvalidateMapSurface();
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

	InvalidateMapSurface();
	return true;
}

void UGridMapWidget::PanMapByPixels(const FVector2D& DeltaPixels)
{
	if (!bHasRenderableMap || DeltaPixels.IsNearlyZero())
	{
		return;
	}

	PanOffsetPixels += DeltaPixels;
	InvalidateMapSurface();
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

	AGridLevelRuntimeActor* Runtime = OwningPartyPawn ? OwningPartyPawn->LevelRuntimeActor.Get() : nullptr;
	if (!bHasFloorSelection || !Runtime || !Runtime->DungeonAsset)
	{
		RefreshFloorNavigationControls();
		InvalidateMapSurface();
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
	InvalidateMapSurface();
	return bHasRenderableMap;
}

bool UGridMapWidget::ResolvePartyFloorZ(int32& OutFloorZ) const
{
	OutFloorZ = 0;
	const AGridLevelRuntimeActor* Runtime = OwningPartyPawn ? OwningPartyPawn->LevelRuntimeActor.Get() : nullptr;
	if (!Runtime || !Runtime->DungeonAsset)
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
	if (!bHasFloorSelection)
	{
		return false;
	}

	const TArray<int32>& AvailableFloorZs = FloorView.AvailableFloorZs;
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

void UGridMapWidget::InvalidateMapSurface()
{
	if (MapSurface)
	{
		MapSurface->RequestRepaint();
	}
}

void UGridMapWidget::RefreshFloorNavigationControls()
{
	if (Border_FloorNavigation)
	{
		Border_FloorNavigation->SetVisibility(
			bHasFloorSelection ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

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

bool UGridMapWidget::IsScreenPositionInsideMapSurface(const FVector2D& ScreenPosition) const
{
	if (!MapSurface)
	{
		return false;
	}

	const FGeometry& SurfaceGeometry = MapSurface->GetCachedGeometry();
	const FVector2D SurfaceSize = SurfaceGeometry.GetLocalSize();
	if (SurfaceSize.X <= 0.0 || SurfaceSize.Y <= 0.0)
	{
		return false;
	}

	const FVector2D LocalPosition = SurfaceGeometry.AbsoluteToLocal(ScreenPosition);
	return GridMapWidgetPrivate::IsInsideMapViewport(LocalPosition, SurfaceSize, MapDrawPadding);
}

FReply UGridMapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bHasRenderableMap &&
		InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton &&
		IsScreenPositionInsideMapSurface(InMouseEvent.GetScreenSpacePosition()))
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
	if (bIsPanning && MapSurface)
	{
		const FGeometry& SurfaceGeometry = MapSurface->GetCachedGeometry();
		const FVector2D CurrentLocal = SurfaceGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		const FVector2D PreviousLocal = SurfaceGeometry.AbsoluteToLocal(InMouseEvent.GetLastScreenSpacePosition());
		PanMapByPixels(CurrentLocal - PreviousLocal);
		return FReply::Handled();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UGridMapWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bHasRenderableMap &&
		!FMath::IsNearlyZero(InMouseEvent.GetWheelDelta()) &&
		IsScreenPositionInsideMapSurface(InMouseEvent.GetScreenSpacePosition()))
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

void UGridMapSurfaceWidget::InitializeMapSurface(UGridMapWidget* InOwnerMapWidget)
{
	OwnerMapWidget = InOwnerMapWidget;
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UGridMapSurfaceWidget::RequestRepaint()
{
	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 UGridMapSurfaceWidget::NativePaint(
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

	return OwnerMapWidget
		? OwnerMapWidget->PaintMapSurface(AllottedGeometry, OutDrawElements, BaseLayer + 1)
		: BaseLayer;
}

int32 UGridMapWidget::PaintMapSurface(
	const FGeometry& AllottedGeometry,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId) const
{
	if (!bHasRenderableMap || FloorView.Cells.IsEmpty())
	{
		return LayerId;
	}

	using namespace GridMapWidgetPrivate;
	FRenderMetrics Metrics;
	if (!BuildRenderMetrics(
		FloorView,
		AllottedGeometry,
		MapDrawPadding,
		AutoFitMarginCells,
		MaxCellPixels,
		ZoomScale,
		PanOffsetPixels,
		bCenterViewOnParty,
		Metrics))
	{
		return LayerId;
	}

	const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();
	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	const bool bUseTexturedTheme = IsValid(VisualTheme);
	TOptional<FThemeBrushSet> ThemeBrushes;
	if (bUseTexturedTheme)
	{
		ThemeBrushes.Emplace(*VisualTheme);
	}

	const int32 ParchmentLayer = LayerId;
	const int32 CellLayer = ParchmentLayer + 2;
	const int32 HatchLayer = CellLayer + 1;
	const int32 FeatherLayer = HatchLayer + 1;
	const int32 BoundaryLayer = FeatherLayer + 1;
	const int32 PillarLayer = BoundaryLayer + 1;
	const int32 SymbolLayer = PillarLayer + 1;
	const int32 MarkerLayer = SymbolLayer + 2;

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const FVector2D MapViewportSize(
		FMath::Max(0.0f, static_cast<float>(LocalSize.X) - MapDrawPadding.Left - MapDrawPadding.Right),
		FMath::Max(0.0f, static_cast<float>(LocalSize.Y) - MapDrawPadding.Top - MapDrawPadding.Bottom));
	const FPaintGeometry MapViewportPaintGeometry = AllottedGeometry.ToPaintGeometry(
		MapViewportSize,
		FSlateLayoutTransform(FVector2D(MapDrawPadding.Left, MapDrawPadding.Top)));
	OutDrawElements.PushClip(FSlateClippingZone(MapViewportPaintGeometry));

	if (bUseTexturedTheme && VisualTheme->ParchmentTexture)
	{
		FLinearColor ParchmentTint = FLinearColor::White;
		ParchmentTint.A = FMath::Clamp(VisualTheme->ParchmentOpacity, 0.0f, 1.0f);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			static_cast<uint32>(ParchmentLayer),
			MapViewportPaintGeometry,
			&ThemeBrushes->Parchment,
			ESlateDrawEffect::None,
			ParchmentTint);
	}
	else if (bEnableParchmentStyle)
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
			ParchmentEdgeColor,
			ParchmentEdgeThickness,
			ParchmentGrainLineCount);
	}

	TSet<FIntPoint> VisibleCells;
	VisibleCells.Reserve(FloorView.Cells.Num());
	for (const FGridMapFloorCellView& Cell : FloorView.Cells)
	{
		VisibleCells.Add(Cell.MapCell);
		const FVector2f TopLeft = Metrics.CellTopLeft(Cell.MapCell);
		const FPaintGeometry CellPaintGeometry = AllottedGeometry.ToPaintGeometry(
			FVector2D(Metrics.CellSize, Metrics.CellSize),
			FSlateLayoutTransform(FVector2D(TopLeft.X, TopLeft.Y)));

		if (bUseTexturedTheme && VisualTheme->FloorTexture)
		{
			FLinearColor FloorTint = FLinearColor::White;
			FloorTint.A = FMath::Clamp(VisualTheme->FloorOpacity, 0.0f, 1.0f);
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				static_cast<uint32>(CellLayer),
				CellPaintGeometry,
				&ThemeBrushes->Floor,
				ESlateDrawEffect::None,
				FloorTint);
		}
		else
		{
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				static_cast<uint32>(CellLayer),
				CellPaintGeometry,
				WhiteBrush,
				ESlateDrawEffect::None,
				ExploredCellColor);
		}

		if (!bUseTexturedTheme && bEnableParchmentStyle)
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

	if (!bUseTexturedTheme && bEnableParchmentStyle && Metrics.CellSize >= 8.0f)
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

	TMap<FIntPoint, FVector2f> WallPillarCenters;
	for (const FGridMapFloorBoundaryView& Boundary : FloorView.Boundaries)
	{
		FVector2f A;
		FVector2f B;
		GetBoundaryEndpoints(Metrics, Boundary, A, B);

		const bool bVisuallySolidWall =
			Boundary.Kind == EGridMapBoundaryKind::Wall ||
			(Boundary.Kind == EGridMapBoundaryKind::SecretDoor && !Boundary.bDoorOpen);
		if (bUseTexturedTheme && VisualTheme->WallPillarTexture && bVisuallySolidWall)
		{
			FIntPoint PillarVertex;
			FVector2f PillarCenter;
			if (GetWallLeftPillarPlacement(Boundary, A, B, PillarVertex, PillarCenter))
			{
				WallPillarCenters.FindOrAdd(PillarVertex, PillarCenter);
			}
		}

		if (bUseTexturedTheme)
		{
			const float TextureThickness =
				FMath::Max(1.0f, Metrics.CellSize * FMath::Clamp(VisualTheme->BoundaryThicknessRatio, 0.03f, 0.50f));

			if (Boundary.Kind == EGridMapBoundaryKind::SecretDoor)
			{
				// A discovered secret has no dedicated map glyph:
				// closed = ordinary wall, open = ordinary passage.
				if (Boundary.bDoorOpen)
				{
					continue;
				}

				if (VisualTheme->WallTexture)
				{
					DrawTexturedSegment(
						OutDrawElements,
						BoundaryLayer,
						AllottedGeometry,
						A,
						B,
						&ThemeBrushes->Wall,
						TextureThickness);
				}
				else
				{
					DrawLine(OutDrawElements, BoundaryLayer, PaintGeometry, A, B, WallColor, WallThickness);
				}
				continue;
			}

			const FSlateBrush* BoundaryBrush = nullptr;
			switch (Boundary.Kind)
			{
				case EGridMapBoundaryKind::Wall:
					BoundaryBrush = VisualTheme->WallTexture ? &ThemeBrushes->Wall : nullptr;
					break;

				case EGridMapBoundaryKind::Door:
					BoundaryBrush = Boundary.bDoorOpen
						? (VisualTheme->DoorOpenTexture ? &ThemeBrushes->DoorOpen : nullptr)
						: (VisualTheme->DoorClosedTexture ? &ThemeBrushes->DoorClosed : nullptr);
					break;

				default:
					break;
			}

			if (BoundaryBrush)
			{
				DrawTexturedSegment(
					OutDrawElements,
					BoundaryLayer,
					AllottedGeometry,
					A,
					B,
					BoundaryBrush,
					TextureThickness);
				continue;
			}
		}

		switch (Boundary.Kind)
		{
			case EGridMapBoundaryKind::Wall:
				if (!bUseTexturedTheme && bEnableParchmentStyle)
				{
					DrawWallBoundary(
						OutDrawElements,
						BoundaryLayer,
						PaintGeometry,
						A,
						B,
						WallUnderlayColor,
						WallColor,
						WallThickness,
						WallUnderlayThicknessScale,
						Boundary.MapCell,
						Boundary.Edge,
						HandDrawnJitterPixels,
						SecondaryStrokeAlpha,
						WallStoneMarkCount);
				}
				else
				{
					DrawLine(OutDrawElements, BoundaryLayer, PaintGeometry, A, B, WallColor, WallThickness);
				}
				break;

			case EGridMapBoundaryKind::Door:
			case EGridMapBoundaryKind::SecretDoor:
				DrawDoorBoundary(
					OutDrawElements,
					BoundaryLayer,
					PaintGeometry,
					A,
					B,
					Boundary.Kind == EGridMapBoundaryKind::SecretDoor ? SecretDoorColor : DoorColor,
					DoorThickness,
					Boundary.bDoorOpen,
					Boundary.Kind == EGridMapBoundaryKind::SecretDoor,
					Boundary.MapCell,
					Boundary.Edge,
					!bUseTexturedTheme && bEnableParchmentStyle ? HandDrawnJitterPixels : 0.0f,
					!bUseTexturedTheme && bEnableParchmentStyle ? SecondaryStrokeAlpha : 0.0f,
					DoorJambLengthScale,
					!bUseTexturedTheme && bEnableParchmentStyle ? DoorPanelLineCount : 0);
				break;

			default:
				break;
		}
	}

	if (bUseTexturedTheme && VisualTheme->WallPillarTexture)
	{
		const float PillarSize =
			Metrics.CellSize * FMath::Clamp(VisualTheme->WallPillarScale, 0.05f, 0.50f);
		for (const TPair<FIntPoint, FVector2f>& Pillar : WallPillarCenters)
		{
			DrawTexturedSquare(
				OutDrawElements,
				PillarLayer,
				AllottedGeometry,
				Pillar.Value,
				PillarSize,
				&ThemeBrushes->WallPillar);
		}
	}

	for (const FGridMapFloorSymbolView& Symbol : FloorView.Symbols)
	{
		const FSlateBrush* TexturedSymbolBrush =
			bUseTexturedTheme ? GetSymbolBrush(*VisualTheme, *ThemeBrushes, Symbol.Kind) : nullptr;
		if (TexturedSymbolBrush)
		{
			if (Metrics.CellSize >= FMath::Clamp(VisualTheme->SymbolMinCellPixels, 4.0f, 64.0f))
			{
				const float TextureSize =
					Metrics.CellSize * FMath::Clamp(VisualTheme->SymbolScale, 0.20f, 1.00f);
				const bool bDirectionalStairs =
					Symbol.Kind == EGridMapSymbolKind::StairsUp || Symbol.Kind == EGridMapSymbolKind::StairsDown;
				DrawTexturedSquare(
					OutDrawElements,
					SymbolLayer,
					AllottedGeometry,
					Metrics.CellCenter(Symbol.MapCell),
					TextureSize,
					TexturedSymbolBrush,
					bDirectionalStairs ? GetMapFacingAngle(Symbol.Facing) : 0.0f);
			}
			continue;
		}

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
			!bUseTexturedTheme && bEnableParchmentStyle ? HandDrawnJitterPixels : 0.0f,
			!bUseTexturedTheme && bEnableParchmentStyle ? SecondaryStrokeAlpha : 0.0f,
			StairStepCount,
			PitDepthLineCount,
			SymbolVariantCount);
	}

	if (bUseTexturedTheme && VisualTheme->PartyMarkerTexture && FloorView.bHasPartyMarker)
	{
		const float TextureSize =
			Metrics.CellSize * FMath::Clamp(VisualTheme->PartyMarkerScale, 0.20f, 1.00f);
		DrawTexturedSquare(
			OutDrawElements,
			MarkerLayer,
			AllottedGeometry,
			Metrics.CellCenter(FloorView.PartyMapCell),
			TextureSize,
			&ThemeBrushes->PartyMarker,
			GetMapFacingAngle(FloorView.PartyFacing));
	}
	else if (!bUseTexturedTheme && bEnableParchmentStyle)
	{
		DrawPartyMarker(
			OutDrawElements,
			MarkerLayer,
			PaintGeometry,
			Metrics,
			FloorView,
			PartyMarkerColor,
			HandDrawnJitterPixels,
			SecondaryStrokeAlpha,
			PartyMarkerScale);
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
			0.0f,
			PartyMarkerScale);
	}

	OutDrawElements.PopClip();
	return MarkerLayer;
}
