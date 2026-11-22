#pragma once

#include "Core/GridObjectBehavior.h"
#include "EditorTools/GridEditorItemLevelPolicy.h"

/** Physical initial content is authored only through registered ItemIds.
 * New selections retain a valid level or start at N0/N1. Existing
 * same-ID selections must keep their explicit valid level.
 */
struct FGridEditorReceptacleInitialAuthoring
{
    static bool TrySelectCanonicalId(const UGridItemSubsystem* Registry,
        FName NewId, FGridReceptacleInitialItemConfig& InOut)
    {
        if (!IsValid(Registry) || !Registry->IsConfigured() ||
            NewId.IsNone() || !Registry->ContainsItem(NewId) ||
            InOut.Quantity < 1)
            return false;

        if (InOut.ItemId == NewId)
            return FGridEditorItemLevelPolicy::CanAuthorItemLevel(
                Registry, NewId, InOut.ItemLevel);

        int32 Level = InOut.ItemLevel;
        if (!FGridEditorItemLevelPolicy::CanAuthorItemLevel(Registry, NewId, Level))
            Level = FGridEditorItemLevelPolicy::CanAuthorItemLevel(
                Registry, NewId, 0) ? 0 : 1;
        if (!FGridEditorItemLevelPolicy::CanAuthorItemLevel(Registry, NewId, Level))
            return false;
        InOut.ItemId = NewId;
        InOut.ItemLevel = Level;
        return true;
    }

    static bool IsLevelEditable(const UGridItemSubsystem* Registry,
        const FGridReceptacleInitialItemConfig& Item)
    {
        return FGridEditorItemLevelPolicy::CanAuthorItemLevel(
                Registry, Item.ItemId, 1);
    }

    static bool TrySetLevel(const UGridItemSubsystem* Registry,
        int32 NewLevel, FGridReceptacleInitialItemConfig& InOut)
    {
        if (!FGridEditorItemLevelPolicy::CanAuthorItemLevel(
                Registry, InOut.ItemId, NewLevel))
            return false;
        InOut.ItemLevel = NewLevel;
        return true;
    }
};
