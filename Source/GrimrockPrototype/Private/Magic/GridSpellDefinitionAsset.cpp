#include "Magic/GridSpellDefinitionAsset.h"

FPrimaryAssetId UGridSpellDefinitionAsset::GetPrimaryAssetId() const
{
	if (Definition.SpellId.IsNone())
	{
		return Super::GetPrimaryAssetId();
	}

	return FPrimaryAssetId(FPrimaryAssetType(TEXT("GridSpell")), Definition.SpellId);
}
