#pragma once

#include "CoreMinimal.h"

class URPGClassAsset;
class UGridStatusEffectDefinitionAsset;

/** RPG03.9.3 editor-only authoring for Ranger talents and their production status asset. */
struct GRIMROCKPROTOTYPEEDITOR_API FRPGRangerAuthoring
{
	static const TCHAR* RangerAssetPath();
	static const TCHAR* MarkedStatusPath();

	static void ConfigureClass(URPGClassAsset& ClassAsset, const TArray<FName>& FavoredEnemyCategoryIds);
	static bool ConfigureMarkedStatus(UGridStatusEffectDefinitionAsset& StatusAsset);
	static bool CollectProductionFavoredEnemyCategories(TArray<FName>& OutCategoryIds, FString& OutError);
	static bool AuthorProductionAssets(FString& OutError);
};
