#pragma once

#include "CoreMinimal.h"

class URPGClassAsset;

/**
 * Shared Editor-only authoring for the canonical RPG class progression economy.
 *
 * RPG_Class_Progression_1_20_v0_1 is authoritative for Talent Points:
 * +1 at levels 2,4,6,8,10,12,14,16,18,20 (10 total).
 */
struct GRIMROCKPROTOTYPEEDITOR_API FRPGClassProgressionAuthoring
{
	static void ConfigureCanonicalTalentGrants(URPGClassAsset& ClassAsset);
	static bool HasCanonicalTalentGrants(const URPGClassAsset& ClassAsset);
};
