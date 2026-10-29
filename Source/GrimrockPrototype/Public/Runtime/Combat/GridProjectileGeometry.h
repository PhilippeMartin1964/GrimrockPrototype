#pragma once

#include "CoreMinimal.h"
#include "Core/GridObjectBehavior.h"
#include "GridProjectileGeometry.generated.h"

/** Tip axis in the imported Unreal StaticMesh's local coordinate system. */
UENUM(BlueprintType)
enum class EGridProjectileForwardAxis : uint8
{
	PositiveX UMETA(DisplayName = "+X (Unreal forward)"),
	NegativeX UMETA(DisplayName = "-X"),
	PositiveY UMETA(DisplayName = "+Y (World North)"),
	NegativeY UMETA(DisplayName = "-Y"),
	PositiveZ UMETA(DisplayName = "+Z"),
	NegativeZ UMETA(DisplayName = "-Z")
};

/** One geometric flight-frame rule for arrows, thrown items, monster shots and spells. */
struct GRIMROCKPROTOTYPE_API FGridProjectileGeometry
{
	static FVector MeshTipAxis(EGridProjectileForwardAxis Axis)
	{
		switch (Axis)
		{
			case EGridProjectileForwardAxis::NegativeX: return -FVector::ForwardVector;
			case EGridProjectileForwardAxis::PositiveY: return FVector::RightVector;
			case EGridProjectileForwardAxis::NegativeY: return -FVector::RightVector;
			case EGridProjectileForwardAxis::PositiveZ: return FVector::UpVector;
			case EGridProjectileForwardAxis::NegativeZ: return -FVector::UpVector;
			default: return FVector::ForwardVector;
		}
	}

	static FQuat MeshToFlightRotation(EGridProjectileForwardAxis Axis)
	{
		return FQuat::FindBetweenNormals(MeshTipAxis(Axis), FVector::ForwardVector);
	}

	/**
	 * A combat Miss uses a T-shaped aiming plane: Forward from shooter to
	 * target, Right perpendicular to it, and a left/right miss waypoint beyond
	 * the monster's collision bounds. The moving projectile heads for that
	 * waypoint without teleportation; geometry still blocks it normally.
	 */
	static bool CalculateMissDeflection(const FVector& Source, const FVector& Target,
		const FVector& TargetBoxExtent, float ProjectileRadius, float SafetyMargin,
		int32 SideSign, FVector& OutDirection, FVector& OutAimPoint)
	{
		OutDirection = FVector::ZeroVector;
		OutAimPoint = FVector::ZeroVector;
		if (Source.ContainsNaN() || Target.ContainsNaN() || TargetBoxExtent.ContainsNaN() ||
			!FMath::IsFinite(ProjectileRadius) || !FMath::IsFinite(SafetyMargin) ||
			(SideSign != -1 && SideSign != 1))
		{
			return false;
		}
		FVector Forward = Target - Source;
		Forward.Z = 0.0f;
		if (!Forward.Normalize())
		{
			return false;
		}
		const FVector Right = FVector::CrossProduct(Forward, FVector::UpVector);
		const FVector Extent(FMath::Abs(TargetBoxExtent.X), FMath::Abs(TargetBoxExtent.Y),
			FMath::Abs(TargetBoxExtent.Z));
		const float HalfWidth = FMath::Abs(Right.X) * Extent.X + FMath::Abs(Right.Y) * Extent.Y;
		const float Clearance = HalfWidth + FMath::Max(0.0f, ProjectileRadius) + FMath::Max(0.0f, SafetyMargin);
		OutAimPoint = Target + Right * (static_cast<float>(SideSign) * Clearance);
		OutDirection = (OutAimPoint - Source).GetSafeNormal();
		return !OutDirection.IsNearlyZero();
	}

	/** Select the cardinal grid heading of a projectile's horizontal velocity. */
	static EGridEdge FlightCardinalEdge(const FVector& WorldVelocity)
	{
		if (WorldVelocity.ContainsNaN() ||
			FVector2D(WorldVelocity.X, WorldVelocity.Y).SizeSquared() <= KINDA_SMALL_NUMBER)
		{
			return EGridEdge::None;
		}
		if (FMath::Abs(WorldVelocity.X) > FMath::Abs(WorldVelocity.Y))
		{
			return WorldVelocity.X >= 0.0f ? EGridEdge::East : EGridEdge::West;
		}
		return WorldVelocity.Y >= 0.0f ? EGridEdge::North : EGridEdge::South;
	}

	/**
	 * A door on the rear boundary of the launch cell must not hit a projectile
	 * going away from it, even if its render/collision mesh extends into that
	 * cell. The SAME shared boundary may be authored on either adjacent cell.
	 * Doors on the forward or side boundaries remain physical blockers.
	 */
	static bool IsRearBoundaryDoor(int32 SourceX, int32 SourceY, const FVector& WorldVelocity,
		int32 DoorX, int32 DoorY, EGridEdge DoorEdge)
	{
		EGridEdge Forward = FlightCardinalEdge(WorldVelocity);
		EGridEdge Rear = EGridEdge::None;
		int32 RearCellX = SourceX;
		int32 RearCellY = SourceY;
		switch (Forward)
		{
			case EGridEdge::North: Rear = EGridEdge::South; --RearCellY; break;
			case EGridEdge::East: Rear = EGridEdge::West; --RearCellX; break;
			case EGridEdge::South: Rear = EGridEdge::North; ++RearCellY; break;
			case EGridEdge::West: Rear = EGridEdge::East; ++RearCellX; break;
			default: return false;
		}
		return (DoorX == SourceX && DoorY == SourceY && DoorEdge == Rear) ||
			(DoorX == RearCellX && DoorY == RearCellY && DoorEdge == Forward);
	}
};
