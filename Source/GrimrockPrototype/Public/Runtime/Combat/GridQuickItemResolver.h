#pragma once

#include "CoreMinimal.h"
#include "RPG/RPGSkillTypes.h"
#include "Runtime/Combat/GridCombatModifierResolver.h"

/** C7 projection consumed by C8 to resolve Diffusion-style secondary targets. */
struct GRIMROCKPROTOTYPE_API FGridQuickItemSecondaryEffectProjection
{
	int32 TargetCount = 0;
	int32 MagnitudePercent = 0;
	int32 DurationPercent = 0;

	bool IsEnabled() const
	{
		return TargetCount > 0 && MagnitudePercent > 0 && DurationPercent > 0;
	}
};

/** Pure C7 resolver for QuickItem skill scaling and positive-effect modifiers. */
class GRIMROCKPROTOTYPE_API FGridQuickItemResolver
{
public:
	static int32 ResolveSkillRank(const TArray<FRPGSkillRank>& SkillRanks, FName SkillId);

	static bool ResolveEffectProfile(const FGridCombatActionDefinition& Definition, const TArray<FRPGSkillRank>& SkillRanks,
		const FGridResolvedCombatModifiers& Modifiers, FGridCombatActionEffectProfile& OutProfile);

	static void ApplyDirectDamageSkillScaling(const FGridCombatActionDefinition& Definition, const TArray<FRPGSkillRank>& SkillRanks,
		FGridAttackSourceStats& InOutSource);

	static FGridQuickItemSecondaryEffectProjection ResolveSecondaryEffect(const FGridResolvedCombatModifiers& Modifiers);
};
