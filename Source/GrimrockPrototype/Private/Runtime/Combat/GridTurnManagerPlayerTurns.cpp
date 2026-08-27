#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"

namespace
{
	int32 ClampPlayerActionPoints(int32 ActionPoints)
	{
		return FMath::Clamp(ActionPoints, 2, 6);
	}
}

bool UGridTurnManagerComponent::GetPlayerCharacterTurnState(int32 CharacterIndex, FGridPlayerCharacterTurnState& OutTurnState) const
{
	OutTurnState = FGridPlayerCharacterTurnState();
	if (!IsValid(PartyPawn) || !IsValid(PartyPawn->PartyInventoryComponent))
	{
		return false;
	}

	const TArray<FGridCharacterInventoryState>& Characters = PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters;
	if (!Characters.IsValidIndex(CharacterIndex))
	{
		return false;
	}

	const FGridCharacterInventoryState& Character = Characters[CharacterIndex];
	const FGuid CombatantId = ResolvePlayerCombatantId(CharacterIndex);

	if (const FGridPlayerCharacterTurnState* Existing = FindPlayerCharacterTurnState(CombatantId))
	{
		OutTurnState = *Existing;
		OutTurnState.CharacterIndex = CharacterIndex;
	}
	else
	{
		OutTurnState.CharacterIndex = CharacterIndex;
		OutTurnState.CharacterId = CombatantId;
		OutTurnState.MaximumActionPoints = ClampPlayerActionPoints(BasePlayerActionPointsPerTurn);

		if (!InitiativeOrder.IsEmpty())
		{
			if (const FGridCombatantInitiativeEntry* Entry = FindInitiativeEntry(EGridCombatantSide::Party, CombatantId))
			{
				OutTurnState.State = Entry->State;
				OutTurnState.RemainingActionPoints = Entry->State == EGridCombatantTurnState::Active ? OutTurnState.MaximumActionPoints : 0;
			}
		}
		else if (bCombatActive && CurrentPhase == EGridCombatPhase::PlayerPhase)
		{
			OutTurnState.State = EGridCombatantTurnState::Active;
			OutTurnState.RemainingActionPoints = OutTurnState.MaximumActionPoints;
		}
		else if (bCombatActive && (CurrentPhase == EGridCombatPhase::EnemyPhase || CurrentPhase == EGridCombatPhase::EndingRound))
		{
			OutTurnState.State = EGridCombatantTurnState::Completed;
		}
	}

	if (Character.Resources.CurrentHealth <= 0)
	{
		OutTurnState.State = EGridCombatantTurnState::Defeated;
		OutTurnState.RemainingActionPoints = 0;
	}
	return true;
}

bool UGridTurnManagerComponent::CanCharacterAct(int32 CharacterIndex) const
{
	if (!bInitialized || !bCombatActive || CurrentPhase != EGridCombatPhase::PlayerPhase || bPartyInputLocked || bPlayerAttackResolutionInProgress ||
		!IsPartyAtRest())
	{
		return false;
	}

	if (!InitiativeOrder.IsEmpty() && !IsActivePlayerCharacter(CharacterIndex))
	{
		return false;
	}

	FGridPlayerCharacterTurnState TurnState;
	return GetPlayerCharacterTurnState(CharacterIndex, TurnState) && TurnState.State == EGridCombatantTurnState::Active && TurnState.RemainingActionPoints > 0;
}

bool UGridTurnManagerComponent::CanCharacterSpendActionPoints(int32 CharacterIndex, int32 ActionPointCost) const
{
	if (ActionPointCost < 0 || !CanCharacterAct(CharacterIndex))
	{
		return false;
	}

	FGridPlayerCharacterTurnState TurnState;
	return GetPlayerCharacterTurnState(CharacterIndex, TurnState) && TurnState.CanSpend(ActionPointCost);
}

bool UGridTurnManagerComponent::HasCharacterCommittedAttackThisPhase(int32 CharacterIndex) const
{
	FGridPlayerCharacterTurnState TurnState;
	return GetPlayerCharacterTurnState(CharacterIndex, TurnState) && TurnState.MaximumActionPoints > 0 &&
		TurnState.RemainingActionPoints < TurnState.MaximumActionPoints;
}

void UGridTurnManagerComponent::BeginPlayerCharacterPhase()
{
	PlayerCharacterTurnStates.Reset();
	if (!IsValid(PartyPawn) || !IsValid(PartyPawn->PartyInventoryComponent))
	{
		return;
	}

	const TArray<FGridCharacterInventoryState>& Characters = PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters;
	PlayerCharacterTurnStates.Reserve(Characters.Num());
	const int32 MaximumActionPoints = ClampPlayerActionPoints(BasePlayerActionPointsPerTurn);

	for (int32 CharacterIndex = 0; CharacterIndex < Characters.Num(); ++CharacterIndex)
	{
		const FGridCharacterInventoryState& Character = Characters[CharacterIndex];

		FGridPlayerCharacterTurnState TurnState;
		TurnState.CharacterIndex = CharacterIndex;
		TurnState.CharacterId = ResolvePlayerCombatantId(CharacterIndex);
		TurnState.MaximumActionPoints = MaximumActionPoints;
		if (Character.Resources.CurrentHealth > 0)
		{
			TurnState.State = InitiativeOrder.IsEmpty() ? EGridCombatantTurnState::Active : EGridCombatantTurnState::Waiting;
			TurnState.RemainingActionPoints = InitiativeOrder.IsEmpty() ? MaximumActionPoints : 0;
		}
		else
		{
			TurnState.State = EGridCombatantTurnState::Defeated;
		}
		PlayerCharacterTurnStates.Add(TurnState);
	}

	for (const FGridPlayerCharacterTurnState& TurnState : PlayerCharacterTurnStates)
	{
		BroadcastPlayerCharacterTurnState(TurnState);
	}
}

