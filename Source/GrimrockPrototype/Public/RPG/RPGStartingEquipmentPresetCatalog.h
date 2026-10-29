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

    /** ARMEMENT01.13: prepare three approved N1 orientations on a transient class.
     * Not used by New Game until localized ST_UI and regression tests are validated.
     * Missing future support items are never fabricated.
     */
    static bool TryPopulateApprovedTransientClass(URPGClassAsset& TransientClass, FString& OutError);
};
