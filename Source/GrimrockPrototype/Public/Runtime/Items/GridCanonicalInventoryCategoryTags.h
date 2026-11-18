#pragma once

#include "CoreMinimal.h"

/** Presentation-only tags on canonical non-ARM definitions. They do not
 * authorize equipment, throwing, combat, crafting or item consumption.
 * These names are intentionally disjoint from old gameplay tags.
 */
namespace GridCanonicalInventoryCategoryTags
{
    inline FName Weapon() { return FName(TEXT("InventoryCategory.Weapon")); }
    inline FName Food() { return FName(TEXT("InventoryCategory.Food")); }
    inline FName Component() { return FName(TEXT("InventoryCategory.Component")); }

    inline int32 Count(const TArray<FName>& Tags)
    {
        return int32(Tags.Contains(Weapon())) + int32(Tags.Contains(Food())) +
            int32(Tags.Contains(Component()));
    }
}
