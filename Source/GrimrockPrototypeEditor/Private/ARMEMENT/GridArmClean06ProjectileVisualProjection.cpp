#include "ARMEMENT/GridArmClean06ProjectileVisualProjection.h"

namespace
{
	bool TranslateMode(EGridThrowVisualMode Source, EGridCanonicalProjectileVisualMode& Out)
	{
		switch (Source)
		{
		case EGridThrowVisualMode::Stable:
			Out = EGridCanonicalProjectileVisualMode::Stable;
			return true;
		case EGridThrowVisualMode::Tumble:
			Out = EGridCanonicalProjectileVisualMode::Tumble;
			return true;
		case EGridThrowVisualMode::Spin:
			Out = EGridCanonicalProjectileVisualMode::Spin;
			return true;
		default:
			return false;
		}
	}
}

bool GridArmClean06ProjectileVisualProjection::Matches(
	const UGridItemDefinitionAsset& Source,
	const FGridCanonicalWeaponVisual& Destination)
{
	if (Source.bCombatThrowWeapon != Destination.ProjectileAppearance.bEnabled)
	{
		return false;
	}
	if (!Source.bCombatThrowWeapon)
	{
		return Destination.ProjectileMesh.IsNull();
	}
	EGridCanonicalProjectileVisualMode ExpectedMode;
	if (!TranslateMode(Source.ThrowVisualMode, ExpectedMode))
	{
		return false;
	}
	const FGridCanonicalProjectileVisual& Visual = Destination.ProjectileAppearance;
	return Source.WorldMesh.ToSoftObjectPath() ==
			Destination.ProjectileMesh.ToSoftObjectPath() &&
		Source.ThrowVisualRelativeRotation.Equals(Visual.RelativeRotation) &&
		Source.ThrowVisualRelativeScale.Equals(Visual.RelativeScale) &&
		Source.ProjectileMeshForwardAxis == Visual.MeshForwardAxis &&
		ExpectedMode == Visual.Mode &&
		Source.ThrowVisualTumbleAxis.Equals(Visual.TumbleAxis) &&
		Source.ThrowVisualTumbleDegreesPerSecond == Visual.TumbleDegreesPerSecond &&
		Source.ThrowVisualSpinDegreesPerSecond == Visual.SpinDegreesPerSecond;
}

bool GridArmClean06ProjectileVisualProjection::TryProject(
	const FGridWeaponPipelineRow& Row,
	const UGridItemDefinitionAsset& Source,
	FGridCanonicalWeaponVisual& Destination)
{
	if (Row.ProductionState != TEXT("Active") ||
		Source.ItemDefinitionId != FName(*Row.ItemDefinitionId) ||
		Source.WeaponArchetypeId != FName(*Row.ArchetypeId) ||
		Destination.VisualId != FName(*Row.VisualId) ||
		Row.IsThrowable() != Source.bCombatThrowWeapon ||
		Destination.ProjectileAppearance.bEnabled ||
		!Destination.ProjectileMesh.IsNull())
	{
		return false;
	}
	if (!Source.bCombatThrowWeapon)
	{
		return Matches(Source, Destination);
	}
	EGridCanonicalProjectileVisualMode MappedMode;
	if (!TranslateMode(Source.ThrowVisualMode, MappedMode) ||
		Source.WorldMesh.IsNull() ||
		Source.ThrowVisualRelativeRotation.ContainsNaN() ||
		Source.ThrowVisualRelativeScale.ContainsNaN() ||
		Source.ThrowVisualTumbleAxis.ContainsNaN() ||
		!FMath::IsFinite(Source.ThrowVisualTumbleDegreesPerSecond) ||
		!FMath::IsFinite(Source.ThrowVisualSpinDegreesPerSecond))
	{
		return false;
	}

	// Every physical projectile shape and visual detail comes from the real
	// DataAsset. Different world/projectile meshes remain distinct roles.
	FGridCanonicalProjectileVisual Preview;
	Preview.bEnabled = true;
	Preview.RelativeRotation = Source.ThrowVisualRelativeRotation;
	Preview.RelativeScale = Source.ThrowVisualRelativeScale;
	Preview.MeshForwardAxis = Source.ProjectileMeshForwardAxis;
	Preview.Mode = MappedMode;
	Preview.TumbleAxis = Source.ThrowVisualTumbleAxis;
	Preview.TumbleDegreesPerSecond = Source.ThrowVisualTumbleDegreesPerSecond;
	Preview.SpinDegreesPerSecond = Source.ThrowVisualSpinDegreesPerSecond;

	Destination.ProjectileMesh = Source.WorldMesh;
	Destination.ProjectileAppearance = Preview;
	return Matches(Source, Destination);
}
