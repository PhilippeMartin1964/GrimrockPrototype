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
     * validates the resulting data. MON18.8 handles v5 -> v6 explicitly so
     * authoritative MON15/MON16 state is never routed through v1-v3 repair.
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
