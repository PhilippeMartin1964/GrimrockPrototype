#pragma once

#include "CoreMinimal.h"

class UGridItemDefinitionAsset;

/** ARMEMENT01.17.2A: one authored consumable that causes a Fire canonical
 * interaction on an existing surface; no talent, no recipe, no surface creation.
 * The C++ contract is preflighted before any native .uasset is saved.
 */
class FRPGStartingSurfaceCatalystAuthoring
{
public:
	static constexpr int32 ActionPoints = 2;
	static constexpr int32 Range = 4;

	static void Configure(UGridItemDefinitionAsset& Item);
	static bool Matches(const UGridItemDefinitionAsset& Item);
	/** Raw production fields must match without depending on the runtime projection bridge. */
	static bool MatchesSerializedProduction(const UGridItemDefinitionAsset& Item);
	static bool Run(bool bApply, FString& OutError);
};
