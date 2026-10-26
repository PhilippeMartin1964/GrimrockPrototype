#include "UI/GridEquipmentTooltipGlobalProjection.h"

#include "Runtime/GridEquipmentPreviewTypes.h"
#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "UI/GridWeaponTooltipProjection.h"
#include "UI/UILocalizationText.h"

namespace
{
    const TCHAR* LabelKey(FName Id)
    {
        struct FEntry { const TCHAR* Id; const TCHAR* Key; };
        static const FEntry Entries[] = {
            { TEXT("Strength"), TEXT("Inventory.Tooltip.Global.Stat.Strength") },
            { TEXT("Dexterity"), TEXT("Inventory.Tooltip.Global.Stat.Dexterity") },
            { TEXT("Constitution"), TEXT("Inventory.Tooltip.Global.Stat.Constitution") },
            { TEXT("Intelligence"), TEXT("Inventory.Tooltip.Global.Stat.Intelligence") },
            { TEXT("Wisdom"), TEXT("Inventory.Tooltip.Global.Stat.Wisdom") },
            { TEXT("Charisma"), TEXT("Inventory.Tooltip.Global.Stat.Charisma") },
            { TEXT("MaxHealth"), TEXT("Inventory.Tooltip.Global.Stat.MaxHealth") },
            { TEXT("MaxMana"), TEXT("Inventory.Tooltip.Global.Stat.MaxMana") },
            { TEXT("CarryWeight"), TEXT("Inventory.Tooltip.Global.Stat.CarryWeight") },
            { TEXT("PhysicalResistance"), TEXT("Inventory.Tooltip.Global.Stat.PhysicalResistance") },
            { TEXT("FireResistance"), TEXT("Inventory.Tooltip.Global.Stat.FireResistance") },
            { TEXT("IceResistance"), TEXT("Inventory.Tooltip.Global.Stat.IceResistance") },
            { TEXT("LightningResistance"), TEXT("Inventory.Tooltip.Global.Stat.LightningResistance") },
            { TEXT("PoisonResistance"), TEXT("Inventory.Tooltip.Global.Stat.PoisonResistance") },
            { TEXT("HolyResistance"), TEXT("Inventory.Tooltip.Global.Stat.HolyResistance") },
            { TEXT("NecroticResistance"), TEXT("Inventory.Tooltip.Global.Stat.NecroticResistance") },
            { TEXT("ArcaneResistance"), TEXT("Inventory.Tooltip.Global.Stat.ArcaneResistance") },
            { TEXT("CurrentPhysicalArmor"), TEXT("Inventory.Tooltip.Global.Stat.CurrentPhysicalArmor") }
        };
        for (const FEntry& Entry : Entries)
        {
            if (Id == FName(Entry.Id)) return Entry.Key;
        }
        return nullptr;
    }

    FText FormatGlobal(const TCHAR* Key, const FFormatNamedArguments& Args)
    {
        return FText::Format(FUILocalizationText::Get(Key), Args);
    }

