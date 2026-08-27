#include "RPG/RPGClassVisualAsset.h"

FPrimaryAssetId URPGClassVisualAsset::GetPrimaryAssetId() const
{
	if (ClassId.IsNone())
	{
		return Super::GetPrimaryAssetId();
	}
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("RPGClassVisual")), ClassId);
}

bool URPGClassVisualAsset::IsValidDefinition() const
{
	return !ClassId.IsNone() && !ClassIcon.IsNull();
}

bool URPGClassVisualAsset::IsValidForClass(FName InClassId) const
{
	return IsValidDefinition() && !InClassId.IsNone() && ClassId == InClassId;
}
