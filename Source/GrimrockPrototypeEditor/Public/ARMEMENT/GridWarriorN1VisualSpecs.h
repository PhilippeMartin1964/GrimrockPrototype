#pragma once

#include "CoreMinimal.h"

// ARMEMENT01.10.2.1: planned asset names, never generated placeholders.
// The source of gameplay statistics remains DA_WeaponCatalogue.
// These entries control only editor-side visual registration.
namespace ARMEMENT011021
{
    struct FWarriorVisualSpec
    {
        const TCHAR* ArchetypeId;
        const TCHAR* VisualId;
        const TCHAR* FamilyTag;
        const TCHAR* Stem;       // Mesh: SM_Weapon_<Stem>_01, icon T_Weapon_<Stem>_01
        const TCHAR* Material;   // MI_<Material>
    };

    static const FWarriorVisualSpec Specs[] =
    {
        { TEXT("ARM014"), TEXT("VIS_StraightSword_01"), TEXT("Sword"), TEXT("StraightSword"), TEXT("StraightSword") },
        { TEXT("ARM015"), TEXT("VIS_LongSword_01"), TEXT("Sword"), TEXT("LongSword"), TEXT("LongSword") },
        { TEXT("ARM025"), TEXT("VIS_WarPick_01"), TEXT("Pick"), TEXT("WarPick"), TEXT("WarPick") },
        { TEXT("ARM031"), TEXT("VIS_GreatAxe_01"), TEXT("Axe"), TEXT("GreatAxe"), TEXT("GreatAxe") }
    };

    static FString MeshPath(const FWarriorVisualSpec& Spec)
    {
        const FString Name = FString::Printf(TEXT("SM_Weapon_%s_01"), Spec.Stem);
        return FString::Printf(TEXT("/Game/GrimrockPrototype/Meshes/Weapons/%s.%s"), *Name, *Name);
    }

    static FString IconPath(const FWarriorVisualSpec& Spec)
    {
        const FString Name = FString::Printf(TEXT("T_Weapon_%s_01"), Spec.Stem);
        return FString::Printf(TEXT("/Game/GrimrockPrototype/UI/Icons/Weapons/%s.%s"), *Name, *Name);
    }

    static FString MaterialPath(const FWarriorVisualSpec& Spec)
    {
        const FString Name = FString::Printf(TEXT("MI_%s"), Spec.Material);
        return FString::Printf(TEXT("/Game/GrimrockPrototype/Art/Materials/Instances/Weapons/%s.%s"), *Name, *Name);
    }

    static FString PbrPath(const FWarriorVisualSpec& Spec, const TCHAR* Suffix)
    {
        const FString Name = FString::Printf(TEXT("T_%s_01_%s"), Spec.Stem, Suffix);
        return FString::Printf(TEXT("/Game/GrimrockPrototype/Art/Textures/Final/Weapons/%s/%s.%s"),
            Spec.Stem, *Name, *Name);
    }
}
