#pragma once
#if WITH_DEV_AUTOMATION_TESTS
#include "Runtime/Items/GridCanonicalItemCatalogues.h"
#include "Runtime/Items/GridCanonicalInventoryCategoryTags.h"

/** Production tests accept only the two explicit X.61 registry states.
 * Exact append order/classes and existing identities are checked, not a
 * permissive lower bound. Original content tests still check their own rows.
 */
namespace GridCanonicalLootRegistryTestUtils
{
    inline bool HasExactRegistryShape(const UGridItemSystemAsset& Root)
    {
        const int32 Count = Root.ItemDefinitions.Num();
        if (Count != 34 && Count != 38) return false;
        const FName IDs[] = {TEXT("GoblinKnife"), TEXT("EmptyVial"), TEXT("Item_RatMeat"), TEXT("Item_RatTooth")};
        TSet<FName> Seen;
        for (int32 I = 0; I < Count; ++I)
        {
            const auto* D = Root.ItemDefinitions[I].Get();
            if (!IsValid(D) || D->ItemId.IsNone() || Seen.Contains(D->ItemId)) return false;
            Seen.Add(D->ItemId);
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
