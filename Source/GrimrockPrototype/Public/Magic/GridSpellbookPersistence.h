#pragma once

#include "CoreMinimal.h"
#include "Magic/GridSpellbookTypes.h"
#include "Runtime/GridInventoryTypes.h"
#include "GridSpellbookPersistence.generated.h"

/**
 * MON18.8 persistent spell knowledge for one stable CharacterId.
 * Static spell definitions and presentation data are deliberately excluded.
 */
USTRUCT(BlueprintType)
struct FGridCharacterSpellbookSaveState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Magic|Spellbook|Save")
	FGuid CharacterId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Magic|Spellbook|Save")
	TArray<FName> KnownSpellIds;
};

/**
 * MON18.8 conversion boundary between transient runtime Spellbook state and
 * the versioned SaveGame snapshot. CharacterId and SpellId are the only
 * persisted identities.
 */
struct GRIMROCKPROTOTYPE_API FGridSpellbookPersistence
{
	/**
     * Captures a deterministic sparse snapshot. Empty per-character
     * spellbooks are omitted, but every runtime entry must resolve to exactly
     * one character in the party state.
     */
	static bool CapturePartySpellbooks(const FGridPartyInventoryState& PartyState, const FGridPartySpellbookState& RuntimeState,
		TArray<FGridCharacterSpellbookSaveState>& OutSavedStates, FString& OutError);

	/**
     * Validates a saved snapshot against stable CharacterIds without resolving
     * spell definitions. Unknown-but-non-empty SpellIds are preserved so
     * temporarily missing content remains recoverable.
     */
	static bool ValidateSavedPartySpellbooks(
		const FGridPartyInventoryState& PartyState, const TArray<FGridCharacterSpellbookSaveState>& SavedStates, FString& OutError);

	/**
     * Atomically rebuilds one runtime spellbook container for every active and
     * pooled character. OutRuntimeState is untouched on failure.
     */
	static bool RestorePartySpellbooks(const FGridPartyInventoryState& PartyState, const TArray<FGridCharacterSpellbookSaveState>& SavedStates,
		FGridPartySpellbookState& OutRuntimeState, FString& OutError);
};
