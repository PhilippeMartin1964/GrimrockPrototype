#pragma once

#include "CoreMinimal.h"

// ARMEMENT01.10.2.1: planned asset names, never generated placeholders.
// The source of gameplay statistics remains DA_WeaponCatalogue.
// These entries control only editor-side visual registration.
namespace ARMEMENT011121
{
    struct FAssassinVisualSpec
    {
        const TCHAR* ArchetypeId;
        const TCHAR* VisualId;
        const TCHAR* FamilyTag;
        const TCHAR* Stem;       // Mesh: SM_Weapon_<Stem>_01, icon T_Weapon_<Stem>_01
        const TCHAR* Material;   // MI_<Material>
    };

    static const FAssassinVisualSpec Specs[] =
    {
        { TEXT("ARM002"), TEXT("VIS_AssassinDagger_01"), TEXT("Light"), TEXT("AssassinDagger"), TEXT("AssassinDagger") }
    };

    static FString MeshPath(const FAssassinVisualSpec& Spec)
    {
        const FString Name = FString::Printf(TEXT("SM_Weapon_%s_01"), Spec.Stem);
        return FString::Printf(TEXT("/Game/GrimrockPrototype/Meshes/Weapons/%s.%s"), *Name, *Name);
    }

    static FString IconPath(const FAssassinVisualSpec& Spec)
    {
        const FString Name = FString::Printf(TEXT("T_Weapon_%s_01"), Spec.Stem);
        return FString::Printf(TEXT("/Game/GrimrockPrototype/UI/Icons/Weapons/%s.%s"), *Name, *Name);
    }

    static FString MaterialPath(const FAssassinVisualSpec& Spec)
    {
        const FString Name = FString::Printf(TEXT("MI_%s"), Spec.Material);
        return FString::Printf(TEXT("/Game/GrimrockPrototype/Art/Materials/Instances/Weapons/%s.%s"), *Name, *Name);
    }

    static FString PbrPath(const FAssassinVisualSpec& Spec, const TCHAR* Suffix)
    {
        const FString Name = FString::Printf(TEXT("T_%s_01_%s"), Spec.Stem, Suffix);
        return FString::Printf(TEXT("/Game/GrimrockPrototype/Art/Textures/Final/Weapons/%s/%s.%s"),
            Spec.Stem, *Name, *Name);
    }
}
