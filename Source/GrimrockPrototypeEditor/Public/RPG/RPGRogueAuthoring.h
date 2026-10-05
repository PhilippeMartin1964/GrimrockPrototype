#pragma once

#include "CoreMinimal.h"

class URPGClassAsset;
class UGridStatusEffectDefinitionAsset;

/** RPG03.9.2 editor-only authoring for the 15 Rogue talents and their status assets. */
struct GRIMROCKPROTOTYPEEDITOR_API FRPGRogueAuthoring
{
	static const TCHAR* RogueAssetPath();
	static void GetRequiredStatusIds(TArray<FName>& OutStatusIds);
	static FString GetStatusObjectPath(FName EffectId);

	static void ConfigureClass(URPGClassAsset& ClassAsset);
	static bool ConfigureStatus(UGridStatusEffectDefinitionAsset& StatusAsset, FName EffectId);
	static bool AuthorProductionAssets(FString& OutError);
};
