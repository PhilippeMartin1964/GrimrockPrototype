#pragma once

#include "Runtime/GridInventoryTypes.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Runtime/Items/GridItemCacheParityAudit.h"
#include "UObject/UnrealType.h"

/** Post-5G93 read-only proof. Historical cache normalization belongs to
 * the exact pre-cutover commits, not the current serialized schema. */
namespace GridEditorInstanceCacheEvidence
{
    inline bool HasRetiredSchema()
    {
        const UScriptStruct* Schema = FGridItemInstance::StaticStruct();
        return !Schema->FindPropertyByName(TEXT("DisplayName")) &&
            !Schema->FindPropertyByName(TEXT("Weight"));
    }

    struct FCounts
    {
        int32 Instances = 0;
        int32 Invalid = 0;
        int32 ProjectionFailures = 0;
        bool bRetiredSchema = false;
        bool IsValid() const { return bRetiredSchema && Invalid == 0 && ProjectionFailures == 0; }
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
        Counts.bRetiredSchema = HasRetiredSchema();
        Visit(Party, [&](const FGridItemInstance& Item)
        {
            if (!Item.RuntimeObjectId.IsValid() && Item.ItemDefinitionId.IsNone() &&
                !Item.ReadableContentAsset && Item.ReadableContentId.IsNone()) return;
            ++Counts.Instances;
            FGridResolvedItem Canonical;
            if (!Item.IsValid() || !Registry.ResolveItem(Item.ItemDefinitionId, Item.ItemLevel, Canonical) ||
                !Canonical.IsValid() || Canonical.ItemId != Item.ItemDefinitionId ||
                Canonical.ItemLevel != Item.ItemLevel ||
                (Canonical.bIsArmament ? Item.ItemLevel < 1 : Item.ItemLevel != 0) ||
                Item.Quantity > FMath::Max(1, Canonical.MaxStack) ||
                !FMath::IsFinite(Canonical.WeightKg) || Canonical.WeightKg < 0.f)
            {
                ++Counts.Invalid;
                return;
            }
            if (!GridItemCacheParityAudit::HasCanonicalProjection(Registry, Item)) ++Counts.ProjectionFailures;
        });
        return Counts;
    }
}
