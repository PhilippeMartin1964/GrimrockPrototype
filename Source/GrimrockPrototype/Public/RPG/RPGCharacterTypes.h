#pragma once

#include "CoreMinimal.h"
#include "RPGCharacterTypes.generated.h"

class URPGClassAsset;
class URPGRaceAsset;
class UTexture2D;

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

UENUM (BlueprintType)
enum class ERPGCharacterPortraitGender : uint8
{
    Male UMETA (DisplayName = "Masculin"),
    Female UMETA (DisplayName = "Feminin")
};

USTRUCT (BlueprintType)
struct FRPGCharacterPortraitVariant
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Portrait")
    FName VariantId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Portrait")
    FText DisplayName;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Portrait")
    TSoftObjectPtr<UTexture2D> Portrait;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Portrait", meta = (MultiLine = "true"))
    FText Description;

    bool IsValidDefinition () const
    {
        return !VariantId.IsNone () && !Portrait.IsNull ();
    }
};

USTRUCT (BlueprintType)
struct FRPGCharacterVisualSelection
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Visual")
    FName RaceId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Visual")
    ERPGCharacterPortraitGender Gender = ERPGCharacterPortraitGender::Male;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Visual")
    FName PortraitVariantId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Visual")
    TSoftObjectPtr<UTexture2D> Portrait;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Visual")
    FName ClassId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Visual")
    TSoftObjectPtr<UTexture2D> ClassIcon;
};

USTRUCT (BlueprintType)
struct FRPGCharacterPortraitOption
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Portrait")
    FName PortraitId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Portrait")
    FText DisplayName;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Portrait")
    TSoftObjectPtr<UTexture2D> Portrait;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Portrait", meta = (MultiLine = "true"))
    FText Description;

    bool IsValidDefinition () const
    {
        return !PortraitId.IsNone () && !Portrait.IsNull ();
    }
};

USTRUCT (BlueprintType)
struct FRPGCharacterCreationRequest
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation")
    FText DisplayName;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation")
    TObjectPtr<URPGRaceAsset> RaceDefinition = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation")
    TObjectPtr<URPGClassAsset> ClassDefinition = nullptr;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation")
    ERPGCharacterPortraitGender PortraitGender = ERPGCharacterPortraitGender::Male;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation")
    FName PortraitVariantId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation")
    TSoftObjectPtr<UTexture2D> Portrait;

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation")
    TSoftObjectPtr<UTexture2D> ClassIcon;
};
