#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RPG/RPGCharacterTypes.h"
#include "RPGCharacterPortraitSetAsset.generated.h"

UCLASS (BlueprintType)
class GRIMROCKPROTOTYPE_API URPGCharacterPortraitSetAsset : public UPrimaryDataAsset
{
    GENERATED_BODY ()

public:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Portrait Set")
    FName RaceId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Portrait Set")
    FText DisplayName;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Portrait Set", meta = (MultiLine = "true"))
    FText Description;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Portrait Set")
    TArray<FRPGCharacterPortraitVariant> MalePortraits;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Portrait Set")
    TArray<FRPGCharacterPortraitVariant> FemalePortraits;

    UFUNCTION (BlueprintPure, Category = "RPG|Portrait Set")
    bool IsValidDefinition () const;

    UFUNCTION (BlueprintPure, Category = "RPG|Portrait Set")
    bool IsValidForRace (FName InRaceId) const;

    UFUNCTION (BlueprintPure, Category = "RPG|Portrait Set")
    void GetPortraitsForGender (
        ERPGCharacterPortraitGender Gender,
        TArray<FRPGCharacterPortraitVariant>& OutPortraits) const;

    UFUNCTION (BlueprintPure, Category = "RPG|Portrait Set")
    bool FindPortraitVariant (
        ERPGCharacterPortraitGender Gender,
        FName VariantId,
        FRPGCharacterPortraitVariant& OutVariant) const;

    UFUNCTION (BlueprintPure, Category = "RPG|Portrait Set")
    bool GetFirstValidPortrait (
        ERPGCharacterPortraitGender Gender,
        FRPGCharacterPortraitVariant& OutVariant) const;

    const TArray<FRPGCharacterPortraitVariant>& GetPortraitsForGenderRef (
        ERPGCharacterPortraitGender Gender) const;
};
