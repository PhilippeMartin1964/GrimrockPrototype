#pragma once

#include "CoreMinimal.h"
#include "UI/GridSkillsUiTypes.h"

class UGridPartyInventoryComponent;
class URPGSkillAsset;

/**
 * Stateless MON20.8.4 read-model builder for the Skills/Talents page.
 * It owns no character, Skill, Talent or progression state.
 */
struct GRIMROCKPROTOTYPE_API FGridSkillsPageService
{
    static bool TryBuildCharacterView (
        UGridPartyInventoryComponent* PartyInventoryComponent,
        int32 CharacterIndex,
        const TArray<const URPGSkillAsset*>& SkillDefinitions,
        FGridSkillsPageView& OutView);

    static bool TryBuildSelectedCharacterView (
        UGridPartyInventoryComponent* PartyInventoryComponent,
        const TArray<const URPGSkillAsset*>& SkillDefinitions,
        FGridSkillsPageView& OutView);

    /** Loads valid canonical RPGSkill PrimaryDataAssets in stable SkillId order. */
    static void ResolveCanonicalSkillDefinitions (
        TArray<const URPGSkillAsset*>& OutDefinitions);
};
