#include "RPG/RPGClassProgressionTransactionService.h"

#include "RPG/RPGClassAsset.h"
#include "RPG/RPGAuthoringIdentityResolver.h"
#include "Runtime/GridPartyInventoryComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogGridClassProgression, Log, All);

namespace
{
	struct FRuntimeClassProgressionState
	{
		TWeakObjectPtr<UGridPartyInventoryComponent> InventoryComponent;
		TSet<FName> SatisfiedRequirements;
	};

	TMap<FGuid, FRuntimeClassProgressionState> RuntimeStates;

	URPGClassAsset* ResolveClassDefinition(const FGridCharacterInventoryState& Character)
	{
		if (URPGClassAsset* ClassDefinition = Character.ClassDefinition.Get();
			FRPGAuthoringIdentityResolver::IsMatchingClassDefinition(Character.ClassId, ClassDefinition))
		{
			return ClassDefinition;
		}
		return FRPGAuthoringIdentityResolver::ResolveClassById(Character.ClassId);
	}

	bool BuildSelectionSet(const TArray<FName>& ChoiceIds, TSet<FName>& OutChoiceIds)
	{
		OutChoiceIds.Reset();
		for (const FName ChoiceId : ChoiceIds)
		{
			if (ChoiceId.IsNone() || OutChoiceIds.Contains(ChoiceId))
			{
				OutChoiceIds.Reset();
				return false;
			}
			OutChoiceIds.Add(ChoiceId);
		}
		return true;
	}

	void NormalizeSelectionOrder(const URPGClassAsset& ClassDefinition, const TSet<FName>& SelectedChoiceIds, TArray<FName>& OutSelectedChoiceIds)
	{
		OutSelectedChoiceIds.Reset(SelectedChoiceIds.Num());
		for (const FRPGClassProgressionChoiceDefinition& Choice : ClassDefinition.ProgressionChoices)
		{
			if (SelectedChoiceIds.Contains(Choice.ChoiceId))
			{
				OutSelectedChoiceIds.Add(Choice.ChoiceId);
			}
		}
	}

	bool BuildRuntimeStateFromCharacter(const FGridCharacterInventoryState& Character, UGridPartyInventoryComponent* InventoryComponent,
		FRuntimeClassProgressionState& OutState, FText& OutError)
	{
		OutState = FRuntimeClassProgressionState();
		OutError = FText::GetEmpty();

		if (!Character.CharacterId.IsValid())
		{
			OutError = FText::FromString(TEXT("Un personnage ne possède pas de CharacterId valide."));
			return false;
		}

		TSet<FName> SelectedChoiceIds;
		if (!BuildSelectionSet(Character.SelectedClassProgressionChoiceIds, SelectedChoiceIds))
		{
			OutError = FText::FromString(FString::Printf(TEXT("Les choix persistants du personnage %s contiennent un identifiant vide ou dupliqué."),
				*Character.CharacterId.ToString(EGuidFormats::Digits)));
			return false;
		}

		URPGClassAsset* ClassDefinition = ResolveClassDefinition(Character);
		if (!ClassDefinition)
		{
			if (!SelectedChoiceIds.IsEmpty())
			{
				OutError =
					FText::FromString(FString::Printf(TEXT("La classe du personnage %s est introuvable alors que des choix de progression sont enregistrés."),
						*Character.CharacterId.ToString(EGuidFormats::Digits)));
				return false;
			}

			OutState.InventoryComponent = InventoryComponent;
			if (!Character.ClassId.IsNone())
			{
				OutState.SatisfiedRequirements.Add(Character.ClassId);
			}
			return true;
		}

		int32 GrantedPoints = 0;
		int32 SpentPoints = 0;
		int32 RemainingPoints = 0;
		if (!FRPGClassProgressionService::TryGetChoicePointBalance(
				ClassDefinition, Character.Level, SelectedChoiceIds, GrantedPoints, SpentPoints, RemainingPoints))
		{
			OutError = FText::FromString(FString::Printf(TEXT("Les choix persistants du personnage %s ne sont pas valides pour son niveau et sa classe."),
				*Character.CharacterId.ToString(EGuidFormats::Digits)));
			return false;
		}

		TSet<FName> SatisfiedRequirements;
		if (!FRPGClassProgressionService::CollectSatisfiedRequirements(ClassDefinition, Character.Level, SelectedChoiceIds, SatisfiedRequirements))
		{
			OutError = FText::FromString(
				FString::Printf(TEXT("La projection des choix persistants du personnage %s a échoué."), *Character.CharacterId.ToString(EGuidFormats::Digits)));
			return false;
		}

		OutState.InventoryComponent = InventoryComponent;
		OutState.SatisfiedRequirements = MoveTemp(SatisfiedRequirements);
		return true;
	}

