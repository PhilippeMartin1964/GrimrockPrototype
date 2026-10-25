#include "UI/GridWeaponTooltipProjection.h"

#include "Runtime/GridItemDefinitionAsset.h"
#include "Runtime/Combat/GridWeaponCatalogueAsset.h"

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
                    return NSLOCTEXT("GridWeaponTooltip", "Slashing", "Physiques — tranchants");
                case EGridPhysicalDamageSubtype::Piercing:
                    return NSLOCTEXT("GridWeaponTooltip", "Piercing", "Physiques — perforants");
                case EGridPhysicalDamageSubtype::Bludgeoning:
                    return NSLOCTEXT("GridWeaponTooltip", "Bludgeoning", "Physiques — contondants");
                default:
                    return NSLOCTEXT("GridWeaponTooltip", "Physical", "Physiques");
            }
        }
        const UEnum* Enum = StaticEnum<EGridDamageType>();
        return Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(DamageType))
            : FText::GetEmpty();
    }

    FText AttributeLabel(EGridAttackScalingAttribute Attribute)
    {
        switch (Attribute)
        {
            case EGridAttackScalingAttribute::Strength:
                return NSLOCTEXT("GridWeaponTooltip", "Strength", "Force");
            case EGridAttackScalingAttribute::Dexterity:
                return NSLOCTEXT("GridWeaponTooltip", "Dexterity", "Dextérité");
            case EGridAttackScalingAttribute::Constitution:
                return NSLOCTEXT("GridWeaponTooltip", "Constitution", "Constitution");
            case EGridAttackScalingAttribute::Intelligence:
                return NSLOCTEXT("GridWeaponTooltip", "Intelligence", "Intelligence");
            case EGridAttackScalingAttribute::Wisdom:
                return NSLOCTEXT("GridWeaponTooltip", "Wisdom", "Sagesse");
            case EGridAttackScalingAttribute::Charisma:
                return NSLOCTEXT("GridWeaponTooltip", "Charisma", "Charisme");
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
    Out.FamilyText = FText::FromString(Profile->Family);
    Out.ScalingAttributeText = AttributeLabel(Profile->ScalingAttribute);
    Out.DamageTypeText = Out.bPassive
        ? FText::GetEmpty() : TypeLabel(Profile->DamageType, Profile->PhysicalSubtype);
    Out.DiceText = Out.bPassive
        ? FText::GetEmpty() : FormatDice(Profile->DamageDice);

    if (!Out.bPassive)
    {
        Add(Out.Facts, TEXT("WeaponDice"),
            NSLOCTEXT("GridWeaponTooltip", "Dice", "Dégâts (dés)"), Out.DiceText);
        AddInteger(Out.Facts, TEXT("WeaponMinDamage"),
            NSLOCTEXT("GridWeaponTooltip", "MinDamage", "Dégâts min."),
            Profile->DamageDice.GetMinimumDamage());
        AddInteger(Out.Facts, TEXT("WeaponMaxDamage"),
            NSLOCTEXT("GridWeaponTooltip", "MaxDamage", "Dégâts max."),
            Profile->DamageDice.GetMaximumDamage());
        AddInteger(Out.Facts, TEXT("WeaponAccuracy"),
            NSLOCTEXT("GridWeaponTooltip", "Accuracy", "Précision"),
            Profile->AccuracyBonus, true);
        AddInteger(Out.Facts, TEXT("WeaponAP"),
            NSLOCTEXT("GridWeaponTooltip", "AP", "Coût en PA"),
            Profile->ActionPointCost);
        AddInteger(Out.Facts, TEXT("WeaponRange"),
            NSLOCTEXT("GridWeaponTooltip", "Range", "Portée (cases)"),
            Profile->RangeCells);
        Add(Out.Facts, TEXT("WeaponDamageType"),
            NSLOCTEXT("GridWeaponTooltip", "DamageType", "Type de dégâts"),
            Out.DamageTypeText);
        if (!Out.ScalingAttributeText.IsEmpty())
        {
            Add(Out.Facts, TEXT("WeaponScaling"),
                NSLOCTEXT("GridWeaponTooltip", "Scaling", "Attribut"),
                Out.ScalingAttributeText);
        }
    }

    if (Profile->PhysicalArmor > 0)
    {
        AddInteger(Out.Facts, TEXT("WeaponPhysicalArmor"),
            NSLOCTEXT("GridWeaponTooltip", "ArmorPhysical", "Armure physique"),
            Profile->PhysicalArmor);
    }
    if (Profile->MagicalArmor > 0)
    {
        AddInteger(Out.Facts, TEXT("WeaponMagicalArmor"),
            NSLOCTEXT("GridWeaponTooltip", "ArmorMagical", "Armure magique"),
            Profile->MagicalArmor);
    }
    return true;
}
