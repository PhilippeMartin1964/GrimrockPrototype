#pragma once

#include "../RPG/GridStartingKitStaticMetadataReadModel.h"

/**
 * Read-only frontend projection of a starting equipment entry. Registered
 * entries require an explicit canonical ID and physical N1–N20 / N0.
 * No ItemInstance, UPackage or mutable starting kit is touched.
 */
struct FGridStartingKitDisplayReadView
{
    bool bRegisteredIdentity = false;
    bool bResolved = false;
    int32 FixedItemLevel = 0;
    FText DisplayName;
};

struct FGridStartingKitDisplayReadModel
{
    static FGridStartingKitDisplayReadView Resolve(
        const UGridItemSubsystem* Registry, int32 AuthoredItemLevel, FName ItemId)
    {
        FGridStartingKitDisplayReadView View;
        const auto Metadata = FGridStartingKitStaticMetadataReadModel::Resolve(Registry, ItemId, AuthoredItemLevel);
        View.bRegisteredIdentity = Metadata.bRegisteredIdentity;
        if (Metadata.bResolved)
        {
            View.bResolved = true;
            View.FixedItemLevel = AuthoredItemLevel;
            View.DisplayName = Metadata.DisplayName;
        }
        return View;
    }
};