	bool ResolveCharacter(UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex, FGridCharacterInventoryState*& OutCharacter,
		URPGClassAsset*& OutClassDefinition)
	{
		OutCharacter = nullptr;
		OutClassDefinition = nullptr;
		if (!IsValid(PartyInventoryComponent) || !PartyInventoryComponent->IsValidCharacterIndex(CharacterIndex))
		{
			return false;
		}

		FGridCharacterInventoryState& Character = PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex];
		if (!Character.CharacterId.IsValid())
		{
			return false;
		}

		URPGClassAsset* ClassDefinition = ResolveClassDefinition(Character);
		if (!ClassDefinition)
		{
			return false;
		}

		OutCharacter = &Character;
		OutClassDefinition = ClassDefinition;
		return true;
	}

	ERPGClassProgressionCommitRejectReason DiagnoseCandidateFailure(const URPGClassAsset& ClassDefinition, int32 CharacterLevel, int32 GrantedPoints,
		const TSet<FName>& CandidateSelection, const TArray<FName>& RequestedChoiceIds)
	{
		int32 TotalCost = 0;
		for (const FName SelectedChoiceId : CandidateSelection)
		{
			const FRPGClassProgressionChoiceDefinition* SelectedChoice = ClassDefinition.FindProgressionChoice(SelectedChoiceId);
			if (!SelectedChoice)
			{
				return ERPGClassProgressionCommitRejectReason::UnknownChoice;
			}
			if (CharacterLevel < SelectedChoice->MinimumLevel)
			{
				return ERPGClassProgressionCommitRejectReason::LevelTooLow;
			}
			for (const FName PrerequisiteId : SelectedChoice->PrerequisiteChoiceIds)
			{
				if (!CandidateSelection.Contains(PrerequisiteId))
				{
					return ERPGClassProgressionCommitRejectReason::MissingPrerequisite;
				}
			}
			TotalCost += SelectedChoice->PointCost;
		}

		if (TotalCost > GrantedPoints)
		{
			return ERPGClassProgressionCommitRejectReason::InsufficientChoicePoints;
		}

		for (const FName RequestedChoiceId : RequestedChoiceIds)
		{
			if (!ClassDefinition.FindProgressionChoice(RequestedChoiceId))
			{
				return ERPGClassProgressionCommitRejectReason::UnknownChoice;
			}
		}
		return ERPGClassProgressionCommitRejectReason::InvalidCurrentSelection;
	}
}

FRPGClassProgressionCommittedNativeSignature& FRPGClassProgressionTransactionService::OnClassProgressionCommitted()
{
	static FRPGClassProgressionCommittedNativeSignature Delegate;
	return Delegate;
}

bool FRPGClassProgressionTransactionService::RefreshCharacterProjection(UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex)
{
	FGridCharacterInventoryState* Character = nullptr;
	URPGClassAsset* ClassDefinition = nullptr;
	if (!ResolveCharacter(PartyInventoryComponent, CharacterIndex, Character, ClassDefinition))
	{
		return false;
	}

	FRuntimeClassProgressionState RebuiltState;
	FText RebuildError;
	if (!BuildRuntimeStateFromCharacter(*Character, PartyInventoryComponent, RebuiltState, RebuildError))
	{
		return false;
	}

	RuntimeStates.Add(Character->CharacterId, MoveTemp(RebuiltState));
	return true;
}

bool FRPGClassProgressionTransactionService::TryGetSelectedChoiceIds(
	UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex, TArray<FName>& OutSelectedChoiceIds)
{
	OutSelectedChoiceIds.Reset();
	if (!RefreshCharacterProjection(PartyInventoryComponent, CharacterIndex))
	{
		return false;
	}

	OutSelectedChoiceIds = PartyInventoryComponent->PartyInventoryState.ActiveCharacters[CharacterIndex].SelectedClassProgressionChoiceIds;
	return true;
}

