#pragma once

#include "CoreMinimal.h"
#include "RPG/StatusEffects/GridStatusEffectDefinitionAsset.h"

/**
 * Pure aggregation of boolean combat restrictions contributed by active
 * status effects. Effect ids are deliberately irrelevant: Stun, Silence and
 * Immobilize are data configurations of the same control profile.
 */
class GRIMROCKPROTOTYPE_API FGridStatusEffectControlResolver
{
public:
	static FGridStatusEffectControlProfile Resolve(const FGridStatusEffectCollection& StatusEffects);
};
