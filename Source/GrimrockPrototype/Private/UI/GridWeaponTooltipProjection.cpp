#include "UI/GridWeaponTooltipProjection.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"
#include "UI/UILocalizationText.h"

namespace
{
    FText FormatDice(const FGridDamageDiceProfile& Dice)
    {
        const FString Suffix = Dice.FlatBonus == 0
            ? TEXT("")
            : FString::Printf(TEXT(" + %d"), Dice.FlatBonus);
        return FText::FromString(FString::Printf(TEXT("%dd%d%s"),
            Dice.DiceCount, Dice.DiceSides, *Suffix));
    }

    FText TypeLabel(EGridDamageType DamageType, EGridPhysicalDamageSubtype Subtype)
    {
        if (DamageType == EGridDamageType::Physical)
        {
            switch (Subtype)
            {
                case EGridPhysicalDamageSubtype::Slashing:
                    return FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Damage.Slashing"));
                case EGridPhysicalDamageSubtype::Piercing:
                    return FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Damage.Piercing"));
                case EGridPhysicalDamageSubtype::Bludgeoning:
                    return FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Damage.Bludgeoning"));
                default:
                    return FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Damage.Physical"));
            }
        }
        switch (DamageType)
        {
            case EGridDamageType::Fire:
                return FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Damage.Fire"));
            case EGridDamageType::Ice:
                return FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Damage.Ice"));
            case EGridDamageType::Lightning:
                return FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Damage.Lightning"));
            case EGridDamageType::Poison:
                return FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Damage.Poison"));
            case EGridDamageType::Holy:
                return FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Damage.Holy"));
            case EGridDamageType::Necrotic:
                return FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Damage.Necrotic"));
            case EGridDamageType::Arcane:
                return FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Damage.Arcane"));
            default: return FText::GetEmpty();
        }
    }

    FText AttributeLabel(EGridAttackScalingAttribute Attribute)
    {
        switch (Attribute)
        {
            case EGridAttackScalingAttribute::Strength:
                return FUILocalizationText::Get(TEXT("CharacterCreation.Attribute.Strength"));
            case EGridAttackScalingAttribute::Dexterity:
                return FUILocalizationText::Get(TEXT("CharacterCreation.Attribute.Dexterity"));
            case EGridAttackScalingAttribute::Constitution:
                return FUILocalizationText::Get(TEXT("CharacterCreation.Attribute.Constitution"));
            case EGridAttackScalingAttribute::Intelligence:
                return FUILocalizationText::Get(TEXT("CharacterCreation.Attribute.Intelligence"));
            case EGridAttackScalingAttribute::Wisdom:
                return FUILocalizationText::Get(TEXT("CharacterCreation.Attribute.Wisdom"));
            case EGridAttackScalingAttribute::Charisma:
                return FUILocalizationText::Get(TEXT("CharacterCreation.Attribute.Charisma"));
            default: return FText::GetEmpty();
        }
    }

    void Add(TArray<FGridWeaponTooltipFact>& Facts,
        FName Id, const FText& Label, const FText& Value)
    {
        FGridWeaponTooltipFact& Fact = Facts.AddDefaulted_GetRef();
        Fact.StatId = Id;
        Fact.Label = Label;
        Fact.ValueText = Value;
    }

    void AddInteger(TArray<FGridWeaponTooltipFact>& Facts,
        FName Id, const FText& Label, int32 Value, bool bSigned = false)
    {
        Add(Facts, Id, Label, FText::FromString(bSigned
            ? FString::Printf(TEXT("%+d"), Value)
            : FString::FromInt(Value)));
    }
}

