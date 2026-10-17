#pragma once

#include "CoreMinimal.h"

#include "RPG/RPGSkillAsset.h"
#include "UI/GridSkillsUiTypes.h"
#include "UI/UILocalizationText.h"

/**
 * Player-facing Skill-name resolution used at progression feedback boundaries.
 *
 * Technical SkillIds are identities/logging data, never UI fallback text.
 */
namespace GridSkillsWidgetFeedback
{
	inline FText ResolvePlayerFacingSkillName(
		const FGridSkillsPageView& View,
		FName SkillId,
		const URPGSkillAsset* CanonicalDefinition)
	{
		if (!SkillId.IsNone())
		{
			if (const FGridSkillEntryView* Entry = View.Skills.FindByPredicate(
				[SkillId](const FGridSkillEntryView& Candidate)
				{
					return Candidate.SkillId == SkillId;
				}))
			{
				if (!Entry->DisplayName.IsEmpty())
				{
					return Entry->DisplayName;
				}
			}

			if (IsValid(CanonicalDefinition) &&
				CanonicalDefinition->SkillId == SkillId &&
				!CanonicalDefinition->DisplayName.IsEmpty())
			{
				return CanonicalDefinition->DisplayName;
			}
		}

		return FUILocalizationText::Get(TEXT("Skill.Fallback.Unknown"));
	}
}
