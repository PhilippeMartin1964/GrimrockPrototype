#pragma once

#include "Runtime/Items/GridItemSubsystem.h"

// Reserved puzzle-only metadata; does not confer equipment or action permissions.
namespace GridPuzzleCategoryTags
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

    inline TArray<FName> WithoutReserved(const TArray<FName>& Tags)
    {
        TArray<FName> Result;
        for (FName Tag : Tags) if (!IsReserved(Tag)) Result.Add(Tag);
        return Result;
    }

    inline bool IsValidMetadata(const TArray<FName>& Tags, bool bArm)
    {
        if (CountReserved(Tags) == 0) return true;
        if (bArm || CountReserved(Tags) != 1) return false;
        for (FName Tag : Tags)
            if (IsReserved(Tag))
                return ForType(FName(*Tag.ToString().RightChop(15))) == Tag;
        return false;
    }

    // Isolated historical comparison: the only accepted delta is an appended
    // tag explicitly proven by the caller's migration evidence.
    inline bool MatchesBeforeOrAfter(const TArray<FName>& Actual,
        const TArray<FName>& Original, FName Type)
    {
        const FName Tag = ForType(Type);
        if (Tag.IsNone() || CountReserved(Original) != 0) return false;
        TArray<FName> Migrated = Original;
        Migrated.Add(Tag);
        return Actual == Original || Actual == Migrated;
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
