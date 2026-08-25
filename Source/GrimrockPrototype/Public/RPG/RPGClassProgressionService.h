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
	InsufficientChoicePoints
};

/**
 * Pure MON15.4 progression rules. It derives class choice currency and granted
 * requirement tags without mutating character state. MON15.5 will own the
 * selection transaction; MON15.6 will own persistence/migration.
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

	/** Convenience overload for the current runtime before MON15.5 selections. */
	static bool CollectAutomaticSatisfiedRequirements(const URPGClassAsset* ClassDefinition, int32 CharacterLevel, TSet<FName>& OutSatisfiedRequirements);
};
