#pragma once

#include "CoreMinimal.h"
#include "Core/GridObjectPaletteAsset.h"

class UGridItemSubsystem;

/** Read-only canonical palette planning; no writes and no native asset creation. */
enum class EGridWeaponPipelinePaletteState : uint8
{
    Missing,
    PresentSame,
    Conflict
};

class FGridWeaponPipelinePalette
{
public:
    static EGridWeaponPipelinePaletteState Inspect(
        const UGridObjectPaletteAsset& Palette, FName ItemId,
        const UGridItemSubsystem* Registry, bool bAllowItemsCategory,
        FString& Detail);

    // New entries require an explicit physical level; existing authored levels
    // are validated and preserved independently of this new-entry default.
    static bool Propose(
        const UGridObjectPaletteAsset& Palette, FName ItemId,
        const UGridItemSubsystem* Registry, int32 NewPhysicalLevel, bool bAllowItemsCategory,
        TArray<FGridObjectPaletteEntry>& Out, bool& bAdded, FString& Error);
};
