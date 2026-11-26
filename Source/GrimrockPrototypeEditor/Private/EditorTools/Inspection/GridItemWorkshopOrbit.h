#pragma once

#include "CoreMinimal.h"

/** Object-centred camera state, independent of the native editor navigation. */
struct FGridItemWorkshopOrbit
{
	FVector Target = FVector::ZeroVector;
	double Radius = 1.0;
	double Distance = 35.0;
	FQuat Orientation = FRotator(-12.0, 35.0, 0.0).Quaternion();
	bool bReady = false;

	double MinDistance() const { return Radius + 12.0; }
	double MaxDistance() const { return FMath::Max(MinDistance() * 2.0, Radius * 15.0); }

	void SetBounds(const FBoxSphereBounds& Bounds, bool bRecenter, const FRotator& InitialRotation = FRotator(-12.0, 35.0, 0.0))
	{
		Target = Bounds.Origin;
		Radius = FMath::Max(1.0, Bounds.SphereRadius);
		if (bRecenter || !bReady)
		{
			Orientation = InitialRotation.Quaternion().GetNormalized();
			Distance = FMath::Max(Radius * 3.5, MinDistance() * 1.25);
		}
		Distance = FMath::Clamp(Distance, MinDistance(), MaxDistance());
		bReady = true;
	}

	void Drag(const FVector2D& Delta)
	{
		if (bReady)
		{
			// Grab the object: screen-right drag reveals/moves its front toward
			// screen right, screen-down drag toward screen down. Both axes are
			// camera-local, even when the camera is upside down.
			const FQuat Horizontal(FVector::UpVector, FMath::DegreesToRadians(Delta.X * 0.35));
			const FQuat Vertical(FVector::RightVector, FMath::DegreesToRadians(Delta.Y * 0.35));
			Orientation = (Orientation * Horizontal * Vertical).GetNormalized();
		}
	}

	void Zoom(double WheelDelta)
	{
		if (bReady)
		{
			Distance = FMath::Clamp(Distance * FMath::Pow(0.85, FMath::Clamp(WheelDelta, -20.0, 20.0)), MinDistance(), MaxDistance());
		}
	}

	FRotator ViewRotation() const { return Orientation.Rotator(); }
	FVector CameraLocation() const { return Target - Orientation.GetAxisX() * Distance; }
	void Clear() { bReady = false; }
};
