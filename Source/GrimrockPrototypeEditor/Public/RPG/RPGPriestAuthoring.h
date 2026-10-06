#pragma once

#include "CoreMinimal.h"

class URPGClassAsset;
class UGridStatusEffectDefinitionAsset;

/** RPG03.9.5 editor-only production authoring for the Priest class. */
struct GRIMROCKPROTOTYPEEDITOR_API FRPGPriestAuthoring
{
	static const TCHAR* PriestAssetPath();

	/** B1 authors Restoration + Protection. B2 extends the same class with Exorcism. */
	static void ConfigureClass(URPGClassAsset& ClassAsset);

	/** Configures Priest-owned production statuses. */
	static bool ConfigureStatus(UGridStatusEffectDefinitionAsset& StatusAsset, FName EffectId);
};