bool FRPGClassProgressionTransactionService::TryGetChoicePointBalance(
	UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex, int32& OutGrantedPoints, int32& OutSpentPoints, int32& OutRemainingPoints)
{
	OutGrantedPoints = 0;
	OutSpentPoints = 0;
	OutRemainingPoints = 0;

	FGridCharacterInventoryState* Character = nullptr;
	URPGClassAsset* ClassDefinition = nullptr;
	if (!ResolveCharacter(PartyInventoryComponent, CharacterIndex, Character, ClassDefinition) ||
		!RefreshCharacterProjection(PartyInventoryComponent, CharacterIndex))
	{
		return false;
	}

	TSet<FName> SelectedChoiceIds;
	if (!BuildSelectionSet(Character->SelectedClassProgressionChoiceIds, SelectedChoiceIds))
	{
		return false;
	}

	return FRPGClassProgressionService::TryGetChoicePointBalance(
		ClassDefinition, Character->Level, SelectedChoiceIds, OutGrantedPoints, OutSpentPoints, OutRemainingPoints);
}

bool FRPGClassProgressionTransactionService::TryCommitChoices(UGridPartyInventoryComponent* PartyInventoryComponent, int32 CharacterIndex,
	const TArray<FName>& ChoiceIdsToCommit, FRPGClassProgressionCommitResult& OutResult)
{
	OutResult = FRPGClassProgressionCommitResult();
	if (!IsValid(PartyInventoryComponent))
	{
		OutResult.RejectReason = ERPGClassProgressionCommitRejectReason::InvalidInventory;
		return false;
	}
	if (!PartyInventoryComponent->IsValidCharacterIndex(CharacterIndex))
	{
		OutResult.RejectReason = ERPGClassProgressionCommitRejectReason::InvalidCharacter;
		return false;
	}
	if (ChoiceIdsToCommit.IsEmpty())
	{
		OutResult.RejectReason = ERPGClassProgressionCommitRejectReason::EmptyRequest;
		return false;
	}

	FGridCharacterInventoryState* Character = nullptr;
	URPGClassAsset* ClassDefinition = nullptr;
	if (!ResolveCharacter(PartyInventoryComponent, CharacterIndex, Character, ClassDefinition))
	{
		OutResult.RejectReason = ERPGClassProgressionCommitRejectReason::InvalidClassDefinition;
		return false;
	}

	TSet<FName> CurrentSelection;
	if (!BuildSelectionSet(Character->SelectedClassProgressionChoiceIds, CurrentSelection) ||
		!FRPGClassProgressionService::TryGetChoicePointBalance(
			ClassDefinition, Character->Level, CurrentSelection, OutResult.GrantedPoints, OutResult.SpentPointsBefore, OutResult.RemainingPoints))
	{
		OutResult.RejectReason = ERPGClassProgressionCommitRejectReason::InvalidCurrentSelection;
		return false;
	}

	TSet<FName> RequestedUnique;
	TSet<FName> CandidateSelection = CurrentSelection;
	for (const FName ChoiceId : ChoiceIdsToCommit)
	{
		if (ChoiceId.IsNone() || RequestedUnique.Contains(ChoiceId))
		{
			OutResult.RejectReason = ERPGClassProgressionCommitRejectReason::DuplicateRequest;
			return false;
		}
		RequestedUnique.Add(ChoiceId);

		const FRPGClassProgressionChoiceDefinition* Choice = ClassDefinition->FindProgressionChoice(ChoiceId);
		if (!Choice)
		{
			OutResult.RejectReason = ERPGClassProgressionCommitRejectReason::UnknownChoice;
			return false;
		}
		if (CurrentSelection.Contains(ChoiceId))
		{
			OutResult.RejectReason = ERPGClassProgressionCommitRejectReason::AlreadySelected;
			return false;
		}
		if (Character->Level < Choice->MinimumLevel)
		{
			OutResult.RejectReason = ERPGClassProgressionCommitRejectReason::LevelTooLow;
			return false;
		}
		CandidateSelection.Add(ChoiceId);
	}

	int32 SpentAfter = 0;
	int32 RemainingAfter = 0;
	int32 GrantedAfter = 0;
	if (!FRPGClassProgressionService::TryGetChoicePointBalance(ClassDefinition, Character->Level, CandidateSelection, GrantedAfter, SpentAfter, RemainingAfter))
	{
		OutResult.RejectReason = DiagnoseCandidateFailure(*ClassDefinition, Character->Level, OutResult.GrantedPoints, CandidateSelection, ChoiceIdsToCommit);
		return false;
	}

	TArray<FName> NormalizedChoices;
	NormalizeSelectionOrder(*ClassDefinition, CandidateSelection, NormalizedChoices);

	FGridCharacterInventoryState CandidateCharacter = *Character;
	CandidateCharacter.SelectedClassProgressionChoiceIds = NormalizedChoices;
	FRuntimeClassProgressionState CandidateRuntimeState;
	FText ProjectionError;
	if (!BuildRuntimeStateFromCharacter(CandidateCharacter, PartyInventoryComponent, CandidateRuntimeState, ProjectionError))
	{
		OutResult.RejectReason = ERPGClassProgressionCommitRejectReason::InvalidCurrentSelection;
		return false;
	}

	Character->SelectedClassProgressionChoiceIds = MoveTemp(NormalizedChoices);
	RuntimeStates.Add(Character->CharacterId, MoveTemp(CandidateRuntimeState));

	OutResult.bCommitted = true;
	OutResult.RejectReason = ERPGClassProgressionCommitRejectReason::None;
	OutResult.GrantedPoints = GrantedAfter;
	OutResult.SpentPointsAfter = SpentAfter;
	OutResult.RemainingPoints = RemainingAfter;
	OutResult.CommittedChoiceIds = ChoiceIdsToCommit;

	PartyInventoryComponent->NotifyPartyInventoryChanged(CharacterIndex);
	OnClassProgressionCommitted().Broadcast(PartyInventoryComponent, CharacterIndex, OutResult.CommittedChoiceIds, OutResult.RemainingPoints);

	UE_LOG(LogGridClassProgression, Log, TEXT("[GridClassProgression] Character=%d CharacterId=%s Level=%d Committed=%d Granted=%d Spent=%d Remaining=%d"),
		CharacterIndex, *Character->CharacterId.ToString(EGuidFormats::Digits), Character->Level, OutResult.CommittedChoiceIds.Num(), OutResult.GrantedPoints,
		OutResult.SpentPointsAfter, OutResult.RemainingPoints);
	return true;
}

