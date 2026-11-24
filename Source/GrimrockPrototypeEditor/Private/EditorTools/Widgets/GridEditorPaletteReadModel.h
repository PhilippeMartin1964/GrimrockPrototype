#pragma once

#include "Core/GridObjectPaletteAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** Editor-only, read-only projection from the same canonical registry used
 * by the runtime. No legacy fallback for a persisted ItemId. */
struct FGridEditorPaletteReadView
{
    bool bRegisteredSource = false;
    bool bResolved = false;
    FName ItemId = NAME_None;
    FText DisplayName;
    FText Description;
    TSoftObjectPtr<UTexture2D> Icon;
};

struct FGridEditorPaletteReadModel
{
    static FGridEditorPaletteReadView Resolve(
        const FGridObjectPaletteEntry& Entry, const UGridItemSubsystem* Registry)
    {
        FGridEditorPaletteReadView Out;
        if (!Entry.IsDirectItemEntry())
        {
            Out.DisplayName = Entry.GetEffectiveDisplayName();
            Out.Icon = Entry.Icon.Get();
            Out.bResolved = true;
            return Out; // World objects are not item definitions.
        }

        Out.bRegisteredSource = true;
        Out.ItemId = Entry.ItemId;
        Out.DisplayName = FText::FromName(Entry.ItemId);
        Out.Description = FText::GetEmpty();
        Out.Icon.Reset();
        if (!IsValid(Registry) || !Registry->IsConfigured() ||
            !Registry->ContainsItem(Entry.ItemId) ||
            Entry.DefaultWorldObjectDefinition ||
            Entry.Icon || Entry.DefaultMonsterDefinition || Entry.DefaultStoryCompanionDefinition ||
            Entry.DefaultItemLevel < 0 || Entry.DefaultItemLevel > 20)
        {
            return Out; // Invalid canonical identity never uses legacy text/icon.
        }

        FGridResolvedItem Item;
        if (!Registry->ResolveItem(Entry.ItemId,
                Entry.DefaultItemLevel, Item) ||
            !Item.IsValid() || Item.ItemId != Entry.ItemId ||
            Item.ItemLevel != Entry.DefaultItemLevel ||
            (Item.bIsArmament && Item.ItemLevel < 1) ||
            (!Item.bIsArmament && Item.ItemLevel != 0))
        {
            return Out;
        }
        Out.DisplayName = Item.DisplayName.IsEmpty()
            ? FText::FromName(Entry.ItemId) : Item.DisplayName;
        Out.Description = Item.Description;
        Out.Icon = Item.ItemIcon;
        Out.bResolved = true;
        return Out;
    }
};
