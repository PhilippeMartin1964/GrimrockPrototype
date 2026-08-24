#pragma once

#include "CoreMinimal.h"

class UGridPartyInventoryComponent;
struct FRPGCharacterCreationRequest;

enum class ERPGCustomRecruitRejectReason : uint8
{
    None,
    InvalidInventory,
    InitialCharacterMissing,
    InvalidPartyState,
    PartyFull,
    InvalidName,
    InvalidRace,
    InvalidClass,
    InvalidCombatActionSourceClass,
    InvalidAttributes,
    CharacterIdGenerationFailed,
    RecruitmentFailed
};

struct FRPGCustomRecruitResult
{
    bool bCommitted = false;
    ERPGCustomRecruitRejectReason RejectReason =
        ERPGCustomRecruitRejectReason::None;
    FGuid CharacterId;
    int32 CharacterIndex = INDEX_NONE;
    int32 ActiveCountBefore = 0;
    int32 ActiveCountAfter = 0;
    FString Error;
};

/**
 * MON20.5 transaction that builds one custom recruit from the existing
 * character-creation request, stages it in CharacterPool and delegates the
 * authoritative activation to FRPGPartyRecruitmentService.
 */
struct GRIMROCKPROTOTYPE_API FRPGCustomRecruitService
{
    static bool TryCreateAndRecruit (
        UGridPartyInventoryComponent* PartyInventoryComponent,
        const FRPGCharacterCreationRequest& Request,
        FRPGCustomRecruitResult& OutResult);
};
