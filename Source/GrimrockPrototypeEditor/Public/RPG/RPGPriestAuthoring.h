#pragma once

#include "CoreMinimal.h"

class URPGClassAsset;
class UGridStatusEffectDefinitionAsset;

/** RPG03.9.5 editor-only production authoring for the Priest class. */
struct GRIMROCKPROTOTYPEEDITOR_API FRPGPriestAuthoring
{
	static const TCHAR* PriestAssetPath();
	static FString GetStatusObjectPath(FName EffectId);

	/** Authors the complete Restoration, Protection and Exorcism talent tree. */
	static void ConfigureClass(URPGClassAsset& ClassAsset);

	/** Configures Priest-owned production statuses. */
	static bool ConfigureStatus(UGridStatusEffectDefinitionAsset& StatusAsset, FName EffectId);

	/** Complete set of Priest-owned statuses materialized by RPG03.9.5C. */
	static void GetRequiredPriestStatusIds(TArray<FName>& OutStatusIds);

	/** Materializes DA_Class_Priest and every Priest-owned status through Unreal Editor. */
	static bool AuthorProductionAssets(FString& OutError);
};
