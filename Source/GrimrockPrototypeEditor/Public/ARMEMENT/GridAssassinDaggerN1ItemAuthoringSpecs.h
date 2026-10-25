#pragma once

#include "CoreMinimal.h"

// ARMEMENT01.11.2.2: item identities for the approved Rogue Assassin N1 item.
// Combat numbers and affinity are NEVER duplicated here; they come from
// the canonical workbook through DA_WeaponCatalogue.
namespace ARMEMENT011122
{
    struct FAssassinItemSpec
    {
        const TCHAR* ArchetypeId;
        const TCHAR* AssetName;
        const TCHAR* ItemDefinitionId;
        const TCHAR* AttackId;
    };

    static const FAssassinItemSpec AssassinItems[] =
    {
        { TEXT("ARM002"), TEXT("DA_Weapon_AssassinDagger_01"), TEXT("Weapon_AssassinDagger_01"), TEXT("Attack_AssassinDagger_01") }
    };
}
