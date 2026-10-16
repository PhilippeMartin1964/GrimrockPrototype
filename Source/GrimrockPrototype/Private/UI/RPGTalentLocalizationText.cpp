#include "UI/RPGTalentLocalizationText.h"

#include "UI/RPGTalentDetailTextSet.h"
#include "UI/UILocalizationText.h"

bool FRPGTalentLocalizationText::IsDeferredRecipeTextId(FName TextId)
{
	return !TextId.IsNone() &&
		TextId.ToString().StartsWith(TEXT("Talent.Recipe."), ESearchCase::CaseSensitive);
}

FText FRPGTalentLocalizationText::Get(FName TextId)
{
	if (TextId.IsNone())
	{
		return FText::GetEmpty();
	}
	if (IsDeferredRecipeTextId(TextId))
	{
		return URPGTalentDetailTextSet::ResolveProductionText(TextId);
	}
	const FString Key = TextId.ToString();
	return FUILocalizationText::Get(*Key);
}
