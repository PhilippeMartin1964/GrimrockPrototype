#pragma once
#if WITH_DEV_AUTOMATION_TESTS
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridCanonicalInventoryCategoryTags.h"

/** Production tests accept only the explicit X.61 and X.66 registry states.
 * Exact append order/classes and existing identities are checked, not a
 * permissive lower bound. Original content tests still check their own rows.
 */
namespace GridCanonicalLootRegistryTestUtils
{
    inline bool HasExactRegistryShape(const UGridItemSystemAsset& Root)
    {
        const int32 Count = Root.ItemDefinitions.Num();
        if (Count != 34 && Count != 38 && Count != 48) return false;
        const FName IDs[] = {TEXT("GoblinKnife"), TEXT("EmptyVial"), TEXT("Item_RatMeat"), TEXT("Item_RatTooth")};
        const FName AlchemyIds[] = {TEXT("Item_Antidote"), TEXT("Item_Bomb_Fire"), TEXT("Item_Bomb_Toxic"),
            TEXT("Item_DefensiveElixir_Fire"), TEXT("Item_DefensiveElixir_Ice"),
            TEXT("Item_DefensiveElixir_Lightning"), TEXT("Item_DefensiveElixir_Poison"),
            TEXT("Item_Flask_Acid"), TEXT("Item_Flask_CorrosiveCloud"), TEXT("Item_Panacea")};
        const FName ActionIds[] = {TEXT("Action_Alchemist_Antidote"), TEXT("Action_Alchemist_FireBomb"),
            TEXT("Action_Alchemist_ToxicBomb"), TEXT("Action_Alchemist_DefensiveElixir_Fire"),
            TEXT("Action_Alchemist_DefensiveElixir_Ice"), TEXT("Action_Alchemist_DefensiveElixir_Lightning"),
            TEXT("Action_Alchemist_DefensiveElixir_Poison"), TEXT("Action_Alchemist_AcidFlask"),
            TEXT("Action_Alchemist_CorrosiveCloud"), TEXT("Action_Alchemist_Panacea")};
        if (Count == 48 && (!Root.ActionCatalogue ||
            Root.ActionCatalogue->DetailedActions.Num() != 14)) return false;
        TSet<FName> Seen;
        for (int32 I = 0; I < Count; ++I)
        {
            const auto* D = Root.ItemDefinitions[I].Get();
            if (!IsValid(D) || D->ItemId.IsNone() || Seen.Contains(D->ItemId)) return false;
            Seen.Add(D->ItemId);
            if (I < 38)
                for (const FName Id : AlchemyIds) if (D->ItemId == Id) return false;
            if (I >= 38)
            {
                const int32 J = I - 38;
                const auto* C = Cast<UGridConsumableDefinitionAsset>(D);
                const bool bPotion = J == 0 || (J >= 3 && J <= 6) || J == 9;
                if (!C || C->ItemId != AlchemyIds[J] || C->WeightKg != 0.f || C->MaxStack != 1 ||
                    C->UseActions.Num() != 1 || C->UseActions[0].ActionId != ActionIds[J] ||
                    C->UseActions[0].PresentationId != FName(TEXT("PRES_AttackNoFX")) ||
                    C->Tags.Contains(GridCanonicalInventoryCategoryTags::Potion()) != bPotion ||
                    Root.ActionCatalogue->DetailedActions[4+J].Definition.ActionId != ActionIds[J])
                    return false;
                continue;
            }
            if (I < 34)
            {
                for (const FName Id : IDs) if (D->ItemId == Id) return false;
                continue;
            }
            const int32 J = I-34;
            const UClass* Class = J == 0 ? UGridNonArmamentDefinitionAsset::StaticClass() :
                J == 2 ? UGridConsumableDefinitionAsset::StaticClass() :
                UGridGenericStackableDefinitionAsset::StaticClass();
            const FName Tag = J == 0 ? GridCanonicalInventoryCategoryTags::Weapon() :
                J == 2 ? GridCanonicalInventoryCategoryTags::Food() : GridCanonicalInventoryCategoryTags::Component();
            if (D->ItemId != IDs[J] || D->GetClass() != Class ||
                D->Tags != TArray<FName>{Tag}) return false;
        }
        return true;
    }
}
#endif
