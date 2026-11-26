#pragma once

#include "CoreMinimal.h"

/** Object-centred inspection camera. Distances are actual Unreal centimetres.
 * Pan is a camera-plane offset, not a modification of the mesh or its pivot.
 */
struct FGridItemWorkshopOrbit
{
	static constexpr double NearClipCm = 0.1;
	static constexpr double SurfaceClearanceCm = 0.5;
	static constexpr double DefaultFovDegrees = 90.0;
	static constexpr double DetailFovDegrees = 2.0;

	FVector Target = FVector::ZeroVector;
	FVector BoxExtent = FVector::OneVector;
	double Radius = 1.0;
	double RequestedDistance = 35.0;
	FVector2D PanOffset = FVector2D::ZeroVector;
	FQuat Orientation = FRotator(-12.0, 35.0, 0.0).Quaternion();
	bool bReady = false;

	// Frontmost plane of the actual world-space AABB, projected onto the
	// viewing direction. A long sword no longer imposes its length as
	// the minimum viewing distance when inspected broadside.
	double MinDistance() const
	{
		const FVector Forward = Orientation.GetAxisX().GetAbs();
		return FVector::DotProduct(Forward, BoxExtent) + SurfaceClearanceCm;
	}
	double MaxDistance() const { return FMath::Max(MinDistance() * 2.0, Radius * 15.0); }

	double MinRequestedDistance() const
	{
		return MinDistance() * FMath::Tan(FMath::DegreesToRadians(DetailFovDegrees * 0.5)) /
			FMath::Tan(FMath::DegreesToRadians(DefaultFovDegrees * 0.5));
	}

	void UpdateDistance()
	{
		RequestedDistance = FMath::Clamp(RequestedDistance, MinRequestedDistance(), MaxDistance());
	}

	double GetDistance() const { return FMath::Max(RequestedDistance, MinDistance()); }

	/** At the physical front plane, continue into optical detail zoom.
	 * No object scaling and no camera penetration. */
	double ViewFov() const
	{
		const double Ratio = FMath::Min(1.0, RequestedDistance / GetDistance());
		return FMath::Clamp(FMath::RadiansToDegrees(2.0 * FMath::Atan(
			FMath::Tan(FMath::DegreesToRadians(DefaultFovDegrees * 0.5)) * Ratio)),
			DetailFovDegrees, DefaultFovDegrees);
	}

	void SetBounds(const FBoxSphereBounds& Bounds, bool bRecenter, const FRotator& InitialRotation = FRotator(-12.0, 35.0, 0.0))
	{
		Target = Bounds.Origin;
		BoxExtent = Bounds.BoxExtent.GetAbs();
		Radius = FMath::Max(1.0, Bounds.SphereRadius);
		if (bRecenter || !bReady)
		{
			Orientation = InitialRotation.Quaternion().GetNormalized();
			RequestedDistance = FMath::Max(Radius * 3.5, MinDistance() * 1.25);
			PanOffset = FVector2D::ZeroVector;
		}
		UpdateDistance();
		bReady = true;
	}

	void Drag(const FVector2D& Delta)
	{
		if (bReady)
		{
			// Keep the accepted quaternion navigation and mouse directions.
			const FQuat Horizontal(FVector::UpVector, FMath::DegreesToRadians(Delta.X * 0.35));
			const FQuat Vertical(FVector::RightVector, FMath::DegreesToRadians(Delta.Y * 0.35));
			Orientation = (Orientation * Horizontal * Vertical).GetNormalized();
			UpdateDistance();
		}
	}

	void Zoom(double WheelDelta)
	{
		if (bReady)
		{
			RequestedDistance *= FMath::Pow(0.85, FMath::Clamp(WheelDelta, -20.0, 20.0));
			UpdateDistance();
		}
	}

	/** Convert a screen drag to world centimetres at the pivot plane.
	 * The sensitivity decreases naturally with the magnification. */
	void Pan(const FVector2D& DeltaPixels, double ProjectionAxisPixels)
	{
		if (bReady && ProjectionAxisPixels > 0.0)
		{
			const double CmPerPixel = 2.0 * GetDistance() *
				FMath::Tan(FMath::DegreesToRadians(ViewFov() * 0.5)) / ProjectionAxisPixels;
			PanOffset += DeltaPixels * CmPerPixel;
		}
	}

	// Slate Y is down. Moving the camera left/up moves the image right/down.
	FVector AimLocation() const
	{
		return Target - Orientation.GetAxisY() * PanOffset.X + Orientation.GetAxisZ() * PanOffset.Y;
	}
	FRotator ViewRotation() const { return Orientation.Rotator(); }
	FVector CameraLocation() const { return AimLocation() - Orientation.GetAxisX() * GetDistance(); }
	void Clear()
	{
		bReady = false;
		PanOffset = FVector2D::ZeroVector;
	}
};
