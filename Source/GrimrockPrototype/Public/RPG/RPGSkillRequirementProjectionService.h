#pragma once

#include "CoreMinimal.h"

class URPGSkillAsset;
struct FGridCharacterInventoryState;

/**
 * Pure MON20.8 bridge from ranked Skills to the existing RequirementId
 * vocabulary. RequirementIds are derived on demand and are never persisted.
 */
struct GRIMROCKPROTOTYPE_API FRPGSkillRequirementProjectionService
{
    /**
     * Atomically appends all requirements granted by the character's positive
     * skill ranks. Failure preserves OutRequirements exactly as supplied.
     */
    static bool AppendSatisfiedRequirements (
        const FGridCharacterInventoryState& CharacterState,
        TSet<FName>& OutRequirements,
        FString& OutError);

    /**
     * Testable overload with an injected canonical SkillId resolver.
     */
    static bool AppendSatisfiedRequirements (
        const FGridCharacterInventoryState& CharacterState,
        TFunctionRef<const URPGSkillAsset* (FName)> DefinitionResolver,
        TSet<FName>& OutRequirements,
        FString& OutError);

    /** Resolve one canonical RPGSkill PrimaryDataAsset by its stable SkillId. */
    static const URPGSkillAsset* ResolveDefinitionBySkillId (FName SkillId);
};
