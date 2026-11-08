#pragma once

#include "CoreMinimal.h"

/** FIREFX11: deterministic, purely visual ignition across burning oil cells.
 * Only orthogonally adjacent cells propagate fire; disconnected islands start
 * at their nearest cell to the impact, without creating intermediate fire.
 */
struct FGridFireIgnitionWave
{
	/** Shortest orthogonal paths are counted in integer cell steps.
	 * A single float multiplication at the end avoids accumulated drift
	 * and keeps the algorithm independent of TMap readback during BFS.
	 */
	static void BuildDelays(FIntPoint Impact, const TArray<FIntPoint>& Cells,
		float SecondsPerCell, TMap<FIntPoint, float>& OutDelays)
	{
		OutDelays.Reset();
		TSet<FIntPoint> Remaining;
		for (const FIntPoint& Cell : Cells) Remaining.Add(Cell);
		const float Step = FMath::Max(0.f, SecondsPerCell);
		const FIntPoint Directions[] = {
			FIntPoint(0, 1), FIntPoint(1, 0), FIntPoint(0, -1), FIntPoint(-1, 0)
		};
		struct FCellStep
		{
			FIntPoint Cell = FIntPoint::ZeroValue;
			int32 Steps = 0;
		};
		while (!Remaining.IsEmpty())
		{
			bool bHasSeed = false;
			FIntPoint Seed = FIntPoint::ZeroValue;
			int32 Distance = MAX_int32;
			for (const FIntPoint& Candidate : Remaining)
			{
				const int32 Manhattan = FMath::Abs(Candidate.X - Impact.X) +
					FMath::Abs(Candidate.Y - Impact.Y);
				if (!bHasSeed || Manhattan < Distance ||
					(Manhattan == Distance &&
					(Candidate.Y < Seed.Y ||
					(Candidate.Y == Seed.Y && Candidate.X < Seed.X))))
				{
					Seed = Candidate;
					Distance = Manhattan;
					bHasSeed = true;
				}
			}
			TArray<FCellStep> Queue;
			FCellStep First;
			First.Cell = Seed;
			First.Steps = Distance;
			Queue.Add(First);
			Remaining.Remove(Seed);
			for (int32 Head = 0; Head < Queue.Num(); ++Head)
			{
				const FCellStep Current = Queue[Head];
				OutDelays.Add(Current.Cell, static_cast<float>(Current.Steps) * Step);
				for (const FIntPoint& Direction : Directions)
				{
					const FIntPoint Adjacent = Current.Cell + Direction;
					if (Remaining.Remove(Adjacent) > 0)
					{
						FCellStep Next;
						Next.Cell = Adjacent;
						Next.Steps = Current.Steps + 1;
						Queue.Add(Next);
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
