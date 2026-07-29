#pragma once

#include "CoreMinimal.h"
#include "GridMonsterBalanceTypes.generated.h"

class UGridMonsterDefinitionAsset;

DECLARE_LOG_CATEGORY_EXTERN (LogGridMonsterBalance, Log, All);

/** Raw, data-oriented overview. This is not a complete DPS calculation. */
USTRUCT (BlueprintType)
struct FGridMonsterBalanceSnapshot
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly)
    FName MonsterId = NAME_None;

    UPROPERTY (BlueprintReadOnly)
    int32 DangerLevel = 0;

    UPROPERTY (BlueprintReadOnly)
    int32 MaxHealth = 0;

    UPROPERTY (BlueprintReadOnly)
    int32 PhysicalArmor = 0;

    UPROPERTY (BlueprintReadOnly)
    int32 MagicalArmor = 0;

    UPROPERTY (BlueprintReadOnly)
    int32 Initiative = 0;

    UPROPERTY (BlueprintReadOnly)
    int32 Accuracy = 0;

    UPROPERTY (BlueprintReadOnly)
    int32 Evasion = 0;

    UPROPERTY (BlueprintReadOnly)
    int32 ActionPointsPerTurn = 0;

    UPROPERTY (BlueprintReadOnly)
    int32 SightRangeCells = 0;

    UPROPERTY (BlueprintReadOnly)
    int32 HearingRangeCells = 0;

    UPROPERTY (BlueprintReadOnly)
    int32 AttackCount = 0;

    UPROPERTY (BlueprintReadOnly)
    int32 MinimumBaseDamage = 0;

    UPROPERTY (BlueprintReadOnly)
    int32 MaximumBaseDamage = 0;

    UPROPERTY (BlueprintReadOnly)
    float AverageBaseDamage = 0.0f;

    UPROPERTY (BlueprintReadOnly)
    int32 MinimumAttackActionPointCost = 0;

    UPROPERTY (BlueprintReadOnly)
    int32 MaximumAttackActionPointCost = 0;

    UPROPERTY (BlueprintReadOnly)
    int32 ExperienceReward = 0;
};

class GRIMROCKPROTOTYPE_API FGridMonsterBalanceAnalyzer
{
public:
    static bool BuildSnapshot (
        const UGridMonsterDefinitionAsset* Definition,
        FGridMonsterBalanceSnapshot& OutSnapshot);
};
