#include "Magic/GridPartySpellbookComponent.h"

#include "GameFramework/Actor.h"
#include "Magic/GridProductionSpellLibrary.h"
#include "Magic/GridSpellbookPersistence.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace GridPartySpellbookPrivate
{
	bool IsCanonicalSpellId(FName SpellId)
	{
		if (SpellId.IsNone())
		{
			return false;
		}

		TArray<FGridSpellDefinition> Definitions;
		FGridProductionSpellLibrary::BuildAll(Definitions);
		const FGridSpellDefinition* Definition = Definitions.FindByPredicate(
			[SpellId](const FGridSpellDefinition& Candidate)
			{
				return Candidate.SpellId == SpellId;
			});
		return Definition && FGridSpellContract::ValidateDefinition(*Definition) == EGridSpellValidationError::None;
	}

	void SortSpellIds(TArray<FName>& SpellIds)
	{
		SpellIds.Sort([](const FName Left, const FName Right) { return Left.ToString() < Right.ToString(); });
	}
}

using namespace GridPartySpellbookPrivate;

UGridPartySpellbookComponent::UGridPartySpellbookComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGridPartySpellbookComponent::InitializeSpellbookComponent(UGridPartyInventoryComponent* InInventoryComponent)
{
	InventoryComponentOverride = InInventoryComponent;
}

UGridPartyInventoryComponent* UGridPartySpellbookComponent::ResolveInventoryComponent() const
{
	if (InventoryComponentOverride.IsValid())
	{
		return InventoryComponentOverride.Get();
	}
	if (AActor* Owner = GetOwner())
	{
		return Owner->FindComponentByClass<UGridPartyInventoryComponent>();
	}
	return nullptr;
}

FGridCharacterInventoryState* UGridPartySpellbookComponent::FindMutableCharacter(FGuid CharacterId) const
{
	if (!CharacterId.IsValid())
	{
		return nullptr;
	}
	UGridPartyInventoryComponent* Inventory = ResolveInventoryComponent();
	if (!Inventory)
	{
		return nullptr;
	}
	if (FGridCharacterInventoryState* Active = Inventory->PartyInventoryState.ActiveCharacters.FindByPredicate(
			[&CharacterId](const FGridCharacterInventoryState& Character) { return Character.CharacterId == CharacterId; }))
	{
		return Active;
	}
	return Inventory->PartyInventoryState.CharacterPool.FindByPredicate(
		[&CharacterId](const FGridCharacterInventoryState& Character) { return Character.CharacterId == CharacterId; });
}

const FGridCharacterInventoryState* UGridPartySpellbookComponent::FindCharacter(FGuid CharacterId) const
{
	return FindMutableCharacter(CharacterId);
}

bool UGridPartySpellbookComponent::EnsureCharacterSpellbook(FGuid CharacterId)
{
	return FindCharacter(CharacterId) != nullptr;
}

bool UGridPartySpellbookComponent::RemoveCharacterSpellbook(FGuid CharacterId)
{
	FGridCharacterInventoryState* Character = FindMutableCharacter(CharacterId);
	if (!Character)
	{
		return false;
	}

	const bool bChanged = !Character->KnownSpellIds.IsEmpty();
	Character->KnownSpellIds.Reset();
	if (bChanged)
	{
		OnSpellbookChanged.Broadcast();
	}
	return true;
}

EGridSpellbookMutationResult UGridPartySpellbookComponent::LearnSpell(FGuid CharacterId, FName SpellId)
{
	FGridCharacterInventoryState* Character = FindMutableCharacter(CharacterId);
	if (!Character)
	{
		return EGridSpellbookMutationResult::InvalidCharacter;
	}
	if (!IsCanonicalSpellId(SpellId))
	{
		return EGridSpellbookMutationResult::InvalidSpell;
	}
	if (Character->KnownSpellIds.Contains(SpellId))
	{
		return EGridSpellbookMutationResult::AlreadyKnown;
	}

	Character->KnownSpellIds.Add(SpellId);
	SortSpellIds(Character->KnownSpellIds);
	OnSpellbookChanged.Broadcast();
	return EGridSpellbookMutationResult::Success;
}

EGridSpellbookMutationResult UGridPartySpellbookComponent::ForgetSpell(FGuid CharacterId, FName SpellId)
{
	FGridCharacterInventoryState* Character = FindMutableCharacter(CharacterId);
	if (!Character)
	{
		return EGridSpellbookMutationResult::InvalidCharacter;
	}
	if (SpellId.IsNone())
	{
		return EGridSpellbookMutationResult::InvalidSpell;
	}
	if (Character->KnownSpellIds.RemoveSingle(SpellId) == 0)
	{
		return EGridSpellbookMutationResult::NotKnown;
	}

	OnSpellbookChanged.Broadcast();
	return EGridSpellbookMutationResult::Success;
}

bool UGridPartySpellbookComponent::KnowsSpell(FGuid CharacterId, FName SpellId) const
{
	const FGridCharacterInventoryState* Character = FindCharacter(CharacterId);
	return Character && !SpellId.IsNone() && Character->KnownSpellIds.Contains(SpellId);
}

TArray<FName> UGridPartySpellbookComponent::GetKnownSpellIds(FGuid CharacterId) const
{
	const FGridCharacterInventoryState* Character = FindCharacter(CharacterId);
	return Character ? Character->KnownSpellIds : TArray<FName>();
}

bool UGridPartySpellbookComponent::GetCharacterSpellbookState(FGuid CharacterId, FGridCharacterSpellbookState& OutState) const
{
	OutState = FGridCharacterSpellbookState();
	const FGridCharacterInventoryState* Character = FindCharacter(CharacterId);
	if (!Character)
	{
		return false;
	}

	OutState.CharacterId = Character->CharacterId;
	OutState.KnownSpellIds = Character->KnownSpellIds;
	return true;
}

void UGridPartySpellbookComponent::ResetAllSpellbooks()
{
	UGridPartyInventoryComponent* Inventory = ResolveInventoryComponent();
	if (!Inventory)
	{
		return;
	}

	bool bChanged = false;
	const auto ResetCharacters = [&bChanged](TArray<FGridCharacterInventoryState>& Characters)
	{
		for (FGridCharacterInventoryState& Character : Characters)
		{
			bChanged |= !Character.KnownSpellIds.IsEmpty();
			Character.KnownSpellIds.Reset();
		}
	};
	ResetCharacters(Inventory->PartyInventoryState.ActiveCharacters);
	ResetCharacters(Inventory->PartyInventoryState.CharacterPool);

	if (bChanged)
	{
		OnSpellbookChanged.Broadcast();
	}
}

bool UGridPartySpellbookComponent::ValidateSpellbookState(FString& OutError) const
{
	const UGridPartyInventoryComponent* Inventory = ResolveInventoryComponent();
	if (!Inventory)
	{
		OutError = TEXT("Party inventory component is unavailable.");
		return false;
	}
	return FGridSpellbookPersistence::ValidatePartySpellbooks(Inventory->PartyInventoryState, OutError);
}
