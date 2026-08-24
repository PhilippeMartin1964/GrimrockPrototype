#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RPG/RPGSkillTypes.h"
#include "RPGSkillAsset.generated.h"

/** One binary RequirementId grant unlocked when a Skill reaches a rank. */
USTRUCT (BlueprintType)
struct GRIMROCKPROTOTYPE_API FRPGSkillRequirementGrant
{
    GENERATED_BODY ()

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "RPG|Skills|Requirements",
        meta = (ClampMin = "1"))
    int32 MinimumRank = 1;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "RPG|Skills|Requirements")
    TArray<FName> GrantedRequirementIds;

    bool IsValid (int32 SkillMaxRank) const
    {
        if (MinimumRank < 1 ||
            MinimumRank > SkillMaxRank ||
            GrantedRequirementIds.IsEmpty ())
        {
            return false;
        }

        TSet<FName> UniqueRequirementIds;
        for (const FName RequirementId : GrantedRequirementIds)
        {
            if (RequirementId.IsNone () ||
                UniqueRequirementIds.Contains (RequirementId))
            {
                return false;
            }
            UniqueRequirementIds.Add (RequirementId);
        }
        return true;
    }
};

/** Data-driven definition of one ranked character skill. */
UCLASS (BlueprintType)
class GRIMROCKPROTOTYPE_API URPGSkillAsset : public UPrimaryDataAsset
{
    GENERATED_BODY ()

public:
    /** Stable canonical identity: RPGSkill:<SkillId>. */
    virtual FPrimaryAssetId GetPrimaryAssetId () const override;

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

    /** Extra binary capabilities granted at explicit rank thresholds. */
    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "RPG|Skills|Requirements")
    TArray<FRPGSkillRequirementGrant> RequirementGrants;

    UFUNCTION (BlueprintPure, Category = "RPG|Skills")
    bool IsValidDefinition () const;
};
