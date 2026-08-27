#include "RPG/RPGCustomRecruitService.h"

#include "RPG/RPGCharacterRulesLibrary.h"
#include "RPG/RPGClassAsset.h"
#include "RPG/RPGPartyRecruitmentService.h"
#include "RPG/RPGRaceAsset.h"
#include "Runtime/GridPartyInventoryComponent.h"

namespace RPGCustomRecruitServicePrivate
{
	FString NormalizeDisplayName(const FText& DisplayName)
	{
		FString Result = DisplayName.ToString();
		Result.TrimStartAndEndInline();
		return Result;
	}

	void InitializeEmptyHotbar(FGridCharacterInventoryState& Character)
	{
		Character.CombatHotbarSlots.SetNum(FGridCombatHotbarBinding::SlotCount);
		for (int32 SlotIndex = 0; SlotIndex < FGridCombatHotbarBinding::SlotCount; ++SlotIndex)
		{
			Character.CombatHotbarSlots[SlotIndex].Reset(SlotIndex);
		}
	}

	bool IsCharacterIdInUse(const FGridPartyInventoryState& State, const FGuid& CharacterId)
	{
		if (!CharacterId.IsValid())
		{
			return true;
		}

		return State.ActiveCharacters.ContainsByPredicate(
				   [&CharacterId](const FGridCharacterInventoryState& Character)
				   {
					   return Character.CharacterId == CharacterId;
				   }) ||
			State.CharacterPool.ContainsByPredicate(
				[&CharacterId](const FGridCharacterInventoryState& Character)
				{
					return Character.CharacterId == CharacterId;
				});
	}

	bool GenerateUniqueCharacterId(const FGridPartyInventoryState& State, FGuid& OutCharacterId)
	{
		OutCharacterId.Invalidate();
		constexpr int32 MaxAttempts = 16;
		for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
		{
			const FGuid CandidateId = FGuid::NewGuid();
			if (!IsCharacterIdInUse(State, CandidateId))
			{
				OutCharacterId = CandidateId;
				return true;
			}
		}
		return false;
	}

	FGridCharacterInventoryState BuildCandidate(const FRPGCharacterCreationRequest& Request, URPGClassAsset* CombatActionSourceClass,
		const FString& NormalizedName, const FGuid& CharacterId, int32 InventorySlotCount, const FRPGAttributes& FinalAttributes)
	{
		FGridCharacterInventoryState Candidate;
		Candidate.CharacterId = CharacterId;
		Candidate.DisplayName = FText::FromString(NormalizedName);
		Candidate.RaceId = Request.RaceDefinition->RaceId;
		Candidate.RaceDisplayName = Request.RaceDefinition->DisplayName;
		Candidate.ClassId = Request.ClassDefinition->ClassId;
		Candidate.ClassDisplayName = Request.ClassDefinition->DisplayName;
		Candidate.ClassDefinition = CombatActionSourceClass;
		Candidate.Level = 1;
		Candidate.Experience = 0;
		Candidate.Attributes = FinalAttributes;
		Candidate.DerivedStats = URPGCharacterRulesLibrary::CalculateDerivedStats(FinalAttributes, Request.ClassDefinition, Candidate.Level);
		Candidate.PortraitGender = Request.PortraitGender;
		Candidate.PortraitVariantId = Request.PortraitVariantId;
		Candidate.Portrait = Request.Portrait;
		Candidate.ClassIcon = Request.ClassIcon;
		Candidate.MaxCarryWeight = URPGCharacterRulesLibrary::CalculateMaxCarryWeight(FinalAttributes);
		Candidate.CurrentWeight = 0.0f;
		Candidate.InventorySlots.SetNum(FMath::Max(0, InventorySlotCount));
		InitializeEmptyHotbar(Candidate);
		return Candidate;
	}
}

