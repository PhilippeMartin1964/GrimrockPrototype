#include "RPG/RPGSkillCheckService.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGPartyProgressionResolver.h"
#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillService.h"
#include "Runtime/GridInventoryTypes.h"

int32 FRPGSkillCheckService::GetGoverningAttributeValue(const FRPGAttributes& Attributes, ERPGSkillGoverningAttribute GoverningAttribute)
{
	switch (GoverningAttribute)
	{
		case ERPGSkillGoverningAttribute::Strength:
			return Attributes.Strength;
		case ERPGSkillGoverningAttribute::Dexterity:
			return Attributes.Dexterity;
		case ERPGSkillGoverningAttribute::Constitution:
			return Attributes.Constitution;
		case ERPGSkillGoverningAttribute::Intelligence:
			return Attributes.Intelligence;
		case ERPGSkillGoverningAttribute::Wisdom:
			return Attributes.Wisdom;
		case ERPGSkillGoverningAttribute::Charisma:
			return Attributes.Charisma;
		case ERPGSkillGoverningAttribute::None:
		default:
			return 0;
	}
}

namespace
{
    // One authority for eligibility and the immutable components of a skill
    // check. Both real rolls and zero-RNG previews pass through here.
    bool TryProjectSkillCheck(
        const FGridCharacterInventoryState& CharacterState,
        const URPGSkillAsset* SkillDefinition, int32 Difficulty,
        FRPGSkillCheckResult& OutResult,
        ERPGSkillGoverningAttribute OverrideAttribute,
        const FRPGSkillCheckContext* Context)
    {
	OutResult = FRPGSkillCheckResult();

	if (!IsValid(SkillDefinition) || !SkillDefinition->IsValidDefinition())
	{
		OutResult.RejectReason = ERPGSkillCheckRejectReason::InvalidDefinition;
		return false;
	}

	OutResult.SkillId = SkillDefinition->SkillId;
	OutResult.GoverningAttribute = OverrideAttribute == ERPGSkillGoverningAttribute::None
		? SkillDefinition->GoverningAttribute : OverrideAttribute;
	OutResult.Difficulty = Difficulty;

	if (Difficulty <= 0)
	{
		OutResult.RejectReason = ERPGSkillCheckRejectReason::InvalidDifficulty;
		return false;
	}

	if (!FRPGSkillService::ValidateSkillState(CharacterState))
	{
		OutResult.RejectReason = ERPGSkillCheckRejectReason::InvalidCharacterState;
		return false;
	}

	const int32 Rank = FRPGSkillService::GetSkillRank(CharacterState, SkillDefinition->SkillId);
	OutResult.Rank = Rank;

	if (Rank > SkillDefinition->MaxRank)
	{
		OutResult.RejectReason = ERPGSkillCheckRejectReason::InvalidCharacterState;
		return false;
	}

	if (Rank == 0 && !SkillDefinition->bAllowUntrainedChecks)
	{
		OutResult.RejectReason = ERPGSkillCheckRejectReason::UntrainedNotAllowed;
		return false;
	}

	OutResult.AttributeValue = FRPGSkillCheckService::GetGoverningAttributeValue(CharacterState.Attributes, OutResult.GoverningAttribute);
	OutResult.AttributeModifier = OutResult.GoverningAttribute == ERPGSkillGoverningAttribute::None
		? 0
		: URPGCharacterRulesLibrary::GetAttributeModifier(OutResult.AttributeValue);

	const URPGClassAsset* ClassDefinition = CharacterState.ClassDefinition.Get();
	if (IsValid(ClassDefinition))
	{
		for (const FName ChoiceId : CharacterState.SelectedClassProgressionChoiceIds)
		{
			const FRPGClassProgressionChoiceDefinition* Choice = ClassDefinition->FindProgressionChoice(ChoiceId);
			if (!Choice) continue;
			for (const FRPGSkillProgressionModifier& Modifier : Choice->SkillModifiers)
			{
				if (Modifier.SkillId != SkillDefinition->SkillId)
				{
					continue;
				}
				if (Modifier.bRequireRangedContext && (!Context || !Context->bRangedContext))
				{
					continue;
				}
				if (!Modifier.RelatedMonsterCategoryIds.IsEmpty() &&
					(!Context || !Modifier.RelatedMonsterCategoryIds.Contains(Context->RelatedMonsterCategoryId)))
				{
					continue;
				}
				OutResult.ClassModifier += Modifier.CheckModifier;
				OutResult.ProgressionModifier += Modifier.CheckModifier;
				OutResult.SafeFailureMargin = FMath::Max(OutResult.SafeFailureMargin, Modifier.SafeFailureMargin);
			}
		}
	}

        return true;
    }
}

