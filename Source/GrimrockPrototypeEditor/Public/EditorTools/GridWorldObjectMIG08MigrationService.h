#pragma once

#include "CoreMinimal.h"

class UGridLevelAsset;
class UGridObjectPaletteAsset;

struct GRIMROCKPROTOTYPEEDITOR_API FGridWorldObjectMIG08MigrationResult
{
	bool bChanged = false;
	TArray<FString> Changes;
	TArray<FString> Warnings;
	TArray<FString> Errors;

	bool HasErrors() const
	{
		return !Errors.IsEmpty();
	}
};

/**
 * WORLDOBJ-MIG08 source-only migration helpers.
 *
 * These helpers mutate loaded UObject instances only. Persisting the changes is
 * deliberately owned by the editor commandlet so unit tests can exercise the
 * migration without ever touching real .uasset files.
 */
class GRIMROCKPROTOTYPEEDITOR_API FGridWorldObjectMIG08MigrationService
{
public:
	static FGridWorldObjectMIG08MigrationResult MigrateLevelAsset(UGridLevelAsset& LevelAsset);
	static FGridWorldObjectMIG08MigrationResult MigratePaletteAsset(UGridObjectPaletteAsset& PaletteAsset);
};
