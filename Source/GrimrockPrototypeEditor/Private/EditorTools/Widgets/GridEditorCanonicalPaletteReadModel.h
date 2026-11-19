#pragma once

#include "Core/GridObjectPaletteAsset.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** Editor-only, read-only projection from the same canonical registry used
 * by the runtime. No legacy fallback for a persisted CanonicalItemId. */
struct FGridEditorCanonicalPaletteReadView
{
    bool bCanonicalSource = false;
    bool bResolved = false;
    FName ItemId = NAME_None;
    FText DisplayName;
    FText Description;
    TSoftObjectPtr<UTexture2D> Icon;
};

struct FGridEditorCanonicalPaletteReadModel
{
    static FGridEditorCanonicalPaletteReadView Resolve(
        const FGridObjectPaletteEntry& Entry, const UGridItemSubsystem* Registry)
    {
        FGridEditorCanonicalPaletteReadView Out;
        if (!Entry.IsDirectItemEntry())
        {
            Out.DisplayName = Entry.GetEffectiveDisplayName();
            Out.Icon = Entry.Icon.Get();
            Out.bResolved = true;
            return Out; // World objects are not item definitions.
        }

        Out.bCanonicalSource = true;
        Out.ItemId = Entry.CanonicalItemId;
        Out.DisplayName = FText::FromName(Entry.CanonicalItemId);
        Out.Description = FText::GetEmpty();
        Out.Icon.Reset();
        if (!IsValid(Registry) || !Registry->IsConfigured() ||
            !Registry->HasCanonicalItem(Entry.CanonicalItemId) ||
            Entry.DefaultWorldObjectDefinition ||
            Entry.Icon || Entry.DefaultMonsterDefinition || Entry.DefaultStoryCompanionDefinition ||
            Entry.CanonicalDefaultItemLevel < 0 || Entry.CanonicalDefaultItemLevel > 20)
        {
            return Out; // Invalid canonical identity never uses legacy text/icon.
        }

        FGridResolvedItem Item;
        if (!Registry->ResolveItem(Entry.CanonicalItemId,
                Entry.CanonicalDefaultItemLevel, Item) ||
            !Item.IsValid() || Item.ItemId != Entry.CanonicalItemId ||
            Item.ItemLevel != Entry.CanonicalDefaultItemLevel ||
            (Item.bIsArmament && Item.ItemLevel < 1) ||
            (!Item.bIsArmament && Item.ItemLevel != 0))
        {
            return Out;
        }
        Out.DisplayName = Item.DisplayName.IsEmpty()
            ? FText::FromName(Entry.CanonicalItemId) : Item.DisplayName;
        Out.Description = Item.Description;
        Out.Icon = Item.ItemIcon;
        Out.bResolved = true;
        return Out;
    }
};
