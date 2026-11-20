#pragma once

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridPartyInventoryComponent.h"
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

    static bool IsDisarmToolInstance(const UGridItemSubsystem* Registry,
        const FGridItemInstance& Item, FName Tag)
    {
        const FName Id(TEXT("Item_DisarmTools_01"));
        FGridResolvedItem Tool;
        return Registry && !Tag.IsNone() && Item.IsValid() &&
            Item.ItemDefinitionId == Id && Item.ItemLevel == 0 &&
            Registry->ResolveItem(Id, 0, Tool) && Tool.IsValid() &&
            !Tool.bIsArmament && Tool.Tags.Contains(Tag) &&
            Item.Quantity <= FMath::Max(1, Tool.MaxStack);
    }

    static bool HasDisarmTool(const UGridItemSubsystem* Registry,
        const FGridCharacterInventoryState& Character, FName Tag)
    {
        for (const auto& Slot : Character.InventorySlots)
        {
            if (!Slot.IsEmpty() && IsDisarmToolInstance(Registry, Slot.Item, Tag))
                return true;
        }
        return false;
    }

    static bool HasDisarmTool(const UGridPartyInventoryComponent* Inventory, int32 Index, FName Tag)
    {
        if (!IsValid(Inventory) || !Inventory->PartyInventoryState.ActiveCharacters.IsValidIndex(Index)) return false;
        return HasDisarmTool(RegistryFor(Inventory), Inventory->PartyInventoryState.ActiveCharacters[Index], Tag);
    }

    static bool IsSurfaceItem(FName Id)
    {
        return Id == FName(TEXT("Item_Flask_Oil")) || Id == FName(TEXT("Item_SurfaceCatalyst_01"));
    }

    // Both approved surface consumables are unlevelled. With an empty
    // inventory, resolve N0 solely to display a disabled shortcut; no
    // virtual owned instance is invented. Execution requires a real row.
    static FSurface ResolveSurface(const UGridItemSubsystem* Registry,
        const FGridCharacterInventoryState& Character, FName Id, FName ActionId, bool bRequireOwned)
    {
        FSurface Out;
        FGridResolvedItem Item;
        if (!Registry || !IsSurfaceItem(Id) || ActionId.IsNone() ||
            !Registry->ResolveItem(Id, 0, Item) || !Item.IsValid() || Item.bIsArmament ||
            Item.AvailableActions.Num() != 1 || Item.AvailableActions[0].ActionId != ActionId ||
            !Registry->ResolveDetailedNonArmAction(Id, 0, ActionId, Out.Action) ||
            Out.Action.SourcePolicy != EGridCombatActionSourcePolicy::QuickItem)
            return FSurface();
        Out.Mesh = Item.WorldMesh;
        for (const auto& Slot : Character.InventorySlots)
        {
            if (!Slot.bOccupied || Slot.Item.ItemDefinitionId != Id) continue;
            if (Slot.IsEmpty() || !Slot.Item.IsValid() ||
                Slot.Item.ItemLevel != 0 || Slot.Item.Quantity > FMath::Max(1, Item.MaxStack))
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
        return ResolveSurface(RegistryFor(Inventory),
            Inventory->PartyInventoryState.ActiveCharacters[Index], Id, ActionId, bRequireOwned);
    }

    // Combat visuals run after consumption, hence intentionally use the
    // approved static N0 definition, not the now-removed inventory row.
    static TSoftObjectPtr<UStaticMesh> ResolveSurfaceMesh(const UGridItemSubsystem* Registry, FName Id)
    {
        FGridResolvedItem Item;
        return Registry && IsSurfaceItem(Id) && Registry->ResolveItem(Id, 0, Item) &&
            Item.IsValid() && !Item.bIsArmament ? Item.WorldMesh : TSoftObjectPtr<UStaticMesh>();
    }

    static TSoftObjectPtr<UStaticMesh> ResolveSurfaceMesh(const UGridPartyInventoryComponent* Inventory, FName Id)
    {
        return IsValid(Inventory) ? ResolveSurfaceMesh(RegistryFor(Inventory), Id) : TSoftObjectPtr<UStaticMesh>();
    }
};
