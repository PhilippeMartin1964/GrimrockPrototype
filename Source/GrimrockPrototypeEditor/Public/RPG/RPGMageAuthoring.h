#pragma once

#include "CoreMinimal.h"

class URPGClassAsset;
class UGridStatusEffectDefinitionAsset;

/** RPG03.9.4C editor-only authoring for Mage Elemental Affinity and Elemental Overload. */
struct GRIMROCKPROTOTYPEEDITOR_API FRPGMageAuthoring
{
	static const TCHAR* MageAssetPath();
	static const TCHAR* ElementalOverloadStatusPath();

	static void ConfigureClass(URPGClassAsset& ClassAsset);
	static bool ConfigureElementalOverloadStatus(UGridStatusEffectDefinitionAsset& StatusAsset);
	static bool AuthorProductionAssets(FString& OutError);
};
