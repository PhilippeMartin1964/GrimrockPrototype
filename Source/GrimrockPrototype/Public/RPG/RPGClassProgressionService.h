#pragma once

#include "CoreMinimal.h"

class URPGClassAsset;

enum class ERPGClassProgressionChoiceAvailabilityReason : uint8
{
	None,
	InvalidClassDefinition,
	InvalidLevel,
	InvalidSelectionState,
	UnknownChoice,
	AlreadySelected,
	LevelTooLow,
	MissingPrerequisite,
	InsufficientChoicePoints,
	MutuallyExclusiveChoice
};

/**
 * Pure class-progression rules. It derives Talent Point currency and granted
 * requirement tags without mutating character state.
 * FRPGClassProgressionTransactionService owns live selection transactions.
 */
struct GRIMROCKPROTOTYPE_API FRPGClassProgressionService
{
	static int32 GetTotalChoicePointsGranted(const URPGClassAsset* ClassDefinition, int32 CharacterLevel);

	/**
     * Validates a hypothetical set of selected choices and returns its point
     * balance. Outputs are reset to zero when validation fails.
     */
	static bool TryGetChoicePointBalance(const URPGClassAsset* ClassDefinition, int32 CharacterLevel, const TSet<FName>& SelectedChoiceIds,
		int32& OutGrantedPoints, int32& OutSpentPoints, int32& OutRemainingPoints);

	static ERPGClassProgressionChoiceAvailabilityReason GetChoiceAvailability(
		const URPGClassAsset* ClassDefinition, int32 CharacterLevel, const TSet<FName>& SelectedChoiceIds, FName ChoiceId);

	/**
     * Builds the generic requirement set granted by class identity, automatic
     * level grants and a validated hypothetical choice set.
     */
	static bool CollectSatisfiedRequirements(
		const URPGClassAsset* ClassDefinition, int32 CharacterLevel, const TSet<FName>& SelectedChoiceIds, TSet<FName>& OutSatisfiedRequirements);

};