void FRPGClassProgressionTransactionService::AppendRuntimeSatisfiedRequirements(const FGuid& CharacterId, TSet<FName>& InOutSatisfiedRequirements)
{
	if (!CharacterId.IsValid())
	{
		return;
	}

	const FRuntimeClassProgressionState* State = RuntimeStates.Find(CharacterId);
	if (!State)
	{
		return;
	}

	for (const FName RequirementId : State->SatisfiedRequirements)
	{
		InOutSatisfiedRequirements.Add(RequirementId);
	}
}

bool FRPGClassProgressionTransactionService::RebuildRuntimeProjection(const FGridPartyInventoryState& PartyState, FText& OutError)
{
	OutError = FText::GetEmpty();
	TMap<FGuid, FRuntimeClassProgressionState> RebuiltStates;
	TSet<FGuid> CharacterIds;

	for (int32 CharacterIndex = 0; CharacterIndex < PartyState.ActiveCharacters.Num(); ++CharacterIndex)
	{
		const FGridCharacterInventoryState& Character = PartyState.ActiveCharacters[CharacterIndex];
		if (!Character.CharacterId.IsValid() || CharacterIds.Contains(Character.CharacterId))
		{
			OutError = FText::FromString(TEXT("Les personnages actifs possèdent un CharacterId invalide ou dupliqué."));
			return false;
		}
		CharacterIds.Add(Character.CharacterId);

		FRuntimeClassProgressionState RebuiltState;
		if (!BuildRuntimeStateFromCharacter(Character, nullptr, RebuiltState, OutError))
		{
			return false;
		}
		RebuiltStates.Add(Character.CharacterId, MoveTemp(RebuiltState));
	}

	for (const FGridCharacterInventoryState& Character : PartyState.ActiveCharacters)
	{
		RuntimeStates.Remove(Character.CharacterId);
	}
	for (TPair<FGuid, FRuntimeClassProgressionState>& Pair : RebuiltStates)
	{
		RuntimeStates.Add(Pair.Key, MoveTemp(Pair.Value));
	}
	return true;
}

void FRPGClassProgressionTransactionService::ResetRuntimeState(UGridPartyInventoryComponent* PartyInventoryComponent)
{
	if (!PartyInventoryComponent)
	{
		RuntimeStates.Reset();
		return;
	}

	for (auto It = RuntimeStates.CreateIterator(); It; ++It)
	{
		if (It.Value().InventoryComponent.IsValid() && It.Value().InventoryComponent.Get() == PartyInventoryComponent)
		{
			It.RemoveCurrent();
		}
	}
}
