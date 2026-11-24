#pragma once

#include "CoreMinimal.h"

class URPGClassAsset;
class UGridItemSubsystem;

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
    static bool IsProductionClassAsset(const URPGClassAsset* ClassAsset);

    /** ARMEMENT01.13.2: active New Game path, three approved N1 kits per class.
     * Recipes store canonical ItemIds only and resolve exclusively through
     * a configured UGridItemSubsystem, including Editor Automation tests.
     * Called by wizard previews and CreateInitialCharacter on transient copies.
     * Missing future support items are never fabricated.
     */
    static bool TryPopulateApprovedTransientClass(URPGClassAsset& TransientClass, FString& OutError,
        const UGridItemSubsystem* ItemRegistry = nullptr);
};
