#pragma once

#include "CoreMinimal.h"

/** FIREFX11: deterministic, purely visual ignition across burning oil cells.
 * Only orthogonally adjacent cells propagate fire; disconnected islands start
 * at their nearest cell to the impact, without creating intermediate fire.
 */
struct FGridFireIgnitionWave
{
	static void BuildDelays(FIntPoint Impact, const TArray<FIntPoint>& Cells,
		float SecondsPerCell, TMap<FIntPoint, float>& OutDelays)
	{
		OutDelays.Reset();
		TSet<FIntPoint> Unvisited;
		for (const FIntPoint& Cell : Cells) Unvisited.Add(Cell);
		const float Step = FMath::Max(0.f, SecondsPerCell);
		const FIntPoint Directions[] = {
			FIntPoint(0, 1), FIntPoint(1, 0), FIntPoint(0, -1), FIntPoint(-1, 0)
		};
		while (!Unvisited.IsEmpty())
		{
			bool bHaveSeed = false;
			FIntPoint Seed = FIntPoint::ZeroValue;
			int32 BestDistance = MAX_int32;
			for (const FIntPoint& Candidate : Unvisited)
			{
				const int32 Distance = FMath::Abs(Candidate.X - Impact.X) +
					FMath::Abs(Candidate.Y - Impact.Y);
				if (!bHaveSeed || Distance < BestDistance ||
					(Distance == BestDistance &&
						(Candidate.Y < Seed.Y ||
						(Candidate.Y == Seed.Y && Candidate.X < Seed.X))))
				{
					Seed = Candidate;
					BestDistance = Distance;
					bHaveSeed = true;
				}
			}
			TArray<FIntPoint> Frontier;
			Frontier.Add(Seed);
			Unvisited.Remove(Seed);
			OutDelays.Add(Seed, BestDistance * Step);
			for (int32 Head = 0; Head < Frontier.Num(); ++Head)
			{
				const FIntPoint Here = Frontier[Head];
				for (const FIntPoint& Direction : Directions)
				{
					const FIntPoint Next = Here + Direction;
					if (Unvisited.Remove(Next) > 0)
					{
						OutDelays.Add(Next, OutDelays[Here] + Step);
						Frontier.Add(Next);
					}
				}
			}
		}
	}

	static float Ease(float Alpha)
	{
		const float T = FMath::Clamp(Alpha, 0.f, 1.f);
		return T * T * (3.f - 2.f * T);
	}

	static float CoverageFraction(float InitialCoverage, float FinalCoverage, float Alpha)
	{
		const float Final = FMath::Max(.01f, FinalCoverage);
		const float Start = FMath::Min(Final, FMath::Max(.01f, InitialCoverage));
		return FMath::Lerp(Start / Final, 1.f, Ease(Alpha));
	}
};
