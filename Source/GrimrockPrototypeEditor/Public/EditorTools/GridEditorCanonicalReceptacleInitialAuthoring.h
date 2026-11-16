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
        }
        // Unknown legacy definitions retain the old N0 authoring pathway.
        // The quantity is never changed when replacing the source.
        InOut = Proposed;
        return true;
    }

    static bool TrySetLevel(const UGridItemSubsystem* Registry,
        int32 NewLevel, FGridReceptacleInitialItemConfig& InOut)
    {
        if (!IsValid(Registry) || !Registry->IsConfigured() ||
            NewLevel < 0 || NewLevel > 20 ||
            !IsValid(InOut.ItemDefinition.Get()))
        {
            return false;
        }
        if (!InOut.CanonicalItemId.IsNone())
        {
            if (InOut.CanonicalItemId != InOut.ItemDefinition->ItemDefinitionId ||
                !Registry->HasCanonicalItem(InOut.CanonicalItemId))
            {
                return false;
            }
            FGridResolvedItem Family;
            if (!Registry->ResolveItem(InOut.CanonicalItemId, 1, Family) ||
                (Family.bIsArmament && NewLevel == 0) ||
                (!Family.bIsArmament && NewLevel != 0))
            {
                return false;
            }
            FGridResolvedItem Physical;
            if (!Registry->ResolveItem(InOut.CanonicalItemId, NewLevel, Physical) ||
                !Physical.IsValid() ||
                Physical.ItemId != InOut.CanonicalItemId ||
                Physical.ItemLevel != NewLevel)
            {
                return false;
            }
        }
        else if (!FGridEditorCanonicalItemLevelPolicy::CanAuthorLevel(
            Registry, InOut.ItemDefinition.Get(), NewLevel))
        {
            return false; // Legacy-only fixture retains authored default rules.
        }
        InOut.ItemLevel = NewLevel;
        return true;
    }
};
