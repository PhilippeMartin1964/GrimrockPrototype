#pragma once

#include "CoreMinimal.h"
#include "RPGCharacterTypes.generated.h"

class URPGClassAsset;
class URPGRaceAsset;
class UTexture2D;

USTRUCT(BlueprintType)
struct FRPGAttributes
{
	GENERATED_BODY()

	FRPGAttributes() = default;

	FRPGAttributes(int32 InStrength, int32 InDexterity, int32 InConstitution, int32 InIntelligence, int32 InWisdom, int32 InCharisma)
		: Strength(InStrength)
		, Dexterity(InDexterity)
		, Constitution(InConstitution)
		, Intelligence(InIntelligence)
		, Wisdom(InWisdom)
		, Charisma(InCharisma)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Attributes")
	int32 Strength = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Attributes")
	int32 Dexterity = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Attributes")
	int32 Constitution = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Attributes")
	int32 Intelligence = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Attributes")
	int32 Wisdom = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Attributes")
	int32 Charisma = 10;
};

USTRUCT(BlueprintType)
struct FRPGDerivedStats
{
	GENERATED_BODY()

	/** Reconstructible maximum derived from attributes, class and level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Derived Stats")
	int32 MaxHealth = 1;

	/** Reconstructible maximum derived from class and level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Derived Stats")
	int32 MaxMana = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Derived Stats")
	int32 Initiative = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Derived Stats")
	int32 Accuracy = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Derived Stats")
	int32 Evasion = 0;
};

/**
 * Durable mutable resources for one party character.
 *
 * TD07.3.3.3 separates these values from FRPGDerivedStats because damage,
 * mana spending and armor absorption mutate them directly at runtime.
 */
USTRUCT(BlueprintType)
struct FRPGCharacterResources
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Resources")
	int32 CurrentHealth = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Resources")
	int32 CurrentMana = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Resources")
	int32 CurrentPhysicalArmor = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Resources")
	int32 CurrentMagicalArmor = 0;
};

UENUM(BlueprintType)
enum class ERPGCharacterPortraitGender : uint8
{
	Male UMETA(DisplayName = "Masculin"),
	Female UMETA(DisplayName = "Féminin")
};

/** Transient purpose of one character-creation wizard instance. */
UENUM(BlueprintType)
enum class ERPGCharacterCreationContext : uint8
{
	NewGameMainHero UMETA(DisplayName = "Nouveau héros"),
	CustomRecruit UMETA(DisplayName = "Recrue personnalisée")
};

USTRUCT(BlueprintType)
struct FRPGCharacterPortraitVariant
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Portrait")
	FName VariantId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Portrait")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Portrait")
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Portrait", meta = (MultiLine = "true"))
	FText Description;

	bool IsValidDefinition() const
	{
		return !VariantId.IsNone() && !Portrait.IsNull();
	}
};

USTRUCT(BlueprintType)
struct FRPGRaceIllustrationOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Race Illustration")
	FName RaceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Race Illustration")
	ERPGCharacterPortraitGender Gender = ERPGCharacterPortraitGender::Male;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Race Illustration")
	TSoftObjectPtr<UTexture2D> Illustration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Race Illustration")
	TSoftObjectPtr<UTexture2D> RaceEnvironment;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Race Illustration")
	FText DisplayName;

	bool IsValidDefinition() const
	{
		return !RaceId.IsNone() && !Illustration.IsNull();
	}

	bool Matches(FName InRaceId, ERPGCharacterPortraitGender InGender) const
	{
		return IsValidDefinition() && RaceId == InRaceId && Gender == InGender;
	}
};

USTRUCT(BlueprintType)
struct FRPGCharacterVisualSelection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Visual")
	FName RaceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Visual")
	ERPGCharacterPortraitGender Gender = ERPGCharacterPortraitGender::Male;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Visual")
	FName PortraitVariantId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Visual")
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Visual")
	FName ClassId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Visual")
	TSoftObjectPtr<UTexture2D> ClassIcon;
};

USTRUCT(BlueprintType)
struct FRPGCharacterPortraitOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Portrait")
	FName PortraitId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Portrait")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Portrait")
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation|Portrait", meta = (MultiLine = "true"))
	FText Description;

	bool IsValidDefinition() const
	{
		return !PortraitId.IsNone() && !Portrait.IsNull();
	}
};

USTRUCT(BlueprintType)
struct FRPGCharacterCreationRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation")
	TObjectPtr<URPGRaceAsset> RaceDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation")
	TObjectPtr<URPGClassAsset> ClassDefinition = nullptr;

	/**
     * Canonical asset retained for runtime class actions when ClassDefinition
     * is a transient copy used to preview allocated attributes.
     */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation")
	TObjectPtr<URPGClassAsset> CombatActionSourceClassDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation")
	ERPGCharacterPortraitGender PortraitGender = ERPGCharacterPortraitGender::Male;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation")
	FName PortraitVariantId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation")
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RPG|Character Creation")
	TSoftObjectPtr<UTexture2D> ClassIcon;
};
