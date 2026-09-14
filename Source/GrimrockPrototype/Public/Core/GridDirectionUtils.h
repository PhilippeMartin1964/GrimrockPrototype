#pragma once

#include "CoreMinimal.h"
#include "GridTypes.h"

namespace GridDirectionUtils
{
	struct FBoundaryFrame
	{
		FVector Normal = FVector::ZeroVector;
		FVector Tangent = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
	};

	FORCEINLINE EGridEdge RotateLeft(EGridEdge Dir)
	{
		switch (Dir)
		{
			case EGridEdge::North:
				return EGridEdge::East;
			case EGridEdge::East:
				return EGridEdge::South;
			case EGridEdge::South:
				return EGridEdge::West;
			case EGridEdge::West:
				return EGridEdge::North;
			default:
				return EGridEdge::North;
		}
	}

	FORCEINLINE EGridEdge RotateRight(EGridEdge Dir)
	{
		switch (Dir)
		{
			case EGridEdge::North:
				return EGridEdge::West;
			case EGridEdge::West:
				return EGridEdge::South;
			case EGridEdge::South:
				return EGridEdge::East;
			case EGridEdge::East:
				return EGridEdge::North;
			default:
				return EGridEdge::North;
		}
	}

	FORCEINLINE float ToYaw(EGridEdge Dir)
	{
		switch (Dir)
		{
			case EGridEdge::North:
				return 90.f;
			case EGridEdge::East:
				return 0.f;
			case EGridEdge::South:
				return -90.f;
			case EGridEdge::West:
				return 180.f;
			default:
				return 0.f;
		}
	}

	FORCEINLINE bool IsCardinal(EGridEdge Dir)
	{
		return Dir == EGridEdge::North || Dir == EGridEdge::East || Dir == EGridEdge::South || Dir == EGridEdge::West;
	}

	/**
	 * WALL-AXIS01 canonical wall-boundary frame.
	 * Wall-bound meshes are authored with local X as wall normal/front,
	 * local Y as the horizontal wall length and local Z as vertical.
	 */
	FORCEINLINE bool ResolveBoundaryFrame(EGridEdge Dir, FBoundaryFrame& OutFrame)
	{
		OutFrame = FBoundaryFrame();
		if (!IsCardinal(Dir))
		{
			return false;
		}

		OutFrame.Rotation = FRotator(0.f, ToYaw(Dir), 0.f);
		OutFrame.Normal = OutFrame.Rotation.RotateVector(FVector::ForwardVector);
		OutFrame.Tangent = OutFrame.Rotation.RotateVector(FVector::RightVector);
		return true;
	}

	/**
	 * Resolves a transform on a cell boundary using the canonical Y-axis wall convention.
	 * Inset is measured inward from the boundary and AlongWall follows the existing authored U convention.
	 */
	FORCEINLINE bool ResolveBoundaryTransform(const FVector& CellOrigin, float CellSize, EGridEdge Dir, float Vertical, float Inset, float AlongWall,
		FTransform& OutTransform)
	{
		FBoundaryFrame Frame;
		if (!ResolveBoundaryFrame(Dir, Frame))
		{
			return false;
		}

		const FVector CellCenter = CellOrigin + FVector(CellSize * 0.5f, CellSize * 0.5f, Vertical);
		const FVector Position = CellCenter + (Frame.Normal * ((CellSize * 0.5f) - Inset)) - (Frame.Tangent * AlongWall);
		OutTransform = FTransform(Frame.Rotation, Position, FVector::OneVector);
		return true;
	}

	FORCEINLINE EGridEdge GetForward(EGridEdge Facing)
	{
		return Facing;
	}

	FORCEINLINE EGridEdge GetBackward(EGridEdge Facing)
	{
		return RotateRight(RotateRight(Facing));
	}

	FORCEINLINE EGridEdge GetOpposite(EGridEdge Direction)
	{
		switch (Direction)
		{
			case EGridEdge::North:
				return EGridEdge::South;
			case EGridEdge::East:
				return EGridEdge::West;
			case EGridEdge::South:
				return EGridEdge::North;
			case EGridEdge::West:
				return EGridEdge::East;
			default:
				return EGridEdge::None;
		}
	}

	FORCEINLINE EGridEdge GetLeft(EGridEdge Facing)
	{
		return RotateLeft(Facing);
	}

	FORCEINLINE EGridEdge GetRight(EGridEdge Facing)
	{
		return RotateRight(Facing);
	}
}
