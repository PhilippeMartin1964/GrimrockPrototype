#include "RPG/RPGRecipeCatalogAsset.h"

namespace RPGRecipeCatalogPrivate
{
	const TCHAR* PackagePath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/Crafting/DA_RPGRecipeCatalog");
	const TCHAR* AssetName = TEXT("DA_RPGRecipeCatalog");
	const TCHAR* ObjectPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/Crafting/DA_RPGRecipeCatalog.DA_RPGRecipeCatalog");
}

bool FRPGRecipeCatalogEntry::IsValidDefinition() const
{
	return !RecipeId.IsNone() &&
		RecipeId.ToString().StartsWith(TEXT("Recipe_"), ESearchCase::CaseSensitive) &&
		!DisplayName.IsEmpty();
}

bool URPGRecipeCatalogAsset::IsValidDefinition() const
{
	if (Recipes.IsEmpty())
	{
		return false;
	}

	TSet<FName> SeenIds;
	for (const FRPGRecipeCatalogEntry& Recipe : Recipes)
	{
		if (!Recipe.IsValidDefinition() || SeenIds.Contains(Recipe.RecipeId))
		{
			return false;
		}
		SeenIds.Add(Recipe.RecipeId);
	}
	return true;
}

FText URPGRecipeCatalogAsset::ResolveDisplayName(FName RecipeId) const
{
	if (RecipeId.IsNone())
	{
		return FText::GetEmpty();
	}

	const FRPGRecipeCatalogEntry* Found = Recipes.FindByPredicate(
		[RecipeId](const FRPGRecipeCatalogEntry& Recipe)
		{
			return Recipe.RecipeId == RecipeId;
		});
	return Found ? Found->DisplayName : FText::GetEmpty();
}

const TCHAR* URPGRecipeCatalogAsset::ProductionPackagePath()
{
	return RPGRecipeCatalogPrivate::PackagePath;
}

const TCHAR* URPGRecipeCatalogAsset::ProductionAssetName()
{
	return RPGRecipeCatalogPrivate::AssetName;
}

const TCHAR* URPGRecipeCatalogAsset::ProductionObjectPath()
{
	return RPGRecipeCatalogPrivate::ObjectPath;
}

const URPGRecipeCatalogAsset* URPGRecipeCatalogAsset::LoadProduction()
{
	return LoadObject<URPGRecipeCatalogAsset>(nullptr, ProductionObjectPath());
}

FText URPGRecipeCatalogAsset::ResolveProductionDisplayName(FName RecipeId)
{
	const URPGRecipeCatalogAsset* Catalog = LoadProduction();
	return Catalog ? Catalog->ResolveDisplayName(RecipeId) : FText::GetEmpty();
}
