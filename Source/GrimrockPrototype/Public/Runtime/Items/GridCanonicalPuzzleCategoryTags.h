#pragma once

#include "Runtime/Items/GridItemSubsystem.h"

// Reserved puzzle-only metadata; does not confer equipment or action permissions.
namespace GridCanonicalPuzzleCategoryTags
{
    inline bool IsReserved(FName Tag)
    {
        return Tag.ToString().StartsWith(TEXT("PuzzleCategory."), ESearchCase::IgnoreCase);
    }

    inline FName ForType(FName Type)
    {
        static const TSet<FName> Allowed = {
            TEXT("Torch"), TEXT("Weapon"), TEXT("Shield"), TEXT("Armor"), TEXT("Jewelry"),
            TEXT("Key"), TEXT("Gem"), TEXT("Potion"), TEXT("Scroll"), TEXT("Book"),
            TEXT("Food"), TEXT("Component"), TEXT("Quest"), TEXT("Misc")
        };
        return Allowed.Contains(Type) ? FName(*(TEXT("PuzzleCategory.") + Type.ToString())) : NAME_None;
    }

    inline int32 CountReserved(const TArray<FName>& Tags)
    {
        int32 Count = 0;
        for (FName Tag : Tags) if (IsReserved(Tag)) ++Count;
        return Count;
    }

    inline bool Matches(const FGridResolvedItem& Item, FName Type)
    {
        const FName Tag = ForType(Type);
        if (!Item.IsValid() || Tag.IsNone()) return false;
        const int32 Count = CountReserved(Item.Tags);
        if (Item.bIsArmament)
        {
            return Count == 0 && Item.ItemLevel >= 1 && Item.ItemLevel <= 20 &&
                Type == (Item.Weapon.Kind == EGridCanonicalArmamentKind::Shield ? FName(TEXT("Shield")) : FName(TEXT("Weapon")));
        }
        return Item.ItemLevel == 0 && Count == 1 && Item.Tags.Contains(Tag);
    }
}
