#pragma once

#include "CoreMinimal.h"

/**
 * Transitional Talent presentation resolver.
 *
 * Static UI vocabulary resolves from ST_UI. Historical Talent.Recipe.* keys are
 * accepted only as a compatibility input and resolve their Recipe_* identity
 * through the production Recipe catalog. UI-TEXT-CENTRAL01.2D2 will remove this
 * compatibility layer once the Talent read-model carries RecipeId directly.
 */
struct GRIMROCKPROTOTYPE_API FRPGTalentLocalizationText
{
	static bool IsRecipeCompatibilityTextId(FName TextId);
	static FText Get(FName TextId);
};
