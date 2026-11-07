#pragma once

#include "CoreMinimal.h"

/** FIREFX01: topology-only grouping of adjacent fire cells for spatial audio. */
struct FGridFireAudioCluster
{
	FIntPoint Anchor = FIntPoint::ZeroValue;
	FVector2D CenterInCells = FVector2D::ZeroVector;
	int32 CellCount = 0;
};

struct FGridFireAudioClusters
{
	static void Build(const TArray<FIntPoint>& Cells, int32 MaxEmitters,
		TArray<FGridFireAudioCluster>& Out)
	{
		Out.Reset();
		if (MaxEmitters <= 0 || Cells.IsEmpty()) return;
		TArray<FIntPoint> Sorted = Cells;
		Sorted.Sort([](const FIntPoint& A, const FIntPoint& B)
		{
			return A.Y == B.Y ? A.X < B.X : A.Y < B.Y;
		});
		TSet<FIntPoint> Pending;
		for (const FIntPoint& Cell : Sorted) Pending.Add(Cell);
		for (const FIntPoint& Seed : Sorted)
		{
			if (Pending.Remove(Seed) == 0) continue;
			TArray<FIntPoint> Queue;
			Queue.Add(Seed);
			FVector2D Sum(0.f, 0.f);
			for (int32 I = 0; I < Queue.Num(); ++I)
			{
				const FIntPoint Cell = Queue[I];
				Sum += FVector2D(Cell.X + .5f, Cell.Y + .5f);
				const FIntPoint Neighbors[] = {
					FIntPoint(Cell.X + 1, Cell.Y), FIntPoint(Cell.X - 1, Cell.Y),
					FIntPoint(Cell.X, Cell.Y + 1), FIntPoint(Cell.X, Cell.Y - 1)
				};
				for (const FIntPoint& Neighbor : Neighbors)
				{
					if (Pending.Remove(Neighbor) != 0) Queue.Add(Neighbor);
				}
			}
			if (Out.Num() < MaxEmitters)
			{
				FGridFireAudioCluster& Cluster = Out.AddDefaulted_GetRef();
				Cluster.Anchor = Seed;
				Cluster.CenterInCells = Sum / static_cast<float>(Queue.Num());
				Cluster.CellCount = Queue.Num();
			}
		}
	}
};
