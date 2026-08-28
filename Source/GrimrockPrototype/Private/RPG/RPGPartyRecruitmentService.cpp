#include "RPG/RPGPartyRecruitmentService.h"

#include "Runtime/GridPartyInventoryComponent.h"

namespace GridPartyRecruitmentPrivate
{
	bool IsCandidateIdentityValid(const FGridCharacterInventoryState& Candidate, FString& OutError)
	{
		OutError.Empty();

		if (!Candidate.CharacterId.IsValid())
		{
			OutError = TEXT("Candidate CharacterId is invalid.");
			return false;
		}

		FString NormalizedName = Candidate.DisplayName.ToString();
		NormalizedName.TrimStartAndEndInline();
		if (NormalizedName.Len() < 1 || NormalizedName.Len() > 24)
		{
			OutError = TEXT("Candidate display name must contain between 1 and 24 characters.");
			return false;
		}

		if (Candidate.RaceId.IsNone() || Candidate.ClassId.IsNone())
		{
			OutError = TEXT("Candidate RaceId and ClassId must be defined.");
			return false;
		}

		if (Candidate.Level < 1 || Candidate.Experience < 0 || Candidate.LastAcknowledgedLevel < 1 || Candidate.LastAcknowledgedLevel > Candidate.Level)
		{
			OutError = TEXT("Candidate RPG progression or Level-Up acknowledgement state is invalid.");
			return false;
		}

		return true;
	}

	bool NormalizeCandidateHotbar(FGridCharacterInventoryState& Candidate, FString& OutError)
	{
		OutError.Empty();

		if (Candidate.CombatHotbarSlots.IsEmpty())
		{
			Candidate.CombatHotbarSlots.SetNum(FGridCombatHotbarBinding::SlotCount);
			for (int32 SlotIndex = 0; SlotIndex < FGridCombatHotbarBinding::SlotCount; ++SlotIndex)
			{
				Candidate.CombatHotbarSlots[SlotIndex].Reset(SlotIndex);
			}
			return true;
		}

		if (Candidate.CombatHotbarSlots.Num() != FGridCombatHotbarBinding::SlotCount)
		{
			OutError =
				FString::Printf(TEXT("Candidate hotbar has %d slots; expected %d."), Candidate.CombatHotbarSlots.Num(), FGridCombatHotbarBinding::SlotCount);
			return false;
		}

		for (int32 SlotIndex = 0; SlotIndex < Candidate.CombatHotbarSlots.Num(); ++SlotIndex)
		{
			const FGridCombatHotbarBinding& Binding = Candidate.CombatHotbarSlots[SlotIndex];
			if (Binding.SlotIndex != SlotIndex || !Binding.IsValid())
			{
				OutError = FString::Printf(TEXT("Candidate hotbar slot %d is invalid."), SlotIndex);
				return false;
			}
		}

		return true;
	}

	void NormalizeCandidateInventoryOwnership(FGridCharacterInventoryState& Candidate, int32 NewCharacterIndex)
	{
		for (FGridInventorySlot& Slot : Candidate.InventorySlots)
		{
			if (Slot.IsEmpty())
			{
				continue;
			}

			Slot.Item.OwnerType = EGridItemOwnerType::CharacterInventory;
			Slot.Item.OwnerGuid = Candidate.CharacterId;
			Slot.Item.OwnerCharacterIndex = NewCharacterIndex;
			Slot.Item.EquipmentSlot = EGridEquipmentSlot::None;
		}
	}

}

