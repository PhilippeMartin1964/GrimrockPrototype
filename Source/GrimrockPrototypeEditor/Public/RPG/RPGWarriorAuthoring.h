#pragma once

#include "CoreMinimal.h"

class URPGClassAsset;
class UGridStatusEffectDefinitionAsset;

/** Editor-only RPG03.9.1 authoring source for the production Warrior assets. */
struct FRPGWarriorAuthoring
{
	static const TCHAR* WarriorAssetPath();
	static void ConfigureClass(URPGClassAsset& ClassAsset);
	static bool ConfigureStatus(UGridStatusEffectDefinitionAsset& StatusAsset, FName EffectId);
	static bool AuthorProductionAssets(FString& OutError);
	static void GetRequiredStatusIds(TArray<FName>& OutStatusIds);
	static FString GetStatusObjectPath(FName EffectId);
};
