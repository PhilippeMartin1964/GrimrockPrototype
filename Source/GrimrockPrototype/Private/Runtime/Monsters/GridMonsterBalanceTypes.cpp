#include "Runtime/Monsters/GridMonsterBalanceTypes.h"

#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"

DEFINE_LOG_CATEGORY (LogGridMonsterBalance);

bool FGridMonsterBalanceAnalyzer::BuildSnapshot (
    const UGridMonsterDefinitionAsset* Definition,
    FGridMonsterBalanceSnapshot& OutSnapshot)
{
    OutSnapshot = FGridMonsterBalanceSnapshot ();
    if (!IsValid (Definition))
    {
        return false;
    }

    OutSnapshot.MonsterId = Definition->MonsterId;
    OutSnapshot.DangerLevel = Definition->DangerLevel;
    OutSnapshot.MaxHealth = Definition->MaxHealth;
    OutSnapshot.PhysicalArmor = Definition->PhysicalArmor;
    OutSnapshot.MagicalArmor = Definition->MagicalArmor;
    OutSnapshot.Initiative = Definition->Initiative;
    OutSnapshot.Accuracy = Definition->Accuracy;
    OutSnapshot.Evasion = Definition->Evasion;
    OutSnapshot.ActionPointsPerTurn =
        Definition->ActionPointsPerTurn;
    OutSnapshot.SightRangeCells = Definition->SightRangeCells;
    OutSnapshot.HearingRangeCells = Definition->HearingRangeCells;
    OutSnapshot.AttackCount = Definition->Attacks.Num ();
    OutSnapshot.ExperienceReward = Definition->ExperienceReward;

    if (Definition->Attacks.IsEmpty ())
    {
        return true;
    }

    int64 MidpointSumTimesTwo = 0;
    OutSnapshot.MinimumBaseDamage = MAX_int32;
    OutSnapshot.MaximumBaseDamage = MIN_int32;
    OutSnapshot.MinimumAttackActionPointCost = MAX_int32;
    OutSnapshot.MaximumAttackActionPointCost = MIN_int32;

    for (const FGridMonsterAttackDefinition& Attack :
        Definition->Attacks)
    {
        const int32 MinimumDamage =
            Attack.MinDamage + Attack.DamageBonus;
        const int32 MaximumDamage =
            Attack.MaxDamage + Attack.DamageBonus;

        OutSnapshot.MinimumBaseDamage = FMath::Min (
            OutSnapshot.MinimumBaseDamage,
            MinimumDamage);
        OutSnapshot.MaximumBaseDamage = FMath::Max (
            OutSnapshot.MaximumBaseDamage,
            MaximumDamage);
        OutSnapshot.MinimumAttackActionPointCost = FMath::Min (
            OutSnapshot.MinimumAttackActionPointCost,
            Attack.ActionPointCost);
        OutSnapshot.MaximumAttackActionPointCost = FMath::Max (
            OutSnapshot.MaximumAttackActionPointCost,
            Attack.ActionPointCost);
        MidpointSumTimesTwo +=
            static_cast<int64> (MinimumDamage) + MaximumDamage;
    }

    OutSnapshot.AverageBaseDamage =
        static_cast<float> (MidpointSumTimesTwo) /
        (2.0f * Definition->Attacks.Num ());
    return true;
}
