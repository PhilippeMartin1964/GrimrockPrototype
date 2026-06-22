#pragma once

#include "CoreMinimal.h"
#include "RPGCharacterTypes.generated.h"

USTRUCT (BlueprintType)
struct FRPGAttributes
{
    GENERATED_BODY ()

    FRPGAttributes () = default;

    FRPGAttributes (
        int32 InStrength,
        int32 InDexterity,
        int32 InConstitution,
        int32 InIntelligence,
        int32 InWisdom,
        int32 InCharisma)
        : Strength (InStrength)
        , Dexterity (InDexterity)
        , Constitution (InConstitution)
        , Intelligence (InIntelligence)
        , Wisdom (InWisdom)
        , Charisma (InCharisma)
    {
    }

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Attributes")
    int32 Strength = 10;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Attributes")
    int32 Dexterity = 10;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Attributes")
    int32 Constitution = 10;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Attributes")
    int32 Intelligence = 10;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Attributes")
    int32 Wisdom = 10;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Attributes")
    int32 Charisma = 10;
};

USTRUCT (BlueprintType)
struct FRPGDerivedStats
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Derived Stats")
    int32 MaxHealth = 1;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Derived Stats")
    int32 CurrentHealth = 1;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Derived Stats")
    int32 MaxMana = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Derived Stats")
    int32 CurrentMana = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Derived Stats")
    int32 PhysicalArmor = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Derived Stats")
    int32 MagicalArmor = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Derived Stats")
    int32 Initiative = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Derived Stats")
    int32 Accuracy = 0;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Derived Stats")
    int32 Evasion = 0;
};
