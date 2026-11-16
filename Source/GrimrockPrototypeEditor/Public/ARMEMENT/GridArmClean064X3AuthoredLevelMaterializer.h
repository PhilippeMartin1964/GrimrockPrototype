#pragma once

#include "CoreMinimal.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"

/**
 * ARM-CLEAN06.4X.3: Editor-only conversion policy for a freshly authored
 * world placement. A historical ARM N0 means "inherit this SOURCE'S real
 * DefaultWeaponItemLevel", never silently invent N1. Explicit ARM levels
 * remain N1-N20; real non-ARM items remain N0.
 *
 * This does not consult any legacy DataAsset or catalogue at runtime.
 * The one-shot Editor converter supplies the audited historical source
 * level and persists the resulting physical item level before removing
 * legacy item-definition references.
 */
struct FGridArmClean064X3AuthoredLevelMaterializer
{
    static bool TryResolve(const UGridCanonicalItemDefinitionAsset* Canonical,
        int32 AuthoredLevel, int32 SourceDefaultWeaponLevel,
        int32& OutEffectiveLevel, bool& bOutInheritedFromSource)
    {
        OutEffectiveLevel = INDEX_NONE;
        bOutInheritedFromSource = false;

        if (!IsValid(Canonical) || Canonical->ItemId.IsNone() ||
            AuthoredLevel < 0 || AuthoredLevel > 20)
        {
            return false;
        }

        if (const UGridArmamentDefinitionAsset* Weapon =
            Cast<UGridArmamentDefinitionAsset>(Canonical))
        {
            if (Weapon->ArchetypeId.IsNone())
            {
                return false;
            }
            const int32 Fixed = AuthoredLevel == 0
                ? SourceDefaultWeaponLevel : AuthoredLevel;
            if (Fixed < 1 || Fixed > 20)
            {
                return false;
            }
            OutEffectiveLevel = Fixed;
            bOutInheritedFromSource = AuthoredLevel == 0;
            return true;
        }

        if (Cast<UGridNonArmamentDefinitionAsset>(Canonical) &&
            AuthoredLevel == 0)
        {
            OutEffectiveLevel = 0;
            return true;
        }

        return false;
    }
};
