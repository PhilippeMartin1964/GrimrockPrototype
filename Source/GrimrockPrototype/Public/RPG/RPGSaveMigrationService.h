#pragma once

#include "CoreMinimal.h"

class UGrimrockPartySaveGame;

struct FRPGSaveMigrationReport
{
    int32 SourceVersion = 0;
    int32 TargetVersion = 0;
    int32 ReconciledCharacterCount = 0;
    bool bMigrated = false;
};

/**
 * Versioned validation and migration for party RPG save data.
 */
struct GRIMROCKPROTOTYPE_API FRPGSaveMigrationService
{
    /**
     * Migrates compatible legacy snapshots to the current contract, then
     * validates the resulting data. MON18.8 preserves the explicit v5 path,
     * MON19.2.2 adds v6 -> v7 level variables and MON20.9.2 adds the explicit
     * v7 -> v8 transient SkillRanks snapshot migration.
     */
    static bool PrepareLoadedSave (
        UGrimrockPartySaveGame* SaveGame,
        FText& OutError,
        FRPGSaveMigrationReport* OutReport = nullptr);

    /** Strict validation of the current save contract. */
    static bool ValidateCurrentSave (
        UGrimrockPartySaveGame* SaveGame,
        FText& OutError);
};