    // Preserve meaningful neutral deltas for equipment bonuses without
    // flooding the tooltip with every unchanged base character attribute.
    bool IsRelevantNeutralStat(const FGridEquipmentPreviewResult& Preview, FName Id)
    {
        const FGridEquipmentStatBonus& Before = Preview.BeforeBonuses;
        const FGridEquipmentStatBonus& After = Preview.AfterBonuses;
        const FGridDamageResistanceSet& BeforeR = Preview.BeforeResistances;
        const FGridDamageResistanceSet& AfterR = Preview.AfterResistances;
        if (Id == TEXT("Strength")) return Before.StrengthBonus != 0 || After.StrengthBonus != 0;
        if (Id == TEXT("Dexterity")) return Before.DexterityBonus != 0 || After.DexterityBonus != 0;
        if (Id == TEXT("Constitution")) return Before.ConstitutionBonus != 0 || After.ConstitutionBonus != 0;
        if (Id == TEXT("Intelligence")) return Before.IntelligenceBonus != 0 || After.IntelligenceBonus != 0;
        if (Id == TEXT("Wisdom")) return Before.WisdomBonus != 0 || After.WisdomBonus != 0;
        if (Id == TEXT("Charisma")) return Before.CharismaBonus != 0 || After.CharismaBonus != 0;
        if (Id == TEXT("MaxHealth")) return Before.MaxHealthBonus != 0 || After.MaxHealthBonus != 0;
        if (Id == TEXT("MaxMana")) return Before.MaxManaBonus != 0 || After.MaxManaBonus != 0;
        if (Id == TEXT("CarryWeight")) return !FMath::IsNearlyZero(Before.CarryWeightBonus) || !FMath::IsNearlyZero(After.CarryWeightBonus);
        if (Id == TEXT("CurrentPhysicalArmor")) return Before.ArmorBonus != 0 || After.ArmorBonus != 0;
        if (Id == TEXT("PhysicalResistance")) return BeforeR.PhysicalResistance != 0 || AfterR.PhysicalResistance != 0;
        if (Id == TEXT("FireResistance")) return BeforeR.FireResistance != 0 || AfterR.FireResistance != 0;
        if (Id == TEXT("IceResistance")) return BeforeR.IceResistance != 0 || AfterR.IceResistance != 0;
        if (Id == TEXT("LightningResistance")) return BeforeR.LightningResistance != 0 || AfterR.LightningResistance != 0;
        if (Id == TEXT("PoisonResistance")) return BeforeR.PoisonResistance != 0 || AfterR.PoisonResistance != 0;
        if (Id == TEXT("HolyResistance")) return BeforeR.HolyResistance != 0 || AfterR.HolyResistance != 0;
        if (Id == TEXT("NecroticResistance")) return BeforeR.NecroticResistance != 0 || AfterR.NecroticResistance != 0;
        if (Id == TEXT("ArcaneResistance")) return BeforeR.ArcaneResistance != 0 || AfterR.ArcaneResistance != 0;
        return false;
    }

    void AddNumericChanges(const FGridEquipmentPreviewResult& Preview,
        TArray<FGridItemTooltipStatLine>& Out)
    {
        Out.Reset();
        for (const FGridEquipmentPreviewDelta& Numeric : Preview.NumericDeltas)
        {
            // EffectiveCurrentPhysicalArmor is the armor authority: never also
            // display ArmorBonus (nominal), which would count the same shield twice.
            const TCHAR* Key = LabelKey(Numeric.StatId);
            if (!Key || (FMath::IsNearlyZero(Numeric.Delta) &&
                !IsRelevantNeutralStat(Preview, Numeric.StatId)))
            {
                continue;
            }

            FGridItemTooltipStatLine& Line = Out.AddDefaulted_GetRef();
            Line.StatId = Numeric.StatId;
            Line.Label = FUILocalizationText::Get(Key);
            Line.ItemValue = Numeric.After;
            Line.EquippedValue = Numeric.Before;
            Line.Delta = Numeric.Delta;
            Line.bHasComparison = true;
            const bool bNeutral = FMath::IsNearlyZero(Numeric.Delta);
            Line.DeltaState = bNeutral ? EGridItemTooltipDeltaState::Neutral :
                (Numeric.Delta > 0.f ? EGridItemTooltipDeltaState::Positive :
                    EGridItemTooltipDeltaState::Negative);
            Line.ValueText = FText::AsNumber(Numeric.After);
            if (bNeutral)
            {
                Line.DeltaDisplayText = FUILocalizationText::Get(TEXT("Inventory.Tooltip.Global.NeutralDelta"));
            }
            else
            {
                FFormatNamedArguments Args;
                Args.Add(TEXT("Value"), FText::AsNumber(FMath::Abs(Numeric.Delta)));
                Line.DeltaDisplayText = FormatGlobal(Numeric.Delta > 0.f
                    ? TEXT("Inventory.Tooltip.Global.PositiveDelta")
                    : TEXT("Inventory.Tooltip.Global.NegativeDelta"), Args);
            }
        }
    }

