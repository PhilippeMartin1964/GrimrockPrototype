#include "Runtime/Combat/GridTurnManagerComponent.h"

#include "RPG/StatusEffects/GridStatusEffectControlResolver.h"
#include "Runtime/GridPartyInventoryComponent.h"
#include "Runtime/GrimrockPartyPawn.h"
#include "Runtime/Monsters/GridMonsterActor.h"
#include "Runtime/Monsters/GridMonsterCombatComponent.h"
#include "Runtime/Monsters/GridMonsterDefinitionAsset.h"
#include "Runtime/Monsters/GridMonsterMovementComponent.h"

namespace
{
	constexpr int32 InitiativeSeedSalt = 0x4D4F4E12;

	FText ResolveInitiativeCharacterName(const FGridCharacterInventoryState& Character, int32 CharacterIndex)
	{
		return Character.DisplayName.IsEmpty() ? FText::FromString(FString::Printf(TEXT("Hero_%02d"), CharacterIndex + 1)) : Character.DisplayName;
	}
}

void UGridTurnManagerComponent::BuildGlobalInitiativeOrder()
{
	InitiativeOrder.Reset();
	CurrentInitiativeIndex = INDEX_NONE;

	if (IsValid(PartyPawn) && IsValid(PartyPawn->PartyInventoryComponent))
	{
		const TArray<FGridCharacterInventoryState>& Characters = PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters;
		InitiativeOrder.Reserve(Characters.Num() + CombatMonsters.Num());

		for (int32 CharacterIndex = 0; CharacterIndex < Characters.Num(); ++CharacterIndex)
		{
			const FGridCharacterInventoryState& Character = Characters[CharacterIndex];

			FGridInventoryCharacterSummary Summary;
			const bool bHasSummary = PartyPawn->PartyInventoryComponent->GetCharacterSummary(CharacterIndex, Summary);

			FGridCombatantInitiativeEntry Entry;
			Entry.CombatantId = ResolvePlayerCombatantId(CharacterIndex);
			Entry.Side = EGridCombatantSide::Party;
			Entry.CharacterIndex = CharacterIndex;
			Entry.DisplayName = ResolveInitiativeCharacterName(Character, CharacterIndex);
			Entry.Portrait = Character.Portrait;
			Entry.InitiativeBase = 10 + Character.DerivedStats.Initiative;
			Entry.Dexterity = bHasSummary ? Summary.Attributes.Dexterity : Character.Attributes.Dexterity;
			Entry.CurrentHealth = Character.Resources.CurrentHealth;
			Entry.MaximumHealth = FMath::Max(1, Character.DerivedStats.MaxHealth);
			Entry.State = Entry.CurrentHealth > 0 ? EGridCombatantTurnState::Waiting : EGridCombatantTurnState::Defeated;
			InitiativeOrder.Add(Entry);
		}
	}

	for (AGridMonsterActor* Monster : CombatMonsters)
	{
		FString DefinitionError;
		if (!IsValid(Monster) || !Monster->ValidateMonsterDefinition(DefinitionError) || !Monster->bCombatStatsInitialized)
		{
			if (IsValid(Monster))
			{
				UE_LOG(LogGridTurnManager, Error,
					TEXT(
						"[GridInitiative] Monster skipped Monster=%s PersistenceId=%s Definition=%s MonsterId=%s CurrentHealth=%d MaxHealth=%d CombatStatsInitialized=%s RuntimeState=%s Reason=%s ValidationError=\"%s\""),
					*GetNameSafe(Monster), *Monster->ResolvePersistenceId().ToString(), *GetPathNameSafe(Monster->MonsterDefinition),
					Monster->MonsterDefinition ? *Monster->MonsterDefinition->MonsterId.ToString() : TEXT("None"), Monster->CurrentHealth,
					Monster->MonsterDefinition ? Monster->MonsterDefinition->MaxHealth : 0, Monster->bCombatStatsInitialized ? TEXT("true") : TEXT("false"),
					*UEnum::GetValueAsString(Monster->MonsterState), DefinitionError.IsEmpty() ? TEXT("UninitializedCombatState") : TEXT("InvalidDefinition"),
					*DefinitionError);
			}
			continue;
		}

		const FGuid PersistenceId = Monster->ResolvePersistenceId();
		if (!PersistenceId.IsValid())
		{
			continue;
		}

		FGridCombatantInitiativeEntry Entry;
		Entry.CombatantId = PersistenceId;
		Entry.Side = EGridCombatantSide::Monster;
		Entry.DisplayName = ResolveMonsterDisplayName(Monster);
		Entry.Portrait = Monster->MonsterDefinition->Icon;
		Entry.InitiativeBase = Monster->MonsterDefinition->Initiative;
		Entry.CurrentHealth = Monster->CurrentHealth;
		Entry.MaximumHealth = FMath::Max(1, Monster->MonsterDefinition->MaxHealth);
		Entry.State = Monster->IsDead() ? EGridCombatantTurnState::Defeated : EGridCombatantTurnState::Waiting;
		InitiativeOrder.Add(Entry);
	}

	InitiativeRandomStream.Initialize(ActiveEncounterRandomSeed ^ InitiativeSeedSalt);
	FGridInitiativeOrderBuilder::RollAndSort(InitiativeOrder, InitiativeRandomStream);

	EnemyTurnOrder.Reset();
	for (const FGridCombatantInitiativeEntry& Entry : InitiativeOrder)
	{
		if (Entry.Side == EGridCombatantSide::Monster)
		{
			if (AGridMonsterActor* Monster = FindCombatMonsterById(Entry.CombatantId))
			{
				EnemyTurnOrder.Add(Monster);
			}
		}

		UE_LOG(LogGridTurnManager, Log, TEXT("[GridInitiative] Side=%s Id=%s Character=%d Base=%d Roll=%d Total=%d Modifier=%d Effective=%d State=%s"),
			*UEnum::GetValueAsString(Entry.Side), *Entry.CombatantId.ToString(EGuidFormats::Digits), Entry.CharacterIndex, Entry.InitiativeBase,
			Entry.InitiativeRoll, Entry.InitiativeTotal, Entry.InitiativeModifier, Entry.GetEffectiveInitiativeTotal(), *UEnum::GetValueAsString(Entry.State));
	}

	BroadcastInitiativeOrderChanged();
}

