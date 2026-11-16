#pragma once

#include "Core/GridObjectBehavior.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** X.16.5: Pure, read-only lock key proposal.
 * Existing authored AcceptedKeyIds must remain untouched: lock runtime
 * intentionally unions both AcceptedKeyItems and AcceptedKeyIds.
 */
struct FGridLockKeyPreviewStats
{
    int32 PointerSources = 0;
    int32 ExplicitIds = 0;
    int32 AlreadyCovered = 0;
    int32 NewlyProposed = 0;
    int32 MissingPointer = 0;
    int32 UnknownPointer = 0;
    bool bRegistryUnavailable = false;

    bool HasBlockers() const
    {
        return bRegistryUnavailable || MissingPointer > 0 || UnknownPointer > 0;
    }
    void Accumulate(const FGridLockKeyPreviewStats& Other)
    {
        PointerSources += Other.PointerSources;
        ExplicitIds += Other.ExplicitIds;
        AlreadyCovered += Other.AlreadyCovered;
        NewlyProposed += Other.NewlyProposed;
        MissingPointer += Other.MissingPointer;
        UnknownPointer += Other.UnknownPointer;
        bRegistryUnavailable = bRegistryUnavailable || Other.bRegistryUnavailable;
    }
    FString Describe() const
    {
        return FString::Printf(TEXT(
            "pointers=%d explicitIds=%d covered=%d proposed=%d "
            "missingPointers=%d unknownPointers=%d registryUnavailable=%d"),
            PointerSources, ExplicitIds, AlreadyCovered, NewlyProposed,
            MissingPointer, UnknownPointer, bRegistryUnavailable ? 1 : 0);
    }
};

struct FGridLockKeyMigrationPreview
{
    static bool TryPreview(const UGridItemSubsystem* Registry,
        const TArray<TObjectPtr<UGridItemDefinitionAsset>>& ExistingPointers,
        const TArray<FName>& ExistingExplicitIds, TArray<FName>& OutProposedIds,
        FGridLockKeyPreviewStats& OutStats)
    {
        OutProposedIds.Reset();
        OutStats = FGridLockKeyPreviewStats();
        OutStats.PointerSources = ExistingPointers.Num();
        OutStats.ExplicitIds = ExistingExplicitIds.Num();

        if (!IsValid(Registry) || !Registry->IsConfigured())
        {
            OutStats.bRegistryUnavailable = true;
            return false;
        }
        // Preserve user-authored IDs (even IDs outside ItemSystem), their
        // order, duplicates and NAME_None entries exactly. Current runtime
        // ignores None and AddUnique's duplicates without changing any IDs.
        TArray<FName> Draft = ExistingExplicitIds;
        for (const TObjectPtr<UGridItemDefinitionAsset>& Pointer : ExistingPointers)
        {
            const UGridItemDefinitionAsset* Source = Pointer.Get();
            if (!IsValid(Source) || Source->ItemDefinitionId.IsNone())
            {
                ++OutStats.MissingPointer;
                continue;
            }
            if (!Registry->HasCanonicalItem(Source->ItemDefinitionId))
            {
                ++OutStats.UnknownPointer;
                continue;
            }
            if (Draft.Contains(Source->ItemDefinitionId))
            {
                ++OutStats.AlreadyCovered;
            }
            else
            {
                Draft.Add(Source->ItemDefinitionId);
                ++OutStats.NewlyProposed;
            }
        }

        if (OutStats.HasBlockers())
        {
            return false; // Never emit partial new AcceptedKeyIds.
        }
        OutProposedIds = MoveTemp(Draft);
        return true;
    }
};
