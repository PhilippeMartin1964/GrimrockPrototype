#pragma once
#include "CoreMinimal.h"

/** ARMEMENT01.17.2C.2: native UE assets from a strictly scoped commandlet.
 * Does not touch combat definitions or canonical reaction mechanics.
 */
class FRPGSurfaceCatalystVisualAuthoring
{
public:
	static bool Run(bool bApply, FString& OutError);
	static bool MatchesProductionAssets(FString* OutProblem = nullptr);
};
