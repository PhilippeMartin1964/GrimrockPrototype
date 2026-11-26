#pragma once

#include "CoreMinimal.h"

/** Object-centred camera state, independent of the native editor navigation. */
struct FGridItemWorkshopOrbit
{
	FVector Target = FVector::ZeroVector;
	double Radius = 1.0;
	double Distance = 35.0;
	FRotator Rotation = FRotator(-12.0, 35.0, 0.0);
	bool bReady = false;

	double MinDistance() const { return Radius + 12.0; }
	double MaxDistance() const { return FMath::Max(MinDistance() * 2.0, Radius * 15.0); }

	void SetBounds(const FBoxSphereBounds& Bounds, bool bRecenter, const FRotator& InitialRotation = FRotator(-12.0, 35.0, 0.0))
	{
		Target = Bounds.Origin;
		Radius = FMath::Max(1.0, Bounds.SphereRadius);
		if (bRecenter || !bReady)
		{
			Rotation = InitialRotation;
			Rotation.Roll = 0.0;
			Distance = FMath::Max(Radius * 3.5, MinDistance() * 1.25);
		}
		Distance = FMath::Clamp(Distance, MinDistance(), MaxDistance());
		bReady = true;
	}

	void Drag(const FVector2D& Delta)
	{
		if (bReady)
		{
			Rotation.Yaw = FRotator::NormalizeAxis(Rotation.Yaw - Delta.X * 0.35);
			Rotation.Pitch = FMath::Clamp(Rotation.Pitch + Delta.Y * 0.35, -89.0, 89.0);
			Rotation.Roll = 0.0;
		}
	}

	void Zoom(double WheelDelta)
	{
		if (bReady)
		{
			Distance = FMath::Clamp(Distance * FMath::Pow(0.85, FMath::Clamp(WheelDelta, -20.0, 20.0)), MinDistance(), MaxDistance());
		}
	}

	FVector CameraLocation() const { return Target - Rotation.Vector() * Distance; }
	void Clear() { bReady = false; }
};
