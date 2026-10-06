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

bool FRPGSkillCheckService::TryResolveSkillCheck(const FGridCharacterInventoryState& CharacterState, const URPGSkillAsset* SkillDefinition, int32 Difficulty,
	FRandomStream& RandomStream, FRPGSkillCheckResult& OutResult, const FRPGSkillCheckContext* Context)
{
	OutResult = FRPGSkillCheckResult();

	if (!IsValid(SkillDefinition) || !SkillDefinition->IsValidDefinition())
	{
		OutResult.RejectReason = ERPGSkillCheckRejectReason::InvalidDefinition;
		return false;
	}

	OutResult.SkillId = SkillDefinition->SkillId;
	OutResult.GoverningAttribute = SkillDefinition->GoverningAttribute;
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

	OutResult.AttributeValue = GetGoverningAttributeValue(CharacterState.Attributes, SkillDefinition->GoverningAttribute);
	OutResult.AttributeModifier = SkillDefinition->GoverningAttribute == ERPGSkillGoverningAttribute::None
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
				OutResult.ProgressionModifier += Modifier.CheckModifier;
				OutResult.SafeFailureMargin = FMath::Max(OutResult.SafeFailureMargin, Modifier.SafeFailureMargin);
			}
		}
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


bool FRPGSkillCheckService::TryResolveBestPartySkillCheck(const FGridPartyInventoryState& PartyState, const URPGSkillAsset* SkillDefinition,
	int32 Difficulty, FRandomStream& RandomStream, FRPGSkillCheckResult& OutResult, int32& OutCharacterIndex,
	const FRPGSkillCheckContext* Context)
{
	OutResult = FRPGSkillCheckResult();
	OutCharacterIndex = INDEX_NONE;
	if (!IsValid(SkillDefinition) || !SkillDefinition->IsValidDefinition() || Difficulty <= 0)
	{
		OutResult.RejectReason = !IsValid(SkillDefinition) || !SkillDefinition->IsValidDefinition()
			? ERPGSkillCheckRejectReason::InvalidDefinition
			: ERPGSkillCheckRejectReason::InvalidDifficulty;
		return false;
	}

	int32 BestStaticBonus = MIN_int32;
	for (int32 CharacterIndex = 0; CharacterIndex < PartyState.ActiveCharacters.Num(); ++CharacterIndex)
	{
		const FGridCharacterInventoryState& Character = PartyState.ActiveCharacters[CharacterIndex];
		if (Character.Resources.CurrentHealth <= 0 || !FRPGSkillService::ValidateSkillState(Character))
		{
			continue;
		}
		const int32 Rank = FRPGSkillService::GetSkillRank(Character, SkillDefinition->SkillId);
		if (Rank > SkillDefinition->MaxRank || (Rank == 0 && !SkillDefinition->bAllowUntrainedChecks))
		{
			continue;
		}

		const int32 AttributeValue = GetGoverningAttributeValue(Character.Attributes, SkillDefinition->GoverningAttribute);
		const int32 AttributeModifier = SkillDefinition->GoverningAttribute == ERPGSkillGoverningAttribute::None
			? 0
			: URPGCharacterRulesLibrary::GetAttributeModifier(AttributeValue);
		int32 PersonalProgressionModifier = 0;
		if (const URPGClassAsset* ClassDefinition = Character.ClassDefinition.Get())
		{
			for (const FName ChoiceId : Character.SelectedClassProgressionChoiceIds)
			{
				const FRPGClassProgressionChoiceDefinition* Choice = ClassDefinition->FindProgressionChoice(ChoiceId);
				if (!Choice) continue;
				for (const FRPGSkillProgressionModifier& Modifier : Choice->SkillModifiers)
				{
					if (Modifier.SkillId != SkillDefinition->SkillId ||
						(Modifier.bRequireRangedContext && (!Context || !Context->bRangedContext)) ||
						(!Modifier.RelatedMonsterCategoryIds.IsEmpty() &&
							(!Context || !Modifier.RelatedMonsterCategoryIds.Contains(Context->RelatedMonsterCategoryId))))
					{
						continue;
					}
					PersonalProgressionModifier += Modifier.CheckModifier;
				}
			}
		}
		const int32 StaticBonus = Rank + AttributeModifier + PersonalProgressionModifier;
		if (OutCharacterIndex == INDEX_NONE || StaticBonus > BestStaticBonus)
		{
			BestStaticBonus = StaticBonus;
			OutCharacterIndex = CharacterIndex;
		}
	}

	if (!PartyState.ActiveCharacters.IsValidIndex(OutCharacterIndex))
	{
		OutResult.RejectReason = ERPGSkillCheckRejectReason::UntrainedNotAllowed;
		return false;
	}
	if (!TryResolveSkillCheck(
			PartyState.ActiveCharacters[OutCharacterIndex], SkillDefinition, Difficulty, RandomStream, OutResult, Context))
	{
		return false;
	}

	const int32 PartyModifier = FRPGPartyProgressionResolver::ResolveGroupSkillCheckModifier(PartyState, SkillDefinition->SkillId);
	OutResult.ProgressionModifier += PartyModifier;
	OutResult.Total += PartyModifier;
	OutResult.bSuccess = OutResult.Total >= OutResult.Difficulty;
	OutResult.FailureMargin = OutResult.bSuccess ? 0 : OutResult.Difficulty - OutResult.Total;
	OutResult.bSafeFailure = !OutResult.bSuccess && OutResult.SafeFailureMargin > 0 &&
		OutResult.FailureMargin <= OutResult.SafeFailureMargin;
	return true;
}
