#pragma once

#include "CoreMinimal.h"

// ARMEMENT01.10.2: item identities for the approved Warrior N1 batch.
// Combat numbers and affinity are NEVER duplicated here; they come from
// the canonical workbook through DA_WeaponCatalogue.
namespace ARMEMENT01102
{
    struct FWarriorItemSpec
    {
        const TCHAR* ArchetypeId;
        const TCHAR* AssetName;
        const TCHAR* ItemDefinitionId;
        const TCHAR* AttackId;
    };

    static const FWarriorItemSpec WarriorItems[] =
    {
        { TEXT("ARM014"), TEXT("DA_Weapon_StraightSword_01"), TEXT("Weapon_StraightSword_01"), TEXT("Attack_StraightSword_01") },
        { TEXT("ARM015"), TEXT("DA_Weapon_LongSword_01"), TEXT("Weapon_LongSword_01"), TEXT("Attack_LongSword_01") },
        { TEXT("ARM025"), TEXT("DA_Weapon_WarPick_01"), TEXT("Weapon_WarPick_01"), TEXT("Attack_WarPick_01") },
        { TEXT("ARM031"), TEXT("DA_Weapon_GreatAxe_01"), TEXT("Weapon_GreatAxe_01"), TEXT("Attack_GreatAxe_01") }
    };
}
