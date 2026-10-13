#pragma once

#include "CoreMinimal.h"

/**
 * Runtime resolver for the generated UI String Table.
 *
 * UI.csv is the authoring authority; ST_UI is the generated runtime projection.
 * Missing keys are treated as data errors and never replaced with authored
 * player-facing prose in C++.
 */
struct GRIMROCKPROTOTYPE_API FUILocalizationText
{
	static FText Get(const TCHAR* Key);
};
