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
};
