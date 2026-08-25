#include "RPG/StatusEffects/GridStatusEffectControlResolver.h"

FGridStatusEffectControlProfile FGridStatusEffectControlResolver::Resolve(const FGridStatusEffectCollection& StatusEffects)
{
	FGridStatusEffectControlProfile Result;
	for (const FGridStatusEffectRuntimeState& State : StatusEffects.ActiveEffects)
	{
		if (!State.IsValid() || !IsValid(State.DefinitionAsset))
		{
			continue;
		}
		Result.Merge(State.DefinitionAsset->Control);
	}
	return Result;
}
