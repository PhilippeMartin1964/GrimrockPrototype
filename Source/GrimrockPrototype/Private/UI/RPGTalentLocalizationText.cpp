#include "UI/RPGTalentLocalizationText.h"

#include "RPG/RPGRecipeCatalogAsset.h"
#include "UI/UILocalizationText.h"

namespace RPGTalentLocalizationTextPrivate
{
	const FString RecipeCompatibilityPrefix(TEXT("Talent.Recipe."));

	FName RecipeIdFromCompatibilityTextId(FName TextId)
	{
		if (TextId.IsNone())
		{
			return NAME_None;
		}

		const FString Key = TextId.ToString();
		if (!Key.StartsWith(RecipeCompatibilityPrefix, ESearchCase::CaseSensitive))
		{
			return NAME_None;
		}

		const FString RecipeId = Key.RightChop(RecipeCompatibilityPrefix.Len());
		return RecipeId.StartsWith(TEXT("Recipe_"), ESearchCase::CaseSensitive)
			? FName(*RecipeId)
			: NAME_None;
	}
}

bool FRPGTalentLocalizationText::IsRecipeCompatibilityTextId(FName TextId)
{
	return !RPGTalentLocalizationTextPrivate::RecipeIdFromCompatibilityTextId(TextId).IsNone();
}

FText FRPGTalentLocalizationText::Get(FName TextId)
{
	if (TextId.IsNone())
	{
		return FText::GetEmpty();
	}

	const FName RecipeId =
		RPGTalentLocalizationTextPrivate::RecipeIdFromCompatibilityTextId(TextId);
	if (!RecipeId.IsNone())
	{
		return URPGRecipeCatalogAsset::ResolveProductionDisplayName(RecipeId);
	}

	const FString Key = TextId.ToString();
	return FUILocalizationText::Get(*Key);
}
