#include "RPG/RPGTalentRuntimeService.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "RPG/RPGClassProgressionTransactionService.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace RPGTalentRuntimeServicePrivate
{
	bool ResolveTalentContext(UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex, const FGridCharacterInventoryState*& OutCharacter,
		URPGClassAsset*& OutClassDefinition)
	{
		OutCharacter = nullptr;
		OutClassDefinition = nullptr;

		if (!IsValid(PartyInventoryComponent) || !PartyInventoryComponent->IsValidCharacterIndex(CharacterIndex))
		{
			return false;
		}

		const FGridCharacterInventoryState& Character = PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
		if (!Character.CharacterId.IsValid())
		{
			return false;
		}

		URPGClassAsset* ClassDefinition = Character.ClassDefinition.Get();
		if (!FRPGAuthoringIdentityResolver::IsMatchingClassDefinition(Character.ClassId, ClassDefinition))
		{
			ClassDefinition = FRPGAuthoringIdentityResolver::ResolveClassById(Character.ClassId);
		}
		if (!ClassDefinition)
		{
			return false;
		}

		OutCharacter = &Character;
		OutClassDefinition = ClassDefinition;
		return true;
	}

	void BuildTalentView(const FRPGClassProgressionChoiceDefinition& Choice, bool bSelected, ERPGClassProgressionChoiceAvailabilityReason AvailabilityReason,
		FRPGTalentRuntimeView& OutView)
	{
		OutView = FRPGTalentRuntimeView();
		OutView.ChoiceId = Choice.ChoiceId;
		OutView.DisplayName = Choice.DisplayName;
		OutView.Description = Choice.Description;
		OutView.MinimumLevel = Choice.MinimumLevel;
		OutView.PointCost = Choice.PointCost;
		OutView.bSelected = bSelected;
		OutView.AvailabilityReason = AvailabilityReason;
	}

	bool BuildSelectionSet(const TArray<FName>& SelectedChoiceIds, TSet<FName>& OutSelection)
	{
		OutSelection.Reset();
		for (const FName ChoiceId : SelectedChoiceIds)
		{
			if (ChoiceId.IsNone() || OutSelection.Contains(ChoiceId))
			{
				OutSelection.Reset();
				return false;
			}
			OutSelection.Add(ChoiceId);
		}
		return true;
	}
}

bool FRPGTalentRuntimeService::TryGetSelectedTalents(
	UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex, TArray<FRPGTalentRuntimeView>& OutTalents)
{
	OutTalents.Reset();

	const FGridCharacterInventoryState* Character = nullptr;
	URPGClassAsset* ClassDefinition = nullptr;
	if (!RPGTalentRuntimeServicePrivate::ResolveTalentContext(PartyInventoryComponent, CharacterIndex, Character, ClassDefinition))
	{
		return false;
	}

	TArray<FName> SelectedChoiceIds;
	if (!FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds(PartyInventoryComponent, CharacterIndex, SelectedChoiceIds))
	{
		return false;
	}

	TSet<FName> SelectedChoiceSet;
	if (!RPGTalentRuntimeServicePrivate::BuildSelectionSet(SelectedChoiceIds, SelectedChoiceSet))
	{
		return false;
	}

	OutTalents.Reserve(SelectedChoiceIds.Num());
	for (const FRPGClassProgressionChoiceDefinition& Choice : ClassDefinition->ProgressionChoices)
	{
		if (!SelectedChoiceSet.Contains(Choice.ChoiceId))
		{
			continue;
		}

		FRPGTalentRuntimeView View;
		RPGTalentRuntimeServicePrivate::BuildTalentView(
			Choice, true, FRPGClassProgressionService::GetChoiceAvailability(ClassDefinition, Character->Level, SelectedChoiceSet, Choice.ChoiceId), View);
		OutTalents.Add(MoveTemp(View));
	}

	return OutTalents.Num() == SelectedChoiceIds.Num();
}

bool FRPGTalentRuntimeService::TryGetSelectedCharacterTalents(UGridPartyInventoryComponent* PartyInventoryComponent, TArray<FRPGTalentRuntimeView>& OutTalents)
{
	OutTalents.Reset();
	if (!IsValid(PartyInventoryComponent))
	{
		return false;
	}

	return TryGetSelectedTalents(PartyInventoryComponent, PartyInventoryComponent->GetSelectedCharacterIndex(), OutTalents);
}

