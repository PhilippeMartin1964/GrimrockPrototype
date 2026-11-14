#include "Runtime/GridPartyInventoryComponent.h"

#include "GridEquipmentSlotUtils.h"
#include "GridEquipmentArmorReadModel.h"
#include "GridEquipmentPreviewItemReadModel.h"
#include "Runtime/Items/GridItemSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Runtime/GridItemDefinitionAsset.h"

namespace
{
    struct FPreviewEquipmentTotals
    {
        FGridEquipmentStatBonus Bonuses;
        FGridDamageResistanceSet Resistances;
        int32 EffectivePhysicalArmor = 0;
    };

    void AddBonuses(FGridEquipmentStatBonus& A, const FGridEquipmentStatBonus& B)
    {
        A.StrengthBonus += B.StrengthBonus;
        A.DexterityBonus += B.DexterityBonus;
        A.ConstitutionBonus += B.ConstitutionBonus;
        A.IntelligenceBonus += B.IntelligenceBonus;
        A.WisdomBonus += B.WisdomBonus;
        A.CharismaBonus += B.CharismaBonus;
        A.MaxHealthBonus += B.MaxHealthBonus;
        A.MaxManaBonus += B.MaxManaBonus;
        A.CarryWeightBonus += B.CarryWeightBonus;
        A.ArmorBonus += B.ArmorBonus;
        A.MagicalArmorBonus += B.MagicalArmorBonus;
    }

    void AddNumber(TArray<FGridEquipmentPreviewDelta>& Out,
        FName Id, float Before, float After)
    {
        FGridEquipmentPreviewDelta& Row = Out.AddDefaulted_GetRef();
        Row.StatId = Id;
        Row.Before = Before;
        Row.After = After;
        Row.Delta = After - Before;
    }

    void BuildNumbers(const FGridCharacterInventoryState& Character,
        const FPreviewEquipmentTotals& Before, const FPreviewEquipmentTotals& After,
        float BaseMaxWeight, TArray<FGridEquipmentPreviewDelta>& Out)
    {
        Out.Reset();
        // These effective values mirror GetCharacterSummary's equipment projection,
        // not definition-only per-slot comparisons or unimplemented derived formulas.
        AddNumber(Out, TEXT("Strength"),
            FMath::Max(0, Character.Attributes.Strength + Before.Bonuses.StrengthBonus),
            FMath::Max(0, Character.Attributes.Strength + After.Bonuses.StrengthBonus));
        AddNumber(Out, TEXT("Dexterity"),
            FMath::Max(0, Character.Attributes.Dexterity + Before.Bonuses.DexterityBonus),
            FMath::Max(0, Character.Attributes.Dexterity + After.Bonuses.DexterityBonus));
        AddNumber(Out, TEXT("Constitution"),
            FMath::Max(0, Character.Attributes.Constitution + Before.Bonuses.ConstitutionBonus),
            FMath::Max(0, Character.Attributes.Constitution + After.Bonuses.ConstitutionBonus));
        AddNumber(Out, TEXT("Intelligence"),
            FMath::Max(0, Character.Attributes.Intelligence + Before.Bonuses.IntelligenceBonus),
            FMath::Max(0, Character.Attributes.Intelligence + After.Bonuses.IntelligenceBonus));
        AddNumber(Out, TEXT("Wisdom"),
            FMath::Max(0, Character.Attributes.Wisdom + Before.Bonuses.WisdomBonus),
            FMath::Max(0, Character.Attributes.Wisdom + After.Bonuses.WisdomBonus));
        AddNumber(Out, TEXT("Charisma"),
            FMath::Max(0, Character.Attributes.Charisma + Before.Bonuses.CharismaBonus),
            FMath::Max(0, Character.Attributes.Charisma + After.Bonuses.CharismaBonus));
        AddNumber(Out, TEXT("MaxHealth"),
            FMath::Max(1, Character.DerivedStats.MaxHealth + Before.Bonuses.MaxHealthBonus),
            FMath::Max(1, Character.DerivedStats.MaxHealth + After.Bonuses.MaxHealthBonus));
        AddNumber(Out, TEXT("MaxMana"),
            FMath::Max(0, Character.DerivedStats.MaxMana + Before.Bonuses.MaxManaBonus),
            FMath::Max(0, Character.DerivedStats.MaxMana + After.Bonuses.MaxManaBonus));
        // Match GetCharacterSummary: base carry capacity plus equipment bonus.
        AddNumber(Out, TEXT("CarryWeight"),
            FMath::Max(0.f, BaseMaxWeight + Before.Bonuses.CarryWeightBonus),
            FMath::Max(0.f, BaseMaxWeight + After.Bonuses.CarryWeightBonus));
        AddNumber(Out, TEXT("ArmorBonus"),
            Before.Bonuses.ArmorBonus, After.Bonuses.ArmorBonus);
        AddNumber(Out, TEXT("PhysicalResistance"),
            Before.Resistances.PhysicalResistance, After.Resistances.PhysicalResistance);
        AddNumber(Out, TEXT("FireResistance"),
            Before.Resistances.FireResistance, After.Resistances.FireResistance);
        AddNumber(Out, TEXT("IceResistance"),
            Before.Resistances.IceResistance, After.Resistances.IceResistance);
        AddNumber(Out, TEXT("LightningResistance"),
            Before.Resistances.LightningResistance, After.Resistances.LightningResistance);
        AddNumber(Out, TEXT("PoisonResistance"),
            Before.Resistances.PoisonResistance, After.Resistances.PoisonResistance);
        AddNumber(Out, TEXT("HolyResistance"),
            Before.Resistances.HolyResistance, After.Resistances.HolyResistance);
        AddNumber(Out, TEXT("NecroticResistance"),
            Before.Resistances.NecroticResistance, After.Resistances.NecroticResistance);
        AddNumber(Out, TEXT("ArcaneResistance"),
            Before.Resistances.ArcaneResistance, After.Resistances.ArcaneResistance);
        AddNumber(Out, TEXT("CurrentPhysicalArmor"),
            Before.EffectivePhysicalArmor, After.EffectivePhysicalArmor);
    }
}

