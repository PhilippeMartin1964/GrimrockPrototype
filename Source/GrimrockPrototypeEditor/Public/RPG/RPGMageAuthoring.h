#pragma once

#include "CoreMinimal.h"

class URPGClassAsset;
class UGridStatusEffectDefinitionAsset;

/** RPG03.9.4 editor-only production authoring for the Mage class. */
struct GRIMROCKPROTOTYPEEDITOR_API FRPGMageAuthoring
{
	static const TCHAR* MageAssetPath();
	static const TCHAR* ElementalOverloadStatusPath();
	static FString GetStatusObjectPath(FName EffectId);

	static void ConfigureClass(URPGClassAsset& ClassAsset);
	static bool ConfigureElementalOverloadStatus(UGridStatusEffectDefinitionAsset& StatusAsset);
	static bool ConfigureStatus(UGridStatusEffectDefinitionAsset& StatusAsset, FName EffectId);
	static void GetRequiredMageStatusIds(TArray<FName>& OutStatusIds);
	static bool AuthorProductionAssets(FString& OutError);
};