void UGridTurnManagerComponent::ResetInitiativeRound()
{
	CurrentInitiativeIndex = INDEX_NONE;
	FGridInitiativeOrderBuilder::Sort(InitiativeOrder);
	ResetPartyMobilityForRound();
	BeginPlayerCharacterPhase();

	for (FGridCombatantInitiativeEntry& Entry : InitiativeOrder)
	{
		RefreshInitiativeEntryVitals(Entry);
		const AGridMonsterActor* Monster = Entry.Side == EGridCombatantSide::Monster ? FindCombatMonsterById(Entry.CombatantId) : nullptr;
		const bool bDefeated = Entry.Side == EGridCombatantSide::Party ? Entry.CurrentHealth <= 0 : IsValid(Monster) && Monster->IsDead();
		if (bDefeated)
		{
			Entry.State = EGridCombatantTurnState::Defeated;
		}
		else if (Entry.State != EGridCombatantTurnState::Incapacitated)
		{
			Entry.State = EGridCombatantTurnState::Waiting;
		}
	}

	BroadcastInitiativeOrderChanged();
}

void UGridTurnManagerComponent::BeginNextCombatantTurn()
{
	if (!bCombatActive)
	{
		return;
	}

	if (!HasLivingPartyCharacter())
	{
		FinishCombat(EGridCombatPhase::Defeat);
		return;
	}
	if (!HasLivingCombatMonster())
	{
		FinishCombat(EGridCombatPhase::Victory);
		return;
	}

	while (++CurrentInitiativeIndex < InitiativeOrder.Num())
	{
		FGridCombatantInitiativeEntry& Entry = InitiativeOrder[CurrentInitiativeIndex];
		RefreshInitiativeEntryVitals(Entry);
		if (Entry.State == EGridCombatantTurnState::Defeated || Entry.State == EGridCombatantTurnState::Incapacitated)
		{
			OnCombatantStateChanged.Broadcast(Entry);
			continue;
		}

		bool bSkipActivation = false;
		if (Entry.Side == EGridCombatantSide::Party && IsValid(PartyPawn) && IsValid(PartyPawn->PartyInventoryComponent))
		{
			const TArray<FGridCharacterInventoryState>& Characters = PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters;
			if (Characters.IsValidIndex(Entry.CharacterIndex))
			{
				bSkipActivation = FGridStatusEffectControlResolver::Resolve(Characters[Entry.CharacterIndex].StatusEffects).bSkipActivation;
			}
		}
		else if (Entry.Side == EGridCombatantSide::Monster)
		{
			if (const AGridMonsterActor* Monster = FindCombatMonsterById(Entry.CombatantId))
			{
				bSkipActivation = FGridStatusEffectControlResolver::Resolve(Monster->StatusEffects).bSkipActivation;
			}
		}

		if (bSkipActivation)
		{
			if (Entry.Side == EGridCombatantSide::Party)
			{
				if (FGridPlayerCharacterTurnState* TurnState = EnsurePlayerCharacterTurnState(Entry.CharacterIndex))
				{
					if (TurnState->State != EGridCombatantTurnState::Defeated)
					{
						TurnState->MaximumActionPoints = FMath::Clamp(BasePlayerActionPointsPerTurn, 2, 6);
						TurnState->RemainingActionPoints = 0;
						TurnState->State = EGridCombatantTurnState::Completed;
						BroadcastPlayerCharacterTurnState(*TurnState);
					}
				}
			}

			UE_LOG(LogGridTurnManager, Log, TEXT("[MON16.5] ActivationSkipped Side=%s Id=%s Character=%d Round=%d"), *UEnum::GetValueAsString(Entry.Side),
				*Entry.CombatantId.ToString(EGuidFormats::Digits), Entry.CharacterIndex, RoundNumber);

			// Completed is intentional: MON16.2 already treats it as a consumed
			// activation, so Turns effects tick/decrement exactly once here.
			SetInitiativeEntryState(Entry, EGridCombatantTurnState::Completed);

			if (!bCombatActive)
			{
				return;
			}
			if (!HasLivingPartyCharacter())
			{
				FinishCombat(EGridCombatPhase::Defeat);
				return;
			}
			if (!HasLivingCombatMonster())
			{
				FinishCombat(EGridCombatPhase::Victory);
				return;
			}
			continue;
		}

		const bool bStarted = Entry.Side == EGridCombatantSide::Party ? BeginPlayerCombatantTurn(Entry) : BeginMonsterCombatantTurn(Entry);
		if (bStarted)
		{
			return;
		}

		SetInitiativeEntryState(Entry, EGridCombatantTurnState::Incapacitated);
	}

	FinishInitiativeRound();
}

