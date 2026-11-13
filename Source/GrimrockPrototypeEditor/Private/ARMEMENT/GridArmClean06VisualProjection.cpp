#include "ARMEMENT/GridArmClean06VisualProjection.h"

bool GridArmClean06VisualProjection::TryProject(
	const FGridWeaponVisualProfile& Source, FGridCanonicalWeaponVisual& Out)
{
	Out = FGridCanonicalWeaponVisual();
	if (!Source.IsValidProfile())
	{
		return false;
	}
	Out.VisualId = Source.VisualId;
	Out.FamilyTag = Source.FamilyTag;
	Out.ItemIcon = Source.BaseIcon;
	Out.WorldMesh = Source.WorldMesh;
	Out.EquippedMesh = Source.EquippedMesh; // preserve null, resolve fallback only at read
	Out.WorldTransform = Source.WorldVisualTransform;
	Out.OrnamentMesh = Source.OrnamentMesh;
	Out.OrnamentTransform = Source.OrnamentRelativeTransform;
	return EqualsSource(Source, Out);
}

bool GridArmClean06VisualProjection::EqualsSource(
	const FGridWeaponVisualProfile& Source, const FGridCanonicalWeaponVisual& Projection)
{
	return Source.VisualId == Projection.VisualId &&
		Source.FamilyTag == Projection.FamilyTag &&
		Source.BaseIcon.ToSoftObjectPath() == Projection.ItemIcon.ToSoftObjectPath() &&
		Source.WorldMesh.ToSoftObjectPath() == Projection.WorldMesh.ToSoftObjectPath() &&
		Source.EquippedMesh.ToSoftObjectPath() == Projection.EquippedMesh.ToSoftObjectPath() &&
		Source.OrnamentMesh.ToSoftObjectPath() == Projection.OrnamentMesh.ToSoftObjectPath() &&
		Source.WorldVisualTransform.Equals(Projection.WorldTransform) &&
		Source.OrnamentRelativeTransform.Equals(Projection.OrnamentTransform);
}
