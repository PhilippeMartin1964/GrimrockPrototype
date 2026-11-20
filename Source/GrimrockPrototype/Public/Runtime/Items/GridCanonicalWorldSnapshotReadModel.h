#pragma once

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridDungeonRuntimeState.h"
#include "Runtime/Items/GridItemSubsystem.h"

/** Read-only physical identity gate before saved world data can mutate the live dungeon.
 * Old serialized pointers cannot authorize, disambiguate or replace an ItemId.
 */
struct FGridCanonicalWorldSnapshotReadModel
{
    static const UGridItemSubsystem* GetRegistry(const UWorld* World)
    {
        UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
        return GI ? GI->GetSubsystem<UGridItemSubsystem>() : nullptr;
    }

    static bool ValidateLevel(const UGridItemSubsystem* Registry,
        const FGridLevelRuntimeState& State, FString& Error)
    {
        Error.Reset();
        TSet<FGuid> Seen;
        auto Check = [&](const FGuid& Key, const FGridRuntimeItemState& Item,
            const FGuid& ReceptacleId, bool bPending)
        {
            FGridResolvedItem Resolved;
            const bool bContained = ReceptacleId.IsValid();
            if (!Key.IsValid() || Key != Item.ObjectId || Seen.Contains(Key) ||
                Item.bIsContainedInReceptacle != bContained ||
                Item.ReceptacleObjectId != ReceptacleId ||
                ((bPending || bContained) && Item.bEmbeddedProjectile) ||
                Item.ItemDefinitionId.IsNone() || Item.Quantity < 1 ||
                !IsValid(Registry) || !Registry->IsConfigured() ||
                !Registry->ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, Resolved) ||
                !Resolved.IsValid() || Resolved.ItemId != Item.ItemDefinitionId ||
                Resolved.ItemLevel != Item.ItemLevel ||
                (Resolved.bIsArmament ? Item.ItemLevel < 1 : Item.ItemLevel != 0) ||
                Item.Quantity > FMath::Max(1, Resolved.MaxStack) ||
                (Item.bEmbeddedProjectile && (Item.Quantity != 1 ||
                    Resolved.bIsArmament || Resolved.AmmoKindId.IsNone() ||
                    !Item.EmbeddedMonsterPersistenceId.IsValid())))
            {
                Error = FString::Printf(TEXT("Level=%s Object=%s Item=%s N%d Quantity=%d: invalid canonical world snapshot"),
                    *State.LevelId.ToString(), *Key.ToString(), *Item.ItemDefinitionId.ToString(),
                    Item.ItemLevel, Item.Quantity);
                return false;
            }
            Seen.Add(Key);
            return true;
        };
        for (const auto& Pair : State.Items)
            if (!Check(Pair.Key, Pair.Value, FGuid(), false)) return false;
        for (const auto& Pair : State.PendingInboundItems)
            if (!Check(Pair.Key, Pair.Value.ItemState, FGuid(), true)) return false;
        for (const auto& Pair : State.Receptacles)
        {
            if (!Pair.Key.IsValid() || Pair.Key != Pair.Value.ObjectId)
            {
                Error = FString::Printf(TEXT("Level=%s Receptacle=%s: invalid saved owner"),
                    *State.LevelId.ToString(), *Pair.Key.ToString());
                return false;
            }
            for (const FGridRuntimeItemState& Item : Pair.Value.ContainedItems)
                if (!Check(Item.ObjectId, Item, Pair.Key, false)) return false;
        }
        return true;
    }

    static bool ValidateDungeon(const UGridItemSubsystem* Registry,
        const FGridDungeonRuntimeState& Dungeon, FString& Error)
    {
        Error.Reset();
        for (const auto& Pair : Dungeon.LevelStates)
            if (!ValidateLevel(Registry, Pair.Value, Error)) return false;
        return true;
    }
};
