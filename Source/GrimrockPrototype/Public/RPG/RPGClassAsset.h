#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RPG/RPGCharacterTypes.h"
#include "RPGClassAsset.generated.h"

UCLASS (BlueprintType)
class GRIMROCKPROTOTYPE_API URPGClassAsset : public UPrimaryDataAsset
{
    GENERATED_BODY ()

public:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Class")
    FName ClassId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Class")
    FText DisplayName;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Class", meta = (MultiLine = "true"))
    FText Description;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Class")
    FRPGAttributes BaseAttributes;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Class", meta = (ClampMin = "1"))
    int32 HealthAtLevelOne = 1;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Class", meta = (ClampMin = "0"))
    int32 HealthPerLevel = 0;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Class", meta = (ClampMin = "0"))
    int32 ManaAtLevelOne = 0;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Class", meta = (ClampMin = "0"))
    int32 ManaPerLevel = 0;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Class", meta = (ClampMin = "0"))
    int32 BasePhysicalArmor = 0;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Class", meta = (ClampMin = "0"))
    int32 BaseMagicalArmor = 0;

    UFUNCTION (BlueprintPure, Category = "RPG|Class")
    bool IsValidDefinition () const;
};
