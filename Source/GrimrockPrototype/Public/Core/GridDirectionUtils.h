#pragma once

#include "CoreMinimal.h"
#include "GridTypes.h"

namespace GridDirectionUtils
{
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