void UGridTurnManagerComponent::CompletePlayerCharacterPhase()
{
	if (!IsValid(PartyPawn) || !IsValid(PartyPawn->PartyInventoryComponent))
	{
		return;
	}

	const int32 CharacterCount = PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters.Num();
	for (int32 CharacterIndex = 0; CharacterIndex < CharacterCount; ++CharacterIndex)
	{
		FGridPlayerCharacterTurnState* TurnState = EnsurePlayerCharacterTurnState(CharacterIndex);
		if (!TurnState)
		{
			continue;
		}

		RefreshPlayerCharacterVitalState(CharacterIndex);
		TurnState = FindPlayerCharacterTurnState(TurnState->CharacterId);
		if (TurnState && TurnState->State != EGridCombatantTurnState::Defeated && TurnState->State != EGridCombatantTurnState::Incapacitated)
		{
			TurnState->State = EGridCombatantTurnState::Completed;
			BroadcastPlayerCharacterTurnState(*TurnState);
		}
	}
}

void UGridTurnManagerComponent::ClearPlayerCharacterTurnStates()
{
	PlayerCharacterTurnStates.Reset();
}

bool UGridTurnManagerComponent::SpendPlayerCharacterActionPoints(int32 CharacterIndex, int32 ActionPointCost)
{
	if (ActionPointCost < 0)
	{
		return false;
	}

	FGridPlayerCharacterTurnState* TurnState = EnsurePlayerCharacterTurnState(CharacterIndex);
	if (!TurnState || !TurnState->CanSpend(ActionPointCost))
	{
		return false;
	}

	TurnState->RemainingActionPoints = FMath::Max(0, TurnState->RemainingActionPoints - ActionPointCost);
	if (TurnState->RemainingActionPoints == 0 && InitiativeOrder.IsEmpty())
	{
		TurnState->State = EGridCombatantTurnState::Completed;
	}

	UE_LOG(LogGridTurnManager, Log, TEXT("[GridPlayerTurn] Character=%d Cost=%d AP=%d/%d State=%s"), CharacterIndex, ActionPointCost,
		TurnState->RemainingActionPoints, TurnState->MaximumActionPoints, *UEnum::GetValueAsString(TurnState->State));
	BroadcastPlayerCharacterTurnState(*TurnState);
	return true;
}

void UGridTurnManagerComponent::RefreshPlayerCharacterVitalState(int32 CharacterIndex)
{
	if (!IsValid(PartyPawn) || !IsValid(PartyPawn->PartyInventoryComponent))
	{
		return;
	}

	const TArray<FGridCharacterInventoryState>& Characters = PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters;
	if (!Characters.IsValidIndex(CharacterIndex))
	{
		return;
	}

	FGridPlayerCharacterTurnState* TurnState = EnsurePlayerCharacterTurnState(CharacterIndex);
	if (!TurnState)
	{
		return;
	}

	const bool bDefeated = Characters[CharacterIndex].Resources.CurrentHealth <= 0;
	if (bDefeated && TurnState->State != EGridCombatantTurnState::Defeated)
	{
		TurnState->State = EGridCombatantTurnState::Defeated;
		TurnState->RemainingActionPoints = 0;
		BroadcastPlayerCharacterTurnState(*TurnState);
	}

	if (FGridCombatantInitiativeEntry* Entry = FindInitiativeEntry(EGridCombatantSide::Party, ResolvePlayerCombatantId(CharacterIndex)))
	{
		const int32 PreviousHealth = Entry->CurrentHealth;
		RefreshInitiativeEntryVitals(*Entry);
		if (bDefeated)
		{
			SetInitiativeEntryState(*Entry, EGridCombatantTurnState::Defeated);
			BroadcastInitiativeOrderChanged();
		}
		else if (Entry->CurrentHealth != PreviousHealth)
		{
			OnCombatantStateChanged.Broadcast(*Entry);
		}
	}
}

FGridPlayerCharacterTurnState* UGridTurnManagerComponent::EnsurePlayerCharacterTurnState(int32 CharacterIndex)
{
	FGridPlayerCharacterTurnState DerivedState;
	if (!GetPlayerCharacterTurnState(CharacterIndex, DerivedState))
	{
		return nullptr;
	}

	if (FGridPlayerCharacterTurnState* Existing = FindPlayerCharacterTurnState(DerivedState.CharacterId))
	{
		Existing->CharacterIndex = CharacterIndex;
		return Existing;
	}

	const int32 AddedIndex = PlayerCharacterTurnStates.Add(DerivedState);
	return &PlayerCharacterTurnStates[AddedIndex];
}

const FGridPlayerCharacterTurnState* UGridTurnManagerComponent::FindPlayerCharacterTurnState(const FGuid& CharacterId) const
{
	return PlayerCharacterTurnStates.FindByPredicate(
		[&CharacterId](const FGridPlayerCharacterTurnState& TurnState)
		{
			return TurnState.CharacterId == CharacterId;
		});
}

FGridPlayerCharacterTurnState* UGridTurnManagerComponent::FindPlayerCharacterTurnState(const FGuid& CharacterId)
{
	return PlayerCharacterTurnStates.FindByPredicate(
		[&CharacterId](const FGridPlayerCharacterTurnState& TurnState)
		{
			return TurnState.CharacterId == CharacterId;
		});
}

void UGridTurnManagerComponent::BroadcastPlayerCharacterTurnState(const FGridPlayerCharacterTurnState& TurnState)
{
	OnPlayerCharacterTurnStateChanged.Broadcast(TurnState);
}
