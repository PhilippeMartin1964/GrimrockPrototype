#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RPG/RPGRecipeCatalogAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUITEXTCENTRAL012D1RecipeCatalogProductionTest,
	"Game.UI.TextCentral.CENTRAL01.RecipeCatalogProduction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUITEXTCENTRAL012D1RecipeCatalogProductionTest::RunTest(const FString&)
{
	const URPGRecipeCatalogAsset* Catalog = URPGRecipeCatalogAsset::LoadProduction();
	if (!TestNotNull(TEXT("Production Recipe catalog loads"), Catalog))
	{
		return false;
	}

	TestTrue(TEXT("Production Recipe catalog validates"), Catalog->IsValidDefinition());
	TestEqual(TEXT("Production Recipe catalog contains the fifteen Alchemist recipes"),
		Catalog->Recipes.Num(), 15);

	const TArray<FName> ExpectedRecipeIds = {
		TEXT("Recipe_Antidote"),
		TEXT("Recipe_Bomb_Fire"),
		TEXT("Recipe_Bomb_Toxic"),
		TEXT("Recipe_DefensiveElixir_Fire"),
		TEXT("Recipe_DefensiveElixir_Ice"),
		TEXT("Recipe_DefensiveElixir_Lightning"),
		TEXT("Recipe_DefensiveElixir_Poison"),
		TEXT("Recipe_Flask_Acid"),
		TEXT("Recipe_Flask_CorrosiveCloud"),
		TEXT("Recipe_Flask_Oil"),
		TEXT("Recipe_MajorTransmutation_Fire"),
		TEXT("Recipe_MajorTransmutation_Ice"),
		TEXT("Recipe_MajorTransmutation_Oil"),
		TEXT("Recipe_MajorTransmutation_Poison"),
		TEXT("Recipe_Panacea")
	};

	for (const FName RecipeId : ExpectedRecipeIds)
	{
		const FText DisplayName = Catalog->ResolveDisplayName(RecipeId);
		TestFalse(
			*FString::Printf(TEXT("%s exists in the production Recipe catalog"), *RecipeId.ToString()),
			DisplayName.IsEmpty());
		if (!DisplayName.IsEmpty())
		{
			TestFalse(
				*FString::Printf(TEXT("%s never leaks its technical identity"), *RecipeId.ToString()),
				DisplayName.ToString().Contains(TEXT("Recipe_"), ESearchCase::CaseSensitive));
		}
	}
	return true;
}

#endif
