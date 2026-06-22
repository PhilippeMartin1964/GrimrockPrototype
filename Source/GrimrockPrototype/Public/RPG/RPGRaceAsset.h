#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RPG/RPGCharacterTypes.h"
#include "RPGRaceAsset.generated.h"

UCLASS (BlueprintType)
class GRIMROCKPROTOTYPE_API URPGRaceAsset : public UPrimaryDataAsset
{
    GENERATED_BODY ()

public:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Race")
    FName RaceId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Race")
    FText DisplayName;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Race", meta = (MultiLine = "true"))
    FText Description;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Race")
    FRPGAttributes AttributeBonuses = FRPGAttributes { 0, 0, 0, 0, 0, 0 };

    UFUNCTION (BlueprintPure, Category = "RPG|Race")
    bool IsValidDefinition () const;
};