bool UGridPartyInventoryComponent::PreviewEquipFromInventorySlot(
    int32 CharacterIndex, int32 InventorySlotIndex, EGridEquipmentSlot TargetSlot,
    FGridEquipmentPreviewResult& OutPreview) const
{
    OutPreview = FGridEquipmentPreviewResult();
    OutPreview.RequestedSlot = TargetSlot;
    if (!IsValidCharacterIndex(CharacterIndex) ||
        !PartyInventoryState.ActiveEquipment.IsValidIndex(CharacterIndex))
    {
        return false;
    }

    const FGridCharacterInventoryState& Character =
        PartyInventoryState.ActiveCharacters[CharacterIndex];
    if (!Character.InventorySlots.IsValidIndex(InventorySlotIndex) ||
        Character.InventorySlots[InventorySlotIndex].IsEmpty())
    {
        return false;
    }

    const FGridItemInstance& Source = Character.InventorySlots[InventorySlotIndex].Item;
    if (!CanEquipItemToSlot(CharacterIndex, Source, TargetSlot) ||
        !GridEquipmentSlotUtils::IsSupportedSlot(TargetSlot))
    {
        OutPreview.Status = EGridEquipmentPreviewStatus::UnsupportedSlot;
        return false;
    }

    // One GameInstance ItemSystem for both real and projected items; headless
    // diagnostics may still run without a GameInstance during the transition.
    const UWorld* PreviewWorld = GetWorld();
    UGameInstance* PreviewGameInstance = PreviewWorld ? PreviewWorld->GetGameInstance() : nullptr;
    const UGridItemSubsystem* Registry = PreviewGameInstance
        ? PreviewGameInstance->GetSubsystem<UGridItemSubsystem>() : nullptr;

    const FGridCharacterEquipmentState& CurrentEquipment =
        PartyInventoryState.ActiveEquipment[CharacterIndex];
    const bool bIncomingTwoHanded =
        GridEquipmentSlotUtils::IsHandSlot(TargetSlot) &&
        IsTwoHandedItem(Source);
    const bool bReleaseBoth = bIncomingTwoHanded ||
        (GridEquipmentSlotUtils::IsHandSlot(TargetSlot) && IsTwoHandedEquipped(CharacterIndex));
    const EGridEquipmentSlot ActualTarget =
        bIncomingTwoHanded ? EGridEquipmentSlot::MainHand : TargetSlot;
    const FGridItemInstance* CurrentTarget = CurrentEquipment.GetSlot(ActualTarget);
    if (!CurrentTarget)
    {
        return false;
    }

    OutPreview.ActualTargetSlot = ActualTarget;
    OutPreview.bCandidateUsesTwoHands = bIncomingTwoHanded;
    OutPreview.bWouldReleaseBothHands = bReleaseBoth;

    // Mirrors EquipItemFromInventorySlot's physical displaced-items list.
    auto AppendDisplaced = [this, &OutPreview, Registry](EGridEquipmentSlot Slot,
        const FGridItemInstance& Item)
    {
        if (!Item.IsValid()) return;
        FGridEquipmentPreviewDisplacedItem& Entry = OutPreview.DisplacedItems.AddDefaulted_GetRef();
        Entry.OriginalSlot = Slot;
        Entry.ItemDefinitionId = Item.ItemDefinitionId;
        Entry.RuntimeObjectId = Item.RuntimeObjectId;
        const FGridEquipmentPreviewItemReadView Read =
            FGridEquipmentPreviewItemReadModel::Resolve(
                Registry, FindItemDefinition(Item.ItemDefinitionId), Item);
        Entry.DisplayName = Read.DisplayName;
        Entry.RemainingPhysicalArmor = Read.RemainingPhysicalArmor;
    };
    if (bReleaseBoth)
    {
        AppendDisplaced(EGridEquipmentSlot::MainHand, CurrentEquipment.MainHand);
        AppendDisplaced(EGridEquipmentSlot::OffHand, CurrentEquipment.OffHand);
    }
    else
    {
        AppendDisplaced(ActualTarget, *CurrentTarget);
    }

    // Exact source-backed capacity rule: the source bag slot becomes free
    // before the displaced items are returned to inventory.
    OutPreview.FreeInventorySlotsAfterSourceRemoval = 1;
    for (int32 Index = 0; Index < Character.InventorySlots.Num(); ++Index)
    {
        if (Index != InventorySlotIndex && Character.InventorySlots[Index].IsEmpty())
        {
            ++OutPreview.FreeInventorySlotsAfterSourceRemoval;
        }
    }
    OutPreview.RequiredSlotsForDisplacedItems = OutPreview.DisplacedItems.Num();
    OutPreview.bCanEquipNow =
        OutPreview.FreeInventorySlotsAfterSourceRemoval >=
        OutPreview.RequiredSlotsForDisplacedItems;
    OutPreview.Status = OutPreview.bCanEquipNow
        ? EGridEquipmentPreviewStatus::Ready
        : EGridEquipmentPreviewStatus::InventoryFull;

    // Copy only; this routine never calls EquipItemFromInventorySlot, clears
    // inventory, changes an item owner, sends delegates or saves packages.
    FGridCharacterEquipmentState ProjectedEquipment = CurrentEquipment;
    if (bReleaseBoth)
    {
        ProjectedEquipment.MainHand = FGridItemInstance();
        ProjectedEquipment.OffHand = FGridItemInstance();
    }
    FGridItemInstance* Destination = ProjectedEquipment.GetMutableSlot(ActualTarget);
    if (!Destination) return false;
    *Destination = Source; // RuntimeObjectId, level and absorbed armor unchanged.
    Destination->OwnerType = EGridItemOwnerType::EquipmentSlot;
    Destination->EquipmentSlot = ActualTarget;

    auto Gather = [this, &Character, Registry](const FGridCharacterEquipmentState& Equipment)
        -> FPreviewEquipmentTotals
    {
        FPreviewEquipmentTotals Totals;
        int64 PhysicalArmor = FMath::Max(0, Character.Resources.CurrentPhysicalArmor);
        GridEquipmentSlotUtils::ForEachEquipmentItem(Equipment,
            [this, Registry, &Totals, &PhysicalArmor](EGridEquipmentSlot,
                const FGridItemInstance& Item)
            {
                if (!Item.IsValid()) return;
                const FGridEquipmentPreviewItemReadView Read =
                    FGridEquipmentPreviewItemReadModel::Resolve(
                        Registry, FindItemDefinition(Item.ItemDefinitionId), Item);
                // Independently aggregate the same bonus and resistance sources
                // as live equipment; lack of an armor reserve must not mask
                // a real resistance contribution in a read-only projection.
                AddBonuses(Totals.Bonuses, Read.Bonuses);
                Totals.Resistances.Add(Read.Resistances);
                PhysicalArmor += Read.RemainingPhysicalArmor;
            });
        Totals.EffectivePhysicalArmor =
            static_cast<int32>(FMath::Clamp<int64>(PhysicalArmor, 0, MAX_int32));
        return Totals;
    };

    const FPreviewEquipmentTotals Before = Gather(CurrentEquipment);
    const FPreviewEquipmentTotals After = Gather(ProjectedEquipment);
    OutPreview.BeforeBonuses = Before.Bonuses;
    OutPreview.AfterBonuses = After.Bonuses;
    OutPreview.BeforeResistances = Before.Resistances;
    OutPreview.AfterResistances = After.Resistances;
    OutPreview.EffectivePhysicalArmorBefore = Before.EffectivePhysicalArmor;
    OutPreview.EffectivePhysicalArmorAfter = After.EffectivePhysicalArmor;
    BuildNumbers(Character, Before, After,
        CalculateCharacterBaseMaxWeight(CharacterIndex), OutPreview.NumericDeltas);
    OutPreview.bValidPreview = true;
    return true;
}