bool UGridTurnManagerComponent::BeginPlayerCombatantTurn(FGridCombatantInitiativeEntry& Entry)
{
	if (Entry.Side != EGridCombatantSide::Party || !IsValid(PartyPawn) || !IsValid(PartyPawn->PartyInventoryComponent))
	{
		return false;
	}

	FGridPlayerCharacterTurnState* TurnState = EnsurePlayerCharacterTurnState(Entry.CharacterIndex);
	if (!TurnState || TurnState->State == EGridCombatantTurnState::Defeated)
	{
		return false;
	}

	if (!PhaseState.BeginCombatantTurn(EGridCombatantSide::Party))
	{
		return false;
	}

	SetPartyInputLocked(false);
	SetInitiativeEntryState(Entry, EGridCombatantTurnState::Active);
	TurnState->MaximumActionPoints = FMath::Clamp(BasePlayerActionPointsPerTurn, 2, 6);
	TurnState->RemainingActionPoints = TurnState->MaximumActionPoints;
	TurnState->State = EGridCombatantTurnState::Active;
	BroadcastPlayerCharacterTurnState(*TurnState);
	SetPhase(PhaseState.GetPhase());
	PartyPawn->PartyInventoryComponent->SetSelectedCharacterIndex(Entry.CharacterIndex);
	OnActiveCombatantChanged.Broadcast(Entry);

	UE_LOG(LogGridTurnManager, Log, TEXT("[GridInitiative] Active Side=Party Character=%d Id=%s AP=%d/%d"), Entry.CharacterIndex,
		*Entry.CombatantId.ToString(EGuidFormats::Digits), TurnState->RemainingActionPoints, TurnState->MaximumActionPoints);
	return true;
}

