#pragma once

#include "Runtime/Items/GridItemCatalogues.h"
#include "UI/GridEquippedWeaponActionIconPalette.h"
#include "UObject/UObjectGlobals.h"

// Editor Automation-only bridge for retired one-shot migration tests.
// Its source is the persisted canonical ItemSystem, never the historical
// DA_EquippedWeaponActionIcons asset. No package is written or modified.
// The temporary legacy C++ shape can be removed once all projection tests
// and their obsolete helpers have been retired or replaced.
namespace GridEditorCanonicalIconFixture
{
    inline UGridEquippedWeaponActionIconPalette* Create()
    {
        const UGridItemSystemAsset* Root = LoadObject<UGridItemSystemAsset>(nullptr,
            TEXT("/Game/GrimrockPrototype/Core/DataAssets/CanonicalStage/DA_ItemSystem.DA_ItemSystem"));
        if (!IsValid(Root) || !IsValid(Root->WeaponVisualCatalogue.Get()) ||
            Root->WeaponVisualCatalogue->ActionIcons.Num() != 12 ||
            Root->WeaponVisualCatalogue->ActionIcons.Contains(EGridEquippedWeaponActionIconKind::None))
        {
            return nullptr;
        }

        UGridEquippedWeaponActionIconPalette* Transient =
            NewObject<UGridEquippedWeaponActionIconPalette>(GetTransientPackage());
        if (IsValid(Transient))
        {
            Transient->Icons = Root->WeaponVisualCatalogue->ActionIcons;
        }
        return Transient;
    }
}
