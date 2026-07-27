#include "Runtime/Combat/GridCombatLog.h"

#define LOCTEXT_NAMESPACE "GridCombatLog"

DEFINE_LOG_CATEGORY (LogGridCombat);

namespace
{
    FText FormatAttackLead (
        const FText& MonsterName,
        const FText& TargetName,
        FName AttackId)
    {
        FFormatNamedArguments Arguments;
        Arguments.Add (TEXT ("Monster"), MonsterName);
        Arguments.Add (TEXT ("Target"), TargetName);
        Arguments.Add (TEXT ("Attack"), FText::FromName (AttackId));
        return FText::Format (
            LOCTEXT (
                "MonsterAttackLead",
                "{Monster} attaque {Target} avec {Attack}"),
            Arguments);
    }

    FText FormatAppliedDamage (const FGridAttackResult& Result)
    {
        TArray<FText> Parts;
        if (Result.PhysicalArmorDamage > 0)
        {
            Parts.Add (FText::Format (
                LOCTEXT ("PhysicalArmorDamage", "{0} armure physique"),
                FText::AsNumber (Result.PhysicalArmorDamage)));
        }
        if (Result.MagicalArmorDamage > 0)
        {
            Parts.Add (FText::Format (
                LOCTEXT ("MagicalArmorDamage", "{0} armure magique"),
                FText::AsNumber (Result.MagicalArmorDamage)));
        }
        if (Result.HealthDamage > 0)
        {
            Parts.Add (FText::Format (
                LOCTEXT ("HealthDamage", "{0} PV"),
                FText::AsNumber (Result.HealthDamage)));
        }

        FText DamageBreakdown;
        if (Parts.Num () == 1 && Result.HealthDamage > 0)
        {
            DamageBreakdown = FText::Format (
                LOCTEXT ("HealthOnlyDamage", "{0} dégâts"),
                FText::AsNumber (Result.HealthDamage));
        }
        else
        {
            DamageBreakdown = FText::Join (
                LOCTEXT ("DamageSeparator", ", "),
                Parts);
        }

        if (Result.HealthDamage > 0)
        {
            FFormatNamedArguments Arguments;
            Arguments.Add (TEXT ("Damage"), DamageBreakdown);
            Arguments.Add (
                TEXT ("Before"),
                FText::AsNumber (Result.TargetHealthBefore));
            Arguments.Add (
                TEXT ("After"),
                FText::AsNumber (Result.TargetHealthAfter));
            return FText::Format (
                LOCTEXT (
                    "DamageWithHealth",
                    "{Damage}, {Before} \u2192 {After} PV"),
                Arguments);
        }

        return FText::Format (
            LOCTEXT (
                "ArmorAbsorbedDamage",
                "{0} dégâts absorbés par l'armure"),
            FText::AsNumber (Result.GetTotalAppliedDamage ()));
    }
}

FText FGridCombatLogFormatter::FormatCombatStarted ()
{
    return LOCTEXT ("CombatStarted", "Le combat commence.");
}

FText FGridCombatLogFormatter::FormatRoundStarted (int32 RoundNumber)
{
    return FText::Format (
        LOCTEXT ("RoundStarted", "Manche {0}."),
        FText::AsNumber (RoundNumber));
}

FText FGridCombatLogFormatter::FormatPhaseChanged (EGridCombatPhase Phase)
{
    switch (Phase)
    {
    case EGridCombatPhase::Exploration:
        return LOCTEXT ("ExplorationPhase", "Retour à l'exploration.");
    case EGridCombatPhase::StartingCombat:
        return LOCTEXT ("StartingCombatPhase", "Préparation du combat.");
    case EGridCombatPhase::PlayerPhase:
        return LOCTEXT ("PlayerPhase", "Phase du groupe.");
    case EGridCombatPhase::EnemyPhase:
        return LOCTEXT ("EnemyPhase", "Phase des ennemis.");
    case EGridCombatPhase::EndingRound:
        return LOCTEXT ("EndingRoundPhase", "Fin de la manche.");
    case EGridCombatPhase::Victory:
        return FormatCombatEnded (EGridCombatPhase::Victory);
    case EGridCombatPhase::Defeat:
        return FormatCombatEnded (EGridCombatPhase::Defeat);
    default:
        return FText::GetEmpty ();
    }
}

FText FGridCombatLogFormatter::FormatMonsterTurnStarted (
    const FText& MonsterName)
{
    return FText::Format (
        LOCTEXT ("MonsterTurnStarted", "Tour de {0}."),
        MonsterName);
}

FText FGridCombatLogFormatter::FormatMonsterAttack (
    const FText& MonsterName,
    const FText& TargetName,
    FName AttackId,
    const FGridAttackResult& Result)
{
    const FText Lead = FormatAttackLead (
        MonsterName,
        TargetName,
        AttackId);

    if (!Result.bHit)
    {
        FFormatNamedArguments Arguments;
        Arguments.Add (TEXT ("Lead"), Lead);
        Arguments.Add (TEXT ("Roll"), FText::AsNumber (Result.AttackRoll));
        Arguments.Add (
            TEXT ("Defense"),
            FText::AsNumber (Result.DefenseValue));
        return FText::Format (
            LOCTEXT (
                "MonsterAttackMiss",
                "{Lead} : échec (jet {Roll} contre défense {Defense})."),
            Arguments);
    }

    FFormatNamedArguments Arguments;
    Arguments.Add (TEXT ("Lead"), Lead);
    Arguments.Add (TEXT ("Damage"), FormatAppliedDamage (Result));
    return Result.bCriticalHit
        ? FText::Format (
            LOCTEXT (
                "MonsterAttackCritical",
                "{Lead} : coup critique, {Damage}."),
            Arguments)
        : FText::Format (
            LOCTEXT ("MonsterAttackHit", "{Lead} : {Damage}."),
            Arguments);
}

FText FGridCombatLogFormatter::FormatCharacterDefeated (
    const FText& CharacterName)
{
    return FText::Format (
        LOCTEXT ("CharacterDefeated", "{0} est hors combat."),
        CharacterName);
}

FText FGridCombatLogFormatter::FormatMonsterDefeated (
    const FText& MonsterName)
{
    return FText::Format (
        LOCTEXT ("MonsterDefeated", "{0} est vaincu."),
        MonsterName);
}

FText FGridCombatLogFormatter::FormatCombatEnded (
    EGridCombatPhase ResultPhase)
{
    return ResultPhase == EGridCombatPhase::Victory
        ? LOCTEXT ("Victory", "Victoire.")
        : LOCTEXT ("Defeat", "Défaite.");
}

#undef LOCTEXT_NAMESPACE