bool UGridTurnManagerComponent::BeginMonsterCombatantTurn(FGridCombatantInitiativeEntry& Entry)
{
	if (Entry.Side != EGridCombatantSide::Monster)
	{
		return false;
	}

	AGridMonsterActor* Candidate = FindCombatMonsterById(Entry.CombatantId);
	if (!IsValid(Candidate) || Candidate->IsDead() || !PrepareMonsterForCombat(Candidate))
	{
		return false;
	}

	UnbindCurrentMovement();
	UnbindCurrentCombat();
	PendingActions.Reset();
	bHasActiveAction = false;
	ActiveAction = FGridCombatAction();
	ActiveActionTimeoutRemaining = 0.0f;
	ResetActiveAttackState();

	CurrentMonster = Candidate;
	CurrentEnemyIndex = EnemyTurnOrder.IndexOfByPredicate(
		[Candidate](const TObjectPtr<AGridMonsterActor>& Monster)
		{
			return Monster.Get() == Candidate;
		});
	CurrentMonsterMaximumActionPoints = FMath::Max(0, Candidate->MonsterDefinition->ActionPointsPerTurn);
	ActionPointBudget.Reset(CurrentMonsterMaximumActionPoints);
	CurrentMonsterRemainingActionPoints = ActionPointBudget.GetRemainingPoints();
	if (bCollectRuntimeMetrics)
	{
		++RuntimeMetrics.MonsterTurnsStarted;
	}

	BindCurrentMovement(Candidate->FindComponentByClass<UGridMonsterMovementComponent>());
	BindCurrentCombat(Candidate->FindComponentByClass<UGridMonsterCombatComponent>());

	if (!PhaseState.BeginCombatantTurn(EGridCombatantSide::Monster))
	{
		return false;
	}

	SetPartyInputLocked(true);
	SetInitiativeEntryState(Entry, EGridCombatantTurnState::Active);
	SetPhase(PhaseState.GetPhase());
	OnActiveCombatantChanged.Broadcast(Entry);

	FGridCombatLogEntry TurnEntry;
	TurnEntry.RoundNumber = RoundNumber;
	TurnEntry.Phase = CurrentPhase;
	TurnEntry.Type = EGridCombatLogEntryType::MonsterTurnStarted;
	TurnEntry.SourceId = ResolveMonsterLogId(Candidate);
	TurnEntry.SourceDisplayName = ResolveMonsterDisplayName(Candidate);
	TurnEntry.Message = FGridCombatLogFormatter::FormatMonsterTurnStarted(TurnEntry.SourceDisplayName);
	AppendCombatLogEntry(TurnEntry);
	OnMonsterTurnStarted.Broadcast(Candidate);
	PrepareCurrentMonsterActions();
	ExecuteNextAction();
	return true;
}

bool UGridTurnManagerComponent::EndActivePlayerTurn()
{
	if (!CanEndActivePlayerTurn())
	{
		return false;
	}

	FinishActivePlayerTurn();
	return true;
}

bool UGridTurnManagerComponent::CanEndActivePlayerTurn() const
{
	FGridCombatantInitiativeEntry ActiveCombatant;
	return bInitialized && bCombatActive && !bPartyInputLocked && !bPlayerAttackResolutionInProgress && !bHasActiveAction &&
		GetActiveCombatant(ActiveCombatant) && ActiveCombatant.Side == EGridCombatantSide::Party && IsPartyAtRest();
}

void UGridTurnManagerComponent::FinishActivePlayerTurn()
{
	if (!InitiativeOrder.IsValidIndex(CurrentInitiativeIndex))
	{
		return;
	}

	FGridCombatantInitiativeEntry& Entry = InitiativeOrder[CurrentInitiativeIndex];
	if (Entry.Side != EGridCombatantSide::Party || Entry.State != EGridCombatantTurnState::Active)
	{
		return;
	}

	if (FGridPlayerCharacterTurnState* TurnState = EnsurePlayerCharacterTurnState(Entry.CharacterIndex))
	{
		if (TurnState->State != EGridCombatantTurnState::Defeated && TurnState->State != EGridCombatantTurnState::Incapacitated)
		{
			TurnState->State = EGridCombatantTurnState::Completed;
			BroadcastPlayerCharacterTurnState(*TurnState);
		}
	}

	SetInitiativeEntryState(Entry, EGridCombatantTurnState::Completed);
	SetPartyInputLocked(true);
	BeginNextCombatantTurn();
}