bool FRPGPartyRecruitmentService::TryRecruitFromPool(
	UGridPartyInventoryComponent* PartyInventoryComponent, const FGuid& CharacterId, FRPGPartyRecruitmentResult& OutResult)
{
	OutResult = FRPGPartyRecruitmentResult();
	OutResult.CharacterId = CharacterId;

	if (!PartyInventoryComponent)
	{
		OutResult.RejectReason = ERPGPartyRecruitmentRejectReason::InvalidInventory;
		OutResult.Error = TEXT("Party inventory component is missing.");
		return false;
	}

	FGridPartyInventoryState& State = PartyInventoryComponent->PartyInventoryState;
	OutResult.ActiveCountBefore = State.ActiveCharacters.Num();
	OutResult.ActiveCountAfter = OutResult.ActiveCountBefore;

	if (!State.bInitialCharacterCreationCompleted || State.ActiveCharacters.IsEmpty())
	{
		OutResult.RejectReason = ERPGPartyRecruitmentRejectReason::InitialCharacterMissing;
		OutResult.Error = TEXT("Initial party character creation is not completed.");
		return false;
	}

	if (State.ActiveEquipment.Num() != State.ActiveCharacters.Num() || !State.ActiveCharacters.IsValidIndex(State.SelectedCharacterIndex) ||
		State.MaxActiveCharacters < State.ActiveCharacters.Num())
	{
		OutResult.RejectReason = ERPGPartyRecruitmentRejectReason::InvalidPartyState;
		OutResult.Error = TEXT("Active party state is internally inconsistent.");
		return false;
	}

	if (!CharacterId.IsValid())
	{
		OutResult.RejectReason = ERPGPartyRecruitmentRejectReason::InvalidCharacterId;
		OutResult.Error = TEXT("Requested CharacterId is invalid.");
		return false;
	}

	if (State.ActiveCharacters.ContainsByPredicate(
			[&CharacterId](const FGridCharacterInventoryState& Character)
			{
				return Character.CharacterId == CharacterId;
			}))
	{
		OutResult.RejectReason = ERPGPartyRecruitmentRejectReason::DuplicateActiveCharacter;
		OutResult.Error = TEXT("CharacterId is already active in the party.");
		return false;
	}

	if (State.ActiveCharacters.Num() >= State.MaxActiveCharacters)
	{
		OutResult.RejectReason = ERPGPartyRecruitmentRejectReason::PartyFull;
		OutResult.Error = TEXT("Active party is full.");
		return false;
	}

	int32 CandidateIndex = INDEX_NONE;
	int32 CandidateMatchCount = 0;
	for (int32 PoolIndex = 0; PoolIndex < State.CharacterPool.Num(); ++PoolIndex)
	{
		if (State.CharacterPool[PoolIndex].CharacterId != CharacterId)
		{
			continue;
		}

		CandidateIndex = PoolIndex;
		++CandidateMatchCount;
	}

	if (CandidateMatchCount == 0)
	{
		OutResult.RejectReason = ERPGPartyRecruitmentRejectReason::CandidateNotFound;
		OutResult.Error = TEXT("CharacterId was not found in CharacterPool.");
		return false;
	}

	if (CandidateMatchCount != 1 || !State.CharacterPool.IsValidIndex(CandidateIndex))
	{
		OutResult.RejectReason = ERPGPartyRecruitmentRejectReason::AmbiguousCandidate;
		OutResult.Error = TEXT("CharacterPool contains duplicate candidates with the same CharacterId.");
		return false;
	}

	FGridCharacterInventoryState Candidate = State.CharacterPool[CandidateIndex];

	FString CandidateError;
	if (!GridPartyRecruitmentPrivate::IsCandidateIdentityValid(Candidate, CandidateError))
	{
		OutResult.RejectReason = ERPGPartyRecruitmentRejectReason::InvalidCandidate;
		OutResult.Error = CandidateError;
		return false;
	}

	if (Candidate.InventorySlots.IsEmpty())
	{
		Candidate.InventorySlots.SetNum(FMath::Max(0, PartyInventoryComponent->DefaultInventorySlotCountPerCharacter));
	}

	if (!GridPartyRecruitmentPrivate::NormalizeCandidateHotbar(Candidate, CandidateError))
	{
		OutResult.RejectReason = ERPGPartyRecruitmentRejectReason::InvalidCandidate;
		OutResult.Error = CandidateError;
		return false;
	}

	const FGridPartyInventoryState PreviousState = State;
	const int32 NewCharacterIndex = State.ActiveCharacters.Num();

	GridPartyRecruitmentPrivate::NormalizeCandidateInventoryOwnership(Candidate, NewCharacterIndex);

	State.CharacterPool.RemoveAt(CandidateIndex);
	State.ActiveCharacters.Add(MoveTemp(Candidate));
	State.ActiveEquipment.AddDefaulted();

	FString OwnershipError;
	if (!PartyInventoryComponent->ValidateInventoryOwnership(OwnershipError))
	{
		State = PreviousState;
		OutResult.RejectReason = ERPGPartyRecruitmentRejectReason::OwnershipValidationFailed;
		OutResult.Error = OwnershipError;
		OutResult.ActiveCountAfter = OutResult.ActiveCountBefore;
		return false;
	}

	OutResult.bCommitted = true;
	OutResult.RejectReason = ERPGPartyRecruitmentRejectReason::None;
	OutResult.CharacterIndex = NewCharacterIndex;
	OutResult.ActiveCountAfter = State.ActiveCharacters.Num();

	PartyInventoryComponent->NotifyPartyInventoryChanged(NewCharacterIndex);
	return true;
}