bool FGridWeaponTooltipProjection::Build(
    const UGridItemDefinitionAsset* Definition,
    const FGridItemInstance& Instance,
    FGridWeaponTooltipPresentation& Out)
{
    Out = FGridWeaponTooltipPresentation();
    if (!IsValid(Definition) || !Instance.IsValid() ||
        Instance.ItemDefinitionId != Definition->ItemDefinitionId)
    {
        return false;
    }

    const FGridWeaponLevelProfile* Profile = nullptr;
    if (!Definition->TryGetWeaponCatalogueProfile(Instance.ItemLevel, Profile) ||
        !Profile || !Profile->IsValidProfile())
    {
        return false;
    }

    Out.bHasProfile = true;
    Out.bPassive = !Profile->DamageDice.bEnabled;
    Out.ArchetypeId = Profile->ArchetypeId;
    Out.ItemLevel = Profile->ItemLevel;
    // Internal workbook family is not a localized player-facing category.
    // Keep it empty until a canonical family-id mapping is approved.
    Out.FamilyText = FText::GetEmpty();
    Out.ScalingAttributeText = AttributeLabel(Profile->ScalingAttribute);
    Out.DamageTypeText = Out.bPassive
        ? FText::GetEmpty() : TypeLabel(Profile->DamageType, Profile->PhysicalSubtype);
    Out.DiceText = Out.bPassive
        ? FText::GetEmpty() : FormatDice(Profile->DamageDice);

    if (!Out.bPassive)
    {
        Add(Out.Facts, TEXT("WeaponDice"),
            FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Stat.Dice")), Out.DiceText);
        AddInteger(Out.Facts, TEXT("WeaponMinDamage"),
            FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Stat.MinDamage")),
            Profile->DamageDice.GetMinimumDamage());
        AddInteger(Out.Facts, TEXT("WeaponMaxDamage"),
            FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Stat.MaxDamage")),
            Profile->DamageDice.GetMaximumDamage());
        AddInteger(Out.Facts, TEXT("WeaponAccuracy"),
            FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Stat.Accuracy")),
            Profile->AccuracyBonus, true);
        AddInteger(Out.Facts, TEXT("WeaponAP"),
            FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Stat.AP")),
            Profile->ActionPointCost);
        AddInteger(Out.Facts, TEXT("WeaponRange"),
            FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Stat.Range")),
            Profile->RangeCells);
        Add(Out.Facts, TEXT("WeaponDamageType"),
            FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Stat.DamageType")),
            Out.DamageTypeText);
        if (!Out.ScalingAttributeText.IsEmpty())
        {
            Add(Out.Facts, TEXT("WeaponScaling"),
                FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Stat.Scaling")),
                Out.ScalingAttributeText);
        }
    }

    if (Profile->PhysicalArmor > 0)
    {
        AddInteger(Out.Facts, TEXT("WeaponPhysicalArmor"),
            FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Stat.ArmorPhysical")),
            Profile->PhysicalArmor);
    }
    if (Profile->MagicalArmor > 0)
    {
        AddInteger(Out.Facts, TEXT("WeaponMagicalArmor"),
            FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Stat.ArmorMagical")),
            Profile->MagicalArmor);
    }
    return true;
}


namespace
{
    /** Generic text comparison: damage formulas/types are informational, not numerical benefits. */
    void AddTextComparison(TArray<FGridWeaponTooltipComparisonFact>& Out,
        FName Id, const FText& Label, const FText& Candidate, const FText& Equipped)
    {
        FGridWeaponTooltipComparisonFact& Fact = Out.AddDefaulted_GetRef();
        Fact.StatId = Id;
        Fact.Label = Label;
        Fact.CandidateText = Candidate;
        Fact.EquippedText = Equipped;
    }

    void AddNumberComparison(TArray<FGridWeaponTooltipComparisonFact>& Out,
        FName Id, const FText& Label, int32 Candidate, int32 Equipped,
        bool bLowerIsBetter = false, bool bComparable = true, bool bSigned = false)
    {
        FGridWeaponTooltipComparisonFact& Fact = Out.AddDefaulted_GetRef();
        Fact.StatId = Id;
        Fact.Label = Label;
        Fact.CandidateText = FText::FromString(bSigned
            ? FString::Printf(TEXT("%+d"), Candidate) : FString::FromInt(Candidate));
        Fact.EquippedText = FText::FromString(bSigned
            ? FString::Printf(TEXT("%+d"), Equipped) : FString::FromInt(Equipped));
        if (!bComparable)
        {
            return; // Different damage type/scaling: show values, no misleading delta.
        }
        Fact.bHasDelta = true;
        Fact.Delta = static_cast<float>(Candidate - Equipped);
        const int32 Diff = Candidate - Equipped;
        const FString DisplayDelta = Diff == 0 ? FString(TEXT("±0"))
            : FString::Printf(TEXT("%+d"), Diff);
        Fact.DeltaText = FText::FromString(DisplayDelta);
        if (Diff != 0)
        {
            const bool bImprovement = bLowerIsBetter ? Diff < 0 : Diff > 0;
            Fact.DeltaState = bImprovement
                ? EGridItemTooltipDeltaState::Positive
                : EGridItemTooltipDeltaState::Negative;
        }
    }
}

bool FGridWeaponTooltipProjection::BuildComparison(
    const UGridItemDefinitionAsset* CandidateDefinition,
    const FGridItemInstance& Candidate,
    const UGridItemDefinitionAsset* EquippedDefinition,
    const FGridItemInstance& Equipped,
    TArray<FGridWeaponTooltipComparisonFact>& OutFacts)
{
    OutFacts.Reset();
    if (!IsValid(CandidateDefinition) || !IsValid(EquippedDefinition) ||
        !Candidate.IsValid() || !Equipped.IsValid() ||
        Candidate.ItemDefinitionId != CandidateDefinition->ItemDefinitionId ||
        Equipped.ItemDefinitionId != EquippedDefinition->ItemDefinitionId)
    {
        return false;
    }
    const FGridWeaponLevelProfile* C = nullptr;
    const FGridWeaponLevelProfile* E = nullptr;
    if (!CandidateDefinition->TryGetWeaponCatalogueProfile(Candidate.ItemLevel, C) ||
        !EquippedDefinition->TryGetWeaponCatalogueProfile(Equipped.ItemLevel, E) ||
        !C || !E || !C->IsValidProfile() || !E->IsValidProfile())
    {
        return false;
    }
    const bool bCandidatePassive = !C->DamageDice.bEnabled;
    const bool bEquippedPassive = !E->DamageDice.bEnabled;
    if (bCandidatePassive != bEquippedPassive)
    {
        return false; // No fabricated melee-vs-shield comparison.
    }

    if (bCandidatePassive)
    {
        AddNumberComparison(OutFacts, TEXT("WeaponPhysicalArmor"),
            FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Stat.ArmorPhysical")),
            C->PhysicalArmor, E->PhysicalArmor);
        AddNumberComparison(OutFacts, TEXT("WeaponMagicalArmor"),
            FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Stat.ArmorMagical")),
            C->MagicalArmor, E->MagicalArmor);
        return true;
    }

    const bool bSameDamageSemantics =
        C->DamageType == E->DamageType &&
        C->PhysicalSubtype == E->PhysicalSubtype &&
        C->ScalingAttribute == E->ScalingAttribute;
    AddTextComparison(OutFacts, TEXT("WeaponDice"),
        FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Stat.Dice")),
        FormatDice(C->DamageDice), FormatDice(E->DamageDice));
    AddNumberComparison(OutFacts, TEXT("WeaponMinDamage"),
        FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Stat.MinDamage")),
        C->DamageDice.GetMinimumDamage(), E->DamageDice.GetMinimumDamage(),
        false, bSameDamageSemantics);
    AddNumberComparison(OutFacts, TEXT("WeaponMaxDamage"),
        FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Stat.MaxDamage")),
        C->DamageDice.GetMaximumDamage(), E->DamageDice.GetMaximumDamage(),
        false, bSameDamageSemantics);
    AddNumberComparison(OutFacts, TEXT("WeaponAccuracy"),
        FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Stat.Accuracy")),
        C->AccuracyBonus, E->AccuracyBonus, false, true, true);
    AddNumberComparison(OutFacts, TEXT("WeaponAP"),
        FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Stat.AP")),
        C->ActionPointCost, E->ActionPointCost, true);
    AddNumberComparison(OutFacts, TEXT("WeaponRange"),
        FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Stat.Range")),
        C->RangeCells, E->RangeCells);
    AddTextComparison(OutFacts, TEXT("WeaponDamageType"),
        FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Stat.DamageType")),
        TypeLabel(C->DamageType, C->PhysicalSubtype),
        TypeLabel(E->DamageType, E->PhysicalSubtype));
    AddTextComparison(OutFacts, TEXT("WeaponScaling"),
        FUILocalizationText::Get(TEXT("Inventory.Tooltip.Weapon.Stat.Scaling")),
        AttributeLabel(C->ScalingAttribute),
        AttributeLabel(E->ScalingAttribute));
    return true;
}
