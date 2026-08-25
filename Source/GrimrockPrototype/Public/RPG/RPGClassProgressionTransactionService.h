#pragma once

#include "CoreMinimal.h"
#include "RPG/RPGClassProgressionService.h"

class UGridPartyInventoryComponent;
struct FGridPartyInventoryState;
struct FRPGCharacterProgressionSaveState;

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
	ERPGClassProgressionCommitRejectReason RejectReason = ERPGClassProgressionCommitRejectReason::None;
	int32 GrantedPoints = 0;
	int32 SpentPointsBefore = 0;
	int32 SpentPointsAfter = 0;
	int32 RemainingPoints = 0;
	TArray<FName> CommittedChoiceIds;
};

DECLARE_MULTICAST_DELEGATE_FourParams(FRPGClassProgressionCommittedNativeSignature, UGridPartyInventoryComponent*, int32, const TArray<FName>&, int32);

/**
 * Authoritative runtime transaction and projection cache for class choices.
 *
 * MON15.6 persists the selected choices in UGrimrockPartySaveGame. RuntimeStates
 * remains a derived cache keyed by CharacterId and may be rebuilt from a save.
 */
struct GRIMROCKPROTOTYPE_API FRPGClassProgressionTransactionService
{
	/** Rebuilds automatic + committed requirement tags for one live character. */
	static bool RefreshCharacterProjection(UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex);

	/** Reads committed choices for the current character. */
	static bool TryGetSelectedChoiceIds(UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex, TArray<FName>& OutSelectedChoiceIds);

	/** Returns the point balance of the committed selection. */
	static bool TryGetChoicePointBalance(
		UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex, int32& OutGrantedPoints, int32& OutSpentPoints, int32& OutRemainingPoints);

	/**
     * Atomically appends a staged batch of choices. Nothing is mutated when
     * validation fails. A successful batch triggers one inventory notification.
     */
	static bool TryCommitChoices(UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex, const TArray<FName>& ChoiceIdsToCommit,
		FRPGClassProgressionCommitResult& OutResult);

	/** Adds the current runtime projection to a combat catalogue context. */
	static void AppendRuntimeSatisfiedRequirements(const FGuid& CharacterId, TSet<FName>& InOutSatisfiedRequirements);

	/**
     * Captures exactly one progression record per active character. The runtime
     * cache is the authority during play; missing cache entries serialize as an
     * empty selection.
     */
	static bool CapturePersistentState(const FGridPartyInventoryState& PartyState, TArray<FRPGCharacterProgressionSaveState>& OutStates, FText& OutError);

	/**
     * Validates and rebuilds detached runtime projections from persisted data.
     * The rebuilt projections become immediately consumable by MON12 catalogues
     * and bind to the live inventory component on the next refresh.
     */
	static bool RestorePersistentState(
		const FGridPartyInventoryState& PartyState, const TArray<FRPGCharacterProgressionSaveState>& SavedStates, FText& OutError);

	/** Test/session cache reset. Null clears all derived progression state. */
	static void ResetRuntimeState(UGridPartyInventoryComponent* PartyInventoryComponent = nullptr);

	static FRPGClassProgressionCommittedNativeSignature& OnClassProgressionCommitted();
};
