#include "Save/GridCombatSavePolicy.h"

#include "Engine/World.h"
#include "Runtime/Combat/GridTurnManagerComponent.h"
#include "Runtime/GridLevelRuntimeActor.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridCombatSavePolicy, Log, All);

namespace
{
	bool PartyStatesShareStableCharacter(const FGridPartyInventoryState& SavedParty, const FGridPartyInventoryState& RuntimeParty)
	{
		TSet<FGuid> SavedCharacterIds;
		for (const FGridCharacterInventoryState& Character : SavedParty.ActiveCharacters)
		{
			if (Character.CharacterId.IsValid())
			{
				SavedCharacterIds.Add(Character.CharacterId);
			}
		}
		for (const FGridCharacterInventoryState& Character : SavedParty.CharacterPool)
		{
			if (Character.CharacterId.IsValid())
			{
				SavedCharacterIds.Add(Character.CharacterId);
			}
		}
		if (SavedCharacterIds.IsEmpty())
		{
			return false;
		}

		for (const FGridCharacterInventoryState& Character : RuntimeParty.ActiveCharacters)
		{
			if (Character.CharacterId.IsValid() && SavedCharacterIds.Contains(Character.CharacterId))
			{
				return true;
			}
		}
		for (const FGridCharacterInventoryState& Character : RuntimeParty.CharacterPool)
		{
			if (Character.CharacterId.IsValid() && SavedCharacterIds.Contains(Character.CharacterId))
			{
				return true;
			}
		}
		return false;
	}
}

FString FGridCombatSavePolicy::BuildPreCombatCheckpointSlotName(const FString& BaseSlotName)
{
	return BaseSlotName.IsEmpty() ? FString() : BaseSlotName + TEXT("_AutoCombat");
}

bool FGridCombatSavePolicy::IsSaveBlockedByCombatState(const UGridTurnManagerComponent* TurnManager)
{
	if (!IsValid(TurnManager))
	{
		return false;
	}

	if (TurnManager->bCombatActive)
	{
		return true;
	}

	return TurnManager->CurrentPhase != EGridCombatPhase::Exploration && TurnManager->CurrentPhase != EGridCombatPhase::Victory;
}

bool FGridCombatSavePolicy::IsSaveBlockedForParty(const FGridPartyInventoryState& PartyState)
{
	for (TObjectIterator<UGridTurnManagerComponent> It; It; ++It)
	{
		UGridTurnManagerComponent* TurnManager = *It;
		UWorld* World = IsValid(TurnManager) ? TurnManager->GetWorld() : nullptr;
		AGrimrockPartyPawn* PartyPawn = IsValid(TurnManager) ? TurnManager->PartyPawn.Get() : nullptr;
		UGridPartyInventoryComponent* Inventory = IsValid(PartyPawn) ? PartyPawn->PartyInventoryComponent.Get() : nullptr;
		if (!World || !World->IsGameWorld() || !IsValid(PartyPawn) || !IsValid(Inventory) ||
			!PartyStatesShareStableCharacter(PartyState, Inventory->PartyInventoryState))
		{
			continue;
		}

		if (IsSaveBlockedByCombatState(TurnManager))
		{
			UE_LOG(LogGridCombatSavePolicy, Log, TEXT("[MON18.9.1] SaveBlocked CharacterCount=%d Phase=%s Active=%s"), PartyState.ActiveCharacters.Num(),
				*UEnum::GetValueAsString(TurnManager->CurrentPhase), TurnManager->bCombatActive ? TEXT("true") : TEXT("false"));
			return true;
		}
	}

	return false;
}

bool FGridCombatSavePolicy::PreparePreCombatCheckpoint(AGrimrockPartyPawn* PartyPawn, FText& OutError, bool& bOutSkipped)
{
	OutError = FText::GetEmpty();
	bOutSkipped = false;

	if (!IsValid(PartyPawn))
	{
		OutError = FText::FromString(TEXT("Le groupe est indisponible pour le checkpoint pré-combat."));
		return false;
	}

	UGridPartyInventoryComponent* Inventory = PartyPawn->PartyInventoryComponent.Get();
	if (PartyPawn->PartySaveSlotName.IsEmpty() || !IsValid(Inventory) || !Inventory->HasCompletedInitialCharacterCreation())
	{
		bOutSkipped = true;
		UE_LOG(LogGridCombatSavePolicy, Verbose, TEXT("[MON18.9.1] PreCombatCheckpoint Skipped Pawn=%s Reason=TransientOrUninitializedParty"),
			*GetNameSafe(PartyPawn));
		return true;
	}

	UGridTurnManagerComponent* TurnManager =
		IsValid(PartyPawn->LevelRuntimeActor) ? PartyPawn->LevelRuntimeActor->FindComponentByClass<UGridTurnManagerComponent>() : nullptr;
	if (IsSaveBlockedByCombatState(TurnManager))
	{
		OutError = FText::FromString(TEXT("Le checkpoint pré-combat doit être créé avant l'activation du combat."));
		return false;
	}

	const FString MainSlotName = PartyPawn->PartySaveSlotName;
	const FString CheckpointSlotName = BuildPreCombatCheckpointSlotName(MainSlotName);
	if (CheckpointSlotName.IsEmpty())
	{
		OutError = FText::FromString(TEXT("Le nom du checkpoint pré-combat est invalide."));
		return false;
	}

	PartyPawn->PartySaveSlotName = CheckpointSlotName;
	const bool bSaved = PartyPawn->SaveCurrentGame(OutError);
	PartyPawn->PartySaveSlotName = MainSlotName;

	if (!bSaved)
	{
		UE_LOG(LogGridCombatSavePolicy, Warning, TEXT("[MON18.9.1] PreCombatCheckpoint Failed Slot=%s Reason=%s"), *CheckpointSlotName, *OutError.ToString());
		return false;
	}

	UE_LOG(LogGridCombatSavePolicy, Log, TEXT("[MON18.9.1] PreCombatCheckpoint Saved Slot=%s SourceSlot=%s Cell=(%d,%d)"), *CheckpointSlotName, *MainSlotName,
		PartyPawn->CurrentCellX, PartyPawn->CurrentCellY);
	return true;
}
