#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGRecipeCatalogAsset.h"
#include "UI/RPGTalentDetailTextSet.h"

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

	const URPGTalentDetailTextSet* LegacyTextSet = URPGTalentDetailTextSet::LoadProduction();
	if (!TestNotNull(TEXT("Legacy Talent TextSet remains available for D1 parity audit"), LegacyTextSet))
	{
		return false;
	}

	const FString LegacyPrefix(TEXT("Talent.Recipe."));
	int32 LegacyRecipeCount = 0;
	for (const FRPGTalentDetailTextEntry& LegacyEntry : LegacyTextSet->Entries)
	{
		const FString LegacyTextId = LegacyEntry.TextId.ToString();
		if (!LegacyTextId.StartsWith(LegacyPrefix, ESearchCase::CaseSensitive))
		{
			continue;
		}

		++LegacyRecipeCount;
		const FName RecipeId(*LegacyTextId.RightChop(LegacyPrefix.Len()));
		const FText MigratedName = Catalog->ResolveDisplayName(RecipeId);
		TestFalse(
			*FString::Printf(TEXT("%s exists in Recipe catalog"), *RecipeId.ToString()),
			MigratedName.IsEmpty());
		TestTrue(
			*FString::Printf(TEXT("%s preserves the player-facing name"), *RecipeId.ToString()),
			MigratedName.EqualTo(LegacyEntry.Text));
	}

	TestEqual(TEXT("Parity audit covers exactly fifteen legacy Recipe names"),
		LegacyRecipeCount, 15);
	return true;
}

#endif
