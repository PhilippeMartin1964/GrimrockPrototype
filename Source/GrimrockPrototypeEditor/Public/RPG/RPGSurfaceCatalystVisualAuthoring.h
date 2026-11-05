#pragma once
#include "CoreMinimal.h"

/** Read-only validation of approved native artist catalyst visuals. */
class FRPGSurfaceCatalystVisualAuthoring
{
public:
	static bool MatchesProductionAssets(FString* OutProblem = nullptr);
};
