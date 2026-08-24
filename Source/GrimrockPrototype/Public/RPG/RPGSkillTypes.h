#pragma once

#include "CoreMinimal.h"
#include "RPGSkillTypes.generated.h"

/** Attribute contributing to one skill check. */
UENUM (BlueprintType)
enum class ERPGSkillGoverningAttribute : uint8
{
    None UMETA (DisplayName = "Aucun"),
    Strength UMETA (DisplayName = "Force"),
    Dexterity UMETA (DisplayName = "Dextérité"),
    Constitution UMETA (DisplayName = "Constitution"),
    Intelligence UMETA (DisplayName = "Intelligence"),
    Wisdom UMETA (DisplayName = "Sagesse"),
    Charisma UMETA (DisplayName = "Charisme")
};

/** Sparse runtime rank for one skill. Rank zero is represented by no entry. */
USTRUCT (BlueprintType)
struct FRPGSkillRank
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadWrite, Category = "RPG|Skills")
    FName SkillId = NAME_None;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadWrite,
        Category = "RPG|Skills",
        meta = (ClampMin = "0"))
    int32 Rank = 0;

    bool IsValid () const
    {
        return !SkillId.IsNone () && Rank > 0;
    }
};
