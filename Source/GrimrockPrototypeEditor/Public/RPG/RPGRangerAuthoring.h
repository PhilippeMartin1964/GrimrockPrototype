#pragma once

#include "CoreMinimal.h"

class URPGClassAsset;
class UGridStatusEffectDefinitionAsset;

struct GRIMROCKPROTOTYPEEDITOR_API FRPGRangerFavoredEnemyCategoryDefinition
{
	FName CategoryId = NAME_None;
	FText DisplayName;

	FRPGRangerFavoredEnemyCategoryDefinition() = default;
	FRPGRangerFavoredEnemyCategoryDefinition(FName InCategoryId, const FText& InDisplayName)
		: CategoryId(InCategoryId), DisplayName(InDisplayName)
	{
	}

	bool IsValid() const
	{
		return !CategoryId.IsNone() && !DisplayName.IsEmpty();
	}
};

/** RPG03.9.3 editor-only authoring for Ranger talents and their production status asset. */
struct GRIMROCKPROTOTYPEEDITOR_API FRPGRangerAuthoring
{
	static const TCHAR* RangerAssetPath();
	static const TCHAR* MarkedStatusPath();

	static void ConfigureClass(URPGClassAsset& ClassAsset, const TArray<FRPGRangerFavoredEnemyCategoryDefinition>& FavoredEnemyCategories);
	static bool ConfigureMarkedStatus(UGridStatusEffectDefinitionAsset& StatusAsset);
	static bool CollectProductionFavoredEnemyCategories(TArray<FRPGRangerFavoredEnemyCategoryDefinition>& OutCategories, FString& OutError);
	static bool AuthorProductionAssets(FString& OutError);
};