void UGridTurnManagerComponent::FinishInitiativeRound()
{
	if (!bCombatActive || !PhaseState.CompleteInitiativeRound())
	{
		return;
	}

	SetPartyInputLocked(true);
	SetPhase(PhaseState.GetPhase());

	if (!HasLivingPartyCharacter())
	{
		FinishCombat(EGridCombatPhase::Defeat);
		return;
	}
	if (!HasLivingCombatMonster())
	{
		FinishCombat(EGridCombatPhase::Victory);
		return;
	}
	if (!PhaseState.BeginNextRound())
	{
		AbortCombat();
		return;
	}

	RoundNumber = PhaseState.GetRoundNumber();
	if (bCollectRuntimeMetrics)
	{
		++RuntimeMetrics.RoundsStarted;
	}

	FGridCombatLogEntry RoundEntry;
	RoundEntry.RoundNumber = RoundNumber;
	RoundEntry.Phase = PhaseState.GetPhase();
	RoundEntry.Type = EGridCombatLogEntryType::RoundStarted;
	RoundEntry.Message = FGridCombatLogFormatter::FormatRoundStarted(RoundNumber);
	AppendCombatLogEntry(RoundEntry);

	ResetInitiativeRound();
	OnRoundStarted.Broadcast(RoundNumber);
	BeginNextCombatantTurn();
}

void UGridTurnManagerComponent::ClearInitiativeState(bool bBroadcast)
{
	InitiativeOrder.Reset();
	CurrentInitiativeIndex = INDEX_NONE;
	if (bBroadcast)
	{
		OnActiveCombatantChanged.Broadcast(FGridCombatantInitiativeEntry());
		BroadcastInitiativeOrderChanged();
	}
}

void UGridTurnManagerComponent::SetInitiativeEntryState(FGridCombatantInitiativeEntry& Entry, EGridCombatantTurnState NewState)
{
	if (Entry.State == NewState)
	{
		return;
	}

	Entry.State = NewState;
	OnCombatantStateChanged.Broadcast(Entry);
}

void UGridTurnManagerComponent::RefreshInitiativeEntryVitals(FGridCombatantInitiativeEntry& Entry) const
{
	if (Entry.Side == EGridCombatantSide::Party)
	{
		if (!IsValid(PartyPawn) || !IsValid(PartyPawn->PartyInventoryComponent))
		{
			Entry.CurrentHealth = 0;
			return;
		}

		const TArray<FGridCharacterInventoryState>& Characters = PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters;
		if (!Characters.IsValidIndex(Entry.CharacterIndex))
		{
			Entry.CurrentHealth = 0;
			return;
		}

		const FGridCharacterInventoryState& Character = Characters[Entry.CharacterIndex];
		Entry.CurrentHealth = Character.Resources.CurrentHealth;
		Entry.MaximumHealth = FMath::Max(1, Character.DerivedStats.MaxHealth);
		return;
	}

	const AGridMonsterActor* Monster = FindCombatMonsterById(Entry.CombatantId);
	if (!IsValid(Monster) || !IsValid(Monster->MonsterDefinition))
	{
		Entry.CurrentHealth = 0;
		return;
	}

	Entry.CurrentHealth = Monster->CurrentHealth;
	Entry.MaximumHealth = FMath::Max(1, Monster->MonsterDefinition->MaxHealth);
}

void UGridTurnManagerComponent::BroadcastInitiativeOrderChanged()
{
	OnTurnOrderChanged.Broadcast();
}

void UGridTurnManagerComponent::GetUpcomingInitiativeOrder(TArray<FGridCombatantInitiativeEntry>& OutEntries) const
{
	OutEntries.Reset();
	const int32 StartIndex = InitiativeOrder.IsValidIndex(CurrentInitiativeIndex) ? CurrentInitiativeIndex : 0;
	for (int32 Index = StartIndex; Index < InitiativeOrder.Num(); ++Index)
	{
		const FGridCombatantInitiativeEntry& Entry = InitiativeOrder[Index];
		if (Entry.State == EGridCombatantTurnState::Active || Entry.State == EGridCombatantTurnState::Waiting)
		{
			OutEntries.Add(Entry);
		}
	}
}

