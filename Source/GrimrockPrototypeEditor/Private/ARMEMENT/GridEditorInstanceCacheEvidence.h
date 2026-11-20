#pragma once

#include "Runtime/GridInventoryTypes.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/UnrealType.h"

/** Editor-only proof. Never normalize the live inventory or save slots. */
namespace GridEditorInstanceCacheEvidence
{
    struct FCounts
    {
        int32 Instances = 0;
        int32 Invalid = 0;
        int32 NameConflicts = 0;
        int32 WeightConflicts = 0;
        int32 CachedNames = 0;
        int32 CachedWeights = 0;
        bool CanClear() const { return Invalid == 0 && NameConflicts == 0 && WeightConflicts == 0; }
    };

    template<typename TParty, typename TVisitor>
    void Visit(TParty& Party, TVisitor Visitor)
    {
        for (auto* Group : { &Party.ActiveCharacters, &Party.CharacterPool })
            for (auto& Character : *Group)
                for (auto& Slot : Character.InventorySlots) Visitor(Slot.Item);
        for (auto& Equipment : Party.ActiveEquipment)
            for (TFieldIterator<FStructProperty> Field(FGridCharacterEquipmentState::StaticStruct()); Field; ++Field)
                if (Field->Struct == FGridItemInstance::StaticStruct())
                    Visitor(*Field->ContainerPtrToValuePtr<FGridItemInstance>(&Equipment));
        Visitor(Party.CursorItem);
    }

    inline FCounts Inspect(const FGridPartyInventoryState& Party, const UGridItemSubsystem& Registry)
    {
        FCounts Counts;
        Visit(Party, [&](const FGridItemInstance& Item)
        {
            const bool bName = !Item.DisplayName.IsEmpty();
            const bool bWeight = Item.Weight != 0.f;
            Counts.CachedNames += bName ? 1 : 0;
            Counts.CachedWeights += bWeight ? 1 : 0;
            if (!Item.RuntimeObjectId.IsValid() && Item.ItemDefinitionId.IsNone() &&
                !Item.ReadableContentAsset && Item.ReadableContentId.IsNone() && !bName && !bWeight)
                return;
            ++Counts.Instances;
            FGridResolvedItem Canonical;
            if (!Item.IsValid() || !Registry.ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, Canonical) ||
                !Canonical.IsValid() || !FMath::IsFinite(Canonical.WeightKg) || Canonical.WeightKg < 0.f)
            {
                ++Counts.Invalid;
                return;
            }
            if (bName && !Item.DisplayName.EqualTo(Canonical.DisplayName)) ++Counts.NameConflicts;
            if (bWeight && (!FMath::IsFinite(Item.Weight) || Item.Weight != Canonical.WeightKg)) ++Counts.WeightConflicts;
        });
        return Counts;
    }

    inline bool ClearVerifiedCopy(FGridPartyInventoryState& Party, const UGridItemSubsystem& Registry)
    {
        if (!Inspect(Party, Registry).CanClear()) return false;
        Visit(Party, [](FGridItemInstance& Item)
        {
            Item.DisplayName = FText::GetEmpty();
            Item.Weight = 0.f;
        });
        return true;
    }
}
