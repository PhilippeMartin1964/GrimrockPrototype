#pragma once

#include "CoreMinimal.h"

/**
 * Transitional Talent presentation resolver.
 *
 * UI vocabulary resolves from the central ST_UI String Table.
 * Talent.Recipe.* remains on the legacy Talent TextSet until Crafting owns
 * recipe display names.
 */
struct GRIMROCKPROTOTYPE_API FRPGTalentLocalizationText
{
	static bool IsDeferredRecipeTextId(FName TextId);
	static FText Get(FName TextId);
};