bool FRPGSkillCheckService::TryResolveSkillCheck(const FGridCharacterInventoryState& CharacterState, const URPGSkillAsset* SkillDefinition, int32 Difficulty,
	FRandomStream& RandomStream, FRPGSkillCheckResult& OutResult, const FRPGSkillCheckContext* Context)
{
    if (!TryProjectSkillCheck(CharacterState, SkillDefinition, Difficulty,
            OutResult, ERPGSkillGoverningAttribute::None, Context))
    {
        return false; // no RNG on rejection
    }
	OutResult.Roll = RandomStream.RandRange(1, 20);
	OutResult.Total = OutResult.Roll + OutResult.Rank + OutResult.AttributeModifier + OutResult.ProgressionModifier;
	OutResult.bResolved = true;
	OutResult.bSuccess = OutResult.Total >= OutResult.Difficulty;
	OutResult.FailureMargin = OutResult.bSuccess ? 0 : OutResult.Difficulty - OutResult.Total;
	OutResult.bSafeFailure = !OutResult.bSuccess && OutResult.SafeFailureMargin > 0 &&
		OutResult.FailureMargin <= OutResult.SafeFailureMargin;
	return true;
}

bool FRPGSkillCheckService::TryPreviewBestPartySkillCheck(
    const FGridPartyInventoryState& PartyState, const URPGSkillAsset* SkillDefinition,
    int32 Difficulty, FRPGSkillCheckResult& OutStaticResult, int32& OutCharacterIndex,
    ERPGSkillGoverningAttribute OverrideAttribute, const FRPGSkillCheckContext* Context)
{
    OutStaticResult = FRPGSkillCheckResult();
    OutCharacterIndex = INDEX_NONE;
    if (!IsValid(SkillDefinition) || !SkillDefinition->IsValidDefinition() || Difficulty <= 0)
    {
        OutStaticResult.RejectReason = !IsValid(SkillDefinition) || !SkillDefinition->IsValidDefinition()
            ? ERPGSkillCheckRejectReason::InvalidDefinition
            : ERPGSkillCheckRejectReason::InvalidDifficulty;
        return false;
    }

    int32 BestBonus = MIN_int32;
    for (int32 Index = 0; Index < PartyState.ActiveCharacters.Num(); ++Index)
    {
        const FGridCharacterInventoryState& Character = PartyState.ActiveCharacters[Index];
        if (Character.Resources.CurrentHealth <= 0) continue;
        FRPGSkillCheckResult Candidate;
        if (!TryProjectSkillCheck(Character, SkillDefinition, Difficulty,
            Candidate, OverrideAttribute, Context))
        {
            continue;
        }
        const int32 StaticBonus = Candidate.Rank + Candidate.AttributeModifier +
            Candidate.ClassModifier;
        // Stable tie break: the first eligible character in party order wins.
        if (OutCharacterIndex == INDEX_NONE || StaticBonus > BestBonus)
        {
            BestBonus = StaticBonus;
            OutCharacterIndex = Index;
            OutStaticResult = Candidate;
        }
    }
    if (OutCharacterIndex == INDEX_NONE)
    {
        OutStaticResult.RejectReason = ERPGSkillCheckRejectReason::UntrainedNotAllowed;
        return false;
    }
    const int32 Group = FRPGPartyProgressionResolver::ResolveGroupSkillCheckModifier(
        PartyState, SkillDefinition->SkillId);
    OutStaticResult.GroupModifier = Group;
    OutStaticResult.ProgressionModifier += Group;
    OutStaticResult.Total = OutStaticResult.Rank +
        OutStaticResult.AttributeModifier + OutStaticResult.ProgressionModifier;
    // bResolved stays false: no d20 was rolled; this is not a skill check.
    return true;
}

bool FRPGSkillCheckService::TryResolveBestPartySkillCheck(const FGridPartyInventoryState& PartyState, const URPGSkillAsset* SkillDefinition,
	int32 Difficulty, FRandomStream& RandomStream, FRPGSkillCheckResult& OutResult, int32& OutCharacterIndex,
	const FRPGSkillCheckContext* Context)
{
    FRPGSkillCheckResult Preview;
    if (!TryPreviewBestPartySkillCheck(PartyState, SkillDefinition, Difficulty,
        Preview, OutCharacterIndex, ERPGSkillGoverningAttribute::None, Context))
    {
        OutResult = Preview;
        return false; // no RNG consumed
    }
    if (!TryResolveSkillCheck(PartyState.ActiveCharacters[OutCharacterIndex],
        SkillDefinition, Difficulty, RandomStream, OutResult, Context))
    {
        return false;
    }
    OutResult.GroupModifier = Preview.GroupModifier;
    OutResult.ProgressionModifier += Preview.GroupModifier;
    OutResult.Total += Preview.GroupModifier;
    OutResult.bSuccess = OutResult.Total >= OutResult.Difficulty;
    OutResult.FailureMargin = OutResult.bSuccess ? 0 : OutResult.Difficulty - OutResult.Total;
    OutResult.bSafeFailure = !OutResult.bSuccess && OutResult.SafeFailureMargin > 0 &&
        OutResult.FailureMargin <= OutResult.SafeFailureMargin;
    return true;
}