bool FRPGTalentRuntimeService::HasTalent(UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex, FName ChoiceId, bool& OutHasTalent)
{
	OutHasTalent = false;
	if (ChoiceId.IsNone())
	{
		return false;
	}

	const FGridCharacterInventoryState* Character = nullptr;
	URPGClassAsset* ClassDefinition = nullptr;
	if (!RPGTalentRuntimeServicePrivate::ResolveTalentContext(PartyInventoryComponent, CharacterIndex, Character, ClassDefinition) ||
		!ClassDefinition->FindProgressionChoice(ChoiceId))
	{
		return false;
	}

	TArray<FName> SelectedChoiceIds;
	if (!FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds(PartyInventoryComponent, CharacterIndex, SelectedChoiceIds))
	{
		return false;
	}

	OutHasTalent = SelectedChoiceIds.Contains(ChoiceId);
	return true;
}

bool FRPGTalentRuntimeService::HasSelectedCharacterTalent(UGridPartyInventoryComponent* PartyInventoryComponent, FName ChoiceId, bool& OutHasTalent)
{
	OutHasTalent = false;
	if (!IsValid(PartyInventoryComponent))
	{
		return false;
	}

	return HasTalent(PartyInventoryComponent, PartyInventoryComponent->GetSelectedCharacterIndex(), ChoiceId, OutHasTalent);
}

bool FRPGTalentRuntimeService::TryGetTalentPointBalance(
	UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex, FRPGTalentPointBalance& OutBalance)
{
	OutBalance = FRPGTalentPointBalance();

	return FRPGClassProgressionTransactionService::TryGetChoicePointBalance(
		PartyInventoryComponent, CharacterIndex, OutBalance.GrantedPoints, OutBalance.SpentPoints, OutBalance.RemainingPoints);
}

bool FRPGTalentRuntimeService::TryGetSelectedCharacterTalentPointBalance(
	UGridPartyInventoryComponent* PartyInventoryComponent, FRPGTalentPointBalance& OutBalance)
{
	OutBalance = FRPGTalentPointBalance();
	if (!IsValid(PartyInventoryComponent))
	{
		return false;
	}

	return TryGetTalentPointBalance(PartyInventoryComponent, PartyInventoryComponent->GetSelectedCharacterIndex(), OutBalance);
}

bool FRPGTalentRuntimeService::TryGetAvailableTalents(
	UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex, TArray<FRPGTalentRuntimeView>& OutTalents)
{
	OutTalents.Reset();

	const FGridCharacterInventoryState* Character = nullptr;
	URPGClassAsset* ClassDefinition = nullptr;
	if (!RPGTalentRuntimeServicePrivate::ResolveTalentContext(PartyInventoryComponent, CharacterIndex, Character, ClassDefinition))
	{
		return false;
	}

	TArray<FName> SelectedChoiceIds;
	if (!FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds(PartyInventoryComponent, CharacterIndex, SelectedChoiceIds))
	{
		return false;
	}

	TSet<FName> SelectedChoiceSet;
	if (!RPGTalentRuntimeServicePrivate::BuildSelectionSet(SelectedChoiceIds, SelectedChoiceSet))
	{
		return false;
	}

	for (const FRPGClassProgressionChoiceDefinition& Choice : ClassDefinition->ProgressionChoices)
	{
		const ERPGClassProgressionChoiceAvailabilityReason Availability =
			FRPGClassProgressionService::GetChoiceAvailability(ClassDefinition, Character->Level, SelectedChoiceSet, Choice.ChoiceId);
		if (Availability != ERPGClassProgressionChoiceAvailabilityReason::None)
		{
			continue;
		}

		FRPGTalentRuntimeView View;
		RPGTalentRuntimeServicePrivate::BuildTalentView(Choice, false, Availability, View);
		OutTalents.Add(MoveTemp(View));
	}

	return true;
}

bool FRPGTalentRuntimeService::TryGetSelectedCharacterAvailableTalents(
	UGridPartyInventoryComponent* PartyInventoryComponent, TArray<FRPGTalentRuntimeView>& OutTalents)
{
	OutTalents.Reset();
	if (!IsValid(PartyInventoryComponent))
	{
		return false;
	}

	return TryGetAvailableTalents(PartyInventoryComponent, PartyInventoryComponent->GetSelectedCharacterIndex(), OutTalents);
}
