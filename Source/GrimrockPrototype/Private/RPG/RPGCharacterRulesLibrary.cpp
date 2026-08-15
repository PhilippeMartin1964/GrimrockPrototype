#include "RPG/RPGCharacterRulesLibrary.h"

#include "RPG/RPGClassAsset.h"

namespace
{
    constexpr int32 RPGMinimumLevel = 1;
    constexpr int32 RPGMaximumLevel = 20;
    constexpr int32 RPGExperienceStepPerLevel = 1000;

    int32 CalculateCumulativeExperienceUnchecked (int32 Level)
    {
        const int64 PreviousLevels = static_cast<int64> (Level - RPGMinimumLevel);
        const int64 RequiredExperience =
            static_cast<int64> (RPGExperienceStepPerLevel) *
            PreviousLevels *
            (PreviousLevels + 1) /
            2;
        return static_cast<int32> (RequiredExperience);
    }
}

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

int32 URPGCharacterRulesLibrary::GetMinimumLevel ()
{
    return RPGMinimumLevel;
}

int32 URPGCharacterRulesLibrary::GetMaximumLevel ()
{
    return RPGMaximumLevel;
}

int32 URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel (int32 Level)
{
    const int32 SafeLevel = FMath::Clamp (Level, RPGMinimumLevel, RPGMaximumLevel);
    return CalculateCumulativeExperienceUnchecked (SafeLevel);
}

int32 URPGCharacterRulesLibrary::GetLevelForExperience (int32 TotalExperience)
{
    const int32 SafeExperience = NormalizeExperience (TotalExperience);
    for (int32 NextLevel = RPGMinimumLevel + 1; NextLevel <= RPGMaximumLevel; ++NextLevel)
    {
        if (SafeExperience < CalculateCumulativeExperienceUnchecked (NextLevel))
        {
            return NextLevel - 1;
        }
    }

    return RPGMaximumLevel;
}

int32 URPGCharacterRulesLibrary::GetExperienceInCurrentLevel (int32 TotalExperience)
{
    const int32 SafeExperience = NormalizeExperience (TotalExperience);
    const int32 Level = GetLevelForExperience (SafeExperience);
    return SafeExperience - CalculateCumulativeExperienceUnchecked (Level);
}

int32 URPGCharacterRulesLibrary::GetExperienceRemainingToNextLevel (int32 TotalExperience)
{
    const int32 SafeExperience = NormalizeExperience (TotalExperience);
    const int32 Level = GetLevelForExperience (SafeExperience);
    if (Level >= RPGMaximumLevel)
    {
        return 0;
    }

    return CalculateCumulativeExperienceUnchecked (Level + 1) - SafeExperience;
}

bool URPGCharacterRulesLibrary::IsMaximumLevel (int32 Level)
{
    return Level == RPGMaximumLevel;
}

int32 URPGCharacterRulesLibrary::NormalizeExperience (int32 TotalExperience)
{
    const int32 MaximumExperience = CalculateCumulativeExperienceUnchecked (RPGMaximumLevel);
    return FMath::Clamp (TotalExperience, 0, MaximumExperience);
}

bool URPGCharacterRulesLibrary::IsLevelExperienceConsistent (
    int32 Level,
    int32 TotalExperience)
{
    if (Level < RPGMinimumLevel || Level > RPGMaximumLevel)
    {
        return false;
    }

    if (TotalExperience != NormalizeExperience (TotalExperience))
    {
        return false;
    }

    return Level == GetLevelForExperience (TotalExperience);
}
