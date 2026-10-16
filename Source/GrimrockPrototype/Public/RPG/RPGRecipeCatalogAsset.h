#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RPGRecipeCatalogAsset.generated.h"

/** Player-facing metadata for one stable gameplay Recipe_* identity. */
USTRUCT(BlueprintType)
struct GRIMROCKPROTOTYPE_API FRPGRecipeCatalogEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Crafting|Recipe")
	FName RecipeId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Crafting|Recipe")
	FText DisplayName;

	bool IsValidDefinition() const;
};

/**
 * Single production authority for player-facing recipe names.
 *
 * RecipeId remains the gameplay identity. DisplayName is authored content and
 * deliberately lives outside ST_UI because it names domain content, not UI chrome.
 */
UCLASS(BlueprintType)
class GRIMROCKPROTOTYPE_API URPGRecipeCatalogAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RPG|Crafting|Recipe", meta = (TitleProperty = "RecipeId"))
	TArray<FRPGRecipeCatalogEntry> Recipes;

	UFUNCTION(BlueprintPure, Category = "RPG|Crafting|Recipe")
	bool IsValidDefinition() const;

	UFUNCTION(BlueprintPure, Category = "RPG|Crafting|Recipe")
	FText ResolveDisplayName(FName RecipeId) const;

	static const TCHAR* ProductionPackagePath();
	static const TCHAR* ProductionAssetName();
	static const TCHAR* ProductionObjectPath();
	static const URPGRecipeCatalogAsset* LoadProduction();
	static FText ResolveProductionDisplayName(FName RecipeId);
};