    FText SlotLabel(EGridEquipmentSlot Slot)
    {
        if (Slot == EGridEquipmentSlot::MainHand)
            return FUILocalizationText::Get(TEXT("Inventory.Tooltip.Global.MainHand"));
        if (Slot == EGridEquipmentSlot::OffHand)
            return FUILocalizationText::Get(TEXT("Inventory.Tooltip.Global.OffHand"));
        // For non-hand gear, use the requested slot name from the source-of-truth
        // localization mapping; no code-authored label fallbacks.
        const TCHAR* Key = nullptr;
        switch (Slot)
        {
            case EGridEquipmentSlot::Head: Key = TEXT("Inventory.Tooltip.Global.Slot.Head"); break;
            case EGridEquipmentSlot::Chest: Key = TEXT("Inventory.Tooltip.Global.Slot.Chest"); break;
            case EGridEquipmentSlot::Legs: Key = TEXT("Inventory.Tooltip.Global.Slot.Legs"); break;
            case EGridEquipmentSlot::Feet: Key = TEXT("Inventory.Tooltip.Global.Slot.Feet"); break;
            case EGridEquipmentSlot::Amulet: Key = TEXT("Inventory.Tooltip.Global.Slot.Amulet"); break;
            case EGridEquipmentSlot::Ring1: Key = TEXT("Inventory.Tooltip.Global.Slot.Ring1"); break;
            case EGridEquipmentSlot::Ring2: Key = TEXT("Inventory.Tooltip.Global.Slot.Ring2"); break;
            case EGridEquipmentSlot::Shoulders: Key = TEXT("Inventory.Tooltip.Global.Slot.Shoulders"); break;
            case EGridEquipmentSlot::Gloves: Key = TEXT("Inventory.Tooltip.Global.Slot.Gloves"); break;
            case EGridEquipmentSlot::Belt: Key = TEXT("Inventory.Tooltip.Global.Slot.Belt"); break;
            case EGridEquipmentSlot::Cloak: Key = TEXT("Inventory.Tooltip.Global.Slot.Cloak"); break;
            case EGridEquipmentSlot::Talisman: Key = TEXT("Inventory.Tooltip.Global.Slot.Talisman"); break;
            case EGridEquipmentSlot::QuickSlot1: Key = TEXT("Inventory.Tooltip.Global.Slot.QuickSlot1"); break;
            case EGridEquipmentSlot::QuickSlot2: Key = TEXT("Inventory.Tooltip.Global.Slot.QuickSlot2"); break;
            case EGridEquipmentSlot::Shirt: Key = TEXT("Inventory.Tooltip.Global.Slot.Shirt"); break;
            case EGridEquipmentSlot::Bracers: Key = TEXT("Inventory.Tooltip.Global.Slot.Bracers"); break;
            default: break;
        }
        return Key ? FUILocalizationText::Get(Key) : FText::GetEmpty();
    }
}

void FGridEquipmentTooltipGlobalProjection::GatherAllowedSlots(
    const UGridItemDefinitionAsset* Candidate,
    TArray<EGridEquipmentSlot>& OutSlots)
{
    OutSlots.Reset();
    if (!IsValid(Candidate)) return;
    // Equipment logic, not the authored CompatibleEquipmentSlots array, owns
    // hand compatibility. Two-handed items are physically installed once.
    if (Candidate->CanEquipToSlot(EGridEquipmentSlot::MainHand))
        OutSlots.Add(EGridEquipmentSlot::MainHand);
    if (Candidate->GetEffectiveHandUsage() != EGridItemHandUsage::TwoHanded &&
        Candidate->CanEquipToSlot(EGridEquipmentSlot::OffHand))
        OutSlots.Add(EGridEquipmentSlot::OffHand);

    for (EGridEquipmentSlot Slot : Candidate->CompatibleEquipmentSlots)
    {
        if (Slot == EGridEquipmentSlot::None ||
            Slot == EGridEquipmentSlot::MainHand || Slot == EGridEquipmentSlot::OffHand ||
            OutSlots.Contains(Slot) || !Candidate->CanEquipToSlot(Slot))
        {
            continue;
        }
        OutSlots.Add(Slot);
    }
}

