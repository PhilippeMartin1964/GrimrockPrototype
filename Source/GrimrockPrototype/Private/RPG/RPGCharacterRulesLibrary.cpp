#include "RPG/RPGCharacterRulesLibrary.h"

#include "RPG/RPGClassAsset.h"

int32 URPGCharacterRulesLibrary::GetAttributeModifier (int32 AttributeValue)
{
    return FMath::FloorToInt (static_cast<float> (AttributeValue - 10) / 2.0f);
}

FRPGAttributes URPGCharacterRulesLibrary::AddAttributes (
    const FRPGAttributes& BaseAttributes,
    const FRPGAttributes& Bonuses)
{
    FRPGAttributes Result;
    Result.Strength = BaseAttributes.Strength + Bonuses.Strength;
    Result.Dexterity = BaseAttributes.Dexterity + Bonuses.Dexterity;
    Result.Constitution = BaseAttributes.Constitution + Bonuses.Constitution;
    Result.Intelligence = BaseAttributes.Intelligence + Bonuses.Intelligence;
    Result.Wisdom = BaseAttributes.Wisdom + Bonuses.Wisdom;
    Result.Charisma = BaseAttributes.Charisma + Bonuses.Charisma;
    return Result;
}

bool URPGCharacterRulesLibrary::AreAttributesInRange (
    const FRPGAttributes& Attributes,
    int32 MinimumValue,
    int32 MaximumValue)
{
    if (MinimumValue > MaximumValue)
    {
        return false;
    }

    return Attributes.Strength >= MinimumValue && Attributes.Strength <= MaximumValue &&
        Attributes.Dexterity >= MinimumValue && Attributes.Dexterity <= MaximumValue &&
        Attributes.Constitution >= MinimumValue && Attributes.Constitution <= MaximumValue &&
        Attributes.Intelligence >= MinimumValue && Attributes.Intelligence <= MaximumValue &&
        Attributes.Wisdom >= MinimumValue && Attributes.Wisdom <= MaximumValue &&
        Attributes.Charisma >= MinimumValue && Attributes.Charisma <= MaximumValue;
}

float URPGCharacterRulesLibrary::CalculateMaxCarryWeight (
    const FRPGAttributes& Attributes,
    float WeightPerStrength)
{
    return FMath::Max (0, Attributes.Strength) * FMath::Max (0.0f, WeightPerStrength);
}

FRPGDerivedStats URPGCharacterRulesLibrary::CalculateDerivedStats (
    const FRPGAttributes& Attributes,
    const URPGClassAsset* ClassDefinition,
    int32 Level)
{
    FRPGDerivedStats Result;
    if (!ClassDefinition || !ClassDefinition->IsValidDefinition ())
    {
        return Result;
    }

    const int32 SafeLevel = FMath::Max (1, Level);
    const int32 ConstitutionModifier = GetAttributeModifier (Attributes.Constitution);
    const int32 AdditionalLevels = SafeLevel - 1;
    const int32 HealthGainPerLevel = FMath::Max (1, ClassDefinition->HealthPerLevel + ConstitutionModifier);

    Result.MaxHealth = FMath::Max (
        1,
        ClassDefinition->HealthAtLevelOne + ConstitutionModifier + AdditionalLevels * HealthGainPerLevel);
    Result.CurrentHealth = Result.MaxHealth;
    Result.MaxMana = FMath::Max (
        0,
        ClassDefinition->ManaAtLevelOne + AdditionalLevels * ClassDefinition->ManaPerLevel);
    Result.CurrentMana = Result.MaxMana;
    Result.PhysicalArmor = FMath::Max (0, ClassDefinition->BasePhysicalArmor);
    Result.MagicalArmor = FMath::Max (0, ClassDefinition->BaseMagicalArmor);
    Result.Initiative = GetAttributeModifier (Attributes.Dexterity);
    Result.Accuracy = GetAttributeModifier (Attributes.Dexterity);
    Result.Evasion = GetAttributeModifier (Attributes.Dexterity);
    return Result;
}
