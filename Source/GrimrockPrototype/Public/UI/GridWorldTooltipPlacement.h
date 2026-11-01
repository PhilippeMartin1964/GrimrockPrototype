#pragma once

#include "CoreMinimal.h"

/** TRAP01.9.3: presentation-only quadrant preference for world hover tooltips.
 * All coordinates are logical UMG viewport units (not raw desktop pixels).
 */
enum class EGridWorldTooltipQuadrant : uint8
{
	UpperLeft,
	UpperRight,
	LowerLeft,
	LowerRight
};

struct FGridWorldTooltipPlacement
{
	FVector2D Position = FVector2D::ZeroVector;
	EGridWorldTooltipQuadrant Quadrant = EGridWorldTooltipQuadrant::UpperLeft;

	/** Pure positioning; never modifies world, widget, cursor or interaction state. */
	static FGridWorldTooltipPlacement Resolve(
		const FVector2D& Mouse, const FVector2D& TooltipSize, const FVector2D& ViewportSize,
		double CursorGap = 24.0, double ScreenMargin = 8.0)
	{
		const double Gap = FMath::Max(0.0, CursorGap);
		const double Margin = FMath::Max(0.0, ScreenMargin);
		const FVector2D Size(FMath::Max(0.0, TooltipSize.X), FMath::Max(0.0, TooltipSize.Y));
		const FVector2D Candidates[4] =
		{
			FVector2D(Mouse.X - Size.X - Gap, Mouse.Y - Size.Y - Gap), // Above-left: cursor at tooltip bottom-right.
			FVector2D(Mouse.X + Gap, Mouse.Y - Size.Y - Gap),           // Above-right.
			FVector2D(Mouse.X - Size.X - Gap, Mouse.Y + Gap),           // Below-left.
			FVector2D(Mouse.X + Gap, Mouse.Y + Gap)                     // Below-right.
		};

		const double MaxX = FMath::Max(Margin, ViewportSize.X - Margin - Size.X);
		const double MaxY = FMath::Max(Margin, ViewportSize.Y - Margin - Size.Y);
		auto ClampToScreen = [&](const FVector2D& Point)
		{
			return FVector2D(
				FMath::Clamp(Point.X, Margin, MaxX),
				FMath::Clamp(Point.Y, Margin, MaxY));
		};
		auto MakePlacement = [&](int32 Index, const FVector2D& Point)
		{
			FGridWorldTooltipPlacement Result;
			Result.Position = Point;
			Result.Quadrant = static_cast<EGridWorldTooltipQuadrant>(Index);
			return Result;
		};

		const FVector2D ClampedPreferred = ClampToScreen(Candidates[0]);
		// Keep the visual preference when only a small edge correction is needed.
		// Crucial for wide trap tooltips near the screen's horizontal midpoint:
		// flipping right for a few missing pixels would reproduce the old layout.
		const bool bSmallCorrection =
			FMath::Abs(ClampedPreferred.X - Candidates[0].X) <= Gap &&
			FMath::Abs(ClampedPreferred.Y - Candidates[0].Y) <= Gap;
		const bool bCursorStillBelowRight =
			ClampedPreferred.X + Size.X <= Mouse.X - 4.0 &&
			ClampedPreferred.Y + Size.Y <= Mouse.Y - 4.0;
		if (bSmallCorrection && bCursorStillBelowRight)
		{
			return MakePlacement(0, ClampedPreferred);
		}

		// Full fits first, in the agreed order: upper-left, upper-right,
		// lower-left, lower-right. Never let a tooltip obscure the mouse.
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const FVector2D& Candidate = Candidates[Index];
			if (Candidate.X >= Margin && Candidate.Y >= Margin &&
				Candidate.X + Size.X <= ViewportSize.X - Margin &&
				Candidate.Y + Size.Y <= ViewportSize.Y - Margin)
			{
				return MakePlacement(Index, Candidate);
			}
		}

		// No quadrant fits: preserve the largest feasible on-screen position,
		// minimizing the distance required to correct the raw candidate.
		// A tooltip larger than the usable viewport cannot fit entirely.
		double BestCorrection = TNumericLimits<double>::Max();
		int32 BestIndex = 0;
		FVector2D BestPosition = ClampedPreferred;
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const FVector2D Clamped = ClampToScreen(Candidates[Index]);
			const double Correction =
				FMath::Abs(Clamped.X - Candidates[Index].X) +
				FMath::Abs(Clamped.Y - Candidates[Index].Y);
			if (Correction < BestCorrection)
			{
				BestCorrection = Correction;
				BestIndex = Index;
				BestPosition = Clamped;
			}
		}
		return MakePlacement(BestIndex, BestPosition);
	}
};