void UGridTurnManagerComponent::GetInitiativePreview(TArray<FGridInitiativePreviewEntry>& OutEntries, int32 MaximumEntries) const
{
	OutEntries.Reset();
	MaximumEntries = FMath::Clamp(MaximumEntries, 0, 32);
	if (MaximumEntries <= 0 || InitiativeOrder.IsEmpty())
	{
		return;
	}

	const int32 ProjectedCurrentRound = FMath::Max(1, RoundNumber);
	const int32 StartIndex = InitiativeOrder.IsValidIndex(CurrentInitiativeIndex) ? CurrentInitiativeIndex : 0;
	for (int32 Index = StartIndex; Index < InitiativeOrder.Num() && OutEntries.Num() < MaximumEntries; ++Index)
	{
		FGridCombatantInitiativeEntry Entry = InitiativeOrder[Index];
		RefreshInitiativeEntryVitals(Entry);
		if (Entry.State != EGridCombatantTurnState::Active && Entry.State != EGridCombatantTurnState::Waiting || Entry.CurrentHealth <= 0)
		{
			continue;
		}

		FGridInitiativePreviewEntry& Preview = OutEntries.AddDefaulted_GetRef();
		Preview.Combatant = Entry;
		Preview.RoundNumber = ProjectedCurrentRound;
		Preview.ActivationIndex = Index;
		Preview.bIsActive = OutEntries.Num() == 1 && Entry.State == EGridCombatantTurnState::Active;
	}

	TArray<FGridCombatantInitiativeEntry> FutureRoundOrder;
	FutureRoundOrder.Reserve(InitiativeOrder.Num());
	for (const FGridCombatantInitiativeEntry& Entry : InitiativeOrder)
	{
		FGridCombatantInitiativeEntry RefreshedEntry = Entry;
		RefreshInitiativeEntryVitals(RefreshedEntry);
		if (RefreshedEntry.State == EGridCombatantTurnState::Defeated || RefreshedEntry.State == EGridCombatantTurnState::Incapacitated ||
			RefreshedEntry.CurrentHealth <= 0)
		{
			continue;
		}

		FGridCombatantInitiativeEntry& FutureEntry = FutureRoundOrder.AddDefaulted_GetRef();
		FutureEntry = RefreshedEntry;
		FutureEntry.State = EGridCombatantTurnState::Waiting;
	}
	FGridInitiativeOrderBuilder::Sort(FutureRoundOrder);
	if (FutureRoundOrder.IsEmpty())
	{
		return;
	}

	int32 ProjectedRound = ProjectedCurrentRound + 1;
	while (OutEntries.Num() < MaximumEntries)
	{
		for (int32 Index = 0; Index < FutureRoundOrder.Num() && OutEntries.Num() < MaximumEntries; ++Index)
		{
			FGridInitiativePreviewEntry& Preview = OutEntries.AddDefaulted_GetRef();
			Preview.Combatant = FutureRoundOrder[Index];
			Preview.RoundNumber = ProjectedRound;
			Preview.ActivationIndex = Index;
			Preview.bStartsNewRound = Index == 0 && OutEntries.Num() > 1;
		}
		++ProjectedRound;
	}
}

bool UGridTurnManagerComponent::SetCombatantInitiativeModifier(EGridCombatantSide Side, FGuid CombatantId, int32 InitiativeModifier)
{
	FGridCombatantInitiativeEntry* Entry = FindInitiativeEntry(Side, CombatantId);
	if (!Entry)
	{
		return false;
	}
	if (Entry->InitiativeModifier == InitiativeModifier)
	{
		return true;
	}

	const int32 PreviousModifier = Entry->InitiativeModifier;
	Entry->InitiativeModifier = InitiativeModifier;
	const int32 EffectiveInitiativeTotal = Entry->GetEffectiveInitiativeTotal();
	ReorderFutureInitiativeEntries();
	BroadcastInitiativeOrderChanged();

	UE_LOG(LogGridTurnManager, Log, TEXT("[GridInitiative] Modifier Side=%s Id=%s Previous=%d Current=%d EffectiveTotal=%d"), *UEnum::GetValueAsString(Side),
		*CombatantId.ToString(EGuidFormats::Digits), PreviousModifier, InitiativeModifier, EffectiveInitiativeTotal);
	return true;
}

