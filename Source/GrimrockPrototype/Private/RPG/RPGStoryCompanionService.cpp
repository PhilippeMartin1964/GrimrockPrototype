#include "RPG/RPGStoryCompanionService.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGRaceAsset.h"
#include "RPG/RPGStoryCompanionAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace RPGStoryCompanionServicePrivate
{
	bool MatchesDefinition(const FGridCharacterInventoryState& Character, const URPGStoryCompanionAsset& Definition)
	{
		return Character.CharacterId == Definition.CharacterId && Definition.RaceDefinition && Character.RaceId == Definition.RaceDefinition->RaceId &&
			Definition.ClassDefinition && Character.ClassId == Definition.ClassDefinition->ClassId;
	}

	void InitializeEmptyHotbar(FGridCharacterInventoryState& Character)
	{
		Character.CombatHotbarSlots.SetNum(FGridCombatHotbarBinding::SlotCount);
		for (int32 SlotIndex = 0; SlotIndex < FGridCombatHotbarBinding::SlotCount; ++SlotIndex)
		{
			Character.CombatHotbarSlots[SlotIndex].Reset(SlotIndex);
		}
	}

	bool BuildCandidate(const URPGStoryCompanionAsset& Definition, int32 InventorySlotCount, FGridCharacterInventoryState& OutCandidate)
	{
		if (!Definition.IsValidDefinition() || !Definition.RaceDefinition || !Definition.ClassDefinition)
		{
			return false;
		}

		const FRPGAttributes FinalAttributes =
			URPGCharacterRulesLibrary::AddAttributes(Definition.ClassDefinition->BaseAttributes, Definition.RaceDefinition->AttributeBonuses);
		if (!URPGCharacterRulesLibrary::AreAttributesInRange(FinalAttributes))
		{
			return false;
		}

		FGridCharacterInventoryState Candidate;
		Candidate.CharacterId = Definition.CharacterId;
		Candidate.DisplayName = Definition.DisplayName;
		Candidate.RaceId = Definition.RaceDefinition->RaceId;
		Candidate.RaceDisplayName = Definition.RaceDefinition->DisplayName;
		Candidate.ClassId = Definition.ClassDefinition->ClassId;
		Candidate.ClassDisplayName = Definition.ClassDefinition->DisplayName;
		Candidate.ClassDefinition = Definition.ClassDefinition.Get();
		Candidate.Level = Definition.Level;
		Candidate.Experience = URPGCharacterRulesLibrary::GetCumulativeExperienceRequiredForLevel(Definition.Level);
		Candidate.LastAcknowledgedLevel = Candidate.Level;
		Candidate.Attributes = FinalAttributes;
		Candidate.DerivedStats = URPGCharacterRulesLibrary::CalculateDerivedStats(FinalAttributes, Definition.ClassDefinition, Definition.Level);
		Candidate.Resources = URPGCharacterRulesLibrary::InitializeCharacterResources(Candidate.DerivedStats, Definition.ClassDefinition);
		Candidate.PortraitGender = Definition.PortraitGender;
		Candidate.PortraitVariantId = Definition.PortraitVariantId;
		Candidate.Portrait = Definition.Portrait;
		Candidate.ClassIcon = Definition.ClassIcon;
		Candidate.InventorySlots.SetNum(FMath::Max(0, InventorySlotCount));
		InitializeEmptyHotbar(Candidate);

		OutCandidate = MoveTemp(Candidate);
		return true;
	}
}

