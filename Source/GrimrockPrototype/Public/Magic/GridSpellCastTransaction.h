#pragma once

#include "CoreMinimal.h"
#include "Magic/GridSpellTypes.h"
#include "Magic/GridSpellbookTypes.h"
#include "RPG/RPGCharacterTypes.h"
#include "Runtime/Combat/GridCombatTypes.h"
#include "GridSpellCastTransaction.generated.h"

UENUM (BlueprintType)
enum class EGridSpellCastTransactionRejectReason : uint8
{
    None                     UMETA (DisplayName = "None"),
    InvalidSpellDefinition   UMETA (DisplayName = "Invalid Spell Definition"),
    InvalidRequest           UMETA (DisplayName = "Invalid Request"),
    InvalidSpellbook         UMETA (DisplayName = "Invalid Spellbook"),
    CharacterMismatch        UMETA (DisplayName = "Character Mismatch"),
    SpellNotKnown            UMETA (DisplayName = "Spell Not Known"),
    TurnInactive             UMETA (DisplayName = "Turn Inactive"),
    InsufficientActionPoints UMETA (DisplayName = "Insufficient Action Points"),
    InsufficientMana         UMETA (DisplayName = "Insufficient Mana")
};

/** Receipt produced only after both resource costs have been committed. */
USTRUCT (BlueprintType)
struct FGridSpellCastCostReceipt
{
    GENERATED_BODY ()

    UPROPERTY (BlueprintReadOnly, Category = "Magic|Cast")
    FGuid CharacterId;

    UPROPERTY (BlueprintReadOnly, Category = "Magic|Cast")
    FName SpellId = NAME_None;

    UPROPERTY (BlueprintReadOnly, Category = "Magic|Cast")
    int32 ActionPointsSpent = 0;

    UPROPERTY (BlueprintReadOnly, Category = "Magic|Cast")
    int32 ManaSpent = 0;

    bool IsValid () const
    {
        return CharacterId.IsValid () &&
            !SpellId.IsNone () &&
            ActionPointsSpent >= 0 &&
            ManaSpent >= 0;
    }
};

/**
 * MON18.3 transaction boundary for spell costs.
 *
 * It reuses the authoritative character mana and combat turn state. It never
 * resolves targets and never applies spell effects. MON18.4 must validate the
 * target before calling TryCommitCosts in the integrated casting pipeline.
 */
struct GRIMROCKPROTOTYPE_API FGridSpellCastTransactionService
{
    static EGridSpellCastTransactionRejectReason ValidateCostCommit (
        const FGridSpellDefinition& Definition,
        const FGridSpellCastRequest& Request,
        const FGridCharacterSpellbookState& Spellbook,
        const FRPGDerivedStats& CharacterStats,
        const FGridPlayerCharacterTurnState& TurnState);

    /**
     * Atomic with respect to PA/mana: every validation completes before either
     * resource is mutated. On failure, both inputs remain unchanged.
     */
    static bool TryCommitCosts (
        const FGridSpellDefinition& Definition,
        const FGridSpellCastRequest& Request,
        const FGridCharacterSpellbookState& Spellbook,
        FRPGDerivedStats& InOutCharacterStats,
        FGridPlayerCharacterTurnState& InOutTurnState,
        FGridSpellCastCostReceipt& OutReceipt,
        EGridSpellCastTransactionRejectReason& OutRejectReason);
};
