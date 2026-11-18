#pragma once

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"

// 5D: these readers must work when the approved kit and saved instance
// carry only their durable ItemId/level, without a historical DataAsset.
struct FGridCanonicalExplorationItemReadModel
{
    struct FSurface
    {
        bool bResolved = false;
        bool bOwned = false;
        FGridCombatActionDefinition Action;
        TSoftObjectPtr<UStaticMesh> Mesh;
    };

    static const UGridItemSubsystem* RegistryFor(const UGridPartyInventoryComponent* Inventory)
    {
        const UWorld* World = Inventory ? Inventory->GetWorld() : nullptr;
        UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
        return GI ? GI->GetSubsystem<UGridItemSubsystem>() : nullptr;
    }

    static bool HasDisarmTool(const UGridItemSubsystem* Registry,
        const UGridItemDefinitionAsset* Old, const FGridCharacterInventoryState& Character, FName Tag)
    {
        const FName Id(TEXT("Item_DisarmTools_01"));
        if (Tag.IsNone()) return false;
        const bool bCanonical = Registry && Registry->HasCanonicalItem(Id);
        for (const auto& Slot : Character.InventorySlots)
        {
            if (Slot.IsEmpty() || Slot.Item.ItemDefinitionId != Id || !Slot.Item.IsValid()) continue;
            if (bCanonical)
            {
                FGridResolvedItem Resolved;
                if (Slot.Item.ItemLevel == 0 && Registry->ResolveItem(Id, Slot.Item.ItemLevel, Resolved) &&
                    Resolved.IsValid() && !Resolved.bIsArmament &&
                    Slot.Item.Quantity <= FMath::Max(1, Resolved.MaxStack) && Resolved.Tags.Contains(Tag))
                    return true;
            }
            else if (IsValid(Old) && Old->ItemDefinitionId == Id && Old->ItemTags.Contains(Tag))
                return true;
        }
        return false;
    }

    static bool HasDisarmTool(const UGridPartyInventoryComponent* Inventory, int32 Index, FName Tag)
    {
        if (!IsValid(Inventory) || !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(Index)) return false;
        return HasDisarmTool(RegistryFor(Inventory), Inventory->FindItemDefinition(TEXT("Item_DisarmTools_01")),
            Inventory->PartyInventoryState.ActiveCharacters[Index], Tag);
    }

    static bool IsSurfaceItem(FName Id)
    {
        return Id == FName(TEXT("Item_Flask_Oil")) || Id == FName(TEXT("Item_SurfaceCatalyst_01"));
    }

    // Both approved surface consumables are unlevelled. With an empty
    // inventory, resolve N0 solely to display a disabled shortcut; no
    // virtual owned instance is invented. Execution requires a real row.
    static FSurface ResolveSurface(const UGridItemSubsystem* Registry,
        const UGridItemDefinitionAsset* Old, const FGridCharacterInventoryState& Character,
        FName Id, FName ActionId, bool bRequireOwned)
    {
        FSurface Out;
        if (Id.IsNone() || ActionId.IsNone()) return Out;
        const bool bCanonical = Registry && Registry->HasCanonicalItem(Id);
        FGridResolvedItem Item;
        if (bCanonical)
        {
            if (!IsSurfaceItem(Id) || !Registry->ResolveItem(Id, 0, Item) ||
                !Item.IsValid() || Item.bIsArmament || Item.AvailableActions.Num() != 1 ||
                Item.AvailableActions[0].ActionId != ActionId ||
                !Registry->ResolveDetailedNonArmAction(Id, 0, ActionId, Out.Action) ||
                Out.Action.SourcePolicy != EGridCombatActionSourcePolicy::QuickItem)
                return FSurface();
            Out.Mesh = Item.WorldMesh;
        }
        else
        {
            if (!IsValid(Old) || Old->ItemDefinitionId != Id ||
                !Old->BuildQuickItemCombatActionDefinition(Out.Action) ||
                Out.Action.ActionId != ActionId)
                return FSurface();
            Out.Mesh = Old->WorldMesh;
        }
        for (const auto& Slot : Character.InventorySlots)
        {
            if (!Slot.bOccupied || Slot.Item.ItemDefinitionId != Id) continue;
            if (Slot.IsEmpty() || !Slot.Item.IsValid() ||
                (bCanonical && (Slot.Item.ItemLevel != 0 ||
                    Slot.Item.Quantity > FMath::Max(1, Item.MaxStack))))
                return FSurface(); // Known invalid owned source never gets a fallback.
            Out.bOwned = true;
        }
        if (bRequireOwned && !Out.bOwned) return FSurface();
        Out.bResolved = true;
        return Out;
    }

    static FSurface ResolveSurface(const UGridPartyInventoryComponent* Inventory,
        int32 Index, FName Id, FName ActionId, bool bRequireOwned)
    {
        if (!IsValid(Inventory) || !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(Index))
            return FSurface();
        return ResolveSurface(RegistryFor(Inventory), Inventory->FindItemDefinition(Id),
            Inventory->PartyInventoryState.ActiveCharacters[Index], Id, ActionId, bRequireOwned);
    }

    // Combat visuals run after consumption, hence intentionally use the
    // approved static N0 definition, not the now-removed inventory row.
    static TSoftObjectPtr<UStaticMesh> ResolveSurfaceMesh(const UGridPartyInventoryComponent* Inventory, FName Id)
    {
        if (!IsValid(Inventory)) return {};
        const auto* Registry = RegistryFor(Inventory);
        if (Registry && Registry->HasCanonicalItem(Id))
        {
            FGridResolvedItem Item;
            return IsSurfaceItem(Id) && Registry->ResolveItem(Id, 0, Item) && Item.IsValid()
                ? Item.WorldMesh : TSoftObjectPtr<UStaticMesh>();
        }
        const auto* Old = Inventory->FindItemDefinition(Id);
        return IsValid(Old) && Old->ItemDefinitionId == Id ? Old->WorldMesh : TSoftObjectPtr<UStaticMesh>();
    }
};
