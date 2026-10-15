#pragma once

#include "CoreMinimal.h"
#include "UI/GridSkillsUiTypes.h"

class UGridPartyInventoryComponent;
class URPGSkillAsset;
class URPGSkillPresentationAsset;

/**
 * Stateless MON20.8.4 read-model builder for the Skills/Talents page.
 * It owns no character, Skill, Talent or progression state.
 */
struct GRIMROCKPROTOTYPE_API FGridSkillsPageService
{
	static bool TryBuildCharacterView(UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex,
		const TArray<const URPGSkillAsset*>& SkillDefinitions, FGridSkillsPageView& OutView);

	static bool TryBuildSelectedCharacterView(
		UGridPartyInventoryComponent* PartyInventoryComponent, const TArray<const URPGSkillAsset*>& SkillDefinitions, FGridSkillsPageView& OutView);

	/** Preserves a valid preferred SkillId, otherwise selects the first player-facing Skill entry. */
	static FName ResolveSkillSelection(const FGridSkillsPageView& PageView, FName PreferredSkillId);

	/** Builds the presentation-ready detail view for one Skill without mutating character or page state. */
	static bool TryBuildSkillDetailView(
		UGridPartyInventoryComponent* PartyInventoryComponent,
		const FGridSkillsPageView& PageView,
		FName SkillId,
		const TArray<const URPGSkillAsset*>& SkillDefinitions,
		const URPGSkillPresentationAsset* PresentationCatalog,
		FGridSkillDetailView& OutView);

	/** Loads valid canonical RPGSkill PrimaryDataAssets in stable SkillId order. */
	static void ResolveCanonicalSkillDefinitions(TArray<const URPGSkillAsset*>& OutDefinitions);
};
