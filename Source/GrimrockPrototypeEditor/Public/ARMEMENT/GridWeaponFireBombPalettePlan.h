#pragma once

#include "Core/GridObjectPaletteAsset.h"
#include "EditorTools/GridEditorItemLevelPolicy.h"

// ARMEMENT01.10.2.3: editor-only grouping; explicit canonical identities
// and physical defaults for new entries, never a second item definition.
namespace ARMEMENT011023
{
    struct FSpec
    {
        const TCHAR* EntryId;
        const TCHAR* Category;
        int32 NewPhysicalLevel;
    };

    static const FSpec Specs[] =
    {
        { TEXT("Weapon_Dagger_01"), TEXT("Weapon"), 1 },
        { TEXT("Weapon_ShortBow_01"), TEXT("Weapon"), 1 },
        { TEXT("Weapon_RoundShield_01"), TEXT("Weapon"), 1 },
        { TEXT("Weapon_StraightSword_01"), TEXT("Weapon"), 1 },
        { TEXT("Item_Bomb_Fire"), TEXT("Items"), 0 }
    };

    inline bool IsCanonicalDirect(const FGridObjectPaletteEntry& Entry,
        const UGridItemSubsystem* Registry)
    {
        return Entry.IsDirectItemEntry() &&
            !Entry.DefaultWorldObjectDefinition && !Entry.Icon &&
            !Entry.DefaultMonsterDefinition && !Entry.DefaultStoryCompanionDefinition &&
            FGridEditorItemLevelPolicy::CanAuthorItemLevel(
                Registry, Entry.CanonicalItemId, Entry.CanonicalDefaultItemLevel);
    }

    // Copy-first: conflicts cannot expose a partially authored proposal.
    inline bool Propose(
        const TArray<FGridObjectPaletteEntry>& Original,
        const UGridItemSubsystem* Registry,
        const TArray<const FSpec*>& Targets,
        TArray<FGridObjectPaletteEntry>& OutEntries,
        int32& OutAdded, int32& OutMoved, FString& OutError)
    {
        OutEntries.Reset();
        OutAdded = OutMoved = 0;
        OutError.Reset();
        if (!IsValid(Registry) || !Registry->IsConfigured())
        {
            OutError = TEXT("Missing canonical registry");
            return false;
        }
        for (const auto& Entry : Original)
        {
            if (Entry.IsDirectItemEntry() && !IsCanonicalDirect(Entry, Registry))
            {
                OutError = TEXT("Invalid direct source in original palette");
                return false;
            }
        }
        TArray<FGridObjectPaletteEntry> Candidate = Original;
        int32 Added = 0, Moved = 0;
        TSet<FName> RequestedIds;
        for (const FSpec* Target : Targets)
        {
            if (!Target)
            {
                OutError = TEXT("Missing requested spec");
                return false;
            }
            const FName Id(Target->EntryId);
            const FName DesiredCategory(Target->Category);
            if (Id.IsNone() || DesiredCategory.IsNone() || RequestedIds.Contains(Id) ||
                !FGridEditorItemLevelPolicy::CanAuthorItemLevel(Registry, Id, Target->NewPhysicalLevel))
            {
                OutError = FString::Printf(TEXT("Invalid requested canonical identity or level: %s"), Target->EntryId);
                return false;
            }
            RequestedIds.Add(Id);
            FGridObjectPaletteEntry* Existing = nullptr;
            int32 Count = 0;
            for (FGridObjectPaletteEntry& Entry : Candidate)
            {
                if (Entry.EntryId == Id)
                {
                    Existing = &Entry;
                    ++Count;
                }
                else if (Entry.CanonicalItemId == Id)
                {
                    OutError = FString::Printf(TEXT("Item already exposed by another EntryId: %s"), *Entry.EntryId.ToString());
                    return false;
                }
            }
            if (Count > 1)
            {
                OutError = FString::Printf(TEXT("Duplicate existing EntryId: %s"), Target->EntryId);
                return false;
            }
            if (!Existing)
            {
                FGridObjectPaletteEntry& NewEntry = Candidate.AddDefaulted_GetRef();
                NewEntry.EntryId = Id;
                NewEntry.PaletteCategory = DesiredCategory;
                NewEntry.CanonicalItemId = Id;
                NewEntry.CanonicalDefaultItemLevel = Target->NewPhysicalLevel;
                ++Added;
                continue;
            }
            if (!IsCanonicalDirect(*Existing, Registry) || Existing->CanonicalItemId != Id ||
                !Existing->IsValidEntry())
            {
                OutError = FString::Printf(TEXT("Conflicting direct collectible entry: %s"), Target->EntryId);
                return false;
            }
            if (Existing->PaletteCategory == DesiredCategory) continue;
            if (DesiredCategory == FName(TEXT("Weapon")) && Existing->PaletteCategory == FName(TEXT("Items")))
            {
                Existing->PaletteCategory = DesiredCategory;
                ++Moved;
                continue;
            }
            OutError = FString::Printf(TEXT("Unexpected authored category for %s: %s"),
                Target->EntryId, *Existing->PaletteCategory.ToString());
            return false;
        }
        OutEntries = MoveTemp(Candidate);
        OutAdded = Added;
        OutMoved = Moved;
        return true;
    }
}
