#pragma once

#include "CoreMinimal.h"

class URPGClassAsset;
class UGridStatusEffectDefinitionAsset;
class UGridMonsterCategoryAsset;

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
	/** Rebuilds one production monster category identity while preserving serialized DisplayName for the same CategoryId. */
	static bool ConfigureMonsterCategory(UGridMonsterCategoryAsset& CategoryAsset, FName CategoryId);
	static bool CollectProductionFavoredEnemyCategories(TArray<FRPGRangerFavoredEnemyCategoryDefinition>& OutCategories, FString& OutError);
	/** Updates ONLY serialized Favored Enemy titles in DA_Class_Ranger, using the production bestiary as authority. */
	static bool SyncProductionFavoredEnemyTitles(FString& OutError);
	static bool AuthorProductionAssets(FString& OutError);
};