bool FGridEquipmentTooltipGlobalProjection::BuildScenario(
    const UGridPartyInventoryComponent* Inventory,
    int32 CharacterIndex, int32 InventorySlotIndex,
    EGridEquipmentSlot RequestedSlot, const UGridItemDefinitionAsset* CandidateDefinition,
    const FGridItemInstance& Candidate,
    FGridItemTooltipEquipmentComparison& OutScenario)
{
    OutScenario = FGridItemTooltipEquipmentComparison();
    if (!IsValid(Inventory) || !IsValid(CandidateDefinition) || !Candidate.IsValid())
    {
        return false;
    }
    FGridEquipmentPreviewResult Preview;
    if (!Inventory->PreviewEquipFromInventorySlot(CharacterIndex,
        InventorySlotIndex, RequestedSlot, Preview) || !Preview.bValidPreview)
    {
        return false;
    }

    OutScenario.bGlobalEquipmentScenario = true;
    OutScenario.EquipmentSlot = Preview.ActualTargetSlot;

    if (Preview.bCandidateUsesTwoHands)
    {
        OutScenario.SlotLabel = FUILocalizationText::Get(
            TEXT("Inventory.Tooltip.Global.TwoHandScenario"));
    }
    else
    {
        FFormatNamedArguments ScenarioArgs;
        ScenarioArgs.Add(TEXT("Slot"), SlotLabel(Preview.ActualTargetSlot));
        const bool bHand = Preview.ActualTargetSlot == EGridEquipmentSlot::MainHand ||
            Preview.ActualTargetSlot == EGridEquipmentSlot::OffHand;
        OutScenario.SlotLabel = FormatGlobal(bHand
            ? TEXT("Inventory.Tooltip.Global.OneHandScenario")
            : TEXT("Inventory.Tooltip.Global.OneSlotScenario"), ScenarioArgs);
    }

    TArray<FString> DisplacedNames;
    for (const FGridEquipmentPreviewDisplacedItem& Item : Preview.DisplacedItems)
    {
        DisplacedNames.Add(Item.DisplayName.ToString());
    }
    const FText Items = DisplacedNames.IsEmpty()
        ? FUILocalizationText::Get(TEXT("Inventory.Tooltip.Global.NoDisplaced"))
        : FText::FromString(FString::Join(DisplacedNames, TEXT(", ")));
    FFormatNamedArguments DescriptionArgs;
    DescriptionArgs.Add(TEXT("Items"), Items);
    OutScenario.EquippedItemDisplayText = FormatGlobal(
        TEXT("Inventory.Tooltip.Global.Displaced"), DescriptionArgs);

    if (!Preview.bCanEquipNow)
    {
        FFormatNamedArguments CapacityArgs;
        CapacityArgs.Add(TEXT("Required"), FText::AsNumber(Preview.RequiredSlotsForDisplacedItems));
        CapacityArgs.Add(TEXT("Available"), FText::AsNumber(Preview.FreeInventorySlotsAfterSourceRemoval));
        const FText CapacityWarning = FormatGlobal(
            TEXT("Inventory.Tooltip.Global.InventoryFull"), CapacityArgs);
        FFormatNamedArguments CombinedArgs;
        CombinedArgs.Add(TEXT("Displaced"), OutScenario.EquippedItemDisplayText);
        CombinedArgs.Add(TEXT("Warning"), CapacityWarning);
        OutScenario.EquippedItemDisplayText = FormatGlobal(
            TEXT("Inventory.Tooltip.Global.DisplacedAndWarning"), CombinedArgs);
    }

    AddNumericChanges(Preview, OutScenario.StatLines);

    // Compare offensive catalogues only with the actual weapon displaced
    // from the candidate's target hand. Shield losses are represented by
    // effective-armor rows, never via an attack-versus-shield comparison.
    FGridItemInstance EquippedItem;
    if (Inventory->GetEquippedItem(CharacterIndex,
        Preview.ActualTargetSlot, EquippedItem) &&
        EquippedItem.RuntimeObjectId != Candidate.RuntimeObjectId)
    {
        const UGridItemDefinitionAsset* EquippedDef =
            Inventory->FindItemDefinition(EquippedItem.ItemDefinitionId);
        if (EquippedDef)
        {
            FGridWeaponTooltipProjection::BuildComparison(
                CandidateDefinition, Candidate, EquippedDef, EquippedItem,
                OutScenario.WeaponFacts);
        }
    }
    return true;
}
