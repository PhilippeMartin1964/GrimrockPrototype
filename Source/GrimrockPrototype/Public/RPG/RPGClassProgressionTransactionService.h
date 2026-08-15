#pragma once

#include "CoreMinimal.h"
#include "RPG/RPGClassProgressionService.h"

class UGridPartyInventoryComponent;

enum class ERPGClassProgressionCommitRejectReason : uint8
{
    None,
    InvalidInventory,
    InvalidCharacter,
    InvalidClassDefinition,
    InvalidCurrentSelection,
    EmptyRequest,
    DuplicateRequest,
    UnknownChoice,
    AlreadySelected,
    LevelTooLow,
    MissingPrerequisite,
    InsufficientChoicePoints
};

struct FRPGClassProgressionCommitResult
{
    bool bCommitted = false;
    ERPGClassProgressionCommitRejectReason RejectReason =
        ERPGClassProgressionCommitRejectReason::None;
    int32 GrantedPoints = 0;
    int32 SpentPointsBefore = 0;
    int32 SpentPointsAfter = 0;
    int32 RemainingPoints = 0;
    TArray<FName> CommittedChoiceIds;
};

DECLARE_MULTICAST_DELEGATE_FourParams (
    FRPGClassProgressionCommittedNativeSignature,
    UGridPartyInventoryComponent*,
    int32,
    const TArray<FName>&,
    int32);

/**
 * MON15.5 authoritative in-session transaction for class progression choices.
 *
 * The runtime registry is intentionally transient. MON15.6 will move this
 * state into the SaveGame contract and own migration from older saves.
 */
struct GRIMROCKPROTOTYPE_API FRPGClassProgressionTransactionService
{
    /** Rebuilds automatic + committed requirement tags for one character. */
    static bool RefreshCharacterProjection (
        UGridPartyInventoryComponent* PartyInventoryComponent,
        int32 CharacterIndex);

    /** Reads committed choices for the current runtime session. */
    static bool TryGetSelectedChoiceIds (
        UGridPartyInventoryComponent* PartyInventoryComponent,
        int32 CharacterIndex,
        TArray<FName>& OutSelectedChoiceIds);

    /** Returns the point balance of the committed runtime selection. */
    static bool TryGetChoicePointBalance (
        UGridPartyInventoryComponent* PartyInventoryComponent,
        int32 CharacterIndex,
        int32& OutGrantedPoints,
        int32& OutSpentPoints,
        int32& OutRemainingPoints);

    /**
     * Atomically appends a staged batch of choices. Nothing is mutated when
     * validation fails. A successful batch triggers one inventory notification.
     */
    static bool TryCommitChoices (
        UGridPartyInventoryComponent* PartyInventoryComponent,
        int32 CharacterIndex,
        const TArray<FName>& ChoiceIdsToCommit,
        FRPGClassProgressionCommitResult& OutResult);

    /** Adds the current runtime projection to a combat catalogue context. */
    static void AppendRuntimeSatisfiedRequirements (
        const FGuid& CharacterId,
        TSet<FName>& InOutSatisfiedRequirements);

    /** Test/session reset. Null clears all transient progression state. */
    static void ResetRuntimeState (
        UGridPartyInventoryComponent* PartyInventoryComponent = nullptr);

    static FRPGClassProgressionCommittedNativeSignature&
        OnClassProgressionCommitted ();
};