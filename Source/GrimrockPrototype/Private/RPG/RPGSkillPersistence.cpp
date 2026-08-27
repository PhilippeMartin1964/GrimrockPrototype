#include "RPG/RPGSkillPersistence.h"

#include "RPG/RPGSkillAsset.h"
#include "RPG/RPGSkillRequirementProjectionService.h"
#include "RPG/RPGSkillService.h"
#include "Runtime/GridInventoryTypes.h"

namespace RPGSkillPersistencePrivate
{
	bool ValidateDefinitionAndRank(
		FName SkillId, int32 Rank, TFunctionRef<const URPGSkillAsset*(FName)> DefinitionResolver, FString& OutError)
	{
		if (SkillId.IsNone() || Rank <= 0)
		{
			OutError = TEXT("A durable Skill rank contains an invalid SkillId or non-positive Rank.");
			return false;
		}

		const URPGSkillAsset* Definition = DefinitionResolver(SkillId);
		if (!IsValid(Definition) || !Definition->IsValidDefinition() || Definition->SkillId != SkillId)
		{
			OutError = FString::Printf(TEXT("Skill '%s' cannot resolve a matching valid canonical definition."), *SkillId.ToString());
			return false;
		}
		if (Rank > Definition->MaxRank)
		{
			OutError = FString::Printf(TEXT("Skill '%s' durable rank %d exceeds definition MaxRank %d."), *SkillId.ToString(), Rank, Definition->MaxRank);
			return false;
		}
		return true;
	}

	bool ValidateCharacters(const TArray<FGridCharacterInventoryState>& Characters, const TCHAR* Location,
		TFunctionRef<const URPGSkillAsset*(FName)> DefinitionResolver, TSet<FGuid>& InOutCharacterIds, FString& OutError)
	{
		for (int32 Index = 0; Index < Characters.Num(); ++Index)
		{
			const FGridCharacterInventoryState& Character = Characters[Index];
			if (!Character.CharacterId.IsValid())
			{
				OutError = FString::Printf(TEXT("%s[%d] has an invalid CharacterId."), Location, Index);
				return false;
			}
			if (InOutCharacterIds.Contains(Character.CharacterId))
			{
				OutError = FString::Printf(TEXT("CharacterId %s is duplicated or ambiguous in the party state."),
					*Character.CharacterId.ToString(EGuidFormats::Digits));
				return false;
			}
			InOutCharacterIds.Add(Character.CharacterId);

			if (!FRPGSkillService::ValidateSkillState(Character))
			{
				OutError = FString::Printf(TEXT("%s[%d] has a structurally invalid Skill state."), Location, Index);
				return false;
			}

			for (const FRPGSkillRank& Rank : Character.SkillRanks)
			{
				if (!ValidateDefinitionAndRank(Rank.SkillId, Rank.Rank, DefinitionResolver, OutError))
				{
					return false;
				}
			}
		}
		return true;
	}
}

bool FRPGSkillPersistence::ValidatePartySkills(const FGridPartyInventoryState& PartyState, FString& OutError)
{
	return ValidatePartySkills(
		PartyState,
		[](FName SkillId)
		{
			return FRPGSkillPersistence::ResolveDefinitionBySkillId(SkillId);
		},
		OutError);
}

bool FRPGSkillPersistence::ValidatePartySkills(const FGridPartyInventoryState& PartyState,
	TFunctionRef<const URPGSkillAsset*(FName)> DefinitionResolver, FString& OutError)
{
	TSet<FGuid> CharacterIds;
	if (!RPGSkillPersistencePrivate::ValidateCharacters(
			PartyState.ActiveCharacters, TEXT("ActiveCharacter"), DefinitionResolver, CharacterIds, OutError) ||
		!RPGSkillPersistencePrivate::ValidateCharacters(
			PartyState.CharacterPool, TEXT("CharacterPool"), DefinitionResolver, CharacterIds, OutError))
	{
		return false;
	}

	OutError.Reset();
	return true;
}

const URPGSkillAsset* FRPGSkillPersistence::ResolveDefinitionBySkillId(FName SkillId)
{
	return FRPGSkillRequirementProjectionService::ResolveDefinitionBySkillId(SkillId);
}
