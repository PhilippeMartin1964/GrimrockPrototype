#include "RPG/RPGRecipeCatalogAuthoringCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "RPG/RPGRecipeCatalogAsset.h"
#include "UI/RPGTalentDetailTextSet.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace RPGRecipeCatalogAuthoringPrivate
{
	constexpr int32 ExpectedProductionRecipeCount = 15;
	const TCHAR* LegacyTalentTextSetPath =
		TEXT("/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_RPGTalentDetailTextSet.DA_RPGTalentDetailTextSet");
	const FString LegacyRecipePrefix(TEXT("Talent.Recipe."));

	bool BuildRecipeEntries(
		const URPGTalentDetailTextSet& LegacyTextSet,
		TArray<FRPGRecipeCatalogEntry>& OutRecipes,
		FString& OutError)
	{
		OutRecipes.Reset();

		for (const FRPGTalentDetailTextEntry& LegacyEntry : LegacyTextSet.Entries)
		{
			const FString LegacyTextId = LegacyEntry.TextId.ToString();
			if (!LegacyTextId.StartsWith(LegacyRecipePrefix, ESearchCase::CaseSensitive))
			{
				continue;
			}

			FRPGRecipeCatalogEntry Recipe;
			Recipe.RecipeId = FName(*LegacyTextId.RightChop(LegacyRecipePrefix.Len()));
			Recipe.DisplayName = LegacyEntry.Text;
			if (!Recipe.IsValidDefinition())
			{
				OutError = FString::Printf(
					TEXT("Invalid recipe migrated from legacy text id '%s'."),
					*LegacyTextId);
				return false;
			}
			OutRecipes.Add(MoveTemp(Recipe));
		}

		OutRecipes.Sort(
			[](const FRPGRecipeCatalogEntry& A, const FRPGRecipeCatalogEntry& B)
			{
				return A.RecipeId.ToString() < B.RecipeId.ToString();
			});

		if (OutRecipes.Num() != ExpectedProductionRecipeCount)
		{
			OutError = FString::Printf(
				TEXT("Expected %d legacy recipe names, found %d."),
				ExpectedProductionRecipeCount,
				OutRecipes.Num());
			return false;
		}

		TSet<FName> SeenIds;
		for (const FRPGRecipeCatalogEntry& Recipe : OutRecipes)
		{
			if (SeenIds.Contains(Recipe.RecipeId))
			{
				OutError = FString::Printf(
					TEXT("Duplicate migrated recipe id '%s'."),
					*Recipe.RecipeId.ToString());
				return false;
			}
			SeenIds.Add(Recipe.RecipeId);
		}
		return true;
	}

	bool AreEquivalent(
		const TArray<FRPGRecipeCatalogEntry>& Existing,
		const TArray<FRPGRecipeCatalogEntry>& Desired)
	{
		if (Existing.Num() != Desired.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Existing.Num(); ++Index)
		{
			if (Existing[Index].RecipeId != Desired[Index].RecipeId ||
				!Existing[Index].DisplayName.EqualTo(Desired[Index].DisplayName))
			{
				return false;
			}
		}
		return true;
	}
}

URPGRecipeCatalogAuthoringCommandlet::URPGRecipeCatalogAuthoringCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 URPGRecipeCatalogAuthoringCommandlet::Main(const FString& Params)
{
	(void)Params;
	using namespace RPGRecipeCatalogAuthoringPrivate;

	const URPGTalentDetailTextSet* LegacyTextSet =
		LoadObject<URPGTalentDetailTextSet>(nullptr, LegacyTalentTextSetPath);
	if (!LegacyTextSet || !LegacyTextSet->IsValidDefinition())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[UI-TEXT-CENTRAL01.2D1] Unable to load a valid legacy Talent TextSet."));
		return 1;
	}

	TArray<FRPGRecipeCatalogEntry> DesiredRecipes;
	FString Error;
	if (!BuildRecipeEntries(*LegacyTextSet, DesiredRecipes, Error))
	{
		UE_LOG(LogTemp, Error, TEXT("[UI-TEXT-CENTRAL01.2D1] %s"), *Error);
		return 2;
	}

	URPGRecipeCatalogAsset* Catalog =
		LoadObject<URPGRecipeCatalogAsset>(nullptr, URPGRecipeCatalogAsset::ProductionObjectPath());
	UPackage* Package = Catalog ? Catalog->GetOutermost() : nullptr;
	bool bCreated = false;

	if (!Catalog)
	{
		Package = CreatePackage(URPGRecipeCatalogAsset::ProductionPackagePath());
		if (!Package)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[UI-TEXT-CENTRAL01.2D1] Failed to create Recipe catalog package."));
			return 3;
		}

		Catalog = NewObject<URPGRecipeCatalogAsset>(
			Package,
			FName(URPGRecipeCatalogAsset::ProductionAssetName()),
			RF_Public | RF_Standalone);
		if (!Catalog)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[UI-TEXT-CENTRAL01.2D1] Failed to create DA_RPGRecipeCatalog."));
			return 3;
		}
		FAssetRegistryModule::AssetCreated(Catalog);
		bCreated = true;
	}

	const bool bChanged = bCreated || !AreEquivalent(Catalog->Recipes, DesiredRecipes);
	if (bChanged)
	{
		Catalog->Modify();
		Catalog->Recipes = DesiredRecipes;
	}

	if (!Catalog->IsValidDefinition() ||
		Catalog->Recipes.Num() != ExpectedProductionRecipeCount)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[UI-TEXT-CENTRAL01.2D1] Materialized Recipe catalog is invalid."));
		return 4;
	}

	if (bChanged)
	{
		Package->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			URPGRecipeCatalogAsset::ProductionPackagePath(),
			FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		if (!UPackage::SavePackage(Package, Catalog, *Filename, SaveArgs))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[UI-TEXT-CENTRAL01.2D1] Failed to save '%s'."), *Filename);
			return 5;
		}
	}

	UE_LOG(LogTemp, Display,
		TEXT("[UI-TEXT-CENTRAL01.2D1] Recipe catalog materialization succeeded."));
	UE_LOG(LogTemp, Display,
		TEXT("[UI-TEXT-CENTRAL01.2D1] Recipes=%d Changed=%s Source=legacy Talent TextSet Target=%s"),
		Catalog->Recipes.Num(),
		bChanged ? TEXT("true") : TEXT("false"),
		URPGRecipeCatalogAsset::ProductionObjectPath());
	return 0;
}
