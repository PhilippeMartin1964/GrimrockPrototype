#pragma once

#include "CoreMinimal.h"

class UGridPartyInventoryComponent;

enum class ERPGPartyRecruitmentRejectReason : uint8
{
	None,
	InvalidInventory,
	InitialCharacterMissing,
	InvalidPartyState,
	InvalidCharacterId,
	CandidateNotFound,
	AmbiguousCandidate,
	DuplicateActiveCharacter,
	PartyFull,
	InvalidCandidate,
	OwnershipValidationFailed
};

struct FRPGPartyRecruitmentResult
{
	bool bCommitted = false;
	ERPGPartyRecruitmentRejectReason RejectReason = ERPGPartyRecruitmentRejectReason::None;
	FGuid CharacterId;
	int32 CharacterIndex = INDEX_NONE;
	int32 ActiveCountBefore = 0;
	int32 ActiveCountAfter = 0;
	FString Error;
};

/**
 * MON20.2 authoritative transaction that activates one already-built character
 * from FGridPartyInventoryState::CharacterPool.
 *
 * CharacterPool is the existing reserve-like storage. This service deliberately
 * does not create a parallel party registry and does not construct story/custom
 * recruits yet. Those authoring paths feed CharacterPool in later MON20 steps.
 */
struct GRIMROCKPROTOTYPE_API FRPGPartyRecruitmentService
{
	static bool TryRecruitFromPool(UGridPartyInventoryComponent* PartyInventoryComponent, const FGuid& CharacterId, FRPGPartyRecruitmentResult& OutResult);
};
