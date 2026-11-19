#pragma once

#include "Core/GridObjectBehavior.h"
#include "EditorTools/GridEditorCanonicalItemLevelPolicy.h"

/** Physical initial content is authored only through registered ItemIds.
 * New selections retain a valid level or start at N0/N1. Existing
 * same-ID selections must keep their explicit valid level.
 */
struct FGridEditorCanonicalReceptacleInitialAuthoring
{
    static bool TrySelectCanonicalId(const UGridItemSubsystem* Registry,
        FName NewId, FGridReceptacleInitialItemConfig& InOut)
    {
        if (!IsValid(Registry) || !Registry->IsConfigured() ||
            NewId.IsNone() || !Registry->HasCanonicalItem(NewId) ||
            InOut.Quantity < 1 || InOut.ItemDefinition)
            return false;

        if (InOut.CanonicalItemId == NewId)
            return FGridEditorCanonicalItemLevelPolicy::CanAuthorCanonicalLevel(
                Registry, NewId, InOut.ItemLevel);

        int32 Level = InOut.ItemLevel;
        if (!FGridEditorCanonicalItemLevelPolicy::CanAuthorCanonicalLevel(Registry, NewId, Level))
            Level = FGridEditorCanonicalItemLevelPolicy::CanAuthorCanonicalLevel(
                Registry, NewId, 0) ? 0 : 1;
        if (!FGridEditorCanonicalItemLevelPolicy::CanAuthorCanonicalLevel(Registry, NewId, Level))
            return false;
        InOut.CanonicalItemId = NewId;
        InOut.ItemLevel = Level;
        return true;
    }

    static bool IsLevelEditable(const UGridItemSubsystem* Registry,
        const FGridReceptacleInitialItemConfig& Item)
    {
        return !Item.ItemDefinition &&
            FGridEditorCanonicalItemLevelPolicy::CanAuthorCanonicalLevel(
                Registry, Item.CanonicalItemId, 1);
    }

    static bool TrySetLevel(const UGridItemSubsystem* Registry,
        int32 NewLevel, FGridReceptacleInitialItemConfig& InOut)
    {
        if (InOut.ItemDefinition ||
            !FGridEditorCanonicalItemLevelPolicy::CanAuthorCanonicalLevel(
                Registry, InOut.CanonicalItemId, NewLevel))
            return false;
        InOut.ItemLevel = NewLevel;
        return true;
    }
};
