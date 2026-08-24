#pragma once

#include "CoreMinimal.h"
#include "RPGSkillPersistence.generated.h"

class URPGSkillAsset;
struct FGridPartyInventoryState;

/** One persisted positive rank. Runtime-only presentation/capability data is excluded. */
USTRUCT (BlueprintType)
struct GRIMROCKPROTOTYPE_API FRPGSkillRankSaveState
{
    GENERATED_BODY ()

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "RPG|Skills|Save")
    FName SkillId = NAME_None;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "RPG|Skills|Save")
    int32 Rank = 0;

    bool IsValid () const
    {
        return !SkillId.IsNone () && Rank > 0;
    }
};

/** Sparse MON20.9 skill snapshot for one stable party CharacterId. */
USTRUCT (BlueprintType)
struct GRIMROCKPROTOTYPE_API FRPGCharacterSkillSaveState
{
    GENERATED_BODY ()

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "RPG|Skills|Save")
    FGuid CharacterId;

    UPROPERTY (VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "RPG|Skills|Save")
    TArray<FRPGSkillRankSaveState> SkillRanks;
};

/**
 * Explicit persistence boundary for transient character SkillRanks.
 * Snapshots are sparse, CharacterId-keyed, deterministic and restored atomically.
 */
struct GRIMROCKPROTOTYPE_API FRPGSkillPersistence
{
    static bool CapturePartySkills (
        const FGridPartyInventoryState& PartyState,
        TArray<FRPGCharacterSkillSaveState>& OutSavedStates,
        FString& OutError);

    /** Test seam and deterministic capture with an injected canonical resolver. */
    static bool CapturePartySkills (
        const FGridPartyInventoryState& PartyState,
        TFunctionRef<const URPGSkillAsset* (FName)> DefinitionResolver,
        TArray<FRPGCharacterSkillSaveState>& OutSavedStates,
        FString& OutError);

    static bool ValidateSavedPartySkills (
        const FGridPartyInventoryState& PartyState,
        const TArray<FRPGCharacterSkillSaveState>& SavedStates,
        FString& OutError);

    /** Test seam and deterministic validation with an injected canonical resolver. */
    static bool ValidateSavedPartySkills (
        const FGridPartyInventoryState& PartyState,
        const TArray<FRPGCharacterSkillSaveState>& SavedStates,
        TFunctionRef<const URPGSkillAsset* (FName)> DefinitionResolver,
        FString& OutError);

    /**
     * Atomically replaces runtime SkillRanks in active and pooled characters.
     * InOutPartyState is untouched if any snapshot or canonical definition fails.
     */
    static bool RestorePartySkills (
        FGridPartyInventoryState& InOutPartyState,
        const TArray<FRPGCharacterSkillSaveState>& SavedStates,
        FString& OutError);

    /** Test seam and deterministic restore with an injected canonical resolver. */
    static bool RestorePartySkills (
        FGridPartyInventoryState& InOutPartyState,
        const TArray<FRPGCharacterSkillSaveState>& SavedStates,
        TFunctionRef<const URPGSkillAsset* (FName)> DefinitionResolver,
        FString& OutError);

    /** Resolves the canonical RPGSkill:<SkillId> definition used by production save/load. */
    static const URPGSkillAsset* ResolveDefinitionBySkillId (FName SkillId);
};