bool FRPGStoryCompanionService::EnsureCandidateRegistered(
	UGridPartyInventoryComponent* PartyInventoryComponent, const URPGStoryCompanionAsset* CompanionDefinition, FRPGStoryCompanionRegistrationResult& OutResult)
{
	OutResult = FRPGStoryCompanionRegistrationResult();

	if (!PartyInventoryComponent)
	{
		OutResult.Status = ERPGStoryCompanionRegistrationStatus::InvalidInventory;
		OutResult.Error = TEXT("Party inventory component is missing.");
		return false;
	}

	FGridPartyInventoryState& State = PartyInventoryComponent->PartyInventoryState;
	if (!State.bInitialCharacterCreationCompleted || State.ActiveCharacters.IsEmpty() || State.ActiveEquipment.Num() != State.ActiveCharacters.Num() ||
		!State.ActiveCharacters.IsValidIndex(State.SelectedCharacterIndex) || State.MaxActiveCharacters < State.ActiveCharacters.Num())
	{
		OutResult.Status = ERPGStoryCompanionRegistrationStatus::InvalidInventory;
		OutResult.Error = TEXT("Active party state is not ready for companion registration.");
		return false;
	}

	if (!CompanionDefinition || !CompanionDefinition->IsValidDefinition())
	{
		OutResult.Status = ERPGStoryCompanionRegistrationStatus::InvalidDefinition;
		OutResult.Error = TEXT("Story companion definition is invalid.");
		return false;
	}

	OutResult.CharacterId = CompanionDefinition->CharacterId;

	int32 ActiveMatchCount = 0;
	int32 ActiveMatchIndex = INDEX_NONE;
	for (int32 CharacterIndex = 0; CharacterIndex < State.ActiveCharacters.Num(); ++CharacterIndex)
	{
		if (State.ActiveCharacters[CharacterIndex].CharacterId != CompanionDefinition->CharacterId)
		{
			continue;
		}
		++ActiveMatchCount;
		ActiveMatchIndex = CharacterIndex;
	}

	int32 PoolMatchCount = 0;
	int32 PoolMatchIndex = INDEX_NONE;
	for (int32 PoolIndex = 0; PoolIndex < State.CharacterPool.Num(); ++PoolIndex)
	{
		if (State.CharacterPool[PoolIndex].CharacterId != CompanionDefinition->CharacterId)
		{
			continue;
		}
		++PoolMatchCount;
		PoolMatchIndex = PoolIndex;
	}

	if (ActiveMatchCount + PoolMatchCount > 1)
	{
		OutResult.Status = ERPGStoryCompanionRegistrationStatus::IdentityCollision;
		OutResult.Error = TEXT("Story companion CharacterId is duplicated in the party state.");
		return false;
	}

	if (ActiveMatchCount == 1)
	{
		const FGridCharacterInventoryState& Existing = State.ActiveCharacters[ActiveMatchIndex];
		if (!RPGStoryCompanionServicePrivate::MatchesDefinition(Existing, *CompanionDefinition))
		{
			OutResult.Status = ERPGStoryCompanionRegistrationStatus::IdentityCollision;
			OutResult.Error = TEXT("Story companion CharacterId collides with a different active character.");
			return false;
		}

		OutResult.bSucceeded = true;
		OutResult.Status = ERPGStoryCompanionRegistrationStatus::AlreadyActive;
		OutResult.ActiveIndex = ActiveMatchIndex;
		return true;
	}

	if (PoolMatchCount == 1)
	{
		const FGridCharacterInventoryState& Existing = State.CharacterPool[PoolMatchIndex];
		if (!RPGStoryCompanionServicePrivate::MatchesDefinition(Existing, *CompanionDefinition))
		{
			OutResult.Status = ERPGStoryCompanionRegistrationStatus::IdentityCollision;
			OutResult.Error = TEXT("Story companion CharacterId collides with a different pool character.");
			return false;
		}

		OutResult.bSucceeded = true;
		OutResult.Status = ERPGStoryCompanionRegistrationStatus::AlreadyInPool;
		OutResult.PoolIndex = PoolMatchIndex;
		return true;
	}

	FGridCharacterInventoryState Candidate;
	if (!RPGStoryCompanionServicePrivate::BuildCandidate(*CompanionDefinition, PartyInventoryComponent->DefaultInventorySlotCountPerCharacter, Candidate))
	{
		OutResult.Status = ERPGStoryCompanionRegistrationStatus::InvalidDefinition;
		OutResult.Error = TEXT("Story companion candidate could not be built from the definition.");
		return false;
	}

	const int32 NewPoolIndex = State.CharacterPool.Add(MoveTemp(Candidate));

	OutResult.bSucceeded = true;
	OutResult.Status = ERPGStoryCompanionRegistrationStatus::AddedToPool;
	OutResult.PoolIndex = NewPoolIndex;

	PartyInventoryComponent->NotifyPartyInventoryChanged(INDEX_NONE);
	return true;
}
