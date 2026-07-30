#pragma once

#include "CoreMinimal.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "GridCombatLog.generated.h"

DECLARE_LOG_CATEGORY_EXTERN (LogGridCombat, Log, All);

UENUM (BlueprintType)
enum class EGridCombatLogEntryType : uint8
{
    CombatStarted,
    RoundStarted,
    PhaseChanged,
    MonsterTurnStarted,
    AttackHit,
    AttackMiss,
    CharacterDefeated,
    MonsterDefeated,
    Victory,
    Defeat
};

/** Transient, structured feedback emitted by the current combat encounter. */
USTRUCT (BlueprintType)
struct FGridCombatLogEntry
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Category = "Combat|Feedback")
    int32 SequenceNumber = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|Feedback")
    int32 RoundNumber = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|Feedback")
    EGridCombatPhase Phase = EGridCombatPhase::Exploration;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|Feedback")
    EGridCombatLogEntryType Type = EGridCombatLogEntryType::PhaseChanged;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|Feedback")
    FText Message;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|Feedback")
    FName SourceId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|Feedback")
    FText SourceDisplayName;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|Feedback")
    FName TargetId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|Feedback")
    FText TargetDisplayName;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|Feedback")
    int32 TargetCharacterIndex = INDEX_NONE;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|Feedback")
    FName AttackId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|Feedback")
    FName OffensiveItemDefinitionId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|Feedback")
    EGridEquipmentSlot OffensiveEquipmentSlot = EGridEquipmentSlot::None;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|Feedback")
    FGridAttackResult AttackResult;

    UPROPERTY (BlueprintReadOnly, Category = "Combat|Feedback")
    bool bTargetDefeated = false;
};

/** Pure localized text formatter. It never reads actors or resolves combat. */
class GRIMROCKPROTOTYPE_API FGridCombatLogFormatter
{
public:
    static FText FormatCombatStarted ();
    static FText FormatRoundStarted (int32 RoundNumber);
    static FText FormatPhaseChanged (EGridCombatPhase Phase);
    static FText FormatMonsterTurnStarted (const FText& MonsterName);

    static FText FormatMonsterAttack (
        const FText& MonsterName,
        const FText& TargetName,
        FName AttackId,
        const FGridAttackResult& Result);

    static FText FormatPlayerAttack (
        const FText& CharacterName,
        const FText& MonsterName,
        FName AttackId,
        const FGridAttackResult& Result);

    static FText FormatCharacterDefeated (const FText& CharacterName);
    static FText FormatMonsterDefeated (const FText& MonsterName);
    static FText FormatCombatEnded (EGridCombatPhase ResultPhase);
};
