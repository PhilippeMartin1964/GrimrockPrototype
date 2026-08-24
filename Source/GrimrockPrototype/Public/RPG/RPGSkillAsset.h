#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RPG/RPGSkillTypes.h"
#include "RPGSkillAsset.generated.h"

/** Data-driven definition of one ranked character skill. */
UCLASS (BlueprintType)
class GRIMROCKPROTOTYPE_API URPGSkillAsset : public UPrimaryDataAsset
{
    GENERATED_BODY ()

public:
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Skills")
    FName SkillId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Skills")
    FText DisplayName;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "RPG|Skills",
        meta = (MultiLine = "true"))
    FText Description;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Skills")
    ERPGSkillGoverningAttribute GoverningAttribute =
        ERPGSkillGoverningAttribute::None;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "RPG|Skills",
        meta = (ClampMin = "1"))
    int32 MaxRank = 5;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Skills")
    bool bAllowUntrainedChecks = true;

    UFUNCTION (BlueprintPure, Category = "RPG|Skills")
    bool IsValidDefinition () const;
};
