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
 * MON15.6 versioned validation and migration for RPG progression save data.
 */
struct GRIMROCKPROTOTYPE_API FRPGSaveMigrationService
{
    /**
     * Migrates v1-v3 snapshots to the current contract, then validates the
     * resulting progression data. Current-version snapshots are validated
     * without silently repairing corruption.
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
