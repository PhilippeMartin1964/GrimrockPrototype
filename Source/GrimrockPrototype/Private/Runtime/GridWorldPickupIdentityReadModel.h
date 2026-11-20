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
 * Historical definition-first lookup is restricted to isolated
 * registry-less Automation fixtures.
 */
struct FGridWorldPickupIdentityReadModel
{
    static FName Resolve(const AGridItemActor* ItemActor, FName EntryItemId, int32 ItemLevel)
    {
        if (!IsValid(ItemActor))
        {
            return EntryItemId;
        }

        const FName PhysicalItemId = ItemActor->ItemDefinitionId;
        const UWorld* World = ItemActor->GetWorld();
        UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
        const UGridItemSubsystem* Registry = GameInstance
            ? GameInstance->GetSubsystem<UGridItemSubsystem>() : nullptr;

        if (Registry)
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

            // Preserve the persisted physical level: armaments N1-N20,
            // ammunition and other non-ARM N0.
            const FGridReceptacleItemMetadataReadView Physical =
                FGridWorldItemLevelReadModel::ResolvePersistedInstance(
                    Registry, nullptr, Candidate, ItemLevel);
            return Physical.bCanonicalIdentity && Physical.bResolved &&
                Physical.FixedItemLevel == ItemLevel ? Candidate : NAME_None;
        }

        // Explicit canonical initialization in an isolated world has
        // a trustworthy resolved projection, not a legacy definition.
        const FGridResolvedItem& Projected = ItemActor->CanonicalWorldPresentation;
        if (!GameInstance && Projected.IsValid() && !PhysicalItemId.IsNone() &&
            Projected.ItemId == PhysicalItemId && Projected.ItemLevel == ItemLevel &&
            ItemActor->CanonicalWorldItemLevel == ItemLevel &&
            (EntryItemId.IsNone() || EntryItemId == PhysicalItemId))
        {
            return PhysicalItemId;
        }
#if WITH_DEV_AUTOMATION_TESTS
        if (!GameInstance && GIsAutomationTesting)
        {
            const UGridItemDefinitionAsset* Legacy = ItemActor->GetItemDefinitionAsset();
            if (IsValid(Legacy) && !Legacy->ItemDefinitionId.IsNone()) return Legacy->ItemDefinitionId;
            return !PhysicalItemId.IsNone() ? PhysicalItemId : EntryItemId;
        }
#endif
        return NAME_None;
    }
};
