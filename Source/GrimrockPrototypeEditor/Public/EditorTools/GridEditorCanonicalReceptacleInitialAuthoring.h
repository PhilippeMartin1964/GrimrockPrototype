#pragma once

#include "Core/GridObjectBehavior.h"
#include "EditorTools/GridEditorCanonicalItemLevelPolicy.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/**
 * ARM-CLEAN06.4X.16.3: editor-only physical write-through for initial
 * receptacle contents. Existing real entries already persist their
 * CanonicalItemId and exact physical levels (W/X.5).
 */
struct FGridEditorCanonicalReceptacleInitialAuthoring
{
    static bool TrySelectDefinition(const UGridItemSubsystem* Registry,
        UGridItemDefinitionAsset* NewDefinition,
        FGridReceptacleInitialItemConfig& InOut)
    {
        if (!IsValid(Registry) || !Registry->IsConfigured() ||
            (IsValid(NewDefinition) && NewDefinition->ItemDefinitionId.IsNone()))
        {
            return false;
        }
        FGridReceptacleInitialItemConfig Proposed = InOut;
        Proposed.ItemDefinition = NewDefinition;
        Proposed.CanonicalItemId = NAME_None;
        Proposed.ItemLevel = 0;
        if (IsValid(NewDefinition) &&
            Registry->HasCanonicalItem(NewDefinition->ItemDefinitionId))
        {
            FGridResolvedItem Family;
            if (!Registry->ResolveItem(NewDefinition->ItemDefinitionId, 1, Family))
            {
                return false;
            }
            const int32 FixedLevel = Family.bIsArmament
                ? NewDefinition->DefaultWeaponItemLevel : 0;
            if ((Family.bIsArmament && (FixedLevel < 1 || FixedLevel > 20)) ||
                (!Family.bIsArmament && FixedLevel != 0))
            {
                return false;
            }
            FGridResolvedItem Physical;
            if (!Registry->ResolveItem(NewDefinition->ItemDefinitionId, FixedLevel, Physical) ||
                !Physical.IsValid() ||
                Physical.ItemId != NewDefinition->ItemDefinitionId ||
                Physical.ItemLevel != FixedLevel ||
                Physical.bIsArmament != Family.bIsArmament)
            {
                return false;
            }
            Proposed.CanonicalItemId = Physical.ItemId;
            Proposed.ItemLevel = Physical.ItemLevel;
            Proposed.ItemDefinition = nullptr;
        }
        // Unknown legacy definitions retain the old N0 authoring pathway.
        // The quantity is never changed when replacing the source.
        InOut = Proposed;
        return true;
    }

    /** New direct authoring, not a persisted-level migration. Preserve a
     * valid level when changing identity; otherwise explicitly initialize
     * a new ARM at N1 or an unlevelled item at N0, like loose placements.
     * Recommitting the same ID is a no-op, even before native conversion. */
    static bool TrySelectCanonicalId(const UGridItemSubsystem* Registry,
        FName NewId, FGridReceptacleInitialItemConfig& InOut)
    {
        if (!IsValid(Registry) || !Registry->IsConfigured() ||
            NewId.IsNone() || !Registry->HasCanonicalItem(NewId) ||
            InOut.Quantity < 1)
            return false;

        const UGridItemDefinitionAsset* Old = InOut.ItemDefinition.Get();
        if (!InOut.CanonicalItemId.IsNone() && Old &&
            Old->ItemDefinitionId != InOut.CanonicalItemId)
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
        InOut.ItemDefinition = nullptr;
        return true;
    }

    static bool IsLevelEditable(const UGridItemSubsystem* Registry,
        const FGridReceptacleInitialItemConfig& Item)
    {
        if (!Item.CanonicalItemId.IsNone())
        {
            const UGridItemDefinitionAsset* Old = Item.ItemDefinition.Get();
            return (!Old || Old->ItemDefinitionId == Item.CanonicalItemId) &&
                FGridEditorCanonicalItemLevelPolicy::CanAuthorCanonicalLevel(
                    Registry, Item.CanonicalItemId, 1);
        }
        return FGridEditorCanonicalItemLevelPolicy::IsLevelEditable(
            Registry, Item.ItemDefinition.Get());
    }

    static bool TrySetLevel(const UGridItemSubsystem* Registry,
        int32 NewLevel, FGridReceptacleInitialItemConfig& InOut)
    {
        if (!IsValid(Registry) || !Registry->IsConfigured() ||
            NewLevel < 0 || NewLevel > 20)
            return false;
        if (!InOut.CanonicalItemId.IsNone())
        {
            const UGridItemDefinitionAsset* Old = InOut.ItemDefinition.Get();
            if ((Old && Old->ItemDefinitionId != InOut.CanonicalItemId) ||
                !FGridEditorCanonicalItemLevelPolicy::CanAuthorCanonicalLevel(
                    Registry, InOut.CanonicalItemId, NewLevel))
                return false;
        }
        else if (!FGridEditorCanonicalItemLevelPolicy::CanAuthorLevel(
            Registry, InOut.ItemDefinition.Get(), NewLevel))
            return false;
        InOut.ItemLevel = NewLevel;
        return true;
    }
};
