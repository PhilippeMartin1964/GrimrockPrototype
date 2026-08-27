#pragma once

#include "CoreMinimal.h"
#include "RPG/RPGClassProgressionService.h"

class UGridPartyInventoryComponent;
struct FGridPartyInventoryState;

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
 * Class progression transaction service.
 *
 * TD07.3.3.5 makes FGridCharacterInventoryState::SelectedClassProgressionChoiceIds
 * the durable authority. RuntimeStates contains only reconstructible requirement
 * projections keyed by CharacterId.
 */
struct GRIMROCKPROTOTYPE_API FRPGClassProgressionTransactionService
{
	/** Rebuilds automatic + committed requirement tags for one live character. */
	static bool RefreshCharacterProjection(UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex);

	/** Reads committed choices from the character authority. */
	static bool TryGetSelectedChoiceIds(UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex, TArray<FName>& OutSelectedChoiceIds);

	/** Returns the point balance of the committed selection. */
	static bool TryGetChoicePointBalance(
		UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex, int32& OutGrantedPoints, int32& OutSpentPoints, int32& OutRemainingPoints);

	/** Atomically appends a staged batch of choices and rebuilds the derived projection. */
	static bool TryCommitChoices(UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex, const TArray<FName>& ChoiceIdsToCommit,
		FRPGClassProgressionCommitResult& OutResult);

	/** Adds the current derived runtime projection to a combat catalogue context. */
	static void AppendRuntimeSatisfiedRequirements(const FGuid& CharacterId, TSet<FName>& InOutSatisfiedRequirements);

	/** Rebuilds detached derived projections from authoritative character state, e.g. after SaveGame load. */
	static bool RebuildRuntimeProjection(const FGridPartyInventoryState& PartyState, FText& OutError);

	/** Test/session cache reset. Null clears all derived progression state. */
	static void ResetRuntimeState(UGridPartyInventoryComponent* PartyInventoryComponent = nullptr);

	static FRPGClassProgressionCommittedNativeSignature& OnClassProgressionCommitted();
};
