#pragma once

#include "CoreMinimal.h"

class UGridPartyInventoryComponent;
class URPGStoryCompanionAsset;

enum class ERPGStoryCompanionRegistrationStatus : uint8
{
	None,
	AddedToPool,
	AlreadyInPool,
	AlreadyActive,
	InvalidInventory,
	InvalidDefinition,
	IdentityCollision
};

struct FRPGStoryCompanionRegistrationResult
{
	bool bSucceeded = false;
	ERPGStoryCompanionRegistrationStatus Status = ERPGStoryCompanionRegistrationStatus::None;
	FGuid CharacterId;
	int32 PoolIndex = INDEX_NONE;
	int32 ActiveIndex = INDEX_NONE;
	FString Error;
};

/**
 * Registers one story companion candidate into the existing CharacterPool.
 * The operation is idempotent and never creates a parallel companion registry.
 */
struct GRIMROCKPROTOTYPE_API FRPGStoryCompanionService
{
	static bool EnsureCandidateRegistered(UGridPartyInventoryComponent* PartyInventoryComponent, const URPGStoryCompanionAsset* CompanionDefinition,
		FRPGStoryCompanionRegistrationResult& OutResult);
};
