#include "Magic/GridPartySpellbookComponent.h"

UGridPartySpellbookComponent::UGridPartySpellbookComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UGridPartySpellbookComponent::EnsureCharacterSpellbook(FGuid CharacterId)
{
	const bool bAlreadyRegistered = SpellbookState.FindSpellbook(CharacterId) != nullptr;
	const bool bResult = SpellbookState.EnsureCharacter(CharacterId);
	if (bResult && !bAlreadyRegistered)
	{
		OnSpellbookChanged.Broadcast();
	}
	return bResult;
}

bool UGridPartySpellbookComponent::RemoveCharacterSpellbook(FGuid CharacterId)
{
	const bool bResult = SpellbookState.RemoveCharacter(CharacterId);
	if (bResult)
	{
		OnSpellbookChanged.Broadcast();
	}
	return bResult;
}

EGridSpellbookMutationResult UGridPartySpellbookComponent::LearnSpell(FGuid CharacterId, FName SpellId)
{
	const EGridSpellbookMutationResult Result = SpellbookState.LearnSpell(CharacterId, SpellId);
	if (Result == EGridSpellbookMutationResult::Success)
	{
		OnSpellbookChanged.Broadcast();
	}
	return Result;
}

EGridSpellbookMutationResult UGridPartySpellbookComponent::ForgetSpell(FGuid CharacterId, FName SpellId)
{
	const EGridSpellbookMutationResult Result = SpellbookState.ForgetSpell(CharacterId, SpellId);
	if (Result == EGridSpellbookMutationResult::Success)
	{
		OnSpellbookChanged.Broadcast();
	}
	return Result;
}

bool UGridPartySpellbookComponent::KnowsSpell(FGuid CharacterId, FName SpellId) const
{
	return SpellbookState.KnowsSpell(CharacterId, SpellId);
}

TArray<FName> UGridPartySpellbookComponent::GetKnownSpellIds(FGuid CharacterId) const
{
	const FGridCharacterSpellbookState* Spellbook = SpellbookState.FindSpellbook(CharacterId);
	return Spellbook ? Spellbook->KnownSpellIds : TArray<FName>();
}

void UGridPartySpellbookComponent::ResetAllSpellbooks()
{
	const bool bHadSpellbooks = SpellbookState.CharacterSpellbooks.Num() > 0;
	SpellbookState.Reset();
	if (bHadSpellbooks)
	{
		OnSpellbookChanged.Broadcast();
	}
}

bool UGridPartySpellbookComponent::ValidateSpellbookState(FString& OutError) const
{
	OutError.Reset();

	TSet<FGuid> SeenCharacterIds;
	for (const FGridCharacterSpellbookState& Spellbook : SpellbookState.CharacterSpellbooks)
	{
		if (!Spellbook.CharacterId.IsValid())
		{
			OutError = TEXT("Spellbook contains an invalid CharacterId.");
			return false;
		}
		if (SeenCharacterIds.Contains(Spellbook.CharacterId))
		{
			OutError = TEXT("Spellbook contains duplicate CharacterIds.");
			return false;
		}
		SeenCharacterIds.Add(Spellbook.CharacterId);

		TSet<FName> SeenSpellIds;
		for (const FName SpellId : Spellbook.KnownSpellIds)
		{
			if (SpellId.IsNone())
			{
				OutError = TEXT("Spellbook contains NAME_None.");
				return false;
			}
			if (SeenSpellIds.Contains(SpellId))
			{
				OutError = TEXT("Spellbook contains duplicate SpellIds.");
				return false;
			}
			SeenSpellIds.Add(SpellId);
		}
	}

	return true;
}
