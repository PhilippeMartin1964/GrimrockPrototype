#pragma once

#include "CoreMinimal.h"

class UGridItemDefinitionAsset;

/** ARMEMENT01.15: materialize one validated N1 potion through the existing
 * QuickItem/ItemDefinition pipeline. Never overwrite an authored package.
 */
class FRPGStartingHealthPotionAuthoring
{
public:
    static constexpr int32 HealAmount = 20;
    static constexpr int32 ActionPointCost = 1;

    static void Configure(UGridItemDefinitionAsset& Item);
    static bool Matches(const UGridItemDefinitionAsset& Item);
    static bool Run(bool bApply, FString& OutError);
};
