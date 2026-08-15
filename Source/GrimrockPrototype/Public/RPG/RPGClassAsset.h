#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RPG/RPGCharacterTypes.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "RPGClassAsset.generated.h"

/** Automatic class progression granted when a character reaches one level. */
USTRUCT (BlueprintType)
struct FRPGClassProgressionLevelGrant
{
    GENERATED_BODY ()

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "RPG|Class|Progression",
        meta = (ClampMin = "1"))
    int32 Level = 1;

    /** Choice currency earned cumulatively. MON15.4 only models it; MON15.5 spends it. */
    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "RPG|Class|Progression",
        meta = (ClampMin = "0"))
    int32 ChoicePointsGranted = 0;

    /** Requirement tags automatically satisfied at this level and above. */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Class|Progression")
    TArray<FName> GrantedRequirementIds;
};

/** One optional class progression choice offered to the player. */
USTRUCT (BlueprintType)
struct FRPGClassProgressionChoiceDefinition
{
    GENERATED_BODY ()

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Class|Progression")
    FName ChoiceId = NAME_None;

    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Class|Progression")
    FText DisplayName;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "RPG|Class|Progression",
        meta = (MultiLine = "true"))
    FText Description;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "RPG|Class|Progression",
        meta = (ClampMin = "1"))
    int32 MinimumLevel = 1;

    UPROPERTY (
        EditAnywhere,
        BlueprintReadOnly,
        Category = "RPG|Class|Progression",
        meta = (ClampMin = "1"))
    int32 PointCost = 1;

    /** Other ChoiceIds that must already be selected. */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Class|Progression")
    TArray<FName> PrerequisiteChoiceIds;

    /** Extra generic requirement tags satisfied by this choice. ChoiceId itself is also granted. */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Class|Progression")
    TArray<FName> GrantedRequirementIds;
};

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

    /** Class abilities and spells contributed to the common combat catalogue. */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Class|Combat Actions")
    TArray<FGridCombatActionDefinition> CombatActions;

    /** Data-driven automatic grants. Entries are cumulative by character level. */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Class|Progression")
    TArray<FRPGClassProgressionLevelGrant> ProgressionLevelGrants;

    /** Optional choices evaluated by MON15.4 and committed later by MON15.5. */
    UPROPERTY (EditAnywhere, BlueprintReadOnly, Category = "RPG|Class|Progression")
    TArray<FRPGClassProgressionChoiceDefinition> ProgressionChoices;

    UFUNCTION (BlueprintPure, Category = "RPG|Class")
    bool IsValidDefinition () const;

    const FRPGClassProgressionChoiceDefinition* FindProgressionChoice (
        FName ChoiceId) const;
};
