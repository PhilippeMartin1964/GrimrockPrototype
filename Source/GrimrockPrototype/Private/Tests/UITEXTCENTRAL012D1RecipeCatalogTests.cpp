#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "RPG/RPGRecipeCatalogAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUITEXTCENTRAL012D1RecipeCatalogModelTest,
	"Game.UI.TextCentral.CENTRAL01.RecipeCatalogModel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUITEXTCENTRAL012D1RecipeCatalogModelTest::RunTest(const FString&)
{
	URPGRecipeCatalogAsset* Catalog = NewObject<URPGRecipeCatalogAsset>();

	FRPGRecipeCatalogEntry Fire;
	Fire.RecipeId = TEXT("Recipe_Bomb_Fire");
	Fire.DisplayName = FText::FromString(TEXT("Bombe incendiaire"));
	Catalog->Recipes.Add(Fire);

	FRPGRecipeCatalogEntry Antidote;
	Antidote.RecipeId = TEXT("Recipe_Antidote");
	Antidote.DisplayName = FText::FromString(TEXT("Antidote"));
	Catalog->Recipes.Add(Antidote);

	TestTrue(TEXT("Recipe catalog accepts unique Recipe_* identities"), Catalog->IsValidDefinition());
	TestTrue(TEXT("Recipe catalog resolves authored display names"),
		Catalog->ResolveDisplayName(TEXT("Recipe_Bomb_Fire")).EqualTo(Fire.DisplayName));
	TestTrue(TEXT("Unknown recipe resolves empty"),
		Catalog->ResolveDisplayName(TEXT("Recipe_Unknown")).IsEmpty());

	URPGRecipeCatalogAsset* DuplicateCatalog = NewObject<URPGRecipeCatalogAsset>();
	DuplicateCatalog->Recipes = { Fire, Fire };
	TestFalse(TEXT("Recipe catalog rejects duplicate gameplay identities"),
		DuplicateCatalog->IsValidDefinition());

	FRPGRecipeCatalogEntry Invalid;
	Invalid.RecipeId = TEXT("Bomb_Fire");
	Invalid.DisplayName = Fire.DisplayName;
	URPGRecipeCatalogAsset* InvalidCatalog = NewObject<URPGRecipeCatalogAsset>();
	InvalidCatalog->Recipes = { Invalid };
	TestFalse(TEXT("Recipe catalog rejects identities outside Recipe_* namespace"),
		InvalidCatalog->IsValidDefinition());

	return true;
}

#endif