bool FRPGCustomRecruitService::TryCreateAndRecruit(
	UGridPartyInventoryComponent* PartyInventoryComponent, const FRPGCharacterCreationRequest& Request, FRPGCustomRecruitResult& OutResult)
{
	OutResult = FRPGCustomRecruitResult();

	if (!PartyInventoryComponent)
	{
		OutResult.RejectReason = ERPGCustomRecruitRejectReason::InvalidInventory;
		OutResult.Error = TEXT("Party inventory component is missing.");
		return false;
	}

	FGridPartyInventoryState& State = PartyInventoryComponent->PartyInventoryState;
	OutResult.ActiveCountBefore = State.ActiveCharacters.Num();
	OutResult.ActiveCountAfter = OutResult.ActiveCountBefore;

	if (!State.bInitialCharacterCreationCompleted || State.ActiveCharacters.IsEmpty())
	{
		OutResult.RejectReason = ERPGCustomRecruitRejectReason::InitialCharacterMissing;
		OutResult.Error = TEXT("Initial party character creation is not completed.");
		return false;
	}

	if (State.ActiveEquipment.Num() != State.ActiveCharacters.Num() || !State.ActiveCharacters.IsValidIndex(State.SelectedCharacterIndex) ||
		State.MaxActiveCharacters < State.ActiveCharacters.Num())
	{
		OutResult.RejectReason = ERPGCustomRecruitRejectReason::InvalidPartyState;
		OutResult.Error = TEXT("Active party state is internally inconsistent.");
		return false;
	}

	if (State.ActiveCharacters.Num() >= State.MaxActiveCharacters)
	{
		OutResult.RejectReason = ERPGCustomRecruitRejectReason::PartyFull;
		OutResult.Error = TEXT("Active party is full.");
		return false;
	}

	const FString NormalizedName = RPGCustomRecruitServicePrivate::NormalizeDisplayName(Request.DisplayName);
	if (NormalizedName.Len() < 1 || NormalizedName.Len() > 24)
	{
		OutResult.RejectReason = ERPGCustomRecruitRejectReason::InvalidName;
		OutResult.Error = TEXT("Custom recruit display name must contain between 1 and 24 characters.");
		return false;
	}

	if (!Request.RaceDefinition || !Request.RaceDefinition->IsValidDefinition())
	{
		OutResult.RejectReason = ERPGCustomRecruitRejectReason::InvalidRace;
		OutResult.Error = TEXT("A valid race definition is required.");
		return false;
	}

	if (!Request.ClassDefinition || !Request.ClassDefinition->IsValidDefinition())
	{
		OutResult.RejectReason = ERPGCustomRecruitRejectReason::InvalidClass;
		OutResult.Error = TEXT("A valid class definition is required.");
		return false;
	}

	URPGClassAsset* CombatActionSourceClass =
		Request.CombatActionSourceClassDefinition ? Request.CombatActionSourceClassDefinition.Get() : Request.ClassDefinition.Get();
	if (!CombatActionSourceClass || !CombatActionSourceClass->IsValidDefinition() || CombatActionSourceClass->ClassId != Request.ClassDefinition->ClassId)
	{
		OutResult.RejectReason = ERPGCustomRecruitRejectReason::InvalidCombatActionSourceClass;
		OutResult.Error = TEXT("The canonical class action source is invalid.");
		return false;
	}

	const FRPGAttributes FinalAttributes =
		URPGCharacterRulesLibrary::AddAttributes(Request.ClassDefinition->BaseAttributes, Request.RaceDefinition->AttributeBonuses);
	if (!URPGCharacterRulesLibrary::AreAttributesInRange(FinalAttributes))
	{
		OutResult.RejectReason = ERPGCustomRecruitRejectReason::InvalidAttributes;
		OutResult.Error = TEXT("All custom recruit attributes must be between 6 and 20.");
		return false;
	}

	FGuid CharacterId;
	if (!RPGCustomRecruitServicePrivate::GenerateUniqueCharacterId(State, CharacterId))
	{
		OutResult.RejectReason = ERPGCustomRecruitRejectReason::CharacterIdGenerationFailed;
		OutResult.Error = TEXT("A unique custom recruit CharacterId could not be generated.");
		return false;
	}

	FGridCharacterInventoryState Candidate = RPGCustomRecruitServicePrivate::BuildCandidate(
		Request, CombatActionSourceClass, NormalizedName, CharacterId, PartyInventoryComponent->DefaultInventorySlotCountPerCharacter, FinalAttributes);

	const FGridPartyInventoryState PreviousState = State;
	State.CharacterPool.Add(MoveTemp(Candidate));

	FRPGPartyRecruitmentResult RecruitmentResult;
	if (!FRPGPartyRecruitmentService::TryRecruitFromPool(PartyInventoryComponent, CharacterId, RecruitmentResult) || !RecruitmentResult.bCommitted)
	{
		State = PreviousState;
		OutResult.RejectReason = ERPGCustomRecruitRejectReason::RecruitmentFailed;
		OutResult.Error = RecruitmentResult.Error.IsEmpty() ? TEXT("Custom recruit activation failed.") : RecruitmentResult.Error;
		OutResult.CharacterId.Invalidate();
		OutResult.CharacterIndex = INDEX_NONE;
		OutResult.ActiveCountAfter = OutResult.ActiveCountBefore;
		return false;
	}

	OutResult.bCommitted = true;
	OutResult.RejectReason = ERPGCustomRecruitRejectReason::None;
	OutResult.CharacterId = CharacterId;
	OutResult.CharacterIndex = RecruitmentResult.CharacterIndex;
	OutResult.ActiveCountAfter = State.ActiveCharacters.Num();
	return true;
}
