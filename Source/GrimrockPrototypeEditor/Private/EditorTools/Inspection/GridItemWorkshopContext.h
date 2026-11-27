#pragma once

#include "Runtime/Inspection/GridItemInspectionTypes.h"
#include "Runtime/Combat/GridProjectileGeometry.h"

enum class EGridWorkshopContext : uint8 { Examination, Floor, Decor, Held, Flight };
enum class EGridWorkshopLighting : uint8 { Studio, Torch, Dungeon };

/** Author-only static presentations of the canonical item. No simulated
 * possession, actions, physics or gameplay light state. */
struct FGridWorkshopContext
{
	static bool CanShow(const FGridItemInspectionView& View, EGridWorkshopContext Context)
	{
		if (Context == EGridWorkshopContext::Held)
		{
			return View.IsValid() && View.Item.LightEmitter.HasEmitter() && !View.Item.EquippedMesh.IsNull();
		}
		if (Context == EGridWorkshopContext::Flight)
		{
			return View.HasLevel() &&
				(View.HasProjectileAppearance() ||
					(!View.Item.ProjectileAppearance.bEnabled && View.Item.Weapon.Projectile.bGenericThrowAuthored && View.HasMeshReference()));
		}
		return View.HasMeshReference();
	}

	static FGridItemInspectionView Project(const FGridItemInspectionView& View, EGridWorkshopContext Context)
	{
		if (Context == EGridWorkshopContext::Examination || Context == EGridWorkshopContext::Floor || Context == EGridWorkshopContext::Decor)
		{
			return View;
		}
		if (!CanShow(View, Context))
		{
			return FGridItemInspectionView();
		}
		FGridItemInspectionView Result = View;
		// World-local ornament attachment has not been authored for held/flight meshes.
		Result.Item.OrnamentMesh.Reset();
		Result.Item.OrnamentTransform = FTransform::Identity;
		if (Context == EGridWorkshopContext::Held)
		{
			Result.Item.WorldMesh = View.Item.EquippedMesh;
		}
		else
		{
			const FGridProjectileVisual& Appearance = View.Item.ProjectileAppearance;
			Result.Item.WorldMesh = Appearance.bEnabled ? View.Item.ProjectileMesh : View.Item.WorldMesh;
			// Matches the physical thrown item's static launch pose. Stable
			// projectiles align their tip axis; spin/tumble use authored rotation.
			const FQuat Rotation = Appearance.Mode == EGridProjectileVisualMode::Stable
				? FGridProjectileGeometry::MeshToFlightRotation(Appearance.MeshForwardAxis)
				: Appearance.RelativeRotation.Quaternion();
			FVector Scale = Appearance.RelativeScale.ContainsNaN() ? FVector(1.5) : Appearance.RelativeScale.GetAbs();
			Scale.X = FMath::Clamp(Scale.X, 0.01, 100.0);
			Scale.Y = FMath::Clamp(Scale.Y, 0.01, 100.0);
			Scale.Z = FMath::Clamp(Scale.Z, 0.01, 100.0);
			Result.Item.WorldVisualTransform = FTransform(Rotation, FVector::ZeroVector, Scale);
		}
		return Result;
	}

	static bool HasFloor(EGridWorkshopContext Context)
	{
		return Context == EGridWorkshopContext::Floor || Context == EGridWorkshopContext::Decor;
	}
};