void UGridTurnManagerComponent::ReorderFutureInitiativeEntries()
{
	const int32 FirstFutureIndex = InitiativeOrder.IsValidIndex(CurrentInitiativeIndex) ? CurrentInitiativeIndex + 1 : 0;

	TArray<int32> WaitingIndices;
	TArray<FGridCombatantInitiativeEntry> WaitingEntries;
	for (int32 Index = FirstFutureIndex; Index < InitiativeOrder.Num(); ++Index)
	{
		if (InitiativeOrder[Index].State != EGridCombatantTurnState::Waiting)
		{
			continue;
		}
		WaitingIndices.Add(Index);
		WaitingEntries.Add(InitiativeOrder[Index]);
	}

	FGridInitiativeOrderBuilder::Sort(WaitingEntries);
	for (int32 Index = 0; Index < WaitingIndices.Num(); ++Index)
	{
		InitiativeOrder[WaitingIndices[Index]] = WaitingEntries[Index];
	}
}

bool UGridTurnManagerComponent::GetActiveCombatant(FGridCombatantInitiativeEntry& OutCombatant) const
{
	OutCombatant = FGridCombatantInitiativeEntry();
	if (!InitiativeOrder.IsValidIndex(CurrentInitiativeIndex) || InitiativeOrder[CurrentInitiativeIndex].State != EGridCombatantTurnState::Active)
	{
		return false;
	}

	OutCombatant = InitiativeOrder[CurrentInitiativeIndex];
	return true;
}

bool UGridTurnManagerComponent::IsActivePlayerCharacter(int32 CharacterIndex) const
{
	FGridCombatantInitiativeEntry ActiveCombatant;
	return GetActiveCombatant(ActiveCombatant) && ActiveCombatant.Side == EGridCombatantSide::Party && ActiveCombatant.CharacterIndex == CharacterIndex;
}

FGuid UGridTurnManagerComponent::ResolvePlayerCombatantId(int32 CharacterIndex) const
{
	if (IsValid(PartyPawn) && IsValid(PartyPawn->PartyInventoryComponent))
	{
		const TArray<FGridCharacterInventoryState>& Characters = PartyPawn->PartyInventoryComponent->PartyInventoryState.ActiveCharacters;
		if (Characters.IsValidIndex(CharacterIndex) && Characters[CharacterIndex].CharacterId.IsValid())
		{
			return Characters[CharacterIndex].CharacterId;
		}
	}

	return FGuid(0x50415254, 0, 0, static_cast<uint32>(FMath::Max(0, CharacterIndex) + 1));
}

AGridMonsterActor* UGridTurnManagerComponent::FindCombatMonsterById(const FGuid& CombatantId) const
{
	const TObjectPtr<AGridMonsterActor>* Found = CombatMonsters.FindByPredicate(
		[&CombatantId](const TObjectPtr<AGridMonsterActor>& Monster)
		{
			return IsValid(Monster) && Monster->ResolvePersistenceId() == CombatantId;
		});
	return Found ? Found->Get() : nullptr;
}

FGridCombatantInitiativeEntry* UGridTurnManagerComponent::FindInitiativeEntry(EGridCombatantSide Side, const FGuid& CombatantId)
{
	return InitiativeOrder.FindByPredicate(
		[Side, &CombatantId](const FGridCombatantInitiativeEntry& Entry)
		{
			return Entry.Side == Side && Entry.CombatantId == CombatantId;
		});
}

const FGridCombatantInitiativeEntry* UGridTurnManagerComponent::FindInitiativeEntry(EGridCombatantSide Side, const FGuid& CombatantId) const
{
	return InitiativeOrder.FindByPredicate(
		[Side, &CombatantId](const FGridCombatantInitiativeEntry& Entry)
		{
			return Entry.Side == Side && Entry.CombatantId == CombatantId;
		});
}
