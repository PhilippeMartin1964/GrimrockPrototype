#include "ARMEMENT/GridArmClean06ActionIconsProjection.h"

bool GridArmClean06ActionIconsProjection::IsIdentical(
	const UGridEquippedWeaponActionIconPalette& Source,
	const UGridCanonicalWeaponVisualCatalogueAsset& Destination)
{
	if (Source.Icons.Num() != Destination.ActionIcons.Num())
	{
		return false;
	}
	for (const TPair<EGridEquippedWeaponActionIconKind, TSoftObjectPtr<UTexture2D>>& Pair : Source.Icons)
	{
		const TSoftObjectPtr<UTexture2D>* Found = Destination.ActionIcons.Find(Pair.Key);
		if (!Found || Found->ToSoftObjectPath() != Pair.Value.ToSoftObjectPath())
		{
			return false;
		}
	}
	return true;
}

bool GridArmClean06ActionIconsProjection::TryProject(
	const UGridEquippedWeaponActionIconPalette& Source,
	UGridCanonicalWeaponVisualCatalogueAsset& Destination)
{
	// No overwrites, no phantom "None" action, no empty migration presented as success.
	if (Source.Icons.IsEmpty() ||
		Source.Icons.Contains(EGridEquippedWeaponActionIconKind::None) ||
		!Destination.ActionIcons.IsEmpty())
	{
		return false;
	}
	Destination.ActionIcons = Source.Icons;
	if (!IsIdentical(Source, Destination))
	{
		Destination.ActionIcons.Reset();
		return false;
	}
	return true;
}
