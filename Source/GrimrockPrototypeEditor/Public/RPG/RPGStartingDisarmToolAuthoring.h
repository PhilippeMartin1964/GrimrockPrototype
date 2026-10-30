#pragma once

#include "CoreMinimal.h"

class UGridItemDefinitionAsset;

/**
 * ARMEMENT01.16.1 — N1 Saboteur disarm-tool authoring.
 *
 * Produces the physical tool definition only. The tool is intentionally NOT
 * granted by any starter kit until a real trap-target/disarm executor exists.
 * No fake QuickItem action or fabricated disarm success is permitted.
 */
class FRPGStartingDisarmToolAuthoring final
{
public:
    static void Configure(UGridItemDefinitionAsset& Item);
    static bool Matches(const UGridItemDefinitionAsset& Item);
    static bool Run(bool bApply, FString& OutError);
};
