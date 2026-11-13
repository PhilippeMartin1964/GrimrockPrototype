#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"

bool UGridCanonicalItemDefinitionAsset::HasValidIdentity() const
{
	if (ItemId.IsNone() || !FMath::IsFinite(WeightKg) || WeightKg < 0.0f)
	{
		return false;
	}
	TSet<FName> Seen;
	for (FName Tag : Tags)
	{
		if (Tag.IsNone() || Seen.Contains(Tag))
		{
			return false;
		}
		Seen.Add(Tag);
	}
	return true;
}
