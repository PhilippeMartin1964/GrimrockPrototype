#pragma once

#include "Runtime/GridInventoryTypes.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "UObject/UnrealType.h"

/** Editor-only proof. Never normalize the live inventory or save slots. */
namespace GridEditorInstanceCacheEvidence
{
    // Source assets are read only in Editor evidence. No runtime fallback.
    using FLegacySources = TMap<FName, const UGridItemDefinitionAsset*>;

    inline bool IsExplainedWeight(const FGridItemInstance& Item, float CanonicalWeight,
        const UGridItemDefinitionAsset* Legacy)
    {
        if (!FMath::IsFinite(Item.Weight) || Item.Weight < 0.f ||
            !FMath::IsFinite(CanonicalWeight) || CanonicalWeight < 0.f) return false;
        // Small absolute tolerance is restricted to floating-point rounding.
        if (FMath::IsNearlyEqual(Item.Weight, CanonicalWeight, 0.000001f)) return true;
        if (!IsValid(Legacy) || Legacy->ItemDefinitionId != Item.ItemDefinitionId) return false;
        const auto Matches = [&](float Unit)
        {
            if (!FMath::IsFinite(Unit) || Unit < 0.f) return false;
            if (FMath::IsNearlyEqual(Item.Weight, Unit, 0.000001f)) return true;
            return Item.Quantity > 1 && FMath::IsFinite(Unit * Item.Quantity) &&
                FMath::IsNearlyEqual(Item.Weight, Unit * Item.Quantity, 0.000001f);
        };
        if (Matches(Legacy->Weight) || Matches(CanonicalWeight)) return true;
        const FGridWeaponLevelProfile* Profile = nullptr;
        return Legacy->TryGetWeaponCatalogueProfile(Item.ItemLevel, Profile) &&
            Profile && Matches(Profile->WeightKg);
    }

    struct FCounts
    {
        int32 Instances = 0;
        int32 Invalid = 0;
        int32 NameConflicts = 0;
        int32 WeightConflicts = 0;
        int32 ExplainedWeightConflicts = 0;
        int32 CachedNames = 0;
        int32 CachedWeights = 0;
        bool CanClear() const { return Invalid == 0 && NameConflicts == 0 && WeightConflicts == ExplainedWeightConflicts; }
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

    inline FCounts Inspect(const FGridPartyInventoryState& Party, const UGridItemSubsystem& Registry,
        const FLegacySources* Historical = nullptr)
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
            if (bWeight && (!FMath::IsFinite(Item.Weight) || Item.Weight != Canonical.WeightKg))
            {
                ++Counts.WeightConflicts;
                if (Historical)
                {
                    const auto* Source = Historical->Find(Item.ItemDefinitionId);
                    if (IsExplainedWeight(Item, Canonical.WeightKg, Source ? *Source : nullptr))
                        ++Counts.ExplainedWeightConflicts;
                }
            }
        });
        return Counts;
    }

    inline bool ClearVerifiedCopy(FGridPartyInventoryState& Party, const UGridItemSubsystem& Registry,
        const FLegacySources* Historical = nullptr)
    {
        if (!Inspect(Party, Registry, Historical).CanClear()) return false;
        Visit(Party, [](FGridItemInstance& Item)
        {
            Item.DisplayName = FText::GetEmpty();
            Item.Weight = 0.f;
        });
        return true;
    }
}
