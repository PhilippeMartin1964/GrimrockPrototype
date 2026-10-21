#pragma once

#include "CoreMinimal.h"

class URPGClassAsset;
class UGridItemDefinitionAsset;

/**
 * ARMEMENT01.9.2: deterministic, source-controlled starting-kit recipes for
 * the six production classes. This code never saves/modifies a production
 * DataAsset; it materializes kits only into a transient class copy.
 *
 * Explicitly authored StartingEquipmentKits always take precedence.
 */
class GRIMROCKPROTOTYPE_API FRPGStartingEquipmentPresetCatalog
{
public:
    /** Recover starter item definitions independently of dungeon placement. */
    static UGridItemDefinitionAsset* ResolveKnownStartingItemDefinition(FName ItemDefinitionId);

    static bool IsProductionClassAsset(const URPGClassAsset* ClassAsset);

    /** Populate an initially unconfigured transient class. Transactional. */
    static bool TryPopulateTransientClass(URPGClassAsset& TransientClass, FString& OutError);
};
