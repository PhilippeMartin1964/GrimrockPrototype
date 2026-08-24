#pragma once

#include "CoreMinimal.h"
#include "RPG/RPGClassProgressionService.h"

class UGridPartyInventoryComponent;

/** Read-only presentation/domain view of one MON15 progression choice as a talent. */
struct FRPGTalentRuntimeView
{
    FName ChoiceId = NAME_None;
    FText DisplayName;
    FText Description;
    int32 MinimumLevel = 1;
    int32 PointCost = 0;
    bool bSelected = false;
    ERPGClassProgressionChoiceAvailabilityReason AvailabilityReason =
        ERPGClassProgressionChoiceAvailabilityReason::None;
};

/** Read-only alias of the authoritative MON15 choice-point balance. */
struct FRPGTalentPointBalance
{
    int32 GrantedPoints = 0;
    int32 SpentPoints = 0;
    int32 RemainingPoints = 0;
};

/**
 * Stateless MON20.7 facade over the authoritative MON15 class progression system.
 *
 * Talent is a domain/presentation name only:
 *   Talent == FRPGClassProgressionChoiceDefinition
 *   Talent id == ChoiceId
 *   Talent points == ChoicePoints
 *
 * This service never stores or mutates a talent selection. Mutations continue to
 * go through FRPGClassProgressionTransactionService::TryCommitChoices().
 */
struct GRIMROCKPROTOTYPE_API FRPGTalentRuntimeService
{
    static bool TryGetSelectedTalents (
        UGridPartyInventoryComponent* PartyInventoryComponent,
        int32 CharacterIndex,
        TArray<FRPGTalentRuntimeView>& OutTalents);

    static bool TryGetSelectedCharacterTalents (
        UGridPartyInventoryComponent* PartyInventoryComponent,
        TArray<FRPGTalentRuntimeView>& OutTalents);

    static bool HasTalent (
        UGridPartyInventoryComponent* PartyInventoryComponent,
        int32 CharacterIndex,
        FName ChoiceId,
        bool& OutHasTalent);

    static bool HasSelectedCharacterTalent (
        UGridPartyInventoryComponent* PartyInventoryComponent,
        FName ChoiceId,
        bool& OutHasTalent);

    static bool TryGetTalentPointBalance (
        UGridPartyInventoryComponent* PartyInventoryComponent,
        int32 CharacterIndex,
        FRPGTalentPointBalance& OutBalance);

    static bool TryGetSelectedCharacterTalentPointBalance (
        UGridPartyInventoryComponent* PartyInventoryComponent,
        FRPGTalentPointBalance& OutBalance);

    static bool TryGetAvailableTalents (
        UGridPartyInventoryComponent* PartyInventoryComponent,
        int32 CharacterIndex,
        TArray<FRPGTalentRuntimeView>& OutTalents);

    static bool TryGetSelectedCharacterAvailableTalents (
        UGridPartyInventoryComponent* PartyInventoryComponent,
        TArray<FRPGTalentRuntimeView>& OutTalents);
};
