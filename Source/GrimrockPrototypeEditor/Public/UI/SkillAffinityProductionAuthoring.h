#pragma once

#include "CoreMinimal.h"

class URPGSkillPresentationAsset;

/**
 * Canonical production authoring contract for UI-SKILLS-AFFINITY01.3C.
 * Only DiscouragedClassIds may be changed by ApplyCanonicalDiscouragedAffinities().
 */
struct GRIMROCKPROTOTYPEEDITOR_API FSkillAffinityProductionAuthoring
{
	static constexpr int32 ExpectedProductionSkillCount = 25;

	/** Returns the canonical discouraged classes for SkillId. Non-listed Skills return an empty array. */
	static void GetCanonicalDiscouragedClassIds(FName SkillId, TArray<FName>& OutClassIds);

	/** Applies the canonical DiscouragedClassIds matrix while preserving every other presentation field. */
	static bool ApplyCanonicalDiscouragedAffinities(
		URPGSkillPresentationAsset& Catalog,
		int32& OutChangedSkillCount,
		FString& OutError);

	/** Verifies that all 25 production rows carry exactly the canonical discouraged matrix. */
	static bool HasCanonicalDiscouragedAffinities(
		const URPGSkillPresentationAsset& Catalog,
		FString& OutError);
};
