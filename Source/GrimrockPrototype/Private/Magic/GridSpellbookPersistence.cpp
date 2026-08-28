#include "Magic/GridSpellbookPersistence.h"

#include "Magic/GridProductionSpellLibrary.h"
#include "Runtime/GridInventoryTypes.h"

namespace GridSpellbookPersistencePrivate
{
	bool IsCanonicalSpellId(FName SpellId)
	{
		TArray<FGridSpellDefinition> Definitions;
		FGridProductionSpellLibrary::BuildAll(Definitions);
		const FGridSpellDefinition* Definition = Definitions.FindByPredicate(
			[SpellId](const FGridSpellDefinition& Candidate)
			{
				return Candidate.SpellId == SpellId;
			});
		return Definition && FGridSpellContract::ValidateDefinition(*Definition) == EGridSpellValidationError::None;
	}

	bool ValidateCharacters(const TArray<FGridCharacterInventoryState>& Characters, const TCHAR* Location, TSet<FGuid>& InOutCharacterIds, FString& OutError)
	{
		for (int32 CharacterIndex = 0; CharacterIndex < Characters.Num(); ++CharacterIndex)
		{
			const FGridCharacterInventoryState& Character = Characters[CharacterIndex];
			if (!Character.CharacterId.IsValid())
			{
				OutError = FString::Printf(TEXT("%s[%d] has an invalid CharacterId."), Location, CharacterIndex);
				return false;
			}
			if (InOutCharacterIds.Contains(Character.CharacterId))
			{
				OutError = FString::Printf(
					TEXT("CharacterId %s is duplicated or ambiguous in durable Spellbook state."), *Character.CharacterId.ToString(EGuidFormats::Digits));
				return false;
			}
			InOutCharacterIds.Add(Character.CharacterId);

			TSet<FName> SeenSpellIds;
			FString PreviousSpellId;
			for (int32 SpellIndex = 0; SpellIndex < Character.KnownSpellIds.Num(); ++SpellIndex)
			{
				const FName SpellId = Character.KnownSpellIds[SpellIndex];
				if (SpellId.IsNone())
				{
					OutError = FString::Printf(TEXT("%s[%d] Spellbook contains NAME_None."), Location, CharacterIndex);
					return false;
				}
				if (SeenSpellIds.Contains(SpellId))
				{
					OutError = FString::Printf(TEXT("%s[%d] Spellbook contains duplicate SpellId '%s'."), Location, CharacterIndex, *SpellId.ToString());
					return false;
				}
				if (!IsCanonicalSpellId(SpellId))
				{
					OutError = FString::Printf(TEXT("%s[%d] Spellbook references non-canonical SpellId '%s'."), Location, CharacterIndex, *SpellId.ToString());
					return false;
				}

				const FString CurrentSpellId = SpellId.ToString();
				if (SpellIndex > 0 && !(PreviousSpellId < CurrentSpellId))
				{
					OutError = FString::Printf(TEXT("%s[%d] Spellbook is not deterministically sorted by SpellId."), Location, CharacterIndex);
					return false;
				}
				PreviousSpellId = CurrentSpellId;
				SeenSpellIds.Add(SpellId);
			}
		}
		return true;
	}
}

bool FGridSpellbookPersistence::ValidatePartySpellbooks(const FGridPartyInventoryState& PartyState, FString& OutError)
{
	TSet<FGuid> CharacterIds;
	if (!GridSpellbookPersistencePrivate::ValidateCharacters(PartyState.ActiveCharacters, TEXT("ActiveCharacter"), CharacterIds, OutError) ||
		!GridSpellbookPersistencePrivate::ValidateCharacters(PartyState.CharacterPool, TEXT("CharacterPool"), CharacterIds, OutError))
	{
		return false;
	}

	OutError.Reset();
	return true;
}
