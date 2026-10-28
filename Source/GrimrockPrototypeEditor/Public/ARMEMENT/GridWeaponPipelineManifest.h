#pragma once

#include "CoreMinimal.h"

/** A production row contains identities and explicit UE paths, never gameplay numbers. */
struct FGridWeaponPipelineRow
{
    FString ArchetypeId;
    FString ProductionState;
    FString BuildKind;
    FString ItemDefinitionId;
    FString ItemPath;
    FString AttackId;
    FString VisualId;
    FString VisualFamily;
    FString IconPath;
    FString MeshPath;
    FString MaterialPath;
    FString BaseColorPath;
    FString NormalPath;
    FString OrmPath;
    FString ClassId;
    FString EquipmentSlots;

    bool IsPassive() const { return BuildKind == TEXT("ShieldPassive"); }
    bool IsThrowable() const { return BuildKind == TEXT("ThrowingOneHand"); }
    bool IsMagicAttack() const
    {
        return BuildKind == TEXT("MagicOneHand") || BuildKind == TEXT("MagicTwoHand");
    }
    bool IsTwoHanded() const
    {
        return BuildKind == TEXT("MeleeTwoHand") || BuildKind == TEXT("Ranged") ||
            BuildKind == TEXT("MagicTwoHand");
    }
    bool UsesRangedAction() const
    {
        return BuildKind == TEXT("Ranged") || IsThrowable() || IsMagicAttack();
    }
    bool IsSupportedBuildKind() const
    {
        return BuildKind == TEXT("MeleeOneHand") || BuildKind == TEXT("MeleeTwoHand") ||
            BuildKind == TEXT("Ranged") || IsThrowable() || IsMagicAttack() || IsPassive();
    }
};

/** Strict shared reader for the single production manifest; read-only. */
class FGridWeaponPipelineManifest
{
public:
    static FString DefaultFile();
    static bool Load(const FString& File, TArray<FGridWeaponPipelineRow>& Out,
        FString& Error);
    static const FGridWeaponPipelineRow* Find(
        const TArray<FGridWeaponPipelineRow>& Rows, const FString& ArchetypeId);
};
