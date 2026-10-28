#pragma once

#include "CoreMinimal.h"
#include "Core/GridObjectPaletteAsset.h"

class UGridItemDefinitionAsset;

/** Pure, non-destructive palette planning; no writes and no asset creation. */
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
        const UGridObjectPaletteAsset& Palette, FName EntryId,
        const UGridItemDefinitionAsset* Item, bool bAllowLegacyItems,
        FString& Detail);

    static bool ProposeDirectItem(
        const UGridObjectPaletteAsset& Palette, UGridItemDefinitionAsset* Item,
        FName Category, TArray<FGridObjectPaletteEntry>& Out, bool& bAdded,
        FString& Error);

    static bool Propose(
        const UGridObjectPaletteAsset& Palette, FName EntryId,
        UGridItemDefinitionAsset* Item, bool bAllowLegacyItems,
        TArray<FGridObjectPaletteEntry>& Out, bool& bAdded, FString& Error);
};
