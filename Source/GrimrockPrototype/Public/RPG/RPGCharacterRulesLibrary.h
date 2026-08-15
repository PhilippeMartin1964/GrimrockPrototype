#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RPG/RPGCharacterTypes.h"
#include "RPGCharacterRulesLibrary.generated.h"

class URPGClassAsset;

UCLASS ()
class GRIMROCKPROTOTYPE_API URPGCharacterRulesLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY ()

public:
    UFUNCTION (BlueprintPure, Category = "RPG|Rules")
    static int32 GetAttributeModifier (int32 AttributeValue);

    UFUNCTION (BlueprintPure, Category = "RPG|Rules")
    static FRPGAttributes AddAttributes (const FRPGAttributes& BaseAttributes, const FRPGAttributes& Bonuses);

    UFUNCTION (BlueprintPure, Category = "RPG|Rules")
    static bool AreAttributesInRange (const FRPGAttributes& Attributes, int32 MinimumValue = 6, int32 MaximumValue = 20);

    UFUNCTION (BlueprintPure, Category = "RPG|Rules")
    static float CalculateMaxCarryWeight (const FRPGAttributes& Attributes, float WeightPerStrength = 5.0f);

    UFUNCTION (BlueprintPure, Category = "RPG|Rules")
    static FRPGDerivedStats CalculateDerivedStats (
        const FRPGAttributes& Attributes,
        const URPGClassAsset* ClassDefinition,
        int32 Level);

    /** Lowest supported character level. */
    UFUNCTION (BlueprintPure, Category = "RPG|Progression")
    static int32 GetMinimumLevel ();

    /** Highest supported character level for the current progression model. */
    UFUNCTION (BlueprintPure, Category = "RPG|Progression")
    static int32 GetMaximumLevel ();

    /**
     * Total cumulative XP required to reach Level.
     * Invalid levels are clamped to the supported [MinimumLevel, MaximumLevel] range.
     */
    UFUNCTION (BlueprintPure, Category = "RPG|Progression")
    static int32 GetCumulativeExperienceRequiredForLevel (int32 Level);

    /** Reconstructs the supported level from total cumulative XP. */
    UFUNCTION (BlueprintPure, Category = "RPG|Progression")
    static int32 GetLevelForExperience (int32 TotalExperience);

    /** XP earned since reaching the current level threshold. */
    UFUNCTION (BlueprintPure, Category = "RPG|Progression")
    static int32 GetExperienceInCurrentLevel (int32 TotalExperience);

    /** XP still required to reach the next level, or zero at maximum level. */
    UFUNCTION (BlueprintPure, Category = "RPG|Progression")
    static int32 GetExperienceRemainingToNextLevel (int32 TotalExperience);

    UFUNCTION (BlueprintPure, Category = "RPG|Progression")
    static bool IsMaximumLevel (int32 Level);

    /** Clamps total XP to the valid cumulative range. */
    UFUNCTION (BlueprintPure, Category = "RPG|Progression")
    static int32 NormalizeExperience (int32 TotalExperience);

    /**
     * Validates serialized Level / Experience coherence without mutating either value.
     * Invalid raw XP (negative or above the cap) is considered inconsistent.
     */
    UFUNCTION (BlueprintPure, Category = "RPG|Progression")
    static bool IsLevelExperienceConsistent (int32 Level, int32 TotalExperience);
};
