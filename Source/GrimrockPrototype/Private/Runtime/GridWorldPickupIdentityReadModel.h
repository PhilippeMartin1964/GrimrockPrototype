#pragma once

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridItemActor.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "GridWorldItemLevelReadModel.h"

/** The placed runtime entry and the actor's materialized ItemId describe
 * the physical object. An old DA_Item_* cannot rename a registered item
 * during pickup or SaveGame capture.
 *
 * Only unregistered identities retain their historical definition-first
 * lookup until ARM-CLEAN06.4 has converted the binary assets.
 */
struct FGridWorldPickupIdentityReadModel
{
    static FName Resolve(const AGridItemActor* ItemActor, FName EntryItemId, int32 ItemLevel)
    {
        if (!IsValid(ItemActor))
        {
            return EntryItemId;
        }

        const UGridItemDefinitionAsset* Legacy = ItemActor->GetItemDefinitionAsset();
        const FName LegacyItemId = IsValid(Legacy) ? Legacy->ItemDefinitionId : NAME_None;
        const FName PhysicalItemId = ItemActor->ItemDefinitionId;
        const UWorld* World = ItemActor->GetWorld();
        UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
        const UGridItemSubsystem* Registry = GameInstance
            ? GameInstance->GetSubsystem<UGridItemSubsystem>() : nullptr;

        const bool bKnownCanonical = Registry &&
            (Registry->HasCanonicalItem(EntryItemId) ||
             Registry->HasCanonicalItem(PhysicalItemId) ||
             Registry->HasCanonicalItem(LegacyItemId));
        if (bKnownCanonical)
        {
            const FName Candidate = !EntryItemId.IsNone() ? EntryItemId : PhysicalItemId;
            // An old definition can contradict both identities, but a
            // disagreement between the actual actor and the registered
            // runtime entry is unsafe: do not turn it into another item.
            if (Candidate.IsNone() || !Registry->HasCanonicalItem(Candidate) ||
                (!PhysicalItemId.IsNone() && PhysicalItemId != Candidate))
            {
                return NAME_None;
            }

            // ResolveItem alone permits non-ARM N1 when reading the
            // family. A physical persisted instance has the stricter
            // contract: armaments N1-N20, ammunition and other non-ARM N0.
            const FGridReceptacleItemMetadataReadView Physical =
                FGridWorldItemLevelReadModel::ResolvePersistedInstance(
                    Registry, nullptr, Candidate, ItemLevel);
            return Physical.bCanonicalIdentity && Physical.bResolved &&
                Physical.FixedItemLevel == ItemLevel ? Candidate : NAME_None;
        }

        // Preserve the exact old behavior for IDs not yet migrated.
        if (!LegacyItemId.IsNone())
        {
            return LegacyItemId;
        }
        if (!ItemActor->GetItemDefinitionId().IsNone())
        {
            return ItemActor->GetItemDefinitionId();
        }
        return EntryItemId;
    }
};
