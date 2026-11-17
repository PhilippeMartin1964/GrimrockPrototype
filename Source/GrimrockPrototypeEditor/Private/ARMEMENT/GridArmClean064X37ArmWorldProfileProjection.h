#pragma once

#include "CoreMinimal.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemDefinitionAsset.h"
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "UObject/UnrealType.h"

// ARM-CLEAN06.4X.37: one-shot native authoring only.
// This manifest comes from the verified X.19 native inventory (22 ARM,
// excluding Shuriken and the ThrowingKnife redirector). Never infer extra
// assets from filenames or modify historical definitions.
namespace ARMCLEAN064X37
{
inline constexpr const TCHAR* LegacySuffixes[] =
{
    TEXT("ArcaneOrb_01"), TEXT("ArcaneWand_01"),
    TEXT("AssassinDagger_01"), TEXT("Dagger_01"), TEXT("Dart_01"),
    TEXT("ElementalStaff_01"), TEXT("FireWand_01"), TEXT("GreatAxe_01"),
    TEXT("HolyAegis_01"), TEXT("HolyScepter_01"), TEXT("Javelin_01"),
    TEXT("LongSword_01"), TEXT("Mace_01"), TEXT("MainGauche_01"),
    TEXT("RecurveBow_01"), TEXT("RoundShield_01"), TEXT("ShortBow_01"),
    TEXT("ShortSpear_01"), TEXT("Stiletto_01"), TEXT("StraightSword_01"),
    TEXT("ThrowingKnife_01"), TEXT("WarPick_01")
};

inline FString SourceObjectPath(const TCHAR* Suffix)
{
    const FString Name = FString(TEXT("DA_Weapon_")) + Suffix;
    return FString(TEXT("/Game/GrimrockPrototype/Core/DataAssets/Weapons/")) +
        Name + TEXT(".") + Name;
}

inline bool EqualVisualField(const UGridItemDefinitionAsset& Source,
    const FGridCanonicalWeaponVisual& Destination, const TCHAR* Field)
{
    const FProperty* A = Source.GetClass()->FindPropertyByName(FName(Field));
    const FProperty* B = FGridCanonicalWeaponVisual::StaticStruct()->
        FindPropertyByName(FName(Field));
    if (!A || !B || !B->SameType(A)) { return false; }
    const void* V = A->ContainerPtrToValuePtr<void>(&Source);
    const void* W = B->ContainerPtrToValuePtr<void>(&Destination);
    return B->Identical(V, W);
}

inline bool CopyVisualField(const UGridItemDefinitionAsset& Source,
    FGridCanonicalWeaponVisual& Destination, const TCHAR* Field)
{
    const FProperty* A = Source.GetClass()->FindPropertyByName(FName(Field));
    const FProperty* B = FGridCanonicalWeaponVisual::StaticStruct()->
        FindPropertyByName(FName(Field));
    if (!A || !B || !B->SameType(A)) { return false; }
    const void* V = A->ContainerPtrToValuePtr<void>(&Source);
    void* W = B->ContainerPtrToValuePtr<void>(&Destination);
    B->CopyCompleteValue(W, V);
    return B->Identical(V, W);
}

inline constexpr const TCHAR* VisualFields[] =
{
    TEXT("LightEmitter"), TEXT("VisualEffects"),
    TEXT("bEnableWorldSparkle"), TEXT("WorldSparkleMaterial"),
    TEXT("WorldSparkleColor"), TEXT("WorldSparkleIntensity"),
    TEXT("WorldSparkleSpeed"), TEXT("WorldSparkleVariation")
};

inline bool Matches(const UGridItemDefinitionAsset& Source,
    const FGridCanonicalWeaponVisual& Visual,
    const FGridCanonicalWeaponArchetype& Archetype)
{
    for (const TCHAR* Field : VisualFields)
    {
        if (!EqualVisualField(Source, Visual, Field)) { return false; }
    }
    return Archetype.bUseItemWeightAsWorldPhysicsMass ==
            Source.bUseItemWeightAsWorldPhysicsMass &&
        Archetype.WorldPhysicsInitialTiltDegrees ==
            Source.WorldPhysicsInitialTiltDegrees;
}

inline bool Project(const UGridItemDefinitionAsset& Source,
    FGridCanonicalWeaponVisual& Visual, FGridCanonicalWeaponArchetype& Archetype)
{
    for (const TCHAR* Field : VisualFields)
    {
        if (!CopyVisualField(Source, Visual, Field)) { return false; }
    }
    Archetype.bUseItemWeightAsWorldPhysicsMass =
        Source.bUseItemWeightAsWorldPhysicsMass;
    Archetype.WorldPhysicsInitialTiltDegrees =
        Source.WorldPhysicsInitialTiltDegrees;
    return Matches(Source, Visual, Archetype);
}

struct FPlan
{
    TArray<FGridCanonicalWeaponVisual> Visuals;
    TArray<FGridCanonicalWeaponArchetype> Archetypes;
    int32 SourceCount = 0;
    int32 ChangedProfiles = 0;
};

/** Purely staged copies. Never calls Modify, MarkPackageDirty or SavePackage. */
inline bool BuildPlan(const UGridItemSystemAsset* Root, FPlan& Out, FString& Error)
{
    Out = FPlan();
    if (!IsValid(Root) || !IsValid(Root->WeaponCatalogue) ||
        !IsValid(Root->WeaponVisualCatalogue) ||
        Root->ItemDefinitions.Num() != 34 ||
        Root->WeaponCatalogue->Archetypes.Num() != 82 ||
        Root->WeaponVisualCatalogue->ArchetypeBindings.Num() != 22 ||
        Root->WeaponVisualCatalogue->VisualProfiles.Num() != 22)
    {
        Error = TEXT("Canonical root or exact X.19 manifest counts changed");
        return false;
    }

    int32 CanonicalArms = 0;
    for (const TObjectPtr<UGridCanonicalItemDefinitionAsset>& Candidate : Root->ItemDefinitions)
    {
        CanonicalArms += Cast<UGridArmamentDefinitionAsset>(Candidate.Get()) ? 1 : 0;
    }
    if (CanonicalArms != 22)
    {
        Error = TEXT("Canonical ARM count differs from the X.19 source");
        return false;
    }

    Out.Visuals = Root->WeaponVisualCatalogue->VisualProfiles;
    Out.Archetypes = Root->WeaponCatalogue->Archetypes;
    TSet<FName> SeenItems;
    TSet<FName> SeenArchetypes;
    TSet<FName> SeenVisuals;

    for (const TCHAR* Suffix : LegacySuffixes)
    {
        const FString Path = SourceObjectPath(Suffix);
        const UGridItemDefinitionAsset* Source =
            LoadObject<UGridItemDefinitionAsset>(nullptr, *Path);
        const FName ExpectedId(*(FString(TEXT("Weapon_")) + Suffix));
        if (!IsValid(Source) || Source->ItemDefinitionId != ExpectedId ||
            !Source->bUseWeaponCatalogue || !Source->bUseWeaponVisualCatalogue ||
            Source->WeaponArchetypeId.IsNone() || SeenItems.Contains(ExpectedId) ||
            SeenArchetypes.Contains(Source->WeaponArchetypeId))
        {
            Error = FString::Printf(TEXT("Unverified legacy ARM source: %s"), Suffix);
            return false;
        }

        int32 CanonicalCount = 0;
        for (const TObjectPtr<UGridCanonicalItemDefinitionAsset>& Candidate : Root->ItemDefinitions)
        {
            const UGridArmamentDefinitionAsset* Canonical =
                Cast<UGridArmamentDefinitionAsset>(Candidate.Get());
            if (Canonical && Canonical->ItemId == Source->ItemDefinitionId &&
                Canonical->ArchetypeId == Source->WeaponArchetypeId)
            {
                ++CanonicalCount;
            }
        }
        int32 ArchetypeIndex = INDEX_NONE;
        for (int32 I = 0; I < Out.Archetypes.Num(); ++I)
        {
            if (Out.Archetypes[I].ArchetypeId == Source->WeaponArchetypeId)
            {
                if (ArchetypeIndex != INDEX_NONE)
                {
                    Error = TEXT("Duplicate catalogue archetype");
                    return false;
                }
                ArchetypeIndex = I;
            }
        }
        FName VisualId = NAME_None;
        int32 BindingCount = 0;
        for (const FGridCanonicalWeaponVisualBinding& Binding :
            Root->WeaponVisualCatalogue->ArchetypeBindings)
        {
            if (Binding.ArchetypeId == Source->WeaponArchetypeId)
            {
                ++BindingCount;
                VisualId = Binding.VisualId;
            }
        }
        int32 VisualIndex = INDEX_NONE;
        for (int32 I = 0; I < Out.Visuals.Num(); ++I)
        {
            if (!VisualId.IsNone() && Out.Visuals[I].VisualId == VisualId)
            {
                if (VisualIndex != INDEX_NONE)
                {
                    Error = TEXT("Duplicate visual profile identity");
                    return false;
                }
                VisualIndex = I;
            }
        }
        if (CanonicalCount != 1 || ArchetypeIndex == INDEX_NONE ||
            BindingCount != 1 || VisualIndex == INDEX_NONE ||
            SeenVisuals.Contains(VisualId))
        {
            Error = FString::Printf(TEXT("Ambiguous canonical ARM join: %s"), Suffix);
            return false;
        }

        SeenItems.Add(ExpectedId);
        SeenArchetypes.Add(Source->WeaponArchetypeId);
        SeenVisuals.Add(VisualId);
        const bool bAlreadyEqual = Matches(
            *Source, Out.Visuals[VisualIndex], Out.Archetypes[ArchetypeIndex]);
        if (!Project(*Source, Out.Visuals[VisualIndex], Out.Archetypes[ArchetypeIndex]))
        {
            Error = FString::Printf(TEXT("Property copy/type parity failed: %s"), Suffix);
            return false;
        }
        Out.ChangedProfiles += bAlreadyEqual ? 0 : 1;
        ++Out.SourceCount;
    }

    if (Out.SourceCount != 22 || SeenItems.Num() != 22 ||
        SeenArchetypes.Num() != 22 || SeenVisuals.Num() != 22)
    {
        Error = TEXT("Manifest not bijective (22 item/archetype/visual)");
        return false;
    }
    return true;
}

/** Compare all 22 real sources with the current, reloaded catalogues. */
inline bool VerifyDiskProfiles(const UGridItemSystemAsset* Root, FString& Error)
{
    FPlan Target;
    if (!BuildPlan(Root, Target, Error)) { return false; }
    if (Target.ChangedProfiles != 0)
    {
        Error = FString::Printf(
            TEXT("%d physical ARM profiles differ after native reload"),
            Target.ChangedProfiles);
        return false;
    }
    return true;
}
} // namespace ARMCLEAN064X37
